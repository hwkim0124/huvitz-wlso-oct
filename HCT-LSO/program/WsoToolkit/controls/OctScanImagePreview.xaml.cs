using OpenCvSharp;
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
using WsoNativeLib;

namespace WsoToolkit.controls
{
    public partial class OctScanImagePreview : UserControl
    {
        Mat _imageMat = new();
        Mat _frameMat = new();
        long _frameCount = 0;
        int _imageWidth = 0;
        int _imageHeight = 0;
        float _imageQuality = 0.0f;
        float _snrRatio = 0.0f;
        int _refPoint = 0;
        int _chartCenterLineIndex = -1;
        int _chartCenterLateralPos = -1;

        int _viewportWidth = 1024;
        int _viewportHeight = 512;
        const int AVERAGE_SIZE = 24;

        List<OpenCvSharp.Point> _drawPoints = new();

        Stopwatch _stopwatch = new();

        public bool IsStretchToHeight { get; set; } = true;
        public bool IsPreviewMode { get; set; } = true;
        public bool IsOverlayLayers { get; set; } = true;
        public bool IsOverlayLength { get; set; } = true;
        public bool IsOptimizing { get; set; } = false;
        public bool IsOverlayAlignGuide { get; set; } = false;
        public float BscanRange { get; set; } = 6.0f;
        public int ScanLineIndex { get; set; } = -1;

        private WsoOctScan.OctProtocolDescript? _protocol = null;
        public WsoOctScan.OctProtocolDescript? ProtocolDescriptor
        {
            get { return _protocol; }
            set
            {
                _protocol = value;
            }
        }

        public WsoOctSegm.OctBsegmLayerPoints _upperPoints = new();
        public WsoOctSegm.OctBsegmLayerPoints _lowerPoints = new();
        public WsoOctScan.OctStratumParam _stratumParam = new();

        public OctScanImagePreview()
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

        public void ClearOverlayStatus()
        {
            _upperPoints = new();
            _lowerPoints = new();
        }

        public void SetLayerPoints(WsoOctScan.OctStratumParam param, WsoOctSegm.OctBsegmLayerPoints uppers, WsoOctSegm.OctBsegmLayerPoints lowers)
        {
            _stratumParam = param;
            _upperPoints = uppers;
            _lowerPoints = lowers;
        }

        public void SetChartCenterPosition(int lineIndex, int lateralPos)
        {
            _chartCenterLineIndex = lineIndex;
            _chartCenterLateralPos = lateralPos;
        }

        private void UpdatePreviewStatus()
        {
            _stopwatch.Stop();
            float frate = (1000.0f / _stopwatch.ElapsedMilliseconds);
            _frameCount++;
            frate = _frameCount == 1 ? 0 : frate;
            _stopwatch.Restart();

            if (_frameCount > 1 && _frameCount % AVERAGE_SIZE != 0)
            {
                return;
            }

            string s1 = $"Frame rate:  {frate,-7:F0}";
            string s2 = $"Image size:  {_imageWidth} x {_imageHeight}";
            string s3 = $"\tQuality:  {_imageQuality:F2}  SNR:  {_snrRatio:F2}  Ref. Point:  {_refPoint}";
            lblStatus.Content = s1 + s2 + s3;
        }

        private void UpdatePatternStatus()
        {
            _stopwatch.Stop();
            _frameCount = 0;

            string s1 = $"Image index:  {ScanLineIndex}";
            string s2 = $"\tImage size:  {_imageWidth} x {_imageHeight}";
            string s3 = $"\tQuality:  {_imageQuality:F2}  SNR:  {_snrRatio:F2}  Ref. Point:  {_refPoint}";
            lblStatus.Content = s1 + s2 + s3;
        }

        public void CallbackOctScanPreviewImageCaptured(nint data, int width, int height, float quality, float snr_ratio, int ref_point, int image_index)
        {
            ScanLineIndex = image_index;
            _imageWidth = width;
            _imageHeight = height;
            _imageQuality = quality;
            _snrRatio = snr_ratio;
            _refPoint = ref_point;

            if (IsPreviewMode)
            {
                UpdatePreviewStatus();
            }
            else
            {
                UpdatePatternStatus();
            }

            if (data == 0) return;
            nint step = width * sizeof(byte);

            using (Mat rawFrame = Mat.FromPixelData(height, width, MatType.CV_8UC1, data, step))
            {
                _imageMat = rawFrame.Clone();
                _drawPoints.Clear();
                UpdateFrameImage();
            }    
            
            return;
        }

