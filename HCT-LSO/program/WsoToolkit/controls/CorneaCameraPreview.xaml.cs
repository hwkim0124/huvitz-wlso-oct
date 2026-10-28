using OpenCvSharp;
using OpenCvSharp.WpfExtensions;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;

namespace WsoToolkit.controls
{
    using static WsoNativeLib.CorneaCamera;

    public partial class CorneaCameraPreview : UserControl
    {
        Mat _imageMat = new();
        Mat _frameMat = new();

        int _imageWidth = 640;
        int _imageHeight = 480;
        long _frameCount = 0;

        int _viewportWidth = 640;
        int _viewportHeight = 480;
        const int AVERAGE_SIZE = 90;

        Stopwatch _stopwatch = new();
        private readonly object _syncLock = new object();

        // Frame rate measured from the actual capture-callback rate (see MarkFrameCaptured),
        // ticked before the UI-side frame coalescing so it reflects the true camera rate
        // rather than the throttled preview-update rate. Guarded because MarkFrameCaptured
        // runs on the native callback thread while the status label is read on the UI thread.
        private readonly Stopwatch _captureStopwatch = new();
        private readonly object _fpsLock = new object();
        private double _captureFrameRate = 0.0;
        private bool _hasCaptureFrameRate = false;

        public bool IsStretchToFit { get; set; } = true;
        public bool IsOverlayAlignGuide { get; set; } = false;
        public bool IsOverlayFocusGuide { get; set; } = false;
        public bool IsStageControlEnabled { get; set; } = false;

        // Draws the pupil the eye aligner currently detects in this camera. The position
        // is polled from the native side on every redraw (EyeAlign.GetPupilPosition), so
        // nothing is pushed into this control from a native thread.
        public bool IsOverlayPupilMark { get; set; } = false;

        // Radius reported with a Ctrl+click pupil override (test hook, image pixels).
        const float PupilOverrideRadius = 30.0f;

        public WsoDevice.CameraType CameraType { get; set; } = WsoDevice.CameraType.IrCorneaLeft;
        public WsoCallback.CorneaCameraFrameCaptured? Callback { get; set; } = null;

        public CorneaCameraPreview()
        {
            InitializeComponent();
            RegisterMouseEventHandlers();
        }

        /// <summary>
        /// Tick once per actual captured frame, before any pending/coalescing gate, so the
        /// reported FPS reflects the true capture rate instead of the throttled update rate.
        /// Thread-safe; may be called from the native callback thread.
        /// </summary>
        public void MarkFrameCaptured()
        {
            lock (_fpsLock)
            {
                if (!_captureStopwatch.IsRunning)
                {
                    _captureStopwatch.Restart();
                    return;
                }

                double ms = _captureStopwatch.Elapsed.TotalMilliseconds;
                _captureStopwatch.Restart();
                if (ms > 0.0)
                {
                    double inst = 1000.0 / ms;
                    // Exponential moving average to smooth the reading.
                    _captureFrameRate = _hasCaptureFrameRate ? (_captureFrameRate * 0.9 + inst * 0.1) : inst;
                    _hasCaptureFrameRate = true;
                }
            }
        }

        private void UpdateCameraStatusItems()
        {
            _stopwatch.Stop();
            int frate = (int)(1000.0f / _stopwatch.ElapsedMilliseconds);
            frate = _frameCount == 1 ? 0 : frate;
            _stopwatch.Restart();

            // Prefer the true capture rate (ticked before UI-side coalescing) when available;
            // otherwise fall back to the preview-update interval measured above.
            lock (_fpsLock)
            {
                if (_hasCaptureFrameRate)
                {
                    frate = (int)Math.Round(_captureFrameRate);
                }
            }

            if (_frameCount > 1 && _frameCount % AVERAGE_SIZE != 0)
            {
                return;
            }

            var type = CameraType;
            float again = GetAnalogGain(type);
            float dgain = GetDigitalGain(type);

            string s = string.Format("{0}, {1}x{2}, {3:F0}fps", GetCameraTypeName(type), _imageWidth, _imageHeight, frate);
            s += string.Format(" - a/dgain: {0:F2}, {1:F2}", again, dgain);
            lblStatus.Content = s;
        }

