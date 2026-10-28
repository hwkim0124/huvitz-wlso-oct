using OpenCvSharp;
using OpenCvSharp.WpfExtensions;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;

namespace WsoToolkit.controls
{
    /// <summary>
    /// Combined preview of the two side cornea cameras: the upper half of the left
    /// camera's frame stacked over the lower half of the right camera's frame. The status
    /// line reports each camera's frame size and capture rate.
    ///
    /// The control owns its two frame callbacks (<see cref="LeftCallback"/> and
    /// <see cref="RightCallback"/>); pass them to <c>CorneaCamera.StartPreview</c>. They
    /// run on the native capture threads and only copy the half of the frame that is
    /// shown; composing and display happen on the UI thread, one update per dispatcher
    /// pass (frames arriving while an update is queued are coalesced).
    ///
    /// Optional overlays: the side cameras' focus guide (<see cref="IsOverlayFocusGuide"/>,
    /// each camera's horizontal focus line drawn in its own half plus the common vertical
    /// centre line) and a click-hold X/Y velocity jog of the head
    /// (<see cref="IsStageControlEnabled"/>), the same gesture as on the single-camera
    /// preview but without the Z wheel.
    /// </summary>
    public partial class StereoCorneaPreview : UserControl
    {
        private const double FrameRateSmoothing = 0.9;
        private static readonly TimeSpan StatusRefreshInterval = TimeSpan.FromMilliseconds(500);
        private static readonly TimeSpan FrameStaleAfter = TimeSpan.FromSeconds(1);
        private static readonly Scalar BackgroundGray = Scalar.All(64);
        private static readonly Scalar FocusGuideColor = new(255, 255, 0);
        private const int FocusGuideUnitDivisor = 12;       // focus lines at +/- frame height / 12 from the centre

        private sealed class CameraSide
        {
            public CameraSide(string name)
            {
                Name = name;
            }

            public string Name { get; }
            public Mat Half { get; } = new();           // the half of the last frame that is shown
            public int FrameWidth { get; set; }         // full frame size as captured
            public int FrameHeight { get; set; }
            public Stopwatch Interval { get; } = new(); // time since the last captured frame
            public double FrameRate { get; set; }
            public bool HasFrameRate { get; set; }
        }

        private readonly CameraSide _left = new("Left");
        private readonly CameraSide _right = new("Right");
        private readonly object _frameLock = new();     // guards Half and the frame sizes of both sides
        private readonly object _fpsLock = new();       // guards Interval and the frame rate fields
        private readonly DispatcherTimer _statusTimer = new() { Interval = StatusRefreshInterval };
        private Mat _composite = new();                 // grayscale stack of the two halves
        private Mat _display = new();                   // BGR copy of the composite with overlays
        private int _updatePending;                     // 1 while a composite update is queued

        // Layout of the last composite (UI thread): the row where the right half starts and
        // the full frame heights the focus lines are derived from.
        private int _seamRow;
        private int _leftFrameHeight;
        private int _rightFrameHeight;

        public bool IsOverlayFocusGuide { get; set; } = false;
        public bool IsStageControlEnabled { get; set; } = false;

        /// <summary>Receives the left cornea camera's frames; its upper half is shown.</summary>
        public WsoCallback.CorneaCameraFrameCaptured LeftCallback { get; }

        /// <summary>Receives the right cornea camera's frames; its lower half is shown.</summary>
        public WsoCallback.CorneaCameraFrameCaptured RightCallback { get; }

        public StereoCorneaPreview()
        {
            InitializeComponent();

            // The delegates live as long as the control so the function pointers handed to
            // the native side stay valid (a collected delegate crashes the capture thread).
            LeftCallback = new WsoCallback.CorneaCameraFrameCaptured(OnLeftFrameCaptured);
            RightCallback = new WsoCallback.CorneaCameraFrameCaptured(OnRightFrameCaptured);
            _statusTimer.Tick += (sender, e) => UpdateStatus();
            RegisterMouseEventHandlers();
        }