        public void UpdateFrameImage()
        {
            if (_imageMat.Empty())
            {
                return;
            }

            // Convert to 3 channel image for overlay drawing.
            Cv2.CvtColor(_imageMat, _frameMat, ColorConversionCodes.GRAY2BGR);

            _viewportWidth = imageBorder.ActualWidth > 0 ? (int)imageBorder.ActualWidth : _viewportWidth;
            _viewportHeight = imageBorder.ActualHeight > 0 ? (int)imageBorder.ActualHeight : _viewportHeight;

            if (_imageWidth != _viewportWidth || _imageHeight != _viewportHeight)
            {
                // Fitting only to screen width leaving height as is.
                if (IsStretchToHeight)
                {
                    Cv2.Resize(_frameMat, _frameMat, new OpenCvSharp.Size(_viewportWidth, _viewportHeight), 0, 0, InterpolationFlags.Cubic);
                }
                else
                {
                    var height = Math.Min(_imageHeight, _viewportHeight);
                    _frameMat = new Mat(_frameMat, new OpenCvSharp.Rect(0, 0, _imageWidth, height));
                    Cv2.Resize(_frameMat, _frameMat, new OpenCvSharp.Size(_viewportWidth, _viewportHeight), 0, 0, InterpolationFlags.Cubic);
                }
            }

            RenderOverlayLayers(ref _frameMat);
            RenderOverlayImageInfo(ref _frameMat);
            RenderOverlayAlignGuide(ref _frameMat);
            RenderOverlayChartCenter(ref _frameMat);
            UpdateImageView();
            return;
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
                if (IsPreviewMode || _frameMat.Empty() || !IsOverlayLength)
                {
                    return;
                }
                if (e.RightButton == MouseButtonState.Pressed)
                {
                    _drawPoints.Clear();
                    RedrawFrameImage();
                    return;
                }

                var pos = e.GetPosition(imageViewport);
                var img_sx = 0; // Math.Max((_viewportWidth - _frameMat.Width) / 2, 0);
                var img_sy = 0;
                var img_ex = _viewportWidth; // Math.Min(img_sx + _frameMat.Width, _viewportWidth);
                var img_ey = _viewportHeight; // Math.Max(img_sy + _frameMat.Height, _viewportHeight);

                if (pos.X < img_sx || pos.X >= img_ex || pos.Y < img_sy || pos.Y >= img_ey)
                {
                    return;
                }

                Cv2.Circle(_frameMat, new OpenCvSharp.Point(pos.X, pos.Y), 5, Scalar.Orange, 1);

                // Convert view clicked position into the actual pixel position in image. 
                var x_ratio = (float)_imageWidth / _frameMat.Width;
                var y_ratio = (float)_imageHeight / _frameMat.Height;
                var act_x = (int)((pos.X - img_sx) * x_ratio);
                var act_y = (int)((pos.Y - img_sy) * y_ratio);

                if (!IsStretchToHeight)
                {
                    y_ratio = 1.0f;
                }

                var text = $"({act_x},{act_y})";
                Cv2.PutText(_frameMat, text, new OpenCvSharp.Point(pos.X - 20, pos.Y + 20), HersheyFonts.HersheySimplex, 0.4, new Scalar(0, 255, 0), 1, LineTypes.AntiAlias);

                _drawPoints.Add(new(pos.X, pos.Y));
                if (_drawPoints.Count >= 2)
                {
                    var dx = ((float)Math.Abs(_drawPoints[1].X - _drawPoints[0].X) / _viewportWidth) * _protocol?.scanRangeX ?? 0.0f;
                    var dy = ((float)Math.Abs(_drawPoints[1].Y - _drawPoints[0].Y) * y_ratio * 0.00303f);   // in millimeters
                    var dist = Math.Sqrt(dx * dx + dy * dy);

                    Cv2.Line(_frameMat, _drawPoints[0], _drawPoints[1], Scalar.Orange, 1, LineTypes.AntiAlias);

                    var cx = (_drawPoints[0].X + _drawPoints[1].X) / 2;
                    var cy = (_drawPoints[0].Y + _drawPoints[1].Y) / 2;
                    text = $"{dist:F2} mm";
                    Cv2.PutText(_frameMat, text, new OpenCvSharp.Point(cx, cy + 15), HersheyFonts.HersheySimplex, 0.4, new Scalar(0, 255, 0), 1, LineTypes.AntiAlias);
                    _drawPoints.Clear();
                }

                UpdateImageView();
            };
        }