        public string GetCameraName()
        {
            return GetCameraTypeName(CameraType);
        }

        public bool IsCorneaLeftPreview()
        {
            return CameraType == WsoDevice.CameraType.IrCorneaLeft;
        }

        public bool IsCorneaRightPreview()
        {
            return CameraType == WsoDevice.CameraType.IrCorneaRight;
        }

        public bool IsCorneaLowerPreview()
        {
            return CameraType == WsoDevice.CameraType.IrCorneaLower;
        }

        private string GetCameraTypeName(WsoDevice.CameraType type)
        {
            return type switch
            {
                WsoDevice.CameraType.IrCorneaLeft => "Left",
                WsoDevice.CameraType.IrCorneaRight => "Right",
                WsoDevice.CameraType.IrCorneaLower => "Lower",
                WsoDevice.CameraType.IrRetina => "Retina",
                _ => "Unknown"
            };
        }

        public void CallbackCorneaCameraFrame(nint data, int width, int height)
        {
            if (data == 0) return;

            // Stride for 8-bit grayscale. 
            nint step = width * 1;

            try
            {
                // Passing a nint to Mat.FromPixelData explicitly invokes the zero-copy mapping overload, 
                // creating a lightweight header over your camera buffer.
                using (Mat rawFrame = Mat.FromPixelData(height, width, MatType.CV_8UC1, data, step))
                {
                    lock (_syncLock)
                    {
                        _imageWidth = width;
                        _imageHeight = height;
                        rawFrame.CopyTo(_imageMat);
                    }
                    UpdateCameraStatusItems();
                    UpdateCameraFrameImage();
                }
            }
            catch (Exception e)
            {
                e.ToString();
            }
        }

        public void UpdateCameraFrameImage()
        {
            if (_imageMat.Empty())
            {
                return;
            }

            Cv2.CvtColor(_imageMat, _frameMat, ColorConversionCodes.GRAY2BGR);

            _viewportWidth = imageViewport.ActualWidth > 0 ? (int)imageViewport.ActualWidth : _viewportWidth;
            _viewportHeight = imageViewport.ActualHeight > 0 ? (int)imageViewport.ActualHeight : _viewportHeight;

            // Stretch to fit viewport dimensions
            if (_imageWidth != _viewportWidth || _imageHeight != _viewportHeight)
            {
                if (!IsStretchToFit)
                {
                    int sx = (_viewportWidth - _imageWidth) / 2;
                    int sy = (_viewportHeight - _imageHeight) / 2;

                    if (sx < 0 || sy < 0)
                    {
                        int w = Math.Min(_viewportWidth, _imageWidth);
                        int h = Math.Min(_viewportHeight, _imageHeight);
                        int ix = sx < 0 ? -sx : 0;
                        int iy = sy < 0 ? -sy : 0;
                        _frameMat = new Mat(_frameMat, new OpenCvSharp.Rect(ix, iy, w, h));
                    }
                    else
                    {
                        _frameMat = _imageMat.CopyMakeBorder(sy, sy, sx, sx, BorderTypes.Constant, new Scalar(64, 64, 64));
                    }
                }
            }

            DrawOverlayAlignGuide(ref _frameMat);
            DrawOverlayFocusGuide(ref _frameMat);
            DrawOverlayPupilMark(ref _frameMat);

            // imageViewport.Source = _frameMat.ToWriteableBitmap();
            // Convert to a WPF-consumable BitmapSource
            BitmapSource source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);

            // Freeze the bitmap to optimize cross-thread UI rendering performance
            source.Freeze();

            // Update the XAML Image element
            imageViewport.Source = source;
        }