        /// <summary>Drops the stored halves and shows an empty preview. UI thread only.</summary>
        public void ClearFrames()
        {
            lock (_frameLock)
            {
                _left.Half.Release();
                _right.Half.Release();
                _left.FrameWidth = _left.FrameHeight = 0;
                _right.FrameWidth = _right.FrameHeight = 0;
            }
            lock (_fpsLock)
            {
                ResetFrameRate(_left);
                ResetFrameRate(_right);
            }
            imageViewport.Source = null;
            UpdateStatus();
        }

        // Native capture callbacks
        /////////////////////////////////////////////////////////////////////////////////////////////

        private void OnLeftFrameCaptured(nint data, int width, int height)
        {
            // Upper half of the left camera's frame.
            StoreHalf(_left, data, width, height, 0, height / 2);
        }

        private void OnRightFrameCaptured(nint data, int width, int height)
        {
            // Lower half of the right camera's frame (the odd row, if any, goes here).
            StoreHalf(_right, data, width, height, height / 2, height - height / 2);
        }

        /// <summary>
        /// Native capture thread: ticks the camera's frame rate, copies rows
        /// [top, top + rows) of the 8-bit grayscale frame and queues one composite update.
        /// </summary>
        private void StoreHalf(CameraSide side, nint data, int width, int height, int top, int rows)
        {
            if (data == 0 || width <= 0 || rows <= 0)
            {
                return;
            }
            MarkFrameCaptured(side);

            try
            {
                // A zero-copy header over the camera buffer; only the shown half is copied out.
                nint step = width;
                using Mat frame = Mat.FromPixelData(height, width, MatType.CV_8UC1, data, step);
                using Mat half = new Mat(frame, new OpenCvSharp.Rect(0, top, width, rows));
                lock (_frameLock)
                {
                    side.FrameWidth = width;
                    side.FrameHeight = height;
                    half.CopyTo(side.Half);
                }
            }
            catch (Exception)
            {
                return;     // the frame is dropped; the next one replaces it
            }

            if (Interlocked.CompareExchange(ref _updatePending, 1, 0) == 0)
            {
                Dispatcher.BeginInvoke(UpdateCompositeImage, DispatcherPriority.Background);
            }
        }

        // Composite image (UI thread)
        /////////////////////////////////////////////////////////////////////////////////////////////

        private void UpdateCompositeImage()
        {
            Interlocked.Exchange(ref _updatePending, 0);

            lock (_frameLock)
            {
                int topRows = _left.Half.Empty() ? 0 : _left.Half.Rows;
                int bottomRows = _right.Half.Empty() ? 0 : _right.Half.Rows;
                int width = Math.Max(_left.Half.Empty() ? 0 : _left.Half.Cols, _right.Half.Empty() ? 0 : _right.Half.Cols);
                if (width <= 0 || topRows + bottomRows <= 0)
                {
                    return;
                }

                // Until both cameras have delivered a frame, the missing half is laid out
                // with the other frame's half height so the view does not jump later.
                if (topRows == 0)
                {
                    topRows = _right.FrameHeight / 2;
                }
                if (bottomRows == 0)
                {
                    bottomRows = _left.FrameHeight - _left.FrameHeight / 2;
                }
                int height = topRows + bottomRows;

                if (_composite.Empty() || _composite.Rows != height || _composite.Cols != width)
                {
                    _composite.Dispose();
                    _composite = new Mat(height, width, MatType.CV_8UC1, BackgroundGray);
                }
                if (!_left.Half.Empty())
                {
                    using Mat target = new Mat(_composite, new OpenCvSharp.Rect(0, 0, _left.Half.Cols, _left.Half.Rows));
                    _left.Half.CopyTo(target);
                }
                if (!_right.Half.Empty())
                {
                    using Mat target = new Mat(_composite, new OpenCvSharp.Rect(0, topRows, _right.Half.Cols, _right.Half.Rows));
                    _right.Half.CopyTo(target);
                }

                _seamRow = topRows;
                _leftFrameHeight = _left.Half.Empty() ? 0 : _left.FrameHeight;
                _rightFrameHeight = _right.Half.Empty() ? 0 : _right.FrameHeight;
            }

            Mat shown = _composite;
            if (IsOverlayFocusGuide)
            {
                Cv2.CvtColor(_composite, _display, ColorConversionCodes.GRAY2BGR);
                DrawOverlayFocusGuide(_display);
                shown = _display;
            }

            BitmapSource source = BitmapSourceConverter.ToBitmapSource(shown);
            source.Freeze();
            imageViewport.Source = source;
        }

