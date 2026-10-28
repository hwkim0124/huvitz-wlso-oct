using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;
using WsoNativeLib.Presets;

namespace WsoToolkit.controls
{
    // Internal fixation marker on the live preview: a circle and a cross on the LCD target the
    // patient looks at, with its "row,col" beside it. The host owns the device and reports every
    // position it sets through SetFixationPosition (UI thread); this control only draws.
    //
    // LCD row/col -> displayed frame pixels through LsoFixationOverlayMap, re-read from
    // [FIXATION_OVERLAY] of the device INI each time the toggle is switched on (never written
    // here), then -> viewport space through the Image element as the Zoom Area ROI does, so the
    // marker stays on the same image point under zoom/pan and its strokes keep their width.
    // Hidden while the toggle is off, in review, and with no frame on screen.
    //
    // Grab drag (live, marker shown): a left press within FixationGrabRadius of the marker starts the
    // drag at once (the press is handled, so the Zoom Area selection does not start under it).
    //
    // Long-press drag (live, marker shown): elsewhere, a left press held for LsoLongPressGesture.HoldMs within
    // SlopDip grabs the marker; until then nothing is captured or handled, so a short click, the
    // pan and the Zoom Area handlers see the press as before. While dragging the marker shows the
    // pointer, FixationDragRequested asks the host for a send at most once per SendIntervalMs, and
    // a release asks for the final position. Esc (host window) asks for the pre-drag position; a
    // lost capture ends the drag with no request, so the last position sent stays. A position
    // equal to the last one requested is never requested again.
    public partial class LsoScanImagePreview
    {
        private const double FixationMarkerRadius = 10.0;
        private const double FixationMarkerCrossSize = 16.0;
        private const double FixationMarkerStroke = 2.0;
        private const double FixationLabelGap = 4.0;
        private const double FixationGrabRadius = FixationMarkerRadius + 8.0;

        private static readonly Brush FixationMarkerBrush = Brushes.Lime;

        private LsoFixationOverlayMap _fixationMap = LsoFixationOverlayMap.Default;
        private bool _hasFixationPosition;
        private int _fixationRow;
        private int _fixationCol;

        private static readonly TimeSpan FixationHoldPollInterval = TimeSpan.FromMilliseconds(50);

        private readonly LsoLongPressGesture _fixationGesture = new();
        private DispatcherTimer? _fixationHoldTimer;
        private bool _fixationArmed;
        private bool _fixationDragging;
        private int _fixationDragRow;
        private int _fixationDragCol;
        private int _fixationPreRow;
        private int _fixationPreCol;
        private int _fixationSentRow;
        private int _fixationSentCol;
        private Window? _fixationHostWindow;

        /// <summary>Raised on the UI thread while the marker is dragged: (row, col, isFinal). The
        /// host sends it and reports the device position back through SetFixationPosition.</summary>
        public event Action<int, int, bool>? FixationDragRequested;

        public bool IsFixationOverlayOn => myToggleFixationOverlay.IsChecked == true;

        /// <summary>The LCD position the fixation target was last set to (UI thread). Shown once
        /// a drag in progress ends.</summary>
        public void SetFixationPosition(int row, int col)
        {
            _fixationRow = row;
            _fixationCol = col;
            _hasFixationPosition = true;
            drawFixationOverlay_();
        }

        // Checked/Unchecked rather than Click, so mouse, keyboard and UI Automation toggles behave the same.
        private void myToggleFixationOverlay_Checked(object sender, RoutedEventArgs e)
        {
            _fixationMap = LsoFixationOverlayMap.Load(LsoConfigLocation.DeviceConfigPath());
            drawFixationOverlay_();
        }

        private void myToggleFixationOverlay_Unchecked(object sender, RoutedEventArgs e)
        {
            drawFixationOverlay_();
        }

        private void myFixationOverlay_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            drawFixationOverlay_();
        }

        #region Mapping

        // LCD row/col -> overlay point: zoomAreaOverlayAt_ with the fixation map in front.
        // Null when no frame is laid out.
        private Point? fixationOverlayAt_(int row, int col)
        {
            if (fixationFrame_() is not BitmapSource frame)
            {
                return null;
            }

            (double x, double y) = _fixationMap.ToImage(row, col, frame.PixelWidth, frame.PixelHeight);
            var local = new Point(x / frame.PixelWidth * imageViewport.ActualWidth,
                y / frame.PixelHeight * imageViewport.ActualHeight);
            return imageViewport.TranslatePoint(local, myFixationOverlay);
        }

        // Overlay point -> LCD row/col, the exact inverse (rounded, clamped to the grid), for
        // picking a fixation position on the image. Null when no frame is laid out.
        private (int Row, int Col)? fixationLcdAt_(Point overlayPoint)
        {
            if (fixationFrame_() is not BitmapSource frame)
            {
                return null;
            }

            Point local = myFixationOverlay.TranslatePoint(overlayPoint, imageViewport);
            return _fixationMap.ToLcd(local.X / imageViewport.ActualWidth * frame.PixelWidth,
                local.Y / imageViewport.ActualHeight * frame.PixelHeight,
                frame.PixelWidth, frame.PixelHeight);
        }

