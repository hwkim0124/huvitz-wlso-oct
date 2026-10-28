using System;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using WsoToolkit.controls;
using WsoToolkit.utils;

namespace WsoToolkit
{
    /// <summary>
    /// DevKit "Zoom Area View": a magnified live copy of the ROI placed on the main preview.
    /// Follows every frame and ROI move unless Freeze is on; Save ROI writes the crop at its
    /// native pixel size.
    /// </summary>
    public partial class LsoZoomAreaWindow : Window
    {
        private const double MinZoom = 1.0;
        private const double MaxZoom = 60.0;

        private readonly LsoScanImagePreview _preview;
        private readonly ZoomAreaCanvas _canvas = new();
        private BitmapSource? _crop;
        private Int32Rect _roi;

        public LsoZoomAreaWindow(LsoScanImagePreview preview)
        {
            _preview = preview;
            InitializeComponent();
            _canvas.Background = (Brush)FindResource("ZoomAreaViewportBrush");
            _canvas.ZoomWheeled += () => updateStatus_();
            _canvas.PixelHovered += showPixel_;
            myViewportHost.Child = _canvas;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            _preview.ZoomAreaChanged += refresh_;
            _preview.ShowZoomArea();
            refresh_();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            _preview.ZoomAreaChanged -= refresh_;
            _preview.HideZoomArea();
        }

        private void refresh_()
        {
            if (myCheckFreeze.IsChecked == true)
            {
                return;
            }

            _crop = _preview.CropZoomArea(out _roi);
            _canvas.Source = _crop;
            updateStatus_();
        }

        private void updateStatus_()
        {
            myTbStatusRoi.Text = _crop == null ? "ROI: -" : $"ROI: [{_roi.Width} x {_roi.Height}] @ ({_roi.X}, {_roi.Y})";
            double eff = _canvas.EffectiveZoom;
            myTbStatusZoom.Text = _canvas.Zoom <= 0.0
                ? string.Format(CultureInfo.InvariantCulture, "Zoom: Fit ({0:F1}x)", eff)
                : string.Format(CultureInfo.InvariantCulture, "Zoom: {0:F1}x", eff);
        }

        private void showPixel_(int? x, int? y)
        {
            if (_crop == null || x == null || y == null)
            {
                myTbStatusPixel.Text = "Pixel: -";
                return;
            }

            string value = readPixel_(_crop, x.Value, y.Value);
            myTbStatusPixel.Text = $"Pixel: ({_roi.X + x}, {_roi.Y + y}) [ROI: {x}, {y}] = {value}";
        }

        // Values as stored: 8-bit display bitmap today, 16-bit when a deeper frame is provided.
        private static string readPixel_(BitmapSource image, int x, int y)
        {
            PixelFormat f = image.Format;
            var at = new Int32Rect(x, y, 1, 1);
            if (f == PixelFormats.Gray8)
            {
                var b = new byte[1];
                image.CopyPixels(at, b, 1, 0);
                return $"Gray: {b[0]}";
            }
            if (f == PixelFormats.Gray16)
            {
                var s = new ushort[1];
                image.CopyPixels(at, s, 2, 0);
                return $"Gray: {s[0]}";
            }
            if (f == PixelFormats.Rgb48)
            {
                var s = new ushort[3];
                image.CopyPixels(at, s, 6, 0);
                return rgb_(s[0], s[1], s[2]);
            }
            if (f == PixelFormats.Rgb24)
            {
                var b = new byte[3];
                image.CopyPixels(at, b, 3, 0);
                return rgb_(b[0], b[1], b[2]);
            }
            if (f == PixelFormats.Bgr24)
            {
                var b = new byte[3];
                image.CopyPixels(at, b, 3, 0);
                return rgb_(b[2], b[1], b[0]);
            }

            var bgra = new byte[4];
            BitmapSource src = f == PixelFormats.Bgra32 || f == PixelFormats.Bgr32 || f == PixelFormats.Pbgra32
                ? image
                : new FormatConvertedBitmap(image, PixelFormats.Bgra32, null, 0.0);
            src.CopyPixels(at, bgra, 4, 0);
            return rgb_(bgra[2], bgra[1], bgra[0]);
        }

        private static string rgb_(int r, int g, int b) => r == g && g == b ? $"Gray: {r}" : $"R:{r} G:{g} B:{b}";

        private void myCbZoom_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (myCbZoom.SelectedItem is not ComboBoxItem item || !IsInitialized)
            {
                return;
            }