        // The side cameras' focus guide, as CorneaCameraPreview draws it on each camera: a
        // vertical centre line and horizontal lines at +/- (frame height / 12) around the
        // frame centre. The composite holds the left frame's upper half, so only the upper
        // line of the left camera is in view, just above the seam; and the right frame's
        // lower half, so only the lower line of the right camera, just below it.
        private void DrawOverlayFocusGuide(Mat image)
        {
            int width = image.Width;
            int height = image.Height;
            int centerX = width / 2;
            Cv2.Line(image, centerX, 0, centerX, height - 1, FocusGuideColor, 1, LineTypes.AntiAlias);

            if (_leftFrameHeight > 0)
            {
                int y = _seamRow - _leftFrameHeight / FocusGuideUnitDivisor;
                Cv2.Line(image, 0, y, width - 1, y, FocusGuideColor, 1, LineTypes.AntiAlias);
            }
            if (_rightFrameHeight > 0)
            {
                int y = _seamRow + _rightFrameHeight / FocusGuideUnitDivisor;
                Cv2.Line(image, 0, y, width - 1, y, FocusGuideColor, 1, LineTypes.AntiAlias);
            }
        }

        // Frame rate / status
        /////////////////////////////////////////////////////////////////////////////////////////////

        /// <summary>
        /// Ticks once per captured frame, before the coalescing gate, so the reported rate
        /// is the camera's and not the preview update rate. Native capture thread.
        /// </summary>
        private void MarkFrameCaptured(CameraSide side)
        {
            lock (_fpsLock)
            {
                if (!side.Interval.IsRunning)
                {
                    side.Interval.Restart();
                    return;
                }

                TimeSpan elapsed = side.Interval.Elapsed;
                side.Interval.Restart();
                if (elapsed >= FrameStaleAfter)
                {
                    side.HasFrameRate = false;      // the stream resumed after a pause: start over
                    return;
                }

                double ms = elapsed.TotalMilliseconds;
                if (ms > 0.0)
                {
                    double instant = 1000.0 / ms;
                    // Exponential moving average to smooth the reading.
                    side.FrameRate = side.HasFrameRate ? (side.FrameRate * FrameRateSmoothing + instant * (1.0 - FrameRateSmoothing)) : instant;
                    side.HasFrameRate = true;
                }
            }
        }

        private static void ResetFrameRate(CameraSide side)
        {
            side.Interval.Reset();
            side.FrameRate = 0.0;
            side.HasFrameRate = false;
        }

        private void UpdateStatus()
        {
            lblStatus.Content = $"{DescribeSide(_left)}   |   {DescribeSide(_right)}";
        }

        private string DescribeSide(CameraSide side)
        {
            int width;
            int height;
            lock (_frameLock)
            {
                width = side.FrameWidth;
                height = side.FrameHeight;
            }

            double fps;
            lock (_fpsLock)
            {
                // A camera that has stopped delivering reads 0 fps once its last frame is stale.
                bool live = side.HasFrameRate && side.Interval.IsRunning && side.Interval.Elapsed < FrameStaleAfter;
                fps = live ? side.FrameRate : 0.0;
            }

            if (width <= 0 || height <= 0)
            {
                return $"{side.Name}: no frame";
            }
            return string.Format("{0}: {1}x{2}, {3:F0}fps", side.Name, width, height, fps);
        }