        private BitmapSource? fixationFrame_()
        {
            return imageViewport.Source is BitmapSource frame && frame.PixelWidth > 0 && frame.PixelHeight > 0
                && imageViewport.ActualWidth > 0 && imageViewport.ActualHeight > 0
                ? frame
                : null;
        }

        #endregion Mapping

        #region Drawing

        private void drawFixationOverlay_()
        {
            Canvas canvas = myFixationOverlay;
            canvas.Children.Clear();

            int row = _fixationDragging ? _fixationDragRow : _fixationRow;
            int col = _fixationDragging ? _fixationDragCol : _fixationCol;
            Point? at = IsFixationOverlayOn && !IsReviewMode && _hasFixationPosition
                ? fixationOverlayAt_(row, col)
                : null;
            if (at is not Point p)
            {
                canvas.Visibility = Visibility.Collapsed;
                return;
            }

            canvas.Visibility = Visibility.Visible;

            var circle = new Ellipse
            {
                Width = FixationMarkerRadius * 2.0,
                Height = FixationMarkerRadius * 2.0,
                Stroke = FixationMarkerBrush,
                StrokeThickness = FixationMarkerStroke,
                IsHitTestVisible = false,
            };
            Canvas.SetLeft(circle, p.X - FixationMarkerRadius);
            Canvas.SetTop(circle, p.Y - FixationMarkerRadius);
            canvas.Children.Add(circle);

            double arm = FixationMarkerCrossSize / 2.0;
            addFixationLine_(p.X - arm, p.Y, p.X + arm, p.Y);
            addFixationLine_(p.X, p.Y - arm, p.X, p.Y + arm);

            addFixationLabel_(p, $"{row},{col}");
        }

        private void addFixationLine_(double x1, double y1, double x2, double y2)
        {
            myFixationOverlay.Children.Add(new Line
            {
                X1 = x1, Y1 = y1, X2 = x2, Y2 = y2,
                Stroke = FixationMarkerBrush,
                StrokeThickness = FixationMarkerStroke,
                IsHitTestVisible = false,
            });
        }

        // The Zoom Area tag plate, right of the circle (left of it where the right side would leave
        // the view). Not clamped into the view, so a marker panned out of sight takes it along.
        private void addFixationLabel_(Point p, string text)
        {
            var tag = new Border
            {
                Background = ZoomAreaTagBackground,
                BorderBrush = FixationMarkerBrush,
                BorderThickness = new Thickness(1),
                Padding = new Thickness(4, 1, 4, 1),
                IsHitTestVisible = false,
                Child = new TextBlock { Text = text, Foreground = FixationMarkerBrush, FontSize = 12, FontWeight = FontWeights.Bold },
            };
            tag.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
            Size size = tag.DesiredSize;

            double offset = FixationMarkerRadius + FixationLabelGap;
            double x = p.X + offset;
            if (x + size.Width > myViewportHost.ActualWidth)
            {
                x = p.X - offset - size.Width;
            }

            Canvas.SetLeft(tag, x);
            Canvas.SetTop(tag, p.Y - (size.Height / 2.0));
            myFixationOverlay.Children.Add(tag);
        }

        #endregion Drawing

        #region Drag

