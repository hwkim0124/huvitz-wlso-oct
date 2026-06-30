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
using WsoNativeLib;

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

        public bool IsStretchToFit { get; set; } = true;
        public bool IsOverlayAlignGuide { get; set; } = false;
        public WsoDevice.CameraType CameraType { get; set; } = WsoDevice.CameraType.IrCorneaLeft; 

        public CorneaCameraPreview()
        {
            InitializeComponent();
        }

        private void UpdateCameraStatusItems()
        {
            _stopwatch.Stop();
            int frate = (int)(1000.0f / _stopwatch.ElapsedMilliseconds);
            frate = _frameCount == 1 ? 0 : frate;
            _stopwatch.Restart();

            if (_frameCount > 1 && _frameCount % AVERAGE_SIZE != 0)
            {
                return;
            }

            var type = CameraType;
            float again = GetAnalogGain(type);
            float dgain = GetDigitalGain(type);

            string s = string.Format("{0} x {1}, {2:F0} fps", _imageWidth, _imageHeight, frate);
            s += string.Format(" - Again: {0:F2}, Dgain: {1:F2}", again, dgain);
            lblStatus.Content = s;
        }

        public void CallbackCorneaCameraFrame(nint data, int width, int height)
        {
            if (data == 0) return;

            // Stride for 8-bit grayscale. 
            nint step = width * 1;

            // Passing a nint to Mat.FromPixelData explicitly invokes the zero-copy mapping overload, 
            // creating a lightweight header over your camera buffer.
            using (Mat rawFrame = Mat.FromPixelData(height, width, MatType.CV_8UC1, data, step))
            {
                _imageWidth = width;
                _imageHeight = height;
                _imageMat = rawFrame.Clone();
                UpdateCameraStatusItems();
                UpdateCameraFrameImage();
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

            // imageViewport.Source = _frameMat.ToWriteableBitmap();
            // Convert to a WPF-consumable BitmapSource
            BitmapSource source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);

            // Freeze the bitmap to optimize cross-thread UI rendering performance
            source.Freeze();

            // Update the XAML Image element
            imageViewport.Source = source;
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
        }
    }
}