        private void DrawOverlayFocusGuide(ref Mat image)
        {
            if (IsOverlayFocusGuide != true)
            {
                return;
            }

            var img_w = image.Width;
            var img_h = image.Height;
            var img_cx = img_w / 2;
            var img_cy = img_h / 2;
            var color = new Scalar(255, 255, 0);

            if (CameraType == WsoDevice.CameraType.IrCorneaLeft || CameraType == WsoDevice.CameraType.IrCorneaRight)
            {
                var unit_h = img_h / 12;
                Cv2.Line(image, 0, img_cy - unit_h * 1, img_w - 1, img_cy - unit_h * 1, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, 0, img_cy + unit_h * 1, img_w - 1, img_cy + unit_h * 1, color, 1, LineTypes.AntiAlias);

                Cv2.Line(image, img_cx, 0, img_cx, img_h - 1, color, 1, LineTypes.AntiAlias);
            }
            else if (CameraType == WsoDevice.CameraType.IrCorneaLower)
            {
                Cv2.Line(image, 0, img_cy, img_w-1, img_cy, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, img_cx, 0, img_cx, img_h-1, color, 1, LineTypes.AntiAlias);
            }
            return;
        }

        private void DrawOverlayPupilMark(ref Mat image)
        {
            if (IsOverlayPupilMark != true)
            {
                return;
            }
            if (!EyeAlign.GetPupilPosition(CameraType, out float cx, out float cy, out float radius, out float score))
            {
                return;
            }

            // Pupil coordinates are image pixels; the overlay canvas is the frame itself
            // when stretched to fit, otherwise the same pixel grid cropped/padded, so the
            // coordinates map 1:1 in both cases.
            var color = new Scalar(0, 255, 0);
            int x = (int)Math.Round(cx);
            int y = (int)Math.Round(cy);
            int r = Math.Max(3, (int)Math.Round(radius));
            int arm = r + 8;

            Cv2.Circle(image, x, y, r, color, 2, LineTypes.AntiAlias);
            Cv2.Line(image, x - arm, y, x + arm, y, color, 1, LineTypes.AntiAlias);
            Cv2.Line(image, x, y - arm, x, y + arm, color, 1, LineTypes.AntiAlias);
            Cv2.PutText(image, string.Format("{0:F2}", score), new OpenCvSharp.Point(x + arm + 2, y - 2),
                HersheyFonts.HersheySimplex, 0.4, color, 1, LineTypes.AntiAlias);
        }

        private void DrawOverlayAlignGuide(ref Mat image)
        {
            if (IsOverlayAlignGuide != true)
            {
                return;
            }

            var img_w = image.Width;
            var img_h = image.Height;
            var color = new Scalar(0, 255, 255);

            Cv2.Line(image, img_w / 2, 0, img_w / 2, img_h - 1, color, 1, LineTypes.AntiAlias);
            Cv2.Line(image, 0, img_h / 2, img_w - 1, img_h / 2, color, 1, LineTypes.AntiAlias);

            var cx = img_w / 2;
            var cy = img_h / 2;
            var unit = 80;
            var size = 7;
            for (int i = 1; i < 4; i++)
            {
                Cv2.Line(image, cx - unit * i, cy - size, cx - unit * i, cy + size, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, cx + unit * i, cy - size, cx + unit * i, cy + size, color, 1, LineTypes.AntiAlias);
            }
            for (int i = 1; i < 3; i++)
            {
                Cv2.Line(image, cx - size, cy - unit * i, cx + size, cy - unit * i, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, cx - size, cy + unit * i, cx + size, cy + unit * i, color, 1, LineTypes.AntiAlias);
            }

            unit = 10;
            size = 3;
            for (int i = 1; i < 32; i++)
            {
                Cv2.Line(image, cx - unit * i, cy - size, cx - unit * i, cy + size, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, cx + unit * i, cy - size, cx + unit * i, cy + size, color, 1, LineTypes.AntiAlias);
            }
            for (int i = 1; i < 24; i++)
            {
                Cv2.Line(image, cx - size, cy - unit * i, cx + size, cy - unit * i, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, cx - size, cy + unit * i, cx + size, cy + unit * i, color, 1, LineTypes.AntiAlias);
            }
        }

        private void UserControl_Unloaded(object sender, RoutedEventArgs e)
        {
            _stopwatch.Stop();
            StopCorneaJog("unloaded");
            AttachJogHostWindow(null);
        }

        public void SaveFrameImage(string filename = "", string dirname = ".\\cornea")
        {
            if (_imageMat.Empty())
            {
                return;
            }

            if (string.IsNullOrWhiteSpace(filename))
            {
                string timestamp = DateTime.Now.ToString("yyMMdd_HHmmss");
                filename = $"{GetCameraName()}_{timestamp}.png";
            }

            if (!Directory.Exists(dirname))
            {
                Directory.CreateDirectory(dirname);
            }
            string path = dirname + "\\" + filename;

            lock (_syncLock)
            {
                Cv2.ImWrite(path, _imageMat);
            }
            return;
        }

