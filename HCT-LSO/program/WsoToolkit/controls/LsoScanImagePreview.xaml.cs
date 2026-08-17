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
using WsoToolkit;
using WsoToolkit.utils;

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

        // Live-only (BayerSwGray aiming): convert Bayer->gray in the live display only.
        // The capture/review path (makeReviewImage) uses a separate switch, so it is unaffected.
        public bool IsForceGrayLiveDisplay { get; set; } = false;

        //// Review Mode
        public bool IsReviewMode { get; set; } = true;
        List<Mat> _captureImageList = new List<Mat>();
        public int AcqFrameCount = 0;
        public int SubFrameCount = 0;
        List<ImageAdjustParam> _captureImageAdjustParams = new();
        public int ImageAdjustBrightness = 0;
        private ImageAdjustParam _defaultAdjustPreset = new();
        public Action<ImageAdjustParam>? AdjustPresetChanged;

        // Image Adjustment
        private ImageAdjustmentWindow? _adjustmentWindow;
        private ImageAdjustmentWindow AdjustmentWindow
        {
            get
            {
                if (_adjustmentWindow == null)
                    _adjustmentWindow = new ImageAdjustmentWindow();
                return _adjustmentWindow;
            }
        }

        public void CloseAdjustmentWindow()
        {
            _adjustmentWindow?.ForceClose();
            _adjustmentWindow = null;
        }

        public void SetDefaultAdjustPreset(ImageAdjustParam param)
        {
            _defaultAdjustPreset = CloneAdjustParam(param);
            ImageAdjustBrightness = param.Brightness;
        }

        private ImageAdjustParam CreateAdjustParamFromDefault(int nPixelDepth = 0)
        {
            var param = CloneAdjustParam(_defaultAdjustPreset);
            if (nPixelDepth == 2)
            {
                param.BrightnessRangeMin = -32767;
                param.BrightnessRangeMax = 32767;
            }
            else
            {
                param.BrightnessRangeMin = -127;
                param.BrightnessRangeMax = 127;
            }

            return param;
        }

        private static ImageAdjustParam CloneAdjustParam(ImageAdjustParam source)
        {
            return new ImageAdjustParam(source.Brightness, 0)
            {
                BrightnessRangeMin = source.BrightnessRangeMin,
                BrightnessRangeMax = source.BrightnessRangeMax,
                IsRadialCorrection = source.IsRadialCorrection,
                RadialCorIntensity = source.RadialCorIntensity,
                IsRemoveReflectedLight = source.IsRemoveReflectedLight,
                RadialCorrectionCurve = source.RadialCorrectionCurve?.ToArray(),
            };
        }

        private void PersistAdjustPreset_(ImageAdjustParam adjustParam)
        {
            _defaultAdjustPreset = CloneAdjustParam(adjustParam);
            ImageAdjustBrightness = adjustParam.Brightness;
            AdjustPresetChanged?.Invoke(CloneAdjustParam(adjustParam));
        }

        // Review Slice
        public bool IsReviewSliceMode { get; set; } = false;
        List<List<Mat>> _captureSliceAcqImageList = new ();
        List<List<ImageAdjustParam>> _captureSliceAdjustParams = new();

        // Review ROI
        public bool IsReviewROIMode { get; set; } = false;
        public FrameRoiPosition[] FrameROIs = Array.Empty<FrameRoiPosition>();

        public LsoScanImagePreview()
        {
            InitializeComponent();
        }

        public bool IsOverlayAlignGuide
        {
            get => myAlignGuideOverlay.Visibility == Visibility.Visible;
            set
            {
                myAlignGuideOverlay.Visibility = value ? Visibility.Visible : Visibility.Collapsed;
                if (value)
                {
                    DrawAlignGuide();
                }
            }
        }

        private void myAlignGuideOverlay_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (IsOverlayAlignGuide)
            {
                DrawAlignGuide();
            }
        }

        private void DrawAlignGuide()
        {
            var canvas = myAlignGuideOverlay;
            canvas.Children.Clear();

            double w = canvas.ActualWidth;
            double h = canvas.ActualHeight;
            if (w <= 0 || h <= 0)
            {
                return;
            }

            var brush = System.Windows.Media.Brushes.Yellow;
            double cx = w / 2;
            double cy = h / 2;

            // 주 십자선 (viewport 전체)
            AddGuideLine(brush, cx, 0, cx, h);
            AddGuideLine(brush, 0, cy, w, cy);

            // Major tick
            double unit = 80, size = 7;
            for (int i = 1; i < 4; i++)
            {
                AddGuideLine(brush, cx - unit * i, cy - size, cx - unit * i, cy + size);
                AddGuideLine(brush, cx + unit * i, cy - size, cx + unit * i, cy + size);
            }
            for (int i = 1; i < 3; i++)
            {
                AddGuideLine(brush, cx - size, cy - unit * i, cx + size, cy - unit * i);
                AddGuideLine(brush, cx - size, cy + unit * i, cx + size, cy + unit * i);
            }

            // Minor tick
            unit = 10; size = 3;
            for (int i = 1; i < 32; i++)
            {
                AddGuideLine(brush, cx - unit * i, cy - size, cx - unit * i, cy + size);
                AddGuideLine(brush, cx + unit * i, cy - size, cx + unit * i, cy + size);
            }
            for (int i = 1; i < 24; i++)
            {
                AddGuideLine(brush, cx - size, cy - unit * i, cx + size, cy - unit * i);
                AddGuideLine(brush, cx - size, cy + unit * i, cx + size, cy + unit * i);
            }
        }

        private void AddGuideLine(System.Windows.Media.Brush brush, double x1, double y1, double x2, double y2)
        {
            var line = new System.Windows.Shapes.Line
            {
                X1 = x1,
                Y1 = y1,
                X2 = x2,
                Y2 = y2,
                Stroke = brush,
                StrokeThickness = 1,
                SnapsToDevicePixels = true,
            };
            myAlignGuideOverlay.Children.Add(line);
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
                    if (IsForceGrayLiveDisplay)
                    {
                        // BayerSwGray aiming: display-side Bayer->GRAY->RGB. Does not touch
                        // the source frame or the capture path.
                        using (var grayMat = new Mat())
                        {
                            Cv2.CvtColor(_imageMat, grayMat, ColorConversionCodes.BayerRG2GRAY);
                            Cv2.CvtColor(grayMat, _frameMat, ColorConversionCodes.GRAY2RGB);
                        }
                        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);
                        return;
                    }
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
            _captureSliceAcqImageList.Clear();
            _captureImageAdjustParams.Clear();
            _captureSliceAdjustParams.Clear();
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
            _captureImageAdjustParams.Add(CreateAdjustParamFromDefault(_resultMat.Depth()));
        }

        private void showDisplayMenu_(PreviewDisplayMode mode)
        {
            myGridLive.Visibility = Visibility.Hidden;
            myGridReview.Visibility = Visibility.Hidden;
            myGridReviewSliceMode.Visibility = Visibility.Hidden;
            myGridReviewROIMode.Visibility = Visibility.Hidden;

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
                case PreviewDisplayMode.REVIEW_ROI:
                    {
                        myGridReviewROIMode.Visibility = Visibility.Visible;
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
                case PreviewDisplayMode.REVIEW_ROI:
                    {
                        mySliderReviewROIImageIndex.Minimum = 1;
                        mySliderReviewROIImageIndex.Maximum = _captureSliceAcqImageList.Count;
                        myTbROIAcqCurIndex.Text = "1";
                        myTbROIAcqTotalCount.Text = _captureSliceAcqImageList.Count.ToString();

                        mySliderReviewROISubImageIndex.Minimum = 1;
                        mySliderReviewROISubImageIndex.Maximum = _captureSliceAcqImageList[0].Count;
                        myTbROISubCurIndex.Text = "1";
                        myTbROISubTotalCount.Text = _captureSliceAcqImageList[0].Count.ToString();
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
                    Mat adjustedMat = ApplyImageAdjustments(_captureImageList[nImageIndex], GetAdjustParam(nImageIndex));

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
                        _captureSliceAdjustParams.Add(new List<ImageAdjustParam> { GetAdjustParam(0) });
                    }
                    else
                    {
                        int nFrameCount = _captureImageList.Count();
                        for (int i = 0; i < nFrameCount; ++i)
                        {
                            List<Mat> tempSubFrameList = new (){ _captureImageList[i] };
                            _captureSliceAcqImageList.Add(tempSubFrameList);
                            _captureSliceAdjustParams.Add(new List<ImageAdjustParam> { GetAdjustParam(i) });
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
                        List<ImageAdjustParam> tempParamList = new List<ImageAdjustParam>();
                        Mat[] Sliced = new Mat[nSubFrameCount];

                        for (int j = 0; j < nSubFrameCount; ++j)
                        {
                            int y0 = j * segH;
                            int h = (j < nSubFrameCount) ? segH : (height - y0);  // 마지막은 남은 만큼 모두
                            OpenCvSharp.Rect roi = new OpenCvSharp.Rect(X: 0, Y: y0, Width: width, Height: h);
                            Sliced[j] = new Mat(tempAcqFrameList[i][j], roi).Clone();  // Clone()으로 메모리 분리
                            tempSubFrameList.Add(Sliced[j]);
                            tempParamList.Add(GetAdjustParam(i * chunkSize + j));
                        }

                        // 6) 세로 방향으로 붙이기
                        Mat mergedMat = new Mat();
                        Cv2.VConcat(Sliced, mergedMat);
                        tempSubFrameList.Insert(0, mergedMat);
                        tempParamList.Insert(0, CreateAdjustParamFromDefault());

                        _captureSliceAcqImageList.Add(tempSubFrameList);
                        _captureSliceAdjustParams.Add(tempParamList);
                    }
                }

                mySliderReviewSliceImageIndex.Value = 1;
                mySliderReviewSliceSubImageIndex.Value = 1;

                try
                {
                    Mat adjustedMat = ApplyImageAdjustments(_captureSliceAcqImageList[0][0], GetSliceAdjustParam(0, 0));

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

        public void SetReviewROIMode()
        {
            showDisplayMenu_(PreviewDisplayMode.REVIEW_ROI);

            if (_captureImageList.Count > 0)
            {
                int nAcqFrameCount = AcqFrameCount;
                int nSubFrameCount = SubFrameCount;
                int nFrameCount = _captureImageList.Count;

                FrameRoiPosition[] frameROIs = FrameROIs;

                if (nAcqFrameCount != 1 && nSubFrameCount != 1 && nFrameCount != 2)
                {
                    if (nAcqFrameCount * nSubFrameCount != nFrameCount)
                    {
                        MessageBox.Show("Total frame count is invalid.");
                        return;
                    }
                }

                int chunkSize = nFrameCount / nAcqFrameCount;
                List<List<Mat>> tempAcqFrameList = SplitListBySize(_captureImageList, chunkSize);

                int height = _captureImageList[0].Rows;
                int width = _captureImageList[0].Cols;

                for (int i = 0; i < tempAcqFrameList.Count; ++i)
                {
                    List<Mat> tempSubFrameList = new List<Mat>();
                    List<ImageAdjustParam> tempParamList = new List<ImageAdjustParam>();
                    Mat[] sliced = new Mat[nSubFrameCount];

                    for (int j = 0; j < nSubFrameCount; ++j)
                    {
                        if (j > frameROIs.Length - 1)
                        {
                            sliced[j] = tempAcqFrameList[i][j].Clone();
                            tempSubFrameList.Add(sliced[j]);
                        }
                        else
                        {
                            int startY = Math.Clamp(frameROIs[j].StartY, 0, height);
                            int endY = Math.Clamp(frameROIs[j].EndY, 0, height);
                            OpenCvSharp.Rect roi = new OpenCvSharp.Rect(0, startY, width, endY - startY);
                            sliced[j] = new Mat(tempAcqFrameList[i][j], roi).Clone();
                            tempSubFrameList.Add(sliced[j]);
                        }

                        tempParamList.Add(GetAdjustParam(i * chunkSize + j));
                    }

                    Mat baseImg = new Mat(height, width, MatType.CV_8UC3, Scalar.All(0));

                    for (int index = 0; index < nSubFrameCount; ++index)
                    {
                        if (index > frameROIs.Length - 1)
                        {
                            sliced[index].CopyTo(baseImg);
                            continue;
                        }

                        int startY = Math.Clamp(frameROIs[index].StartY, 0, height);
                        int endY = Math.Clamp(frameROIs[index].EndY, 0, height);
                        int roiHeight = endY - startY;
                        if (roiHeight <= 0)
                        {
                            continue;
                        }

                        OpenCvSharp.Rect roiRect = new OpenCvSharp.Rect(0, startY, width, roiHeight);
                        using Mat baseROI = new Mat(baseImg, roiRect);
                        sliced[index].CopyTo(baseROI);
                    }

                    tempSubFrameList.Insert(0, baseImg);
                    tempParamList.Insert(0, CreateAdjustParamFromDefault());
                    _captureSliceAcqImageList.Add(tempSubFrameList);
                    _captureSliceAdjustParams.Add(tempParamList);
                }

                mySliderReviewROIImageIndex.Value = 1;
                mySliderReviewROISubImageIndex.Value = 1;

                try
                {
                    Mat adjustedMat = ApplyImageAdjustments(_captureSliceAcqImageList[0][0], GetSliceAdjustParam(0, 0));

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

            resetDisplayMenu_(PreviewDisplayMode.REVIEW_ROI, _captureSliceAcqImageList.Count());
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

        private void UpdateReviewSliceStatusItems(int nWidth, int nHeight)
        {
            string s = string.Format("{0} x {1}", nWidth, nHeight);
            lblImageStatusReviewSlice.Content = s;
        }

        private void UpdateReviewROIStatusItems(int nWidth, int nHeight)
        {
            string s = string.Format("{0} x {1}", nWidth, nHeight);
            lblImageStatusReviewROI.Content = s;
        }

        private async Task saveAllImages_()
        {
            if (_captureImageList.Count <= 0)
            {
                return;
            }

            await Task.Run(() =>
            {
                string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss");

                if (!Directory.Exists(strFolderPath))
                {
                    Directory.CreateDirectory(strFolderPath);
                }

                for (int i = 0; i < _captureImageList.Count; ++i)
                {
                    using Mat mat = _captureImageList[i].Clone();
                    string strFileName = strFolderPath + "//" + $"Index_{i + 1}" + ".png";
                    mat.SaveImage(strFileName);
                }
            });
        }

        private async Task saveSliceImage_()
        {
            if (_captureSliceAcqImageList.Count <= 0)
            {
                return;
            }

            await Task.Run(() =>
            {
                string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_Slice";

                if (!Directory.Exists(strFolderPath))
                {
                    Directory.CreateDirectory(strFolderPath);
                }

                int nAcqFrameCount = _captureSliceAcqImageList.Count;

                if (nAcqFrameCount == 1 && _captureSliceAcqImageList[0].Count == 1)
                {
                    string strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_Merged.png";
                    using Mat mat = _captureSliceAcqImageList[0][0].Clone();
                    mat.SaveImage(strFileName);
                }
                else
                {
                    for (int nAcqIndex = 0; nAcqIndex < nAcqFrameCount; ++nAcqIndex)
                    {
                        int nSubFrameCount = _captureSliceAcqImageList[nAcqIndex].Count;

                        for (int nSubFrameIndex = 1; nSubFrameIndex < nSubFrameCount; ++nSubFrameIndex)
                        {
                            string strSlicedFileName = strFolderPath + "//" + $"Sliced_{nAcqIndex}_{nSubFrameIndex}" + ".png";
                            _captureSliceAcqImageList[nAcqIndex][nSubFrameIndex].SaveImage(strSlicedFileName);
                        }

                        string strFileName = strFolderPath + "//" + $"Merged_{nAcqIndex}" + ".png";
                        using Mat mat = _captureSliceAcqImageList[nAcqIndex][0].Clone();
                        mat.SaveImage(strFileName);
                    }
                }
            });
        }

        private async Task saveROICaptureImage_()
        {
            if (_captureSliceAcqImageList.Count <= 0)
            {
                return;
            }

            await Task.Run(() =>
            {
                string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_ROI";

                if (!Directory.Exists(strFolderPath))
                {
                    Directory.CreateDirectory(strFolderPath);
                }

                int nAcqFrameCount = _captureSliceAcqImageList.Count;

                if (nAcqFrameCount == 1 && _captureSliceAcqImageList[0].Count == 1)
                {
                    string strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_Merged.png";
                    using Mat mat = _captureSliceAcqImageList[0][0].Clone();
                    mat.SaveImage(strFileName);
                }
                else
                {
                    for (int nAcqIndex = 0; nAcqIndex < nAcqFrameCount; ++nAcqIndex)
                    {
                        int nSubFrameCount = _captureSliceAcqImageList[nAcqIndex].Count;

                        for (int nSubFrameIndex = 1; nSubFrameIndex < nSubFrameCount; ++nSubFrameIndex)
                        {
                            string strSlicedFileName = strFolderPath + "//" + $"Sliced_{nAcqIndex}_{nSubFrameIndex}" + ".png";
                            _captureSliceAcqImageList[nAcqIndex][nSubFrameIndex].SaveImage(strSlicedFileName);
                        }

                        string strFileName = strFolderPath + "//" + $"Merged_{nAcqIndex}" + ".png";
                        using Mat mat = _captureSliceAcqImageList[nAcqIndex][0].Clone();
                        mat.SaveImage(strFileName);
                    }
                }
            });
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
                else if (IsReviewSliceMode == true)
                {
                    UpdateReviewSliceStatusItems(width, height);
                    SetReviewSliceMode();
                }
                else if (IsReviewROIMode == true)
                {
                    UpdateReviewROIStatusItems(width, height);
                    SetReviewROIMode();
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
                    Mat adjustedMat = ApplyImageAdjustments(_captureImageList[nImageIndex], GetAdjustParam(nImageIndex));

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
                                    Mat adjustedMat = ApplyImageAdjustments(
                                        _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1],
                                        GetSliceAdjustParam(nAcqFrameIndex - 1, nIndex - 1));

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

        private void mySliderReviewROIImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (sender is Slider slider)
            {
                int nIndex = (int)slider.Value;
                if (nIndex > 0)
                {
                    if (_captureSliceAcqImageList.Count() > 0)
                    {
                        int oldSubImageIndex = (int)mySliderReviewROISubImageIndex.Value;

                        mySliderReviewROISubImageIndex.Value = mySliderReviewROISubImageIndex.Minimum;

                        if (oldSubImageIndex == mySliderReviewROISubImageIndex.Minimum)
                        {
                            var args = new RoutedPropertyChangedEventArgs<double>(1, 1)
                            {
                                RoutedEvent = Slider.ValueChangedEvent
                            };

                            mySliderReviewROISubImageIndex.RaiseEvent(args);
                        }
                    }
                }
            }
        }

        private void mySliderReviewROISubImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (sender is Slider slider)
            {
                int nIndex = (int)slider.Value;
                if (nIndex > 0)
                {
                    if (_captureSliceAcqImageList.Count() > 0)
                    {
                        int nAcqFrameIndex = (int)mySliderReviewROIImageIndex.Value;

                        if (nIndex == 1)
                        {

                            _ = Task.Run(() =>
                            {
                                try
                                {
                                    Mat adjustedMat = ApplyImageAdjustments(
                                        _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1],
                                        GetSliceAdjustParam(nAcqFrameIndex - 1, nIndex - 1));

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

                            UpdateReviewROIStatusItems(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Width, _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Height);
                            imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1]);
                        }
                    }
                }
            }
        }

        private ImageAdjustParam GetAdjustParam(int nImageIndex)
        {
            if (nImageIndex >= 0 && nImageIndex < _captureImageAdjustParams.Count)
            {
                return _captureImageAdjustParams[nImageIndex];
            }

            return CreateAdjustParamFromDefault();
        }

        private Mat ApplyImageAdjustments(Mat source, ImageAdjustParam adjustParam)
        {
            Mat adjustedMat = source.Clone();

            if (adjustParam.IsRemoveReflectedLight)
            {
                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
                adjustedMat.Dispose();
                adjustedMat = tempMat;
            }

            if (adjustParam.Brightness != 0)
            {
                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
                adjustedMat.Dispose();
                adjustedMat = tempMat;
            }

            if (adjustParam.IsRadialCorrection)
            {
                Mat correctedMat;
                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
                {
                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
                        adjustedMat, adjustParam.RadialCorrectionCurve);
                }
                else
                {
                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
                        adjustedMat, adjustParam.RadialCorIntensity);
                }

                adjustedMat.Dispose();
                adjustedMat = correctedMat;
            }

            return adjustedMat;
        }

        private Mat adjustBrightnessWinStyle(Mat source, int brightness)
        {
            const int BrightnessMin = -127;
            const int BrightnessMax = 127;
            int boundedBrightness = Math.Clamp(brightness, BrightnessMin, BrightnessMax);
            if (boundedBrightness == 0)
                return source.Clone();

            if (source.Empty())
                throw new ArgumentException("Source image is empty.");

            bool is16Bit = source.Depth() == MatType.CV_16U;
            double maxRange = is16Bit ? 65535.0 : 255.0;

            source.MinMaxLoc(out double minVal, out double maxVal);

            double normalizedBrightness = boundedBrightness / 127.0;
            double center = (minVal + maxVal) / 2.0;
            double halfRange = (maxVal - minVal) / 2.0;

            double contrastFactor = 1.0 - normalizedBrightness * 1.0;
            double newHalfRange = halfRange * contrastFactor;
            double newCenter = center - normalizedBrightness * halfRange;

            double newMin = newCenter - newHalfRange;
            double newMax = newCenter + newHalfRange;

            if (Math.Abs(newMax - newMin) < 1e-6)
                newMax = newMin + 1.0;

            Mat result = new Mat();
            double scale = maxRange / (newMax - newMin);
            double offset = -newMin * scale;

            source.ConvertTo(result, -1, scale, offset);

            if (is16Bit)
            {
                Cv2.Threshold(result, result, 65535, 65535, ThresholdTypes.Trunc);
                Cv2.Threshold(result, result, 0, 0, ThresholdTypes.Tozero);
            }

            if (boundedBrightness > 10)
            {
                Mat denoisedResult = ApplyDenoising(result, boundedBrightness);
                result.Dispose();
                return denoisedResult;
            }

            return result;
        }

        private Mat ApplyDenoising(Mat source, int brightness)
        {
            if (source.Empty())
            {
                return source.Clone();
            }

            Mat result = new Mat();
            bool is16Bit = source.Depth() == MatType.CV_16U;

            try
            {
                if (is16Bit)
                {
                    Mat normalized = new Mat();
                    Mat processed = new Mat();
                    source.ConvertTo(normalized, MatType.CV_8UC3, 1.0 / 256.0);
                    ApplyDenoisingTo8Bit(normalized, processed, brightness);
                    processed.ConvertTo(result, MatType.CV_16UC3, 256.0);
                    normalized.Dispose();
                    processed.Dispose();
                }
                else
                {
                    ApplyDenoisingTo8Bit(source, result, brightness);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Denoising error: {ex.Message}");
                result?.Dispose();
                return source.Clone();
            }

            return result;
        }

        private void ApplyDenoisingTo8Bit(Mat source, Mat result, int brightness)
        {
            if (brightness <= 30)
            {
                double sigma = 0.8 + (brightness / 100.0) * 0.4;
                Cv2.GaussianBlur(source, result, new OpenCvSharp.Size(3, 3), sigma);
            }
            else
            {
                int d = 7;
                double sigmaColor = 40.0 + (brightness / 100.0) * 40.0;
                double sigmaSpace = 40.0 + (brightness / 100.0) * 40.0;

                try
                {
                    Cv2.BilateralFilter(source, result, d, sigmaColor, sigmaSpace);
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"BilateralFilter error, fallback to Gaussian: {ex.Message}");
                    double sigma = 1.0 + (brightness / 100.0) * 0.5;
                    Cv2.GaussianBlur(source, result, new OpenCvSharp.Size(5, 5), sigma);
                }
            }
        }

        private Mat applyRemoveReflectedLight(Mat source)
        {
            string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_ReflectedLight";
            if (Directory.Exists(strFolderPath) == false)
            {
                Directory.CreateDirectory(strFolderPath);
            }

            string[] files = Directory.GetFiles(strFolderPath);
            if (files.Length == 0)
            {
                return source.Clone();
            }

            Mat reflectedMat = Cv2.ImRead(files[0], ImreadModes.Color);
            if (reflectedMat == null || reflectedMat.Empty())
            {
                reflectedMat?.Dispose();
                return source.Clone();
            }

            if (source.Width != reflectedMat.Width || source.Height != reflectedMat.Height)
            {
                reflectedMat.Dispose();
                return source.Clone();
            }

            Mat resultMat = new Mat();
            Cv2.Subtract(source, reflectedMat, resultMat);
            reflectedMat.Dispose();
            return resultMat;
        }

        public void OpenImageAdjustment()
        {
            if (myGridReview.Visibility == Visibility.Visible)
            {
                OpenReviewImageAdjustment_();
            }
            else if (myGridReviewSliceMode.Visibility == Visibility.Visible)
            {
                if ((int)mySliderReviewSliceSubImageIndex.Value != 1)
                {
                    MessageBox.Show("Image Adjust is available only for the merged frame (Sub Frame = 1).");
                    return;
                }

                OpenReviewSliceImageAdjustment_();
            }
            else if (myGridReviewROIMode.Visibility == Visibility.Visible)
            {
                if ((int)mySliderReviewROISubImageIndex.Value != 1)
                {
                    MessageBox.Show("Image Adjust is available only for the merged frame (Sub Frame = 1).");
                    return;
                }

                OpenReviewROIImageAdjustment_();
            }
            else
            {
                MessageBox.Show("Image Adjust is available in Review mode.");
            }
        }

        private void OpenReviewImageAdjustment_()
        {
            if (_captureImageList.Count > 0)
            {
                int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;
                AdjustmentWindow.SetDefaultValues(GetAdjustParam(nImageIndex));
            }
            else
            {
                return;
            }

            AdjustmentWindow.SetAdjustmentCallback(OnReviewImageAdjustmentChanged);
            if (!AdjustmentWindow.IsVisible)
            {
                AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);
                AdjustmentWindow.Show();
            }
            else
            {
                AdjustmentWindow.Activate();
            }
        }

        private void OnReviewImageAdjustmentChanged(ImageAdjustParam adjustParam)
        {
            if (_captureImageList.Count <= 0)
            {
                return;
            }

            int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;
            while (_captureImageAdjustParams.Count <= nImageIndex)
            {
                _captureImageAdjustParams.Add(CreateAdjustParamFromDefault());
            }

            PersistAdjustPreset_(adjustParam);

            _ = Task.Run(() =>
            {
                try
                {
                    Mat adjustedMat = ApplyImageAdjustments(_captureImageList[nImageIndex], adjustParam);

                    _captureImageAdjustParams[nImageIndex].IsRemoveReflectedLight = adjustParam.IsRemoveReflectedLight;
                    _captureImageAdjustParams[nImageIndex].Brightness = adjustParam.Brightness;
                    _captureImageAdjustParams[nImageIndex].IsRadialCorrection = adjustParam.IsRadialCorrection;
                    _captureImageAdjustParams[nImageIndex].RadialCorIntensity = adjustParam.RadialCorIntensity;
                    _captureImageAdjustParams[nImageIndex].RadialCorrectionCurve = adjustParam.RadialCorrectionCurve;

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
            });
        }


        
        private ImageAdjustParam GetSliceAdjustParam(int nAcqImageIndex, int nSubImageIndex)
        {
            if (nAcqImageIndex >= 0 && nAcqImageIndex < _captureSliceAdjustParams.Count
                && nSubImageIndex >= 0 && nSubImageIndex < _captureSliceAdjustParams[nAcqImageIndex].Count)
            {
                return _captureSliceAdjustParams[nAcqImageIndex][nSubImageIndex];
            }

            return CreateAdjustParamFromDefault();
        }

        private void StoreSliceAdjustParam(int nAcqImageIndex, int nSubImageIndex, ImageAdjustParam adjustParam)
        {
            while (_captureSliceAdjustParams.Count <= nAcqImageIndex)
            {
                _captureSliceAdjustParams.Add(new List<ImageAdjustParam>());
            }

            while (_captureSliceAdjustParams[nAcqImageIndex].Count <= nSubImageIndex)
            {
                _captureSliceAdjustParams[nAcqImageIndex].Add(CreateAdjustParamFromDefault());
            }

            _captureSliceAdjustParams[nAcqImageIndex][nSubImageIndex].IsRemoveReflectedLight = adjustParam.IsRemoveReflectedLight;
            _captureSliceAdjustParams[nAcqImageIndex][nSubImageIndex].Brightness = adjustParam.Brightness;
            _captureSliceAdjustParams[nAcqImageIndex][nSubImageIndex].IsRadialCorrection = adjustParam.IsRadialCorrection;
            _captureSliceAdjustParams[nAcqImageIndex][nSubImageIndex].RadialCorIntensity = adjustParam.RadialCorIntensity;
            _captureSliceAdjustParams[nAcqImageIndex][nSubImageIndex].RadialCorrectionCurve = adjustParam.RadialCorrectionCurve;
        }

        private void OpenReviewSliceImageAdjustment_()
        {
            if (_captureSliceAcqImageList.Count > 0)
            {
                int nAcqImageIndex = (int)mySliderReviewSliceImageIndex.Value - 1;
                int nSubImageIndex = (int)mySliderReviewSliceSubImageIndex.Value - 1;
                AdjustmentWindow.SetDefaultValues(GetSliceAdjustParam(nAcqImageIndex, nSubImageIndex));
            }
            else
            {
                return;
            }

            AdjustmentWindow.SetAdjustmentCallback(OnReviewSliceImageAdjustmentChanged);
            if (!AdjustmentWindow.IsVisible)
            {
                AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);
                AdjustmentWindow.Show();
            }
            else
            {
                AdjustmentWindow.Activate();
            }
        }

        private void OnReviewSliceImageAdjustmentChanged(ImageAdjustParam adjustParam)
        {
            if (_captureSliceAcqImageList.Count <= 0)
            {
                return;
            }

            int nAcqImageIndex = (int)mySliderReviewSliceImageIndex.Value - 1;
            int nSubImageIndex = (int)mySliderReviewSliceSubImageIndex.Value - 1;

            _ = Task.Run(() =>
            {
                try
                {
                    Mat adjustedMat = ApplyImageAdjustments(
                        _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex],
                        adjustParam);

                    StoreSliceAdjustParam(nAcqImageIndex, nSubImageIndex, adjustParam);
                    PersistAdjustPreset_(adjustParam);

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
            });
        }

        private void OpenReviewROIImageAdjustment_()
        {
            if (_captureSliceAcqImageList.Count > 0)
            {
                int nAcqImageIndex = (int)mySliderReviewROIImageIndex.Value - 1;
                int nSubImageIndex = (int)mySliderReviewROISubImageIndex.Value - 1;
                AdjustmentWindow.SetDefaultValues(GetSliceAdjustParam(nAcqImageIndex, nSubImageIndex));
            }
            else
            {
                return;
            }

            AdjustmentWindow.SetAdjustmentCallback(OnReviewROIImageAdjustmentChanged);
            if (!AdjustmentWindow.IsVisible)
            {
                AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);
                AdjustmentWindow.Show();
            }
            else
            {
                AdjustmentWindow.Activate();
            }
        }

        private void OnReviewROIImageAdjustmentChanged(ImageAdjustParam adjustParam)
        {
            if (_captureSliceAcqImageList.Count <= 0)
            {
                return;
            }

            int nAcqImageIndex = (int)mySliderReviewROIImageIndex.Value - 1;
            int nSubImageIndex = (int)mySliderReviewROISubImageIndex.Value - 1;

            _ = Task.Run(() =>
            {
                try
                {
                    Mat adjustedMat = ApplyImageAdjustments(
                        _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex],
                        adjustParam);

                    StoreSliceAdjustParam(nAcqImageIndex, nSubImageIndex, adjustParam);
                    PersistAdjustPreset_(adjustParam);

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
            });
        }

        #region EventHandler - Save

        private void myBtReviewSaveAll_Click(object sender, RoutedEventArgs e)
        {
            if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
            {
                string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
                Process.Start("explorer.exe", strFolderPath);
                return;
            }

            _ = saveAllImages_();
        }

        private void myBtReviewSliceSaveAll_Click(object sender, RoutedEventArgs e)
        {
            if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
            {
                string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
                Process.Start("explorer.exe", strFolderPath);
                return;
            }

            _ = saveSliceImage_();
        }

        private void myBtReviewROISaveAll_Click(object sender, RoutedEventArgs e)
        {
            if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
            {
                string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
                Process.Start("explorer.exe", strFolderPath);
                return;
            }

            _ = saveROICaptureImage_();
        }

        #endregion EventHandler - Save
    }
}