        // From UserControl_PreviewMouseDown: a press on the marker grabs it at once (captured and
        // handled); anywhere else it arms the long press and never captures or handles.
        private void armFixationDrag_(MouseButtonEventArgs e)
        {
            if (e.ChangedButton != MouseButton.Left || (Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control
                || !IsFixationOverlayOn || IsReviewMode || !_hasFixationPosition || fixationFrame_() == null
                || mySlitOverlay.IsMeasuring || _isPanning || _fixationArmed || _fixationDragging || IsMouseCaptured
                || _zoomAreaSelecting || _zoomAreaDragging
                || (_zoomAreaActive && isOnZoomAreaHandle_(e.GetPosition(myZoomAreaOverlay))))
            {
                return;
            }

            Point p = e.GetPosition(myFixationOverlay);
            _fixationGesture.Press(p.X, p.Y, Environment.TickCount64);
            if (fixationOverlayAt_(_fixationRow, _fixationCol) is Point marker
                && (p - marker).Length <= FixationGrabRadius)
            {
                startFixationDrag_(p);
                e.Handled = _fixationDragging;
                return;
            }

            _fixationArmed = true;
            if (_fixationHoldTimer == null)
            {
                _fixationHoldTimer = new DispatcherTimer(DispatcherPriority.Input) { Interval = FixationHoldPollInterval };
                _fixationHoldTimer.Tick += fixationHoldTimer_Tick;
            }
            _fixationHoldTimer.Start();
        }

        // Nothing is captured while armed, so a release or a move away outside this control sends
        // no event here; the button state and the pointer are read on every tick instead.
        private void fixationHoldTimer_Tick(object? sender, EventArgs e)
        {
            if (!_fixationArmed)
            {
                disarmFixationDrag_();
                return;
            }

            long now = Environment.TickCount64;
            Point p = Mouse.GetPosition(myFixationOverlay);
            if (Mouse.LeftButton != MouseButtonState.Pressed || IsMouseCaptured
                || _fixationGesture.Move(p.X, p.Y, now) == LsoLongPressEvent.Cancelled)
            {
                disarmFixationDrag_();
                return;
            }

            if (_fixationGesture.Tick(now) == LsoLongPressEvent.DragStart)
            {
                startFixationDrag_(p);
            }
        }

        private void disarmFixationDrag_()
        {
            _fixationArmed = false;
            _fixationHoldTimer?.Stop();
            if (!_fixationDragging)
            {
                _fixationGesture.Release();
            }
        }

        private void startFixationDrag_(Point p)
        {
            _fixationArmed = false;
            _fixationHoldTimer?.Stop();
            if (!CaptureMouse())
            {
                _fixationGesture.Release();
                return;
            }

            _fixationDragging = true;
            _fixationPreRow = _fixationRow;
            _fixationPreCol = _fixationCol;
            _fixationSentRow = _fixationRow;
            _fixationSentCol = _fixationCol;
            _fixationDragRow = _fixationRow;
            _fixationDragCol = _fixationCol;
            Cursor = Cursors.SizeAll;

            // The viewport is click-through and this control takes no keyboard focus, so Esc is
            // read on the host window, as the Zoom Area selection does.
            _fixationHostWindow = Window.GetWindow(this);
            if (_fixationHostWindow != null)
            {
                _fixationHostWindow.PreviewKeyDown += fixationHostKeyDown_;
            }

            showFixationDragAt_(p);
        }

        // True when the move belongs to a drag in progress (and is consumed). A press still
        // waiting for the hold leaves the move to the other handlers.
        private bool fixationMouseMove_(MouseEventArgs e)
        {
            if (_fixationArmed)
            {
                Point p = e.GetPosition(myFixationOverlay);
                if (_fixationGesture.Move(p.X, p.Y, Environment.TickCount64) == LsoLongPressEvent.Cancelled)
                {
                    disarmFixationDrag_();
                }
                return false;
            }

            if (!_fixationDragging)
            {
                return false;
            }

            if (!IsFixationOverlayOn || IsReviewMode)
            {
                endFixationDrag_();
            }
            else if (showFixationDragAt_(e.GetPosition(myFixationOverlay))
                && (_fixationDragRow != _fixationSentRow || _fixationDragCol != _fixationSentCol)
                && _fixationGesture.ShouldSend(Environment.TickCount64))
            {
                _fixationSentRow = _fixationDragRow;
                _fixationSentCol = _fixationDragCol;
                FixationDragRequested?.Invoke(_fixationDragRow, _fixationDragCol, false);
            }

            e.Handled = true;
            return true;
        }

        // True when the release ends a drag (and is consumed). A short press only disarms.
        private bool fixationMouseUp_(MouseButtonEventArgs e)
        {
            if (e.ChangedButton != MouseButton.Left)
            {
                return false;
            }

            if (_fixationArmed)
            {
                disarmFixationDrag_();
                return false;
            }

            if (!_fixationDragging)
            {
                return false;
            }

            finishFixationDrag_(_fixationDragRow, _fixationDragCol);
            e.Handled = true;
            return true;
        }

        private void fixationHostKeyDown_(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape && _fixationDragging)
            {
                finishFixationDrag_(_fixationPreRow, _fixationPreCol);
                e.Handled = true;
            }
        }

        // A dialog or Alt+Tab took the mouse: no request, the last position sent stays.
        private void fixationLostCapture_()
        {
            if (_fixationDragging)
            {
                endFixationDrag_();
            }
        }

        // The marker follows the pointer; false when no frame maps the point.
        private bool showFixationDragAt_(Point overlayPoint)
        {
            if (fixationLcdAt_(overlayPoint) is not { } lcd)
            {
                return false;
            }

            if (lcd.Row != _fixationDragRow || lcd.Col != _fixationDragCol)
            {
                _fixationDragRow = lcd.Row;
                _fixationDragCol = lcd.Col;
                drawFixationOverlay_();
            }
            return true;
        }

        private void finishFixationDrag_(int row, int col)
        {
            bool changed = row != _fixationSentRow || col != _fixationSentCol;
            endFixationDrag_();
            if (changed)
            {
                FixationDragRequested?.Invoke(row, col, true);
            }
        }

        // Clears the drag before releasing the capture, so the LostMouseCapture it raises is a no-op.
        private void endFixationDrag_()
        {
            _fixationDragging = false;
            _fixationGesture.Release();
            if (_fixationHostWindow != null)
            {
                _fixationHostWindow.PreviewKeyDown -= fixationHostKeyDown_;
                _fixationHostWindow = null;
            }

            ClearValue(CursorProperty);
            if (IsMouseCaptured)
            {
                ReleaseMouseCapture();
            }

            drawFixationOverlay_();
        }

        #endregion Drag
    }
}