        // Cornea-click velocity jog. UI thread only, except the hold's stop action, which marshals to the
        // dispatcher. _jogHolding is cleared before ReleaseMouseCapture, which re-enters
        // StopCorneaJog through LostMouseCapture. The device stop runs as the hold's stop action, which
        // StageJog runs once per hold (EndVelocityHold, watchdog or StopAll, whichever removes the hold), and
        // once more when a StopAll raced the start (StageJog.StartVelocityHold).
        private const string JogOwner = "cornea";
        private bool _jogHolding = false;
        private int _jogHoldGeneration = 0;
        private int _jogWheelDelta = 0;
        private System.Windows.Window? _jogHostWindow;
        private readonly System.Windows.Threading.DispatcherTimer _jogFeedbackTimer = new();

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
                // Test hook for the eye aligner while the pupil detector is a stub:
                // Ctrl+left click = "the pupil is here", Ctrl+right click = clear.
                if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
                {
                    var click = e.GetPosition(imageViewport);
                    if (imageViewport.ActualWidth > 0 && imageViewport.ActualHeight > 0)
                    {
                        float ix = (float)(click.X / imageViewport.ActualWidth * _imageWidth);
                        float iy = (float)(click.Y / imageViewport.ActualHeight * _imageHeight);
                        if (e.ChangedButton == MouseButton.Left)
                        {
                            EyeAlign.SetPupilDetectOverride(CameraType, true, ix, iy, PupilOverrideRadius);
                        }
                        else if (e.ChangedButton == MouseButton.Right)
                        {
                            EyeAlign.ClearPupilDetectOverride(CameraType);
                        }
                    }
                    return;
                }