        private void RenderOverlayImageInfo(ref Mat image)
        {
            if (IsPreviewMode)
            {
                if (IsOptimizing)
                {
                    var text = "Scan Optimizing ...";
                    Cv2.PutText(image, text, new OpenCvSharp.Point(40, _viewportHeight - 25), HersheyFonts.HersheySimplex, 0.6, new Scalar(0, 255, 0), 1, LineTypes.AntiAlias);
                }
            }
            else
            {
                var scanLines = _protocol?.scanLines ?? 0;
                if (scanLines > 0)
                {
                    var text = $"{ScanLineIndex + 1}/{scanLines}";
                    Cv2.PutText(image, text, new OpenCvSharp.Point(_viewportWidth - 120, _viewportHeight - 25), HersheyFonts.HersheySimplex, 0.6, new Scalar(0, 255, 0), 1, LineTypes.AntiAlias);
                }
            }
        }

        private void RenderOverlayAlignGuide(ref Mat image)
        {
            if (!IsPreviewMode || IsOverlayAlignGuide != true || BscanRange <= 0.0f)
            {
                return;
            }

            var img_w = image.Width;
            var img_h = image.Height;
            var cen_x = img_w / 2;
            var cen_y = img_h / 2;
            var mm_size = image.Width / BscanRange;
            var color = new Scalar(0, 255, 255);
            var bar_size = 12;

            Cv2.Line(image, img_w / 2, 0, img_w / 2, img_h - 1, color, 1, LineTypes.AntiAlias);
            Cv2.Line(image, 0, img_h / 2, img_w - 1, img_h / 2, color, 1, LineTypes.AntiAlias);

            for (int i = 1; i < 99; i++)
            {
                var x_pos1 = cen_x + (int)(mm_size * i);
                var x_pos2 = cen_x - (int)(mm_size * i);
                if (x_pos1 < img_w)
                {
                    Cv2.Line(image, x_pos1, cen_y - bar_size, x_pos1, cen_y + bar_size, color, 1, LineTypes.AntiAlias);
                    Cv2.Line(image, x_pos2, cen_y - bar_size, x_pos2, cen_y + bar_size, color, 1, LineTypes.AntiAlias);
                }
                else
                {
                    break;
                }
            }
        }

        private void RenderOverlayChartCenter(ref Mat frame)
        {
            if (IsPreviewMode || ScanLineIndex != _chartCenterLineIndex)
            {
                return;
            }
            if (_chartCenterLateralPos <= 0 && _chartCenterLineIndex <= 0)
            {
                return;
            }

            var frame_w = frame.Width;
            var frame_h = frame.Height;
            var xloc = (float)_chartCenterLateralPos / _imageWidth;
            var xpos = (int)(xloc * frame_w);

            OpenCvSharp.Point pt1 = new(xpos, 0);
            OpenCvSharp.Point pt2 = new(xpos, frame_h - 1);
            Cv2.Line(frame, pt1, pt2, Scalar.Orange, 3, LineTypes.AntiAlias);
            return;
        }

        private void RenderOverlayLayers(ref Mat frame)
        {
            if (IsPreviewMode || IsOverlayLayers != true)
            {
                return;
            }

            var src_w = _upperPoints.width;
            var src_h = _upperPoints.height;
            var img_w = frame.Width;
            var img_h = frame.Height;

            float w_ratio = (float)src_w / (float)img_w;
            float h_ratio = (float)img_h / (float)src_h;

            if (!IsStretchToHeight)
            {
                h_ratio = 1.0f;
            }

            var upper = _upperPoints.data;
            var lower = _lowerPoints.data;

            if (src_w > 0)
            {
                for (int i = 0; i < img_w; i++)
                {
                    int x = (int)(i * w_ratio);
                    if (upper[x] < 0 || upper[x] >= src_h)
                    {
                        continue;
                    }

                    int y = (int)(upper[x] * h_ratio);
                    if (y >= 0 && y < img_h)
                    {
                        frame.Set<Vec3b>(y, i, new Vec3b(255, 0, 0));
                    }
                }
            }

            if (src_w > 0)
            {
                for (int i = 0; i < img_w; i++)
                {
                    int x = (int)(i * w_ratio);
                    if (upper[x] < 0 || upper[x] >= src_h)
                    {
                        continue;
                    }

                    int y = (int)(lower[x] * h_ratio);
                    if (y >= 0 && y < img_h)
                    {
                        frame.Set<Vec3b>(y, i, new Vec3b(255, 0, 0));
                    }
                }
            }
            return;
        }
    }
}
