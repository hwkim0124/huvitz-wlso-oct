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
using WsoNativeLib;

namespace WsoToolkit.controls
{
    public partial class OctScanAngioPreview : UserControl
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
        public bool IsOverlayInfo { get; set; } = true;
        public bool IsContrastEnhanced { get; set; } = true;

        private WsoOctScan.OctProtocolDescript? _protocol = null;
        public WsoOctScan.OctProtocolDescript? ProtocolDescriptor
        {
            get { return _protocol; }
            set
            {
                _protocol = value;
            }
        }


        public OctScanAngioPreview()
        {
            InitializeComponent();

            RegisterMouseEventHandlers();
        }

        private void UpdateAngioStatus()
        {
            string s = $"Angio size: {_imageWidth} x {_imageHeight}";
            lblStatus.Content = s;
            return;
        }


        public void CallbackOctScanAngioImageCaptured(nint data, int width, int height)
        {
            if (data == 0) return;
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

                _drawPoints.Clear();

                UpdateAngioStatus();
                ProduceFrameImage();
            }
        }

        public void ProduceFrameImage()
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
            ProduceFrameImage();
        }
        private void RegisterMouseEventHandlers()
        {
            imageViewport.MouseDown += (sender, e) =>
            {
                if (_frameMat.Empty())
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
            };
        }

        public bool SaveImageFile(string path, bool square)
        {
            if (_imageMat.Empty())
            {
                return false;
            }

            if (square)
            {
                int w = _imageMat.Width;
                int h = _imageMat.Height;
                int size = Math.Max(w, h);
                Mat resized = _imageMat.Resize(new OpenCvSharp.Size(size, size));
                return resized.SaveImage(path);
            }

            return _imageMat.SaveImage(path);
        }
    }
}