            _canvas.Zoom = double.Parse((string)item.Tag, CultureInfo.InvariantCulture);
            _canvas.ResetPan();
            updateStatus_();
        }

        private void myCbFilter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!IsInitialized)
            {
                return;
            }

            _canvas.ScalingMode = myCbFilter.SelectedIndex == 1 ? BitmapScalingMode.Linear : BitmapScalingMode.NearestNeighbor;
        }

        private void myCheckGrid_Changed(object sender, RoutedEventArgs e)
        {
            _canvas.ShowGrid = myCheckGrid.IsChecked == true;
        }

        private void myCheckFreeze_Unchecked(object sender, RoutedEventArgs e) => refresh_();

        private void myBtSelectArea_Click(object sender, RoutedEventArgs e) => _preview.StartZoomAreaSelection();

        private void myBtResetRoi_Click(object sender, RoutedEventArgs e) => _preview.ResetZoomAreaRoi();

        private void myBtSaveRoi_Click(object sender, RoutedEventArgs e)
        {
            BitmapSource? crop = _crop;
            if (crop == null)
            {
                MsgBoxUtil.ShowInfo("No ROI image available to save.");
                return;
            }

            var dialog = new Microsoft.Win32.SaveFileDialog
            {
                Title = "Save Zoomed ROI Image",
                Filter = "PNG Image (*.png)|*.png|TIFF Image (*.tif;*.tiff)|*.tif;*.tiff",
                FileName = $"ROI_{crop.PixelWidth}x{crop.PixelHeight}_{DateTime.Now:yyyyMMdd_HHmmss}.png",
            };
            if (dialog.ShowDialog(this) != true)
            {
                return;
            }

            try
            {
                LsoRoiImageWriter.Save(crop, dialog.FileName);
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or NotSupportedException)
            {
                MsgBoxUtil.ShowError($"'{dialog.FileName}' was not written: {ex.Message}");
                return;
            }

            MsgBoxUtil.ShowInfo($"ROI image saved ({crop.PixelWidth} x {crop.PixelHeight}):\n{dialog.FileName}");
        }

        /// <summary>DevKit ZoomCanvas: centred crop at Fit or a fixed zoom, left-drag pan,
        /// middle-click re-centre, wheel x1.25 within 1..60, pixel grid from 4x.</summary>
        private sealed class ZoomAreaCanvas : FrameworkElement
        {
            private static readonly Pen GridPen = frozenPen_(Color.FromArgb(40, 255, 255, 255), 1.0);
            private static readonly Pen BorderPen = frozenPen_(Color.FromArgb(160, 255, 180, 0), 1.0);

            private BitmapSource? _source;
            private double _zoom;
            private bool _showGrid = true;
            private Vector _pan;
            private bool _panning;
            private Point _panStartMouse;
            private Vector _panStart;

            public event Action? ZoomWheeled;
            public event Action<int?, int?>? PixelHovered;

            public ZoomAreaCanvas()
            {
                Cursor = Cursors.Cross;
                RenderOptions.SetBitmapScalingMode(this, BitmapScalingMode.NearestNeighbor);
            }

            public Brush Background { get; set; } = Brushes.Black;

            public BitmapSource? Source
            {
                get => _source;
                set { _source = value; InvalidateVisual(); }
            }

            /// <summary>0 = Fit.</summary>
            public double Zoom
            {
                get => _zoom;
                set { _zoom = value <= 0.0 ? 0.0 : Math.Clamp(value, MinZoom, MaxZoom); InvalidateVisual(); }
            }

            public bool ShowGrid
            {
                get => _showGrid;
                set { _showGrid = value; InvalidateVisual(); }
            }

            public BitmapScalingMode ScalingMode
            {
                set { RenderOptions.SetBitmapScalingMode(this, value); InvalidateVisual(); }
            }

            public double EffectiveZoom
            {
                get
                {
                    if (_zoom > 0.0 || _source == null || ActualWidth <= 0 || ActualHeight <= 0)
                    {
                        return _zoom > 0.0 ? _zoom : 1.0;
                    }
                    return Math.Min(ActualWidth / _source.PixelWidth, ActualHeight / _source.PixelHeight);
                }
            }

            public void ResetPan()
            {
                _pan = new Vector();
                InvalidateVisual();
            }

            private Rect destRect_()
            {
                if (_source == null)
                {
                    return new Rect(RenderSize);
                }

                double z = EffectiveZoom;
                double w = _source.PixelWidth * z;
                double h = _source.PixelHeight * z;
                return new Rect(((ActualWidth - w) / 2.0) + _pan.X, ((ActualHeight - h) / 2.0) + _pan.Y, w, h);
            }

            protected override void OnRender(DrawingContext dc)
            {
                var bounds = new Rect(RenderSize);
                dc.DrawRectangle(Background, null, bounds);

                if (_source == null)
                {
                    var text = new FormattedText(
                        "No ROI image available.\nClick 'Select Area' and drag a rectangle on the main preview.",
                        CultureInfo.InvariantCulture, FlowDirection.LeftToRight, new Typeface("Segoe UI"), 13.0,
                        Brushes.DarkGray, VisualTreeHelper.GetDpi(this).PixelsPerDip);
                    dc.DrawText(text, new Point(20, 20));
                    return;
                }

                dc.PushClip(new RectangleGeometry(bounds));
                Rect dest = destRect_();
                dc.DrawImage(_source, dest);

                double z = EffectiveZoom;
                if (_showGrid && z >= 4.0)
                {
                    int x0 = Math.Max(0, (int)Math.Floor(-dest.X / z));
                    int x1 = Math.Min(_source.PixelWidth, (int)Math.Ceiling((ActualWidth - dest.X) / z));
                    int y0 = Math.Max(0, (int)Math.Floor(-dest.Y / z));
                    int y1 = Math.Min(_source.PixelHeight, (int)Math.Ceiling((ActualHeight - dest.Y) / z));
                    double top = Math.Max(dest.Top, 0.0), bottom = Math.Min(dest.Bottom, ActualHeight);
                    double left = Math.Max(dest.Left, 0.0), right = Math.Min(dest.Right, ActualWidth);
                    for (int x = x0; x <= x1; ++x)
                    {
                        double lx = dest.X + (x * z);
                        dc.DrawLine(GridPen, new Point(lx, top), new Point(lx, bottom));
                    }
                    for (int y = y0; y <= y1; ++y)
                    {
                        double ly = dest.Y + (y * z);
                        dc.DrawLine(GridPen, new Point(left, ly), new Point(right, ly));
                    }
                }

                dc.DrawRectangle(null, BorderPen, dest);
                dc.Pop();
            }

            protected override void OnMouseDown(MouseButtonEventArgs e)
            {
                base.OnMouseDown(e);
                if (e.ChangedButton == MouseButton.Left)
                {
                    _panning = CaptureMouse();
                    _panStartMouse = e.GetPosition(this);
                    _panStart = _pan;
                    Cursor = Cursors.Hand;
                }
                else if (e.ChangedButton == MouseButton.Middle)
                {
                    ResetPan();
                }
            }

            protected override void OnMouseMove(MouseEventArgs e)
            {
                base.OnMouseMove(e);
                Point p = e.GetPosition(this);
                if (_panning)
                {
                    _pan = _panStart + (p - _panStartMouse);
                    InvalidateVisual();
                }

                Rect dest = destRect_();
                if (_source == null || !dest.Contains(p))
                {
                    PixelHovered?.Invoke(null, null);
                    return;
                }

                int px = Math.Clamp((int)Math.Floor((p.X - dest.X) / dest.Width * _source.PixelWidth), 0, _source.PixelWidth - 1);
                int py = Math.Clamp((int)Math.Floor((p.Y - dest.Y) / dest.Height * _source.PixelHeight), 0, _source.PixelHeight - 1);
                PixelHovered?.Invoke(px, py);
            }

            protected override void OnMouseUp(MouseButtonEventArgs e)
            {
                base.OnMouseUp(e);
                if (e.ChangedButton == MouseButton.Left && _panning)
                {
                    ReleaseMouseCapture();
                }
            }

            protected override void OnLostMouseCapture(MouseEventArgs e)
            {
                base.OnLostMouseCapture(e);
                _panning = false;
                Cursor = Cursors.Cross;
            }

            protected override void OnMouseLeave(MouseEventArgs e)
            {
                base.OnMouseLeave(e);
                PixelHovered?.Invoke(null, null);
            }

            protected override void OnMouseWheel(MouseWheelEventArgs e)
            {
                base.OnMouseWheel(e);
                Zoom = EffectiveZoom * (e.Delta > 0 ? 1.25 : 0.8);
                ZoomWheeled?.Invoke();
                e.Handled = true;
            }

            private static Pen frozenPen_(Color color, double thickness)
            {
                var pen = new Pen(new SolidColorBrush(color), thickness);
                pen.Freeze();
                return pen;
            }
        }
    }
}
