using OpenCvSharp;
using System;
using System.Collections.Generic;
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
using OpenCvSharp.WpfExtensions;
using WsoNativeLib;


namespace WsoToolkit.controls
{
    public partial class OctScanEnfacePreview : UserControl
    {
        Mat _imageMat = new();
        Mat _frameMat = new();
        int _imageWidth = 0;
        int _imageHeight = 0;
        int _viewportWidth = 320;
        int _viewportHeight = 320;

        List<OpenCvSharp.Point> _drawPoints = new();

        public double ClipLimit { get; set; } = 2.0;
        public int TileGridSize { get; set; } = 8;

        public int SelectedLineIndex { get; set; } = 0;
        public bool IsPreviewMode { get; set; } = true;
        public bool IsOverlayLength { get; set; } = false;
        public bool IsOverlayCenter { get; set; } = true;
        public bool IsContrastEnhanced { get; set; } = true;

        private WsoOctScan.OctProtocolDescript? _protocol = null;
        public WsoOctScan.OctProtocolDescript? ProtocolDescriptor
        {
            get { return _protocol; }
            set { _protocol = value; }
        }
        private WsoOctScan.OctStratumParam? _stratum = null;
        public WsoOctScan.OctStratumParam? LayerStratumParam
        {
            get { return _stratum; }
            set { _stratum = value; }
        }

        public delegate void OnChartCenterRelocated(float x, float y, int image_idx, int later_pos);

        public OnChartCenterRelocated? ChartCenterRelocated { get; set; } = null;

        public OctScanEnfacePreview()
        {
            InitializeComponent();

            RegisterMouseEventHandlers();
        }

        public bool IsAnalysisMode()
        {
            return !IsPreviewMode;
        }

        public void SetAsPreviewMode()
        {
            IsPreviewMode = true;
            ClearOverlayStatus();
        }
        public void SetAsAnalysisMode()
        {
            IsPreviewMode = false;
        }

        public bool IsChartCenterLocated()
        {
            var point = GetChartCenterPosition();
            return point.X >= 0 && point.Y >= 0;
        }

        public void ClearOverlayStatus()
        {
            _drawPoints.Clear();
        }

        public void SetChartCenterLocation(float x, float y)
        {
            if (_stratum != null)
            {
                var param = _stratum.Value;
                param.chartCenterX = x;
                param.chartCenterY = y;
                _stratum = param;
            }
        }

        private void UpdateEnfaceStatus()
        {
            string s = $"Enface size: {_imageWidth} x {_imageHeight}";
            lblStatus.Content = s;
            return;
        }

        public void CallbackOctScanEnfaceImageCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Stride for 8-bit grayscale. 
            nint step = width * sizeof(byte);

            using (Mat rawFrame = Mat.FromPixelData(height, width, MatType.CV_8UC1, data, step))
            {
                _imageWidth = width;
                _imageHeight = height;
                _imageMat = rawFrame.Clone();

                if (IsContrastEnhanced)
                {
                    Cv2.CreateCLAHE(ClipLimit, new OpenCvSharp.Size(TileGridSize, TileGridSize)).Apply(_imageMat, _imageMat);
                }

                _viewportWidth = imageBorder.ActualWidth > 0 ? (int)imageBorder.ActualWidth : _viewportWidth;
                _viewportHeight = imageBorder.ActualHeight > 0 ? (int)imageBorder.ActualHeight : _viewportHeight;

                UpdateEnfaceStatus();
                UpdateFrameImage();
            }
        }

        public void UpdateFrameImage()
        {
            if (_imageMat.Empty())
            {
                return;
            }

            // Convert to 3 channel image for overlay drawing.
            Cv2.CvtColor(_imageMat, _frameMat, ColorConversionCodes.GRAY2BGR);

            if (_imageWidth != _viewportWidth || _imageHeight != _viewportHeight)
            {
                Cv2.Resize(_frameMat, _frameMat, new OpenCvSharp.Size(_viewportWidth, _viewportHeight), 0, 0, InterpolationFlags.Cubic);
            }

            RenderOverlayLineSelect(ref _frameMat);
            RenderOverlayChartCenter(ref _frameMat);
            UpdateImageView();
        }

        private void UpdateImageView()
        {
            BitmapSource source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);

