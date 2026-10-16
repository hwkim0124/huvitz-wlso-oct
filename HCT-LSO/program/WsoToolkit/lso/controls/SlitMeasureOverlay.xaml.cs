using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace WsoToolkit.controls
{
    /// <summary>
    /// Two-point slit height measurement drawn over the camera viewport
    /// (DevKit LsoPatternForm group D, the part its parent form owned).
    ///
    /// The operator clicks the top and the bottom edge of the illuminated slit; the distance
    /// between them, converted from displayed pixels back to sensor rows, is what the ERS
    /// exposure recommendation multiplies by the line time.
    ///
    /// This control only measures and draws. It never touches hardware and never decides what
    /// the measurement means - the host does that through <see cref="Measured"/>.
    /// </summary>
    public partial class SlitMeasureOverlay : UserControl
    {
        private double? _row1;
        private double? _row2;
        private double? _hoverRow;
        private double? _bandTopRow;
        private double? _bandBottomRow;
        private double? _bandCenterRow;
        private string? _bannerOverride;

        public SlitMeasureOverlay()
        {
            InitializeComponent();
        }

        /// <summary>The image element this overlay sits on top of. Its render size and its
        /// source's pixel height are what the row mapping is built from.</summary>
        public Image? Viewport { get; set; }

        /// <summary>Vertical binning in force on the camera. A binned image row covers this many
        /// sensor rows, and ERS counts sensor rows - the DevKit ignores this and under-recommends
        /// the exposure by exactly this factor.</summary>
        public int VerticalBinning { get; set; } = 1;

        public bool IsMeasuring { get; private set; }

        /// <summary>Raised with the measured slit height in SENSOR rows (always >= 1).</summary>
        public event Action<int>? Measured;

        /// <summary>Raised with the measured slit centre SENSOR row and height SENSOR rows.</summary>
        public event Action<double, int>? CenterMeasured;

        /// <summary>Raised whenever <see cref="IsMeasuring"/> changes, so the host can gate
        /// hit-testing and update its buttons.</summary>
        public event Action<bool>? MeasuringChanged;

        public void StartMeasure()
        {
            _row1 = null;
            _row2 = null;
            _hoverRow = null;
            _bandTopRow = null;
            _bandBottomRow = null;
            _bandCenterRow = null;
            _bannerOverride = null;
            setMeasuring_(true);
            Cursor = Cursors.Cross;
            redraw_();
        }

        public void CancelMeasure()
        {
            if (!IsMeasuring)
            {
                return;
            }

            setMeasuring_(false);
            Cursor = Cursors.Arrow;
            _hoverRow = null;
            redraw_();
        }

        /// <summary>Drops the drawn markers (and any measurement in progress).</summary>
        public void ClearMarkers()
        {
            setMeasuring_(false);
            Cursor = Cursors.Arrow;
            _row1 = null;
            _row2 = null;
            _hoverRow = null;
            _bandTopRow = null;
            _bandBottomRow = null;
            _bandCenterRow = null;
            _bannerOverride = null;
            redraw_();
        }

        public void ShowBand(double topImageRow, double bottomImageRow, double centerImageRow, string banner)
        {
            setMeasuring_(false);
            Cursor = Cursors.Arrow;
            _row1 = null;
            _row2 = null;
            _hoverRow = null;
            _bandTopRow = topImageRow;
            _bandBottomRow = bottomImageRow;
            _bandCenterRow = centerImageRow;
            _bannerOverride = banner;
            redraw_();
        }

        private void setMeasuring_(bool value)
        {
            if (IsMeasuring == value)
            {
                return;
            }

            IsMeasuring = value;
            MeasuringChanged?.Invoke(value);
        }

        #region Mapping

        // Stretch="Uniform": the image is scaled by the smaller of the two ratios and centered,
        // so the letterbox band has to come off before a click means anything.
        //
        // Layout works in DIPs (ImageSource.Width/Height) while a row is a PIXEL, and the two
        // differ whenever the bitmap's DPI is not 96 - so the band is computed in DIPs and the
        // position inside it is converted with the pixel height.
        private bool imageBand_(out double top, out double height, out double pixelHeight)
        {
            top = 0.0;
            height = 0.0;
            pixelHeight = 0.0;

            if (Viewport?.Source is not ImageSource source)
            {
                return false;
            }

            double srcW = source.Width;
            double srcH = source.Height;
            double boxW = Viewport.ActualWidth;
            double boxH = Viewport.ActualHeight;
            if (srcW <= 0 || srcH <= 0 || boxW <= 0 || boxH <= 0)
            {
                return false;
            }

            double scale = Math.Min(boxW / srcW, boxH / srcH);
            height = srcH * scale;
            top = (boxH - height) / 2.0;
            pixelHeight = source is BitmapSource bitmap ? bitmap.PixelHeight : srcH;
            return height > 0 && pixelHeight > 0;
        }

        /// <summary>Control Y -> row of the displayed image (not yet a sensor row).</summary>
        private double? imageRowAt_(double y)
        {
            if (!imageBand_(out double top, out double height, out double pixelHeight))
            {
                return null;
            }

            double row = (y - top) / height * pixelHeight;
            return Math.Clamp(row, 0.0, pixelHeight - 1.0);
        }

        private double controlYOf_(double imageRow)
        {
            if (!imageBand_(out double top, out double height, out double pixelHeight) || pixelHeight <= 0)
            {
                return 0.0;
            }

            return top + imageRow / pixelHeight * height;
        }

        private int sensorRows_(double imageRowSpan)
        {
            int binning = Math.Max(1, VerticalBinning);
            return Math.Max(1, (int)Math.Round(Math.Abs(imageRowSpan) * binning));
        }

        #endregion Mapping

        #region Input

        protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
        {
            base.OnMouseLeftButtonDown(e);

            if (!IsMeasuring)
            {
                return;
            }

            double? row = imageRowAt_(e.GetPosition(this).Y);
            if (row == null)
            {
                return;
            }

            if (_row1 == null)
            {
                _row1 = row;
            }
            else
            {
                _row2 = row;
                setMeasuring_(false);
                Cursor = Cursors.Arrow;
                Measured?.Invoke(sensorRows_(_row2.Value - _row1.Value));
                CenterMeasured?.Invoke((_row1.Value + _row2.Value) / 2.0 * Math.Max(1, VerticalBinning), sensorRows_(_row2.Value - _row1.Value));
            }

            redraw_();
            e.Handled = true;
        }

        protected override void OnMouseRightButtonDown(MouseButtonEventArgs e)
        {
            base.OnMouseRightButtonDown(e);

            if (IsMeasuring)
            {
                CancelMeasure();
                e.Handled = true;
            }
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);

            if (!IsMeasuring)
            {
                return;
            }

            _hoverRow = imageRowAt_(e.GetPosition(this).Y);
            redraw_();
        }

        private void UserControl_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            redraw_();
        }

        #endregion Input

        #region Drawing

        private void redraw_()
        {
            myCanvas.Children.Clear();

            if (!IsMeasuring && _row1 == null && _row2 == null && _bandTopRow == null)
            {
                return;
            }

            if (!imageBand_(out _, out _, out _))
            {
                return;
            }

            if (IsMeasuring && _hoverRow != null)
            {
                addLine_(controlYOf_(_hoverRow.Value), Brushes.Gold, 1.2, dashed: true);
            }

            if (_row1 != null)
            {
                double y = controlYOf_(_row1.Value);
                addLine_(y, Brushes.Cyan, 2.0, dashed: false);
                addLabel_(y, $"P1 (Top): Row {_row1.Value:F0}", Brushes.Cyan);
            }

            if (_row2 != null)
            {
                double y = controlYOf_(_row2.Value);
                addLine_(y, Brushes.Cyan, 2.0, dashed: false);
                addLabel_(y, $"P2 (Bottom): Row {_row2.Value:F0}", Brushes.Cyan);
            }

            if (_row1 != null && _row2 != null)
            {
                double center = (_row1.Value + _row2.Value) / 2.0;
                addLine_(controlYOf_(center), Brushes.Gold, 1.8, dashed: true);
                addLabel_(controlYOf_(center), $"Center: Row {center * Math.Max(1, VerticalBinning):F1}", Brushes.Gold);
            }

            if (_bandTopRow != null && _bandBottomRow != null && _bandCenterRow != null)
            {
                int binning = Math.Max(1, VerticalBinning);
                addLine_(controlYOf_(_bandTopRow.Value), Brushes.Cyan, 2.0, dashed: false);
                addLabel_(controlYOf_(_bandTopRow.Value), $"Top: Row {_bandTopRow.Value * binning:F0}", Brushes.Cyan);
                addLine_(controlYOf_(_bandBottomRow.Value), Brushes.Cyan, 2.0, dashed: false);
                addLabel_(controlYOf_(_bandBottomRow.Value), $"Bottom: Row {_bandBottomRow.Value * binning:F0}", Brushes.Cyan);
                addLine_(controlYOf_(_bandCenterRow.Value), Brushes.Gold, 1.8, dashed: true);
                addLabel_(controlYOf_(_bandCenterRow.Value), $"Center: Row {_bandCenterRow.Value * binning:F1}", Brushes.Gold);
            }

            addBanner_();
        }

        private void addLine_(double y, Brush brush, double thickness, bool dashed)
        {
            var line = new Line
            {
                X1 = 0,
                X2 = ActualWidth,
                Y1 = y,
                Y2 = y,
                Stroke = brush,
                StrokeThickness = thickness,
                IsHitTestVisible = false,
            };
            if (dashed)
            {
                line.StrokeDashArray = new DoubleCollection { 4, 3 };
            }

            myCanvas.Children.Add(line);
        }

        private void addLabel_(double y, string text, Brush foreground)
        {
            var label = new Border
            {
                Background = new SolidColorBrush(Color.FromArgb(190, 0, 40, 60)),
                Padding = new Thickness(4, 1, 4, 1),
                IsHitTestVisible = false,
                Child = new TextBlock
                {
                    Text = text,
                    Foreground = foreground,
                    FontSize = 12,
                    FontWeight = FontWeights.Bold,
                },
            };

            Canvas.SetLeft(label, 10);
            Canvas.SetTop(label, Math.Max(0, y - 18));
            myCanvas.Children.Add(label);
        }

        private void addBanner_()
        {
            string text;
            if (_bannerOverride != null)
            {
                text = _bannerOverride;
            }
            else if (IsMeasuring && _row1 == null)
            {
                text = "SLIT MEASURE: click point 1 (top edge)  [right-click / Esc to cancel]";
            }
            else if (IsMeasuring)
            {
                int delta = _hoverRow != null ? sensorRows_(_hoverRow.Value - _row1!.Value) : 0;
                text = string.Format(CultureInfo.InvariantCulture,
                    "SLIT MEASURE: P1 = row {0:F0} | click point 2 (bottom edge) | {1} rows",
                    _row1!.Value, delta);
            }
            else if (_row1 != null && _row2 != null)
            {
                text = string.Format(CultureInfo.InvariantCulture,
                    "SLIT HEIGHT: {0} sensor rows (binning x{1})",
                    sensorRows_(_row2.Value - _row1.Value), Math.Max(1, VerticalBinning));
            }
            else
            {
                return;
            }

            var banner = new Border
            {
                Background = new SolidColorBrush(Color.FromArgb(200, 0, 0, 0)),
                Padding = new Thickness(6, 3, 6, 3),
                IsHitTestVisible = false,
                Child = new TextBlock
                {
                    Text = text,
                    Foreground = Brushes.Gold,
                    FontSize = 12.5,
                    FontWeight = FontWeights.Bold,
                },
            };

            Canvas.SetLeft(banner, 8);
            Canvas.SetTop(banner, 8);
            myCanvas.Children.Add(banner);
        }

        #endregion Drawing
    }
}
