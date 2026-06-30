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
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace WsoToolkit.controls
{
    public partial class OctReportThicknessMap : UserControl
    {
        Mat _imageMat = new();
        Mat _frameMat = new();
        int _imageWidth = 0;
        int _imageHeight = 0;

        public bool IsStretchToFit { get; set; } = true;

        public OctReportThicknessMap()
        {
            InitializeComponent();
        }

        private void UpdateStatusItems()
        {
            string s = $"Map size: {_imageWidth} x {_imageHeight}";
            lblStatus.Content = s;
            return;
        }

        public bool SaveImageFile(string path)
        {
            if (_imageMat.Empty())
            {
                return false;
            }

            return _imageMat.SaveImage(path);
        }

        public void CallbackOctThicknessMapCaptured(float[] data, int width, int height)
        {
            _imageWidth = width;
            _imageHeight = height;
            UpdateStatusItems();

            if (data == null || data.Length == 0)
            {
                return;
            }
            nint step = width * sizeof(float);

            using (Mat rawFrame = Mat.FromPixelData(height, width, MatType.CV_32F, data, step))
            {
                _imageMat = rawFrame.Clone();
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

            int frameWidth = 320; // (int)imageViewport.ActualWidth;
            int frameHeight = 320; // (int)imageViewport.ActualHeight;

            Cv2.Normalize(_imageMat, _imageMat, 0.0, 1.0, NormTypes.MinMax);

            if (_imageWidth == frameWidth && _imageHeight == frameHeight)
            {
                _frameMat = _imageMat;
            }
            else
            {
                if (IsStretchToFit)
                {
                    Cv2.Resize(_imageMat, _frameMat, new OpenCvSharp.Size(frameWidth, frameHeight), 0, 0, InterpolationFlags.Cubic);
                    imageViewport.Stretch = Stretch.None;
                }
                else
                {
                    _frameMat = _imageMat;
                    // imageViewport.Stretch = Stretch.UniformToFill;
                }
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
    }
}