            // Freeze the bitmap to optimize cross-thread UI rendering performance
            source.Freeze();

            // Update the XAML Image element
            imageViewport.Source = source;
            // imageViewport.Source = _frameMat.ToWriteableBitmap();// OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);
        }

        private void RedrawFrameImage()
        {
            UpdateFrameImage();
        }

        private void RegisterMouseEventHandlers()
        {
            imageViewport.MouseDown += (sender, e) =>
            {
                if (IsPreviewMode || _frameMat.Empty() || !(IsOverlayLength || IsOverlayCenter))
                {
                    return;
                }
                if (e.RightButton == MouseButtonState.Pressed)
                {
                    _drawPoints.Clear();
                    RedrawFrameImage();
                    return;
                }

                if (IsOverlayLength)
                {
                    var pos = e.GetPosition(imageViewport);
                    if (pos.X < 0 || pos.X >= _frameMat.Width || pos.Y < 0 || pos.Y >= _frameMat.Height)
                    {
                        return;
                    }

                    Cv2.Circle(_frameMat, new OpenCvSharp.Point(pos.X, pos.Y), 5, Scalar.Orange, 1);

                    // Convert view clicked position into the actual pixel position in image. 
                    var x_size = (float)_imageWidth / _frameMat.Width;
                    var y_size = (float)_imageHeight / _frameMat.Height;
                    var act_x = (int)(pos.X * x_size);
                    var act_y = (int)(pos.Y * y_size);

                    var text = $"({act_x},{act_y})";
                    Cv2.PutText(_frameMat, text, new OpenCvSharp.Point(pos.X - 20, pos.Y + 20), HersheyFonts.HersheySimplex, 0.4, new Scalar(0, 255, 0), 1, LineTypes.AntiAlias);

                    _drawPoints.Add(new(pos.X, pos.Y));
                    if (_drawPoints.Count >= 2)
                    {
                        var dx = ((float)Math.Abs(_drawPoints[1].X - _drawPoints[0].X) / _frameMat.Width) * _protocol?.scanRangeX ?? 0.0f;
                        var dy = ((float)Math.Abs(_drawPoints[1].Y - _drawPoints[0].Y) / _frameMat.Height) * _protocol?.scanRangeY ?? 0.0f;   // in millimeters
                        var dist = Math.Sqrt(dx * dx + dy * dy);

                        Cv2.Line(_frameMat, _drawPoints[0], _drawPoints[1], Scalar.Orange, 1, LineTypes.AntiAlias);

                        var cx = (_drawPoints[0].X + _drawPoints[1].X) / 2;
                        var cy = (_drawPoints[0].Y + _drawPoints[1].Y) / 2;
                        text = $"{dist:F2} mm";
                        Cv2.PutText(_frameMat, text, new OpenCvSharp.Point(cx, cy + 15), HersheyFonts.HersheySimplex, 0.4, new Scalar(0, 255, 0), 1, LineTypes.AntiAlias);
                        _drawPoints.Clear();
                    }
                    UpdateImageView();
                }
                else if (IsOverlayCenter)
                {
                    var pos = e.GetPosition(imageViewport);
                    var loc = ConvertToLocation(ref _frameMat, (int)pos.X, (int)pos.Y);
                    var idx = ConvertToBscanIndex(ref _frameMat, (int)pos.X, (int)pos.Y);
                    ChartCenterRelocated?.Invoke(loc.X, loc.Y, (int)idx.Y, (int)idx.X);
                }
            };
        }


        private void RenderOverlayLineSelect(ref Mat image)
        {
            if (!IsPreviewMode)
            {
                var scanLines = _protocol?.scanLines ?? 0;
                var scanDirection = _protocol?.scanDirection ?? WsoOctDefs.OctScanDirection.X_TO_Y;

                if (scanLines > 0)
                {
                    var pos = (float)SelectedLineIndex / (float)scanLines;
                    if (scanDirection == WsoOctDefs.OctScanDirection.X_TO_Y)
                    {
                        OpenCvSharp.Point pt1 = new(0, (int)(pos * _frameMat.Height));
                        OpenCvSharp.Point pt2 = new(_frameMat.Width, (int)(pos * _frameMat.Height));
                        Cv2.Line(_frameMat, pt1, pt2, new Scalar(0, 0, 255), 1);
                    }
                    else
                    {
                        OpenCvSharp.Point pt1 = new((int)(pos * _frameMat.Width), 0);
                        OpenCvSharp.Point pt2 = new((int)(pos * _frameMat.Width), _frameMat.Height);
                        Cv2.Line(_frameMat, pt1, pt2, new Scalar(0, 0, 255), 1);
                    }
                }
            }
        }

        private void RenderOverlayChartCenter(ref Mat frame)
        {
            if (!IsPreviewMode && IsOverlayCenter)
            {
                if (_stratum != null && _protocol != null)
                {
                    var chart_x = _stratum?.chartCenterX ?? 0.0f;
                    var chart_y = _stratum?.chartCenterY ?? 0.0f;
                    var range_x = _protocol?.scanRangeX ?? 0.0f;
                    var range_y = _protocol?.scanRangeY ?? 0.0f;
                    if ((range_x * range_y) <= 0.0f)
                    {
                        return;
                    }

                    var size_x = frame.Width / range_x;
                    var size_y = frame.Height / range_y;
                    var org_x = frame.Width / 2;
                    var org_y = frame.Height / 2;
                    var pos_x = (int)(Math.Round(org_x + chart_x * size_x));
                    var pos_y = (int)(Math.Round(org_y + chart_y * size_y));
                    Cv2.DrawMarker(frame, new OpenCvSharp.Point(pos_x, pos_y), new Scalar(0, 255, 0), OpenCvSharp.MarkerTypes.Cross, 10, 2, OpenCvSharp.LineTypes.AntiAlias);
                }
            }
        }

        private Point2f ConvertToLocation(ref Mat frame, int pos_x, int pos_y)
        {
            var point = new Point2f();
            if (_protocol != null)
            {
                var range_x = _protocol?.scanRangeX ?? 0.0f;
                var range_y = _protocol?.scanRangeY ?? 0.0f;
                if (range_x * range_y > 0.0f)
                {
                    var size_x = frame.Width / range_x;
                    var size_y = frame.Height / range_y;
                    var x = pos_x - frame.Width / 2;
                    var y = pos_y - frame.Height / 2;
                    var loc_x = x / size_x;
                    var loc_y = y / size_y;
                    point = new Point2f(loc_x, loc_y);
                }
            }
            return point;
        }

        private Point2f ConvertToBscanIndex(ref Mat frame, int pos_x, int pos_y)
        {
            var point = new Point2f();
            if (_protocol != null)
            {
                var x = (float)pos_x / frame.Width;
                var y = (float)pos_y / frame.Height;
                var lines = _protocol?.scanLines ?? 0;
                var width = _protocol?.scanPoints ?? 0;
                var index = (int)(Math.Round(y * lines));
                var later = (int)(Math.Round(x * width));
                point = new Point2f(later, index);
            }
            return point;
        }

        public OpenCvSharp.Point GetChartCenterPosition()
        {
            var point = new OpenCvSharp.Point(-1, -1);

            if (_stratum != null && _protocol != null)
            {
                var chart_x = _stratum?.chartCenterX ?? 0.0f;
                var chart_y = _stratum?.chartCenterY ?? 0.0f;
                var range_x = _protocol?.scanRangeX ?? 0.0f;
                var range_y = _protocol?.scanRangeY ?? 0.0f;
                var n_lines = _protocol?.scanLines ?? 0;
                var n_points = _protocol?.scanPoints ?? 0;

                if ((range_x * range_y) > 0 && n_lines > 0 && n_points > 0)
                {
                    var size_x = n_points / range_x;
                    var size_y = n_lines / range_y;
                    var org_x = n_points / 2;
                    var org_y = n_lines / 2;
                    var pos_x = (int)(Math.Round(org_x + chart_x * size_x));
                    var pos_y = (int)(Math.Round(org_y + chart_y * size_y));
                    point = new OpenCvSharp.Point(pos_x, pos_y);
                }
            }
            return point;
        }

        public bool SaveImageFile(string path)
        {
            if (_imageMat.Empty())
            {
                return false;
            }

            return _imageMat.SaveImage(path);
        }
    }
}