        // Control lifetime
        /////////////////////////////////////////////////////////////////////////////////////////////

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateStatus();
            _statusTimer.Start();
            AttachJogHostWindow(System.Windows.Window.GetWindow(this));
        }

        private void UserControl_Unloaded(object sender, RoutedEventArgs e)
        {
            _statusTimer.Stop();
            StopStereoJog("unloaded");
            AttachJogHostWindow(null);
        }

        // Click-hold X/Y velocity jog, ported from CorneaCameraPreview (side-camera rules: X from
        // the horizontal, Y from the vertical click position; no Z, no wheel). UI thread only,
        // except the hold's stop action, which marshals to the dispatcher. _jogHolding is cleared
        // before ReleaseMouseCapture, which re-enters StopStereoJog through LostMouseCapture. The
        // device stop runs as the hold's stop action, which StageJog runs once per hold
        // (EndVelocityHold, watchdog or StopAll, whichever removes the hold), and once more when a
        // StopAll raced the start (StageJog.StartVelocityHold).
        /////////////////////////////////////////////////////////////////////////////////////////////
        private const string JogOwner = "stereo-cornea";
        private bool _jogHolding = false;
        private int _jogHoldGeneration = 0;
        private System.Windows.Window? _jogHostWindow;
        private readonly DispatcherTimer _jogFeedbackTimer = new();

        private void RegisterMouseEventHandlers()
        {
            _jogFeedbackTimer.Tick += (sender, e) =>
            {
                _jogFeedbackTimer.Stop();
                if (!_jogHolding)
                {
                    myJogOverlay.Children.Clear();
                    myJogBadge.Visibility = Visibility.Collapsed;
                }
            };

            imageViewport.MouseDown += (sender, e) =>
            {
                if (IsStageControlEnabled && e.ChangedButton == MouseButton.Left && !_jogHolding)
                {
                    StartStereoJog(e);
                }
            };

            imageViewport.MouseUp += (sender, e) =>
            {
                if (e.ChangedButton == MouseButton.Left)
                {
                    StopStereoJog("mouse up");
                }
            };

            imageViewport.LostMouseCapture += (sender, e) => StopStereoJog("lost capture");

            IsEnabledChanged += (sender, e) =>
            {
                if (!(bool)e.NewValue)
                {
                    StopStereoJog("disabled");
                }
            };

            IsVisibleChanged += (sender, e) =>
            {
                if (!(bool)e.NewValue)
                {
                    StopStereoJog("hidden");
                }
            };
        }

        private void StartStereoJog(MouseButtonEventArgs e)
        {
            if (imageViewport.ActualWidth <= 0 || imageViewport.ActualHeight <= 0)
            {
                return;
            }

            System.Windows.Point position = e.GetPosition(imageViewport);
            System.Windows.Point mark = e.GetPosition(myJogOverlay);
            double nx = position.X / imageViewport.ActualWidth;
            double ny = position.Y / imageViewport.ActualHeight;

            // Side-camera rules give Left/Right and Up/Down only: X and Y, never Z.
            LsoHeadMove move = LsoCorneaClickJog.Classify(true, nx, ny);
            if (move == LsoHeadMove.None)
            {
                ShowJogFeedback(mark, Brushes.Gray, LsoHeadMove.None, "no move", 600);
                return;
            }

            string moves = DescribeJogMove(move);
            JogAxis[] axes = JogAxesFor(move);
            foreach (JogAxis axis in axes)
            {
                if (!StageJog.IsAxisFree(axis, out string reason))
                {
                    RefuseStereoJog(mark, $"{moves} refused: {reason}");
                    return;
                }
            }

            if (!imageViewport.CaptureMouse())
            {
                RefuseStereoJog(mark, $"{moves} refused: no capture");
                return;
            }

            int generation = ++_jogHoldGeneration;
            _jogHolding = true;
            HoldStartResult result = StageJog.StartVelocityHold(
                JogOwner, () => StopHeadForJog(generation), () => StartHeadMoves(move), StopHeadWithRetry, axes);
            switch (result)
            {
                case HoldStartResult.Started:
                    break;
                case HoldStartResult.Busy:
                    _jogHolding = false;
                    imageViewport.ReleaseMouseCapture();
                    RefuseStereoJog(mark, $"{moves} refused: busy");
                    return;
                case HoldStartResult.Rejected:
                    EndStereoHold();
                    RefuseStereoJog(mark, $"{moves} rejected");
                    return;
                default:
                    // The second stop's result is the one that counts, so the update that the first stop
                    // posted is dropped (generation bump).
                    _jogHoldGeneration++;
                    StopStereoJog("raced stop all");
                    if (result == HoldStartResult.RacedStopFailed)
                    {
                        ShowStopFailed();
                    }
                    return;
            }

            imageViewport.Cursor = Cursors.Hand;
            ShowJogFeedback(mark, Brushes.LimeGreen, move, moves, 0);
            Console.WriteLine($"[LSO-JOG] stereo cornea {moves}");
        }

