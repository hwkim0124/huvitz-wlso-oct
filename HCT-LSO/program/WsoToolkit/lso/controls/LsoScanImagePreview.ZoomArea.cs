using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace WsoToolkit.controls
{
    // Zoom Area ROI on the main preview (DevKit LsoScanFormUnified "Custom Zoom Area Selection").
    //   Select Area      next left drag draws the ROI (>= 8 DIPs each way), right-click / Esc cancels
    //   drag the label   (or the top edge) moves the ROI; right-click while dragging restores it
    // The ROI is kept in pixels of the displayed bitmap and always clamped inside it. Input is
    // read from this control's Preview mouse events after the zoom/pan handlers, so Ctrl+drag,
    // Ctrl+wheel and middle-click keep their meaning, and nothing here runs while a slit
    // measurement owns the clicks.
    public partial class LsoScanImagePreview
    {
        private const int ZoomAreaDefaultRoiSize = 512;
        private const double ZoomAreaMinSelectDip = 8.0;
        private const double ZoomAreaEdgeHitDip = 6.0;

        private static readonly Brush ZoomAreaStroke = freeze_(new SolidColorBrush(Color.FromArgb(230, 255, 180, 0)));
        private static readonly Brush ZoomAreaStrokeActive = Brushes.Gold;
        private static readonly Brush ZoomAreaFill = freeze_(new SolidColorBrush(Color.FromArgb(45, 255, 200, 0)));
        private static readonly Brush ZoomAreaTagBackground = freeze_(new SolidColorBrush(Color.FromArgb(200, 20, 20, 24)));

        private bool _zoomAreaActive;
        private bool _zoomAreaSelecting;
        private bool _zoomAreaDragging;
        private bool _zoomAreaHover;
        private Point _zoomAreaPress;
        private Rect? _zoomAreaRubber;
        private bool _zoomAreaHasRoi;
        private Int32Rect _zoomAreaRoi;
        private Int32Rect _zoomAreaDragStartRoi;
        private Rect? _zoomAreaTagRect;
        private Window? _zoomAreaHostWindow;

        /// <summary>Raised on the UI thread when the displayed frame or the ROI changes.</summary>
        public event Action? ZoomAreaChanged;

        /// <summary>Optional deeper-bit-depth copy of the frame on screen (e.g. Gray16/Rgb48).
        /// Used only when it has exactly the displayed bitmap's size; otherwise the 8-bit display
        /// bitmap is cropped.</summary>
        public Func<BitmapSource?>? ZoomAreaFrameProvider { get; set; }

        public Int32Rect? ZoomAreaRoi => _zoomAreaHasRoi ? _zoomAreaRoi : null;

        public bool IsZoomAreaSelecting => _zoomAreaSelecting;

        public void ShowZoomArea()
        {
            if (_zoomAreaActive)
            {
                return;
            }

            _zoomAreaActive = true;
            AddHandler(PreviewMouseDownEvent, new MouseButtonEventHandler(zoomAreaMouseDown_));
            AddHandler(PreviewMouseMoveEvent, new MouseEventHandler(zoomAreaMouseMove_));
            AddHandler(PreviewMouseUpEvent, new MouseButtonEventHandler(zoomAreaMouseUp_));
            AddHandler(LostMouseCaptureEvent, new MouseEventHandler(zoomAreaLostCapture_));
            DependencyPropertyDescriptor.FromProperty(Image.SourceProperty, typeof(Image))
                .AddValueChanged(imageViewport, zoomAreaSourceChanged_);
            myZoomScale.Changed += zoomAreaLayoutChanged_;
            myZoomPan.Changed += zoomAreaLayoutChanged_;
            imageViewport.SizeChanged += zoomAreaSizeChanged_;
            myZoomAreaOverlay.SizeChanged += zoomAreaSizeChanged_;

            _zoomAreaHostWindow = Window.GetWindow(this);
            if (_zoomAreaHostWindow != null)
            {
                _zoomAreaHostWindow.PreviewKeyDown += zoomAreaKeyDown_;
            }

            if (!_zoomAreaHasRoi)
            {
                ResetZoomAreaRoi();
            }

            myZoomAreaOverlay.Visibility = Visibility.Visible;
            redrawZoomArea_();
            ZoomAreaChanged?.Invoke();
        }

        public void HideZoomArea()
        {
            if (!_zoomAreaActive)
            {
                return;
            }

            endZoomAreaGesture_();
            _zoomAreaSelecting = false;
            _zoomAreaActive = false;
            RemoveHandler(PreviewMouseDownEvent, new MouseButtonEventHandler(zoomAreaMouseDown_));
            RemoveHandler(PreviewMouseMoveEvent, new MouseEventHandler(zoomAreaMouseMove_));
            RemoveHandler(PreviewMouseUpEvent, new MouseButtonEventHandler(zoomAreaMouseUp_));
            RemoveHandler(LostMouseCaptureEvent, new MouseEventHandler(zoomAreaLostCapture_));
            DependencyPropertyDescriptor.FromProperty(Image.SourceProperty, typeof(Image))
                .RemoveValueChanged(imageViewport, zoomAreaSourceChanged_);
            myZoomScale.Changed -= zoomAreaLayoutChanged_;
            myZoomPan.Changed -= zoomAreaLayoutChanged_;
            imageViewport.SizeChanged -= zoomAreaSizeChanged_;
            myZoomAreaOverlay.SizeChanged -= zoomAreaSizeChanged_;

            if (_zoomAreaHostWindow != null)
            {
                _zoomAreaHostWindow.PreviewKeyDown -= zoomAreaKeyDown_;
                _zoomAreaHostWindow = null;
            }

            myZoomAreaOverlay.Children.Clear();
            myZoomAreaOverlay.Visibility = Visibility.Collapsed;
            ClearValue(CursorProperty);
        }

        public void StartZoomAreaSelection()
        {
            if (!_zoomAreaActive)
            {
                return;
            }

            endZoomAreaGesture_();
            _zoomAreaSelecting = true;
            Cursor = Cursors.Cross;
            redrawZoomArea_();
        }

        public void CancelZoomAreaSelection()
        {
            endZoomAreaGesture_();
            _zoomAreaSelecting = false;
            ClearValue(CursorProperty);
            redrawZoomArea_();
        }

        /// <summary>A centred square of min(512, width, height) pixels.</summary>
        public void ResetZoomAreaRoi()
        {
            if (zoomAreaDisplayFrame_() is not BitmapSource frame)
            {
                _zoomAreaHasRoi = false;
                return;
            }

            int size = Math.Min(ZoomAreaDefaultRoiSize, Math.Min(frame.PixelWidth, frame.PixelHeight));
            _zoomAreaRoi = new Int32Rect((frame.PixelWidth - size) / 2, (frame.PixelHeight - size) / 2, size, size);
            _zoomAreaHasRoi = size > 0;
            redrawZoomArea_();
            ZoomAreaChanged?.Invoke();
        }

        /// <summary>Copies the ROI out of the current frame at native resolution (frozen).</summary>
        public BitmapSource? CropZoomArea(out Int32Rect roi)
        {
            roi = Int32Rect.Empty;
            BitmapSource? frame = zoomAreaSourceFrame_();
            if (frame == null || !_zoomAreaHasRoi)
            {
                return null;
            }

            roi = clampZoomAreaRoi_(_zoomAreaRoi, frame.PixelWidth, frame.PixelHeight);
            if (roi.Width <= 0 || roi.Height <= 0)
            {
                return null;
            }

            int stride = ((roi.Width * frame.Format.BitsPerPixel) + 7) / 8;
            byte[] pixels = new byte[stride * roi.Height];
            frame.CopyPixels(roi, pixels, stride, 0);
            BitmapSource crop = BitmapSource.Create(roi.Width, roi.Height, 96.0, 96.0, frame.Format, frame.Palette, pixels, stride);
            crop.Freeze();
            return crop;
        }

        private BitmapSource? zoomAreaDisplayFrame_() => imageViewport.Source as BitmapSource;

        private BitmapSource? zoomAreaSourceFrame_()
        {
            BitmapSource? display = zoomAreaDisplayFrame_();
            if (display != null && ZoomAreaFrameProvider?.Invoke() is BitmapSource deep
                && deep.PixelWidth == display.PixelWidth && deep.PixelHeight == display.PixelHeight)
            {
                return deep;
            }

            return display;
        }

        private static Int32Rect clampZoomAreaRoi_(Int32Rect roi, int width, int height)
        {
            int w = Math.Clamp(roi.Width, 1, Math.Max(1, width));
            int h = Math.Clamp(roi.Height, 1, Math.Max(1, height));
            int x = Math.Clamp(roi.X, 0, Math.Max(0, width - w));
            int y = Math.Clamp(roi.Y, 0, Math.Max(0, height - h));
            return new Int32Rect(x, y, w, h);
        }

        #region Mapping

        // Overlay point <-> bitmap pixel through the Image element (as SlitMeasureOverlay does):
        // TranslatePoint folds in the letterbox and the preview zoom/pan; the DIP size of the
        // element is converted with the bitmap's pixel size, so a non-96-DPI source maps too.
        private Point? zoomAreaPixelAt_(Point overlayPoint)
        {
            if (zoomAreaDisplayFrame_() is not BitmapSource frame
                || imageViewport.ActualWidth <= 0 || imageViewport.ActualHeight <= 0)
            {
                return null;
            }

            Point local = myZoomAreaOverlay.TranslatePoint(overlayPoint, imageViewport);
            return new Point(local.X / imageViewport.ActualWidth * frame.PixelWidth,
                local.Y / imageViewport.ActualHeight * frame.PixelHeight);
        }

        private Point zoomAreaOverlayAt_(double pixelX, double pixelY, BitmapSource frame)
        {
            var local = new Point(pixelX / frame.PixelWidth * imageViewport.ActualWidth,
                pixelY / frame.PixelHeight * imageViewport.ActualHeight);
            return imageViewport.TranslatePoint(local, myZoomAreaOverlay);
        }

        private bool isOnZoomAreaHandle_(Point p)
        {
            if (!_zoomAreaHasRoi || zoomAreaDisplayFrame_() is not BitmapSource frame)
            {
                return false;
            }

            if (_zoomAreaTagRect is Rect tag)
            {
                tag.Inflate(4.0, 4.0);
                if (tag.Contains(p))
                {
                    return true;
                }
            }

            Point tl = zoomAreaOverlayAt_(_zoomAreaRoi.X, _zoomAreaRoi.Y, frame);
            Point br = zoomAreaOverlayAt_(_zoomAreaRoi.X + _zoomAreaRoi.Width, _zoomAreaRoi.Y + _zoomAreaRoi.Height, frame);
            return p.X >= tl.X - 4.0 && p.X <= br.X + 4.0 && Math.Abs(p.Y - tl.Y) <= ZoomAreaEdgeHitDip;
        }

        #endregion Mapping

        #region Input

        private void zoomAreaMouseDown_(object sender, MouseButtonEventArgs e)
        {
            if (e.Handled || mySlitOverlay.IsMeasuring)
            {
                return;
            }

            if (e.ChangedButton == MouseButton.Right)
            {
                if (_zoomAreaDragging)
                {
                    _zoomAreaRoi = _zoomAreaDragStartRoi;
                    endZoomAreaGesture_();
                    redrawZoomArea_();
                    ZoomAreaChanged?.Invoke();
                    e.Handled = true;
                }
                else if (_zoomAreaSelecting)
                {
                    CancelZoomAreaSelection();
                    e.Handled = true;
                }
                return;
            }

            if (e.ChangedButton != MouseButton.Left || !isOverViewport_(e)
                || (Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
            {
                return;
            }

            Point p = e.GetPosition(myZoomAreaOverlay);
            if (_zoomAreaSelecting)
            {
                _zoomAreaPress = p;
                _zoomAreaRubber = new Rect(p, p);
                CaptureMouse();
                redrawZoomArea_();
                e.Handled = true;
            }
            else if (isOnZoomAreaHandle_(p))
            {
                _zoomAreaDragging = true;
                _zoomAreaPress = p;
                _zoomAreaDragStartRoi = _zoomAreaRoi;
                CaptureMouse();
                Cursor = Cursors.SizeAll;
                redrawZoomArea_();
                e.Handled = true;
            }
        }

        private void zoomAreaMouseMove_(object sender, MouseEventArgs e)
        {
            if (e.Handled || mySlitOverlay.IsMeasuring)
            {
                return;
            }

            Point p = e.GetPosition(myZoomAreaOverlay);

            if (_zoomAreaDragging && e.LeftButton == MouseButtonState.Pressed)
            {
                if (zoomAreaDisplayFrame_() is BitmapSource frame
                    && zoomAreaPixelAt_(_zoomAreaPress) is Point a && zoomAreaPixelAt_(p) is Point b)
                {
                    var moved = new Int32Rect(
                        _zoomAreaDragStartRoi.X + (int)Math.Round(b.X - a.X),
                        _zoomAreaDragStartRoi.Y + (int)Math.Round(b.Y - a.Y),
                        _zoomAreaDragStartRoi.Width, _zoomAreaDragStartRoi.Height);
                    moved = clampZoomAreaRoi_(moved, frame.PixelWidth, frame.PixelHeight);
                    if (moved.X != _zoomAreaRoi.X || moved.Y != _zoomAreaRoi.Y)
                    {
                        _zoomAreaRoi = moved;
                        redrawZoomArea_();
                        ZoomAreaChanged?.Invoke();
                    }
                }
                e.Handled = true;
                return;
            }

            if (_zoomAreaSelecting && _zoomAreaRubber != null && e.LeftButton == MouseButtonState.Pressed)
            {
                _zoomAreaRubber = new Rect(_zoomAreaPress, p);
                redrawZoomArea_();
                e.Handled = true;
                return;
            }

            if (!_zoomAreaSelecting && !_zoomAreaDragging && IsMouseCaptured == false)
            {
                bool over = isOnZoomAreaHandle_(p);
                if (over != _zoomAreaHover)
                {
                    _zoomAreaHover = over;
                    if (over)
                    {
                        Cursor = Cursors.SizeAll;
                    }
                    else
                    {
                        ClearValue(CursorProperty);
                    }
                    redrawZoomArea_();
                }
            }
        }

        private void zoomAreaMouseUp_(object sender, MouseButtonEventArgs e)
        {
            if (e.Handled || e.ChangedButton != MouseButton.Left)
            {
                return;
            }

            if (_zoomAreaDragging)
            {
                endZoomAreaGesture_();
                redrawZoomArea_();
                ZoomAreaChanged?.Invoke();
                e.Handled = true;
                return;
            }

            if (!_zoomAreaSelecting || _zoomAreaRubber == null)
            {
                return;
            }

            Point end = e.GetPosition(myZoomAreaOverlay);
            Point start = _zoomAreaPress;
            if (Math.Abs(end.X - start.X) >= ZoomAreaMinSelectDip && Math.Abs(end.Y - start.Y) >= ZoomAreaMinSelectDip
                && zoomAreaDisplayFrame_() is BitmapSource frame
                && zoomAreaPixelAt_(start) is Point a && zoomAreaPixelAt_(end) is Point b)
            {
                int w = frame.PixelWidth;
                int h = frame.PixelHeight;
                int left = Math.Clamp((int)Math.Round(Math.Min(a.X, b.X)), 0, w - 1);
                int top = Math.Clamp((int)Math.Round(Math.Min(a.Y, b.Y)), 0, h - 1);
                int right = Math.Clamp((int)Math.Round(Math.Max(a.X, b.X)), left + 1, w);
                int bottom = Math.Clamp((int)Math.Round(Math.Max(a.Y, b.Y)), top + 1, h);
                _zoomAreaRoi = new Int32Rect(left, top, right - left, bottom - top);
                _zoomAreaHasRoi = true;
            }

            _zoomAreaSelecting = false;
            endZoomAreaGesture_();
            ClearValue(CursorProperty);
            redrawZoomArea_();
            ZoomAreaChanged?.Invoke();
            e.Handled = true;
        }

        // Something else took the mouse (dialog, Alt+Tab): keep where the ROI got to.
        private void zoomAreaLostCapture_(object sender, MouseEventArgs e)
        {
            if (_zoomAreaDragging || _zoomAreaRubber != null)
            {
                _zoomAreaDragging = false;
                _zoomAreaRubber = null;
                redrawZoomArea_();
            }
        }

        private void zoomAreaKeyDown_(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape && _zoomAreaSelecting)
            {
                CancelZoomAreaSelection();
                e.Handled = true;
            }
        }

        private void endZoomAreaGesture_()
        {
            bool captured = _zoomAreaDragging || _zoomAreaRubber != null;
            _zoomAreaDragging = false;
            _zoomAreaRubber = null;
            _zoomAreaHover = false;
            if (captured && IsMouseCaptured)
            {
                ReleaseMouseCapture();
            }
            if (!_zoomAreaSelecting)
            {
                ClearValue(CursorProperty);
            }
        }

        #endregion Input

        #region Frame / layout

        // A new frame: keep the ROI inside it (a smaller frame shrinks it) and tell the zoom view.
        private void zoomAreaSourceChanged_(object? sender, EventArgs e)
        {
            if (zoomAreaDisplayFrame_() is BitmapSource frame)
            {
                if (!_zoomAreaHasRoi)
                {
                    ResetZoomAreaRoi();
                    return;
                }
                _zoomAreaRoi = clampZoomAreaRoi_(_zoomAreaRoi, frame.PixelWidth, frame.PixelHeight);
            }

            redrawZoomArea_();
            ZoomAreaChanged?.Invoke();
        }

        private void zoomAreaLayoutChanged_(object? sender, EventArgs e) => redrawZoomArea_();

        private void zoomAreaSizeChanged_(object sender, SizeChangedEventArgs e) => redrawZoomArea_();

        #endregion Frame / layout

        #region Drawing

        private void redrawZoomArea_()
        {
            Canvas canvas = myZoomAreaOverlay;
            canvas.Children.Clear();
            _zoomAreaTagRect = null;

            if (!_zoomAreaActive)
            {
                return;
            }

            if (_zoomAreaRubber is Rect rubber)
            {
                addZoomAreaRect_(rubber, ZoomAreaStrokeActive, ZoomAreaFill);
                addZoomAreaTag_(rubber, $"{(int)rubber.Width} x {(int)rubber.Height}", ZoomAreaStrokeActive);
                return;
            }

            if (_zoomAreaSelecting)
            {
                addZoomAreaTag_(new Rect(8, 32, 0, 0), "ZOOM AREA: drag a rectangle  [right-click / Esc to cancel]", ZoomAreaStrokeActive);
            }

            if (!_zoomAreaHasRoi || zoomAreaDisplayFrame_() is not BitmapSource frame || imageViewport.ActualWidth <= 0)
            {
                return;
            }

            Point tl = zoomAreaOverlayAt_(_zoomAreaRoi.X, _zoomAreaRoi.Y, frame);
            Point br = zoomAreaOverlayAt_(_zoomAreaRoi.X + _zoomAreaRoi.Width, _zoomAreaRoi.Y + _zoomAreaRoi.Height, frame);
            var box = new Rect(tl, br);
            Brush stroke = _zoomAreaDragging || _zoomAreaHover ? ZoomAreaStrokeActive : ZoomAreaStroke;
            addZoomAreaRect_(box, stroke, null);

            double len = Math.Min(12.0, Math.Min(box.Width, box.Height) / 3.0);
            addZoomAreaLine_(box.Left, box.Top, box.Left + len, box.Top, stroke);
            addZoomAreaLine_(box.Left, box.Top, box.Left, box.Top + len, stroke);
            addZoomAreaLine_(box.Right, box.Top, box.Right - len, box.Top, stroke);
            addZoomAreaLine_(box.Right, box.Top, box.Right, box.Top + len, stroke);
            addZoomAreaLine_(box.Left, box.Bottom, box.Left + len, box.Bottom, stroke);
            addZoomAreaLine_(box.Left, box.Bottom, box.Left, box.Bottom - len, stroke);
            addZoomAreaLine_(box.Right, box.Bottom, box.Right - len, box.Bottom, stroke);
            addZoomAreaLine_(box.Right, box.Bottom, box.Right, box.Bottom - len, stroke);
            double cx = (box.Left + box.Right) / 2.0;
            double cy = (box.Top + box.Bottom) / 2.0;
            addZoomAreaLine_(cx - 4, cy, cx + 4, cy, stroke);
            addZoomAreaLine_(cx, cy - 4, cx, cy + 4, stroke);

            string text = _zoomAreaDragging
                ? $"PANNING [{_zoomAreaRoi.Width} x {_zoomAreaRoi.Height}] ({_zoomAreaRoi.X}, {_zoomAreaRoi.Y})"
                : _zoomAreaHover
                    ? $"ZOOM ROI [{_zoomAreaRoi.Width} x {_zoomAreaRoi.Height}]  (drag to move)"
                    : $"ZOOM ROI [{_zoomAreaRoi.Width} x {_zoomAreaRoi.Height}]";
            _zoomAreaTagRect = addZoomAreaTag_(box, text, stroke);
        }

        private void addZoomAreaRect_(Rect r, Brush stroke, Brush? fill)
        {
            var rect = new Rectangle
            {
                Width = Math.Max(0.0, r.Width),
                Height = Math.Max(0.0, r.Height),
                Stroke = stroke,
                Fill = fill,
                StrokeThickness = 1.8,
                StrokeDashArray = new DoubleCollection { 4, 3 },
                IsHitTestVisible = false,
            };
            Canvas.SetLeft(rect, r.Left);
            Canvas.SetTop(rect, r.Top);
            myZoomAreaOverlay.Children.Add(rect);
        }

        private void addZoomAreaLine_(double x1, double y1, double x2, double y2, Brush stroke)
        {
            myZoomAreaOverlay.Children.Add(new Line
            {
                X1 = x1, Y1 = y1, X2 = x2, Y2 = y2,
                Stroke = stroke,
                StrokeThickness = 2.5,
                IsHitTestVisible = false,
            });
        }

        // Label above the box (below its top edge when there is no room), kept inside the view.
        private Rect addZoomAreaTag_(Rect anchor, string text, Brush accent)
        {
            var tag = new Border
            {
                Background = ZoomAreaTagBackground,
                BorderBrush = accent,
                BorderThickness = new Thickness(1),
                Padding = new Thickness(4, 1, 4, 1),
                IsHitTestVisible = false,
                Child = new TextBlock { Text = text, Foreground = accent, FontSize = 12, FontWeight = FontWeights.Bold },
            };
            tag.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
            Size size = tag.DesiredSize;

            double viewW = myZoomAreaOverlay.ActualWidth;
            double viewH = myZoomAreaOverlay.ActualHeight;
            double x = Math.Max(0.0, Math.Min(anchor.Left, viewW - size.Width));
            double y = anchor.Top - size.Height - 2.0;
            if (y < 0.0)
            {
                y = anchor.Top + 2.0;
            }
            y = Math.Max(0.0, Math.Min(y, viewH - size.Height));

            Canvas.SetLeft(tag, x);
            Canvas.SetTop(tag, y);
            myZoomAreaOverlay.Children.Add(tag);
            return new Rect(x, y, size.Width, size.Height);
        }

        private static Brush freeze_(SolidColorBrush brush)
        {
            brush.Freeze();
            return brush;
        }

        #endregion Drawing
    }
}