                if (IsStageControlEnabled && e.ChangedButton == MouseButton.Left && !_jogHolding)
                {
                    StartCorneaJog(e);
                }
            };

            imageViewport.MouseUp += (sender, e) =>
            {
                if (e.ChangedButton == MouseButton.Left)
                {
                    StopCorneaJog("mouse up");
                }
            };

            imageViewport.LostMouseCapture += (sender, e) => StopCorneaJog("lost capture");

            imageViewport.MouseWheel += (sender, e) =>
            {
                if (!IsStageControlEnabled)
                {
                    return;
                }
                StepCorneaWheel(e.Delta);
                e.Handled = true;
            };

            IsEnabledChanged += (sender, e) =>
            {
                if (!(bool)e.NewValue)
                {
                    StopCorneaJog("disabled");
                }
            };

            IsVisibleChanged += (sender, e) =>
            {
                if (!(bool)e.NewValue)
                {
                    StopCorneaJog("hidden");
                }
            };

            Loaded += (sender, e) =>
            {
                AttachJogHostWindow(System.Windows.Window.GetWindow(this));
            };
        }

        private void StartCorneaJog(MouseButtonEventArgs e)
        {
            System.Windows.Point position = e.GetPosition(imageViewport);
            System.Windows.Point mark = e.GetPosition(myJogOverlay);
            double nx = position.X / imageViewport.ActualWidth;
            double ny = position.Y / imageViewport.ActualHeight;

            LsoHeadMove move = LsoCorneaClickJog.Classify(IsCorneaLeftPreview() || IsCorneaRightPreview(), nx, ny);
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
                    RefuseCorneaJog(mark, $"{moves} refused: {reason}");
                    return;
                }
            }

            if (!imageViewport.CaptureMouse())
            {
                RefuseCorneaJog(mark, $"{moves} refused: no capture");
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
                    RefuseCorneaJog(mark, $"{moves} refused: busy");
                    return;
                case HoldStartResult.Rejected:
                    EndCorneaHold();
                    RefuseCorneaJog(mark, $"{moves} rejected");
                    return;
                default:
                    // The second stop's result is the one that counts, so the update that the first stop
                    // posted is dropped (generation bump).
                    _jogHoldGeneration++;
                    StopCorneaJog("raced stop all");
                    if (result == HoldStartResult.RacedStopFailed)
                    {
                        ShowStopFailed();
                    }
                    return;
            }

            imageViewport.Cursor = Cursors.Hand;
            ShowJogFeedback(mark, Brushes.LimeGreen, move, moves, 0);
            Console.WriteLine($"[LSO-JOG] cornea {moves}");
        }

        private void StopCorneaJog(string why)
        {
            if (!_jogHolding)
            {
                return;
            }

            EndCorneaHold();
            Console.WriteLine($"[LSO-JOG] cornea stop ({why})");
            ShowJogFeedback(null, Brushes.Gray, LsoHeadMove.None, "Stopped", 800);
        }

        private void EndCorneaHold()
        {
            _jogHolding = false;
            StageJog.EndVelocityHold(JogOwner);
            if (imageViewport.IsMouseCaptured)
            {
                imageViewport.ReleaseMouseCapture();
            }
            imageViewport.Cursor = null;
        }

        private void RefuseCorneaJog(System.Windows.Point mark, string text)
        {
            Console.WriteLine($"[LSO-JOG] cornea {text}");
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
                Dispatcher.BeginInvoke(() => OnCorneaHoldEnded(generation, stopped));
            }
        }

        private static bool StopHeadWithRetry()
        {
            if (AlignStage.StopHeadXYZ())
            {
                return true;
            }
            Console.WriteLine("[LSO-JOG] cornea StopHeadXYZ returned false, retrying once");
            if (AlignStage.StopHeadXYZ())
            {
                return true;
            }
            Console.WriteLine("[LSO-JOG] cornea StopHeadXYZ returned false twice");
            return false;
        }

        // Ignored when a newer hold has started since (generation mismatch). StopCorneaJog is a no-op when this
        // control already released the hold itself (_jogHolding false).
        private void OnCorneaHoldEnded(int generation, bool stopped)
        {
            if (generation != _jogHoldGeneration)
            {
                return;
            }

            StopCorneaJog("stop all");
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
            if (move.HasFlag(LsoHeadMove.Forward) && !AlignStage.MoveHeadForward())
            {
                return false;
            }
            if (move.HasFlag(LsoHeadMove.Backward) && !AlignStage.MoveHeadBackward())
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
            if ((move & (LsoHeadMove.Forward | LsoHeadMove.Backward)) != 0)
            {
                axes.Add(JogAxis.Z);
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
            if (move.HasFlag(LsoHeadMove.Forward)) parts.Add("Z Fwd");
            if (move.HasFlag(LsoHeadMove.Backward)) parts.Add("Z Back");
            return string.Join("  ", parts);
        }

        // One Fine Z step per wheel event once the accumulated delta reaches a notch (120), so a coalesced
        // multi-notch delta still sends one step; a reversal drops the leftover. A wheel event during a click
        // hold is ignored: the hold owns the head until it stops.
        private void StepCorneaWheel(int delta)
        {
            if (delta == 0 || _jogHolding)
            {
                return;
            }

            if (Math.Sign(delta) != Math.Sign(_jogWheelDelta))
            {
                _jogWheelDelta = 0;
            }
            _jogWheelDelta += delta;
            if (Math.Abs(_jogWheelDelta) < Mouse.MouseWheelDeltaForOneLine)
            {
                return;
            }

            int direction = Math.Sign(_jogWheelDelta);
            _jogWheelDelta %= Mouse.MouseWheelDeltaForOneLine;
            string text = StageJog.TryStep(JogAxis.Z, LsoJogSize.Fine, direction, out int sent, out string reason)
                ? $"Z {sent:+0;-0}"
                : $"Z refused: {reason}";
            ShowJogFeedback(null, Brushes.Gray, LsoHeadMove.None, text, 900);
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
                if (arrows.HasFlag(LsoHeadMove.Up) || arrows.HasFlag(LsoHeadMove.Forward)) AddJogArrow(p, 0, -1, brush);
                if (arrows.HasFlag(LsoHeadMove.Down) || arrows.HasFlag(LsoHeadMove.Backward)) AddJogArrow(p, 0, 1, brush);
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
            StopCorneaJog("window deactivated");
        }
    }
}
