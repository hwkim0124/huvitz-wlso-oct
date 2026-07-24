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
    using static WsoNativeLib.CorneaCamera;
    public partial class RetinaCameraPreview : UserControl
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
        public WsoDevice.CameraType CameraType { get; set; } = WsoDevice.CameraType.IrRetina;
        public WsoCallback.CorneaCameraFrameCaptured? Callback { get; set; } = null;

        public RetinaCameraPreview()
        {
            InitializeComponent();
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

            if (checkSwitch.IsChecked == false)
            {
                return;
            }

            // Stride for 8-bit grayscale. 
            nint step = width * 1;

            try
            {
                // Passing a nint to Mat.FromPixelData explicitly invokes the zero-copy mapping overload, 
                // creating a lightweight header over your camera buffer.
                using (Mat rawFrame = Mat.FromPixelData(height, width, MatType.CV_8UC1, data, step))
                {
                    _imageWidth = width;
                    _imageHeight = height;
                    rawFrame.CopyTo(_imageMat);
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
            if (IsOverlayFocusGuide != true || IsOverlayAlignGuide == true)
            {
                return;
            }

            var img_w = image.Width;
            var img_h = image.Height;
            var img_cx = img_w / 2;
            var img_cy = img_h / 2;
            var color = new Scalar(255, 255, 0);

            if (CameraType != WsoDevice.CameraType.IrCorneaLeft && CameraType != WsoDevice.CameraType.IrCorneaRight)
            {
                var unit_h = img_h / 24;
                Cv2.Line(image, 0, img_cy - unit_h * 1, img_w - 1, img_cy - unit_h * 1, color, 1, LineTypes.AntiAlias);
                Cv2.Line(image, 0, img_cy + unit_h * 1, img_w - 1, img_cy + unit_h * 1, color, 1, LineTypes.AntiAlias);
            }
            else if (CameraType == WsoDevice.CameraType.IrCorneaLower)
            {
                Cv2.Line(image, img_cx, 0, img_cx, img_h - 1, color, 1, LineTypes.AntiAlias);
            }
            return;
        }

        private void DrawOverlayAlignGuide(ref Mat image)
        {
            if (IsOverlayAlignGuide != true || IsOverlayFocusGuide == true)
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

        public void Play()
        {
            if (!CorneaCamera.IsPreviewing(CameraType))
            {
                if (Callback != null)
                {
                    CorneaCamera.StartPreview(CameraType, Callback);
                }
            }
            checkSwitch.IsChecked = true;
        }

        public void Stop()
        {
            CorneaCamera.ClosePreview(CameraType);
            checkSwitch.IsChecked = false;
        }

        private void CheckSwitch_Click(object sender, RoutedEventArgs e)
        {
            if (checkSwitch.IsChecked == true)
            {
                Play();
            }
            else
            {
                Stop();
            }
        }
    }
}