        private void StopStereoJog(string why)
        {
            if (!_jogHolding)
            {
                return;
            }

            EndStereoHold();
            Console.WriteLine($"[LSO-JOG] stereo cornea stop ({why})");
            ShowJogFeedback(null, Brushes.Gray, LsoHeadMove.None, "Stopped", 800);
        }

        private void EndStereoHold()
        {
            _jogHolding = false;
            StageJog.EndVelocityHold(JogOwner);
            if (imageViewport.IsMouseCaptured)
            {
                imageViewport.ReleaseMouseCapture();
            }
            imageViewport.Cursor = null;
        }

        private void RefuseStereoJog(System.Windows.Point mark, string text)
        {
            Console.WriteLine($"[LSO-JOG] stereo cornea {text}");
            ShowJogFeedback(mark, Brushes.Red, LsoHeadMove.None, text, 800);
        }

        // The hold's stop action. Runs on the UI thread or on the StageJog watchdog timer thread: device calls,
        // log and BeginInvoke only (never Invoke), so a StopAll from elsewhere also clears this control's UI and
        // a failed stop (false twice, or a throw) is shown even after the UI already said "Stopped".
        private void StopHeadForJog(int generation)
        {
            bool stopped = false;
            try
            {
                stopped = StopHeadWithRetry();
            }
            finally
            {
                Dispatcher.BeginInvoke(() => OnStereoHoldEnded(generation, stopped));
            }
        }

        private static bool StopHeadWithRetry()
        {
            if (AlignStage.StopHeadXYZ())
            {
                return true;
            }
            Console.WriteLine("[LSO-JOG] stereo cornea StopHeadXYZ returned false, retrying once");
            if (AlignStage.StopHeadXYZ())
            {
                return true;
            }
            Console.WriteLine("[LSO-JOG] stereo cornea StopHeadXYZ returned false twice");
            return false;
        }

        // Ignored when a newer hold has started since (generation mismatch). StopStereoJog is a no-op when this
        // control already released the hold itself (_jogHolding false).
        private void OnStereoHoldEnded(int generation, bool stopped)
        {
            if (generation != _jogHoldGeneration)
            {
                return;
            }

            StopStereoJog("stop all");
            if (!stopped)
            {
                ShowStopFailed();
            }
        }

        // Stays up (no timeout) until the next feedback replaces it.
        private void ShowStopFailed()
        {
            ShowJogFeedback(null, Brushes.Red, LsoHeadMove.None, "STOP FAILED - press STOP ALL", 0);
        }

        private static bool StartHeadMoves(LsoHeadMove move)
        {
            if (move.HasFlag(LsoHeadMove.Left) && !AlignStage.MoveHeadLeft())
            {
                return false;
            }
            if (move.HasFlag(LsoHeadMove.Right) && !AlignStage.MoveHeadRight())
            {
                return false;
            }
            if (move.HasFlag(LsoHeadMove.Up) && !AlignStage.MoveHeadUp())
            {
                return false;
            }
            if (move.HasFlag(LsoHeadMove.Down) && !AlignStage.MoveHeadDown())
            {
                return false;
            }
            return true;
        }

