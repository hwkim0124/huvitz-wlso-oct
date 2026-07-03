using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using OpenCvSharp;
using OpenCvSharp.Extensions;
using Microsoft.Windows.Themes;
using System.Drawing;
//using MathNet.Numerics.Statistics;
using System.Threading.Channels;
using System.Windows.Markup;
using System.Windows.Media.Media3D;
//using HlsoToolkit.utils;
using static WsoNativeLib.WsoDevice;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows.Markup.Localizer;
using static WsoNativeLib.WsoConfig;
using System.Dynamic;
using static System.Net.Mime.MediaTypeNames;
using WsoNativeLib;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit.controls
{
    
    public partial class LsoScanImagePreview : UserControl
    {
        private readonly object _matLock = new object();

        Mat _imageMat = new();
        Mat _frameMat = new();

        int _imageWidth = 0;
        int _imageHeight = 0;
        int _imageChannels = 0;
        float _imageQuality = 0.0f;

        ////Frame Rate ///////////////////////////////////
        private readonly FrameRateCalculator _fpsCalc = new();
        //// /////////////////////////////////////////////

        //// Review Mode
        public bool IsReviewMode { get; set; } = true;
        List<Mat> _captureImageList = new List<Mat>();
        public int AcqFrameCount = 0;
        public int SubFrameCount = 0;

        // Review Slice
        public bool IsReviewSliceMode { get; set; } = false;
        List<List<Mat>> _captureSliceAcqImageList = new ();

        public LsoScanImagePreview()
        {
            InitializeComponent();
        }

        private void UpdatePreviewStatusItems()
        {
            string s = string.Format("{0} x {1}, {2} channels, {3:F1} fps, IQS: {4:F2}", _imageWidth, _imageHeight, _imageChannels, _fpsCalc.FrameRate, _imageQuality);
            lblImageStatusLive.Content = s;
        }

        public void UpdateColorFrameImage(int nPixelFormat)
        {
            if (_imageMat.Empty())
            {
                return;
            }

            ColorConversionCodes eConversionCodes;
            ColorPixelFormat eFormat = (ColorPixelFormat)nPixelFormat;
            switch (eFormat)
            {
                case ColorPixelFormat.Mono8:
                case ColorPixelFormat.Mono16:
                    eConversionCodes = ColorConversionCodes.GRAY2RGB;
                    break;
                case ColorPixelFormat.BayerRG8 : 
                case ColorPixelFormat.BayerRG16 :
                    eConversionCodes = ColorConversionCodes.BayerRG2RGB;
                    break;
                case ColorPixelFormat.RGB8Packed:
                case ColorPixelFormat.BGR8:
                //eConversionCodes = ColorConversionCodes.BGR2RGB;
                default:
                    // 지원하지 않는 포맷은 잘못된 변환으로 메모리 오류를 유발하므로 그대로 반환한다.
                    return;
            }

            Cv2.CvtColor(_imageMat, _frameMat, eConversionCodes);
            imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);
        }

        public void UpdateFramerate() => _fpsCalc.Tick();
        public void StartTimer() => _fpsCalc.Start();
        public void StopTimer() => _fpsCalc.Stop();

        public void SetLiveMode()
        {
            showDisplayMenu_(PreviewDisplayMode.LIVE);

            ClearReviewImages();
            IsReviewMode = false;
        }

        public void ClearReviewImages()
        {
            _captureImageList.Clear();
        }

        public void makeReviewImage(ref List<Mat> ImageList, Mat originMat, int nPixelFormat)
        {
            if (originMat.Empty())
            {
                return;
            }

            ColorConversionCodes eConversionCodes = new ColorConversionCodes();
            ColorPixelFormat eFormat = (ColorPixelFormat)nPixelFormat;
            switch (eFormat)
            {
                case ColorPixelFormat.Mono8:
                case ColorPixelFormat.Mono16:
                    eConversionCodes = ColorConversionCodes.GRAY2RGB;
                    return;
                case ColorPixelFormat.BayerRG8:
                case ColorPixelFormat.BayerRG16:
                    eConversionCodes = ColorConversionCodes.BayerRG2RGB;
                    break;
                case ColorPixelFormat.RGB8Packed:
                    return;
                case ColorPixelFormat.BGR8:
                    return;
            }

            Mat _resultMat = new();
            Cv2.CvtColor(originMat, _resultMat, eConversionCodes);

            ImageList.Add(_resultMat);
        }

        private void showDisplayMenu_(PreviewDisplayMode mode)
        {
            myGridLive.Visibility = Visibility.Hidden;
            myGridReview.Visibility = Visibility.Hidden;
            myGridReviewSliceMode.Visibility = Visibility.Hidden;

            switch (mode)
            {
                case PreviewDisplayMode.LIVE:
                    {
                        myGridLive.Visibility = Visibility.Visible;
                    }
                    break;
                case PreviewDisplayMode.REVIEW:
                    {
                        myGridReview.Visibility = Visibility.Visible;
                    }
                    break;
                case PreviewDisplayMode.REVIEW_SLICE:
                    {
                        myGridReviewSliceMode.Visibility = Visibility.Visible;
                    }
                    break;
                default:
                    {
                        myGridLive.Visibility = Visibility.Visible;
                    }
                    break;
            }
        }

        private void resetDisplayMenu_(PreviewDisplayMode mode, int nTotalFrameCount)
        {
            switch (mode)
            {
                case PreviewDisplayMode.LIVE:
                    break;
                case PreviewDisplayMode.REVIEW:
                    {
                        mySliderReviewImageIndex.Maximum = nTotalFrameCount;
                        myTbCurIndex.Text = "1";
                        myTbTotalCount.Text = nTotalFrameCount.ToString();
                    }
                    break;
                case PreviewDisplayMode.REVIEW_SLICE:
                    {
                        mySliderReviewSliceImageIndex.Minimum = 1;
                        mySliderReviewSliceImageIndex.Maximum = _captureSliceAcqImageList.Count;
                        myTbSliceAcqCurIndex.Text = "1";
                        myTbSliceAcqTotalCount.Text = _captureSliceAcqImageList.Count.ToString();

                        mySliderReviewSliceSubImageIndex.Minimum = 1;
                        mySliderReviewSliceSubImageIndex.Maximum = _captureSliceAcqImageList[0].Count;
                        myTbSliceSubCurIndex.Text = "1";
                        myTbSliceSubTotalCount.Text = _captureSliceAcqImageList[0].Count.ToString();
                    }
                    break;
            }
        }

        public void SetReviewMode(int nTotalFrameCount)
        {
            showDisplayMenu_(PreviewDisplayMode.REVIEW);
            resetDisplayMenu_(PreviewDisplayMode.REVIEW, nTotalFrameCount);

            if (_captureImageList.Count > 0)
            {
                mySliderReviewImageIndex.Value = 1;
                var nImageIndex = 0;

                try
                {
                    Mat adjustedMat = _captureImageList[nImageIndex].Clone();

                    Dispatcher.Invoke(() =>
                    {
                        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
                    });
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
                }
            }
        }

        public void SetReviewSliceMode()
        {
            showDisplayMenu_(PreviewDisplayMode.REVIEW_SLICE);

            if (_captureImageList.Count > 0)
            {
                if (SubFrameCount == 1)
                {
                    if (AcqFrameCount == 1)
                    {
                        List<Mat> tempSubFrameList = new List<Mat>() { _captureImageList[0] };
                        _captureSliceAcqImageList.Add(tempSubFrameList);
                    }
                    else
                    {
                        int nFrameCount = _captureImageList.Count();
                        for (int i = 0; i < nFrameCount; ++i)
                        {
                            List<Mat> tempSubFrameList = new (){ _captureImageList[i] };
                            _captureSliceAcqImageList.Add(tempSubFrameList);
                        }
                    }
                }
                else
                {
                    int nAcqFrameCount = AcqFrameCount;
                    int nSubFrameCount = SubFrameCount;
                    int nFrameCount = _captureImageList.Count();

                    if (nAcqFrameCount * nSubFrameCount != nFrameCount)
                    {
                        MessageBox.Show("Total frame count is invalid.");
                        return;
                    }

                    int chunkSize = nFrameCount / nAcqFrameCount; // 한 acq frame 당 요소 개수
                    List<List<Mat>> tempAcqFrameList = SplitListBySize(_captureImageList, chunkSize);

                    // 3) 기본 이미지 크기 (모두 동일하다고 가정)
                    int height = _captureImageList[0].Rows;
                    int width = _captureImageList[0].Cols;

                    // 4) 분할 높이 계산 (정수 나누기)
                    int segH = height / nSubFrameCount;

                    for (int i = 0; i < tempAcqFrameList.Count; ++i)
                    {
                        // 5) 각 이미지에서 해당 분할 영역만큼 잘라내기
                        List<Mat> tempSubFrameList = new List<Mat>();
                        Mat[] Sliced = new Mat[nSubFrameCount];

                        for (int j = 0; j < nSubFrameCount; ++j)
                        {
                            int y0 = j * segH;
                            int h = (j < nSubFrameCount) ? segH : (height - y0);  // 마지막은 남은 만큼 모두
                            OpenCvSharp.Rect roi = new OpenCvSharp.Rect(X: 0, Y: y0, Width: width, Height: h);
                            Sliced[j] = new Mat(tempAcqFrameList[i][j], roi).Clone();  // Clone()으로 메모리 분리
                            tempSubFrameList.Add(Sliced[j]);
                        }

                        // 6) 세로 방향으로 붙이기
                        Mat mergedMat = new Mat();
                        Cv2.VConcat(Sliced, mergedMat);
                        tempSubFrameList.Insert(0, mergedMat);

                        _captureSliceAcqImageList.Add(tempSubFrameList);
                    }
                }

                mySliderReviewSliceImageIndex.Value = 1;
                mySliderReviewSliceSubImageIndex.Value = 1;

                try
                {
                    // 원본 이미지를 복사하여 조정 적용
                    Mat adjustedMat = _captureSliceAcqImageList[0][0].Clone();

                    // 5. UI 업데이트 (UI 스레드에서)
                    Dispatcher.Invoke(() =>
                    {
                        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
                    });

                    adjustedMat.Dispose();
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
                }
            }

            resetDisplayMenu_(PreviewDisplayMode.REVIEW_SLICE, _captureSliceAcqImageList.Count());
        }

        static List<List<Mat>> SplitListBySize(List<Mat> source, int chunkSize)
        {
            var result = new List<List<Mat>>();
            if (chunkSize <= 0)
                throw new ArgumentException("chunkSize는 1 이상의 값이어야 합니다.", nameof(chunkSize));

            int count = source.Count;
            for (int i = 0; i < count; i += chunkSize)
            {
                // (i부터 chunkSize 개수)만큼 잘라서 새로운 List로 만든다.
                int size = Math.Min(chunkSize, count - i);
                List<Mat> chunk = source.GetRange(i, size);
                result.Add(chunk);
            }
            return result;
        }

        private void UpdateReviewStatusItems(int nWidth, int nHeight)
        {
            string s = string.Format("{0} x {1}", nWidth, nHeight);
            lblImageStatusReview.Content = s;
        }

        public void CallbackLsoScanFrameImage(byte[] data, int width, int height, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        {
            _imageWidth = width;
            _imageHeight = height;
            _imageChannels = channels;
            _imageQuality = quality;

            MatType matType = -1;

            switch (nBytesPerPixel)
            {
                case 1:
                    matType = MatType.CV_8UC1;
                    break;
                case 2:
                    matType = MatType.CV_16UC1;
                    break;
                case 3:
                    matType = MatType.CV_8UC3;
                    break;
                default:
                    return;
            }

            // 1) 새 Mat 생성 (언매니지드 메모리 할당)
            var mat = new Mat(height, width, matType);

            // 2) managed 배열 → Mat.Data(IntPtr)로 복사
            int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;
            Marshal.Copy(data, 0, mat.Data, byteCount);

            // 3) 스레드 안전하게 교체 (이전 프레임 Mat을 해제해 언매니지드 메모리 누수를 막는다)
            lock (_matLock)
            {
                _imageMat?.Dispose();
                _imageMat = mat;
            }

            UpdatePreviewStatusItems();
            UpdateColorFrameImage(nPixelFormat);
        }

        public void CallbackLsoScanCaptureFrameImage(byte[] data, int width, int height, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        {
            _imageWidth = width;
            _imageHeight = height;
            _imageChannels = channels;
            _imageQuality = quality;

            MatType matType = -1;

            switch (nBytesPerPixel)
            {
                case 1:
                    matType = MatType.CV_8UC1;
                    break;
                case 2:
                    matType = MatType.CV_16UC1;
                    break;
                case 3:
                    matType = MatType.CV_8UC3;
                    break;
                default:
                    return;
            }

            // 1) 새 Mat 생성 (언매니지드 메모리 할당)
            var mat = new Mat(height, width, matType);

            // 2) managed 배열 → Mat.Data(IntPtr)로 복사

            int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

            // CV_16U 이므로 ushort(2바이트) 사용
            Marshal.Copy(data, 0, mat.Data, byteCount);

            // 3) 스레드 안전하게 교체
            lock (_matLock)
            {
                makeReviewImage(ref _captureImageList, mat, nPixelFormat);
            }

            if (frameCount == totalFrameCount - 1) // Capture
            {
                if (IsReviewMode == true)
                {
                    UpdateReviewStatusItems(width, height);
                    SetReviewMode(totalFrameCount);
                }
                else if (IsReviewSliceMode == true) // Captrue (Slice)
                {
                    UpdateReviewStatusItems(width, height);
                    SetReviewSliceMode();
                }
            }
        }

        private void mySliderReviewImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_captureImageList.Count() <= 0)
            {
                return;
            }

            int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;

            // UI 스레드에서 실행되므로 비동기 처리
            _ = Task.Run(() =>
            {
                try
                {
                    Mat adjustedMat = _captureImageList[nImageIndex].Clone();

                    Dispatcher.Invoke(() =>
                    {
                        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
                    });
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
                }
            });
        }

        private void mySliderReviewSliceImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (sender is Slider slider)
            {
                int nIndex = (int)slider.Value;
                if (nIndex > 0)
                {
                    if (_captureSliceAcqImageList.Count() > 0)
                    {
                        int oldSubImageIndex = (int)mySliderReviewSliceSubImageIndex.Value;

                        mySliderReviewSliceSubImageIndex.Value = mySliderReviewSliceSubImageIndex.Minimum;

                        if (oldSubImageIndex == mySliderReviewSliceSubImageIndex.Minimum)
                        {
                            var args = new RoutedPropertyChangedEventArgs<double>(1, 1)
                            {
                                RoutedEvent = Slider.ValueChangedEvent
                            };

                            // RaiseEvent로 이벤트를 강제로 발생시킨다.
                            mySliderReviewSliceSubImageIndex.RaiseEvent(args);
                        }
                    }
                }
            }
        }

        private void mySliderReviewSliceSubImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (sender is Slider slider)
            {
                int nIndex = (int)slider.Value;
                if (nIndex > 0)
                {
                    if (_captureSliceAcqImageList.Count() > 0)
                    {
                        int nAcqFrameIndex = (int)mySliderReviewSliceImageIndex.Value;

                        if (nIndex == 1)
                        {
                            _ = Task.Run(() =>
                            {
                                try
                                {
                                    // 원본 이미지를 복사하여 조정 적용
                                    Mat adjustedMat = _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Clone();

                                    // 5. UI 업데이트 (UI 스레드에서)
                                    Dispatcher.Invoke(() =>
                                    {
                                        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
                                    });

                                    adjustedMat.Dispose();
                                }
                                catch (Exception ex)
                                {
                                    System.Diagnostics.Debug.WriteLine($"Image error: {ex.Message}");
                                }
                            });
                        }
                        else
                        {
                            UpdateReviewStatusItems(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Width, _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Height);
                            imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1]);
                        }
                    }
                }
            }
        }
    }
}