        private static JogAxis[] JogAxesFor(LsoHeadMove move)
        {
            var axes = new List<JogAxis>();
            if ((move & (LsoHeadMove.Left | LsoHeadMove.Right)) != 0)
            {
                axes.Add(JogAxis.X);
            }
            if ((move & (LsoHeadMove.Up | LsoHeadMove.Down)) != 0)
            {
                axes.Add(JogAxis.Y);
            }
            return axes.ToArray();
        }

        private static string DescribeJogMove(LsoHeadMove move)
        {
            var parts = new List<string>();
            if (move.HasFlag(LsoHeadMove.Left)) parts.Add("X Left");
            if (move.HasFlag(LsoHeadMove.Right)) parts.Add("X Right");
            if (move.HasFlag(LsoHeadMove.Up)) parts.Add("Y Up");
            if (move.HasFlag(LsoHeadMove.Down)) parts.Add("Y Down");
            return string.Join("  ", parts);
        }

        // durationMs 0 keeps the feedback until the next ShowJogFeedback (used while holding).
        private void ShowJogFeedback(System.Windows.Point? mark, Brush brush, LsoHeadMove arrows, string text, int durationMs)
        {
            myJogOverlay.Children.Clear();
            if (mark is System.Windows.Point p)
            {
                const double radius = 9;
                var ring = new Ellipse { Width = radius * 2, Height = radius * 2, Stroke = brush, StrokeThickness = 2 };
                Canvas.SetLeft(ring, p.X - radius);
                Canvas.SetTop(ring, p.Y - radius);
                myJogOverlay.Children.Add(ring);

                if (arrows.HasFlag(LsoHeadMove.Left)) AddJogArrow(p, -1, 0, brush);
                if (arrows.HasFlag(LsoHeadMove.Right)) AddJogArrow(p, 1, 0, brush);
                if (arrows.HasFlag(LsoHeadMove.Up)) AddJogArrow(p, 0, -1, brush);
                if (arrows.HasFlag(LsoHeadMove.Down)) AddJogArrow(p, 0, 1, brush);
            }

            myJogBadge.Text = text;
            myJogBadge.Visibility = Visibility.Visible;
            _jogFeedbackTimer.Stop();
            if (durationMs > 0)
            {
                _jogFeedbackTimer.Interval = TimeSpan.FromMilliseconds(durationMs);
                _jogFeedbackTimer.Start();
            }
        }

        private void AddJogArrow(System.Windows.Point p, double dx, double dy, Brush brush)
        {
            var tail = new System.Windows.Point(p.X + dx * 12, p.Y + dy * 12);
            var tip = new System.Windows.Point(p.X + dx * 30, p.Y + dy * 30);
            var wing1 = new System.Windows.Point(tip.X - dx * 6 - dy * 5, tip.Y - dy * 6 + dx * 5);
            var wing2 = new System.Windows.Point(tip.X - dx * 6 + dy * 5, tip.Y - dy * 6 - dx * 5);
            myJogOverlay.Children.Add(new Polyline
            {
                Stroke = brush,
                StrokeThickness = 2,
                Points = new PointCollection { tail, tip, wing1, tip, wing2 },
            });
        }

        private void AttachJogHostWindow(System.Windows.Window? window)
        {
            if (ReferenceEquals(window, _jogHostWindow))
            {
                return;
            }
            if (_jogHostWindow != null)
            {
                _jogHostWindow.Deactivated -= OnJogHostWindowDeactivated;
            }
            _jogHostWindow = window;
            if (_jogHostWindow != null)
            {
                _jogHostWindow.Deactivated += OnJogHostWindowDeactivated;
            }
        }

        private void OnJogHostWindowDeactivated(object? sender, EventArgs e)
        {
            StopStereoJog("window deactivated");
        }
    }
}
