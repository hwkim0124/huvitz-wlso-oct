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
        //Mat _previewMat = new();

        //Mat _imageIrMat = new();
        //Mat _frameIrMat = new();

        int _imageWidth = 0;
        int _imageHeight = 0;
        int _imageChannels = 0;
        float _imageQuality = 0.0f;

        //const int AVERAGE_SIZE = 8;
        //private const int NUM_CHANNELS = 3;

        ////Frame Rate ///////////////////////////////////
        private readonly FrameRateCalculator _fpsCalc = new();
        //// /////////////////////////////////////////////

        //public bool IsPreviewMode { get; set; } = true;
        //public bool IsOverlayAlignGuide { get; set; } = false;
        //public bool IsOverlayDivideGuide { get; set; } = false;

        //// DivideGuide
        //public int DivideLine = 0;

        //// Review Mode
        public bool IsReviewMode { get; set; } = true;
        List<Mat> _captureImageList = new List<Mat>();
        //List<Tuple<Mat, ImageAdjustParam>> _captureImageAndParamList = new();

        //// Review Slice Mode
        //public bool IsReviewSliceMode { get; set; } = false;
        //List<Mat> _captureSliceImageList = new List<Mat>();
        //List<List<Tuple<Mat, ImageAdjustParam>>> _captureSliceAcqImageList = new List<List<Tuple<Mat, ImageAdjustParam>>>();
        public int AcqFrameCount = 0;
        public int SubFrameCount = 0;

        //// Review ROI Mode
        //public bool IsReviewROIMode { get; set; } = false;
        //public FrameROIPosition[] FrameROIs = new FrameROIPosition[0];


        //// Live Seq ROI Mode
        //public bool IsLiveSeqROIMode { get; set; } = false;
        //public FrameSeqROIParam[] FrameSeqROIs = new FrameSeqROIParam[0];
        //public int SeqTargetImageMaxWidth = 0;
        //public int SeqTargetImageMaxHeight = 0;

        //// Offset ROI Mode
        //public bool IsLiveOffsetROIMode { get; set; } = false;
        //public bool IsReviewOffsetROIMode { get; set; } = false;
        //public FrameOffsetROIParam[] FrameOffsetROIs = new FrameOffsetROIParam[0];
        //public int OffsetRoiImageWidth = 0;
        //public int OffsetRoiImageHeight = 0;
        //public int OffsetRoiTargetImageMaxWidth = 0;
        //public int OffsetRoiTargetImageMaxHeight = 0;
        //public Mat _offsetROIMergedMat = new Mat();

        ////Roll SWTrig Overlap Mode
        //public bool IsLiveRollSWTrigOverlapMode { get; set; } = false;
        //public bool IsReviewRollSWTrigOverlapMode { get; set; } = false;
        //public int ReviewRollSWTrigOverlapBrightness = 0;

        ////Roll SWTrig Manual Mode
        //public bool IsRemoveReflectedLight { get; set; } = false;
        //List<Tuple<Mat, ImageAdjustParam>> _captureShootImageList = new();


        ////////////////////////////////////////////////////////////////////
        ////Mask
        //public bool IsMaskOn { get; set; } = false;

        //private int _maskColorRadius = 1000;
        //public int MaskColorRadius
        //{
        //    get => _maskColorRadius;
        //    set
        //    {
        //        _maskColorRadius = value;
        //        InvalidateColorMaskCache(); // 반지름 변경 시 캐시 무효화
        //    }
        //}

        //private int _maskIrRadius = 240;
        //public int MaskIrRadius
        //{
        //    get => _maskIrRadius;
        //    set
        //    {
        //        _maskIrRadius = value;
        //        InvalidateIrMaskCache(); // 반지름 변경 시 캐시 무효화
        //    }
        //}

        //// 마스크 캐싱을 위한 필드
        //private MaskCache _irMaskCache;
        //private MaskCache _colorMaskCache;

        ///////////////////////////////////////////////////////////////////////

        //// Image Adjustment
        //private ImageAdjustmentWindow? _adjustmentWindow;
        //public int ImageAdjustBrightness = 0;

        //private ImageAdjustmentWindow AdjustmentWindow
        //{
        //    get
        //    {
        //        if (_adjustmentWindow == null)
        //            _adjustmentWindow = new ImageAdjustmentWindow();
        //        return _adjustmentWindow;
        //    }
        //}

        //public void CloseAdjustmentWindow()
        //{
        //    _adjustmentWindow?.ForceClose();
        //    _adjustmentWindow = null;
        //}

        public LsoScanImagePreview()
        {
            InitializeComponent();
        }

        private void UpdatePreviewStatusItems()
        {
            string s = string.Format("{0} x {1}, {2} channels, {3:F1} fps, IQS: {4:F2}", _imageWidth, _imageHeight, _imageChannels, _fpsCalc.FrameRate, _imageQuality);
            lblImageStatusLive.Content = s;
        }

        //private void UpdateReviewOffsetROIStatusItems()
        //{
        //    string s = string.Format("{0} x {1}, {2} channels, IQS: {3:F2}", _imageWidth, _imageHeight, _imageChannels, _imageQuality);
        //    lblImageStatusReviewOffsetROI.Content = s;
        //}

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

            var bitmapSource = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);
            bitmapSource.Freeze();
            imageViewport.Source = bitmapSource;
        }

        //public void UpdateIrFrameImage()
        //{
        //    if (_imageIrMat.Empty())
        //    {
        //        return;
        //    }

        //    Cv2.Flip(_imageIrMat, _imageIrMat, FlipMode.Y);

        //    Cv2.CvtColor(_imageIrMat, _frameIrMat, ColorConversionCodes.GRAY2BGR);

        //    DrawOverlayAlignGuide(ref _frameIrMat);
        //    DrawOverlayDivideGuide(ref _frameIrMat, DivideLine);
        //    DrawIrMask(ref _frameIrMat, _maskIrRadius);

        //    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameIrMat);
        //}

        public void UpdateFramerate() => _fpsCalc.Tick();

        public void StartTimer() => _fpsCalc.Start();

        public void StopTimer() => _fpsCalc.Stop();

        //private void DrawOverlayAlignGuide(ref Mat image)
        //{
        //    if (!IsOverlayAlignGuide) return;

        //    int W = image.Width;
        //    int H = image.Height;
        //    int minDim = Math.Min(W, H);
        //    OpenCvSharp.Point C = new OpenCvSharp.Point(W / 2, H / 2);

        //    // ——— 파라미터 (원하는 비율로 변경) ———
        //    double majorTickRatio = 0.02;  // 큰 눈금 길이 = minDim * 2%
        //    double minorTickRatio = 0.01;  // 작은 눈금 길이 = minDim * 1%
        //    int majorDiv = 4;     // 큰 눈금 개수 (한 방향 기준)
        //    int minorPerMajor = 4;     // 큰 눈금마다 작은 눈금 개수

        //    Scalar color = new Scalar(0, 255, 255);
        //    int thickness = Math.Max(1, (int)(minDim * 0.002)); // 0.2% 두께, 최소 1px

        //    // ——— 계산 ———
        //    int lineLen = (int)minDim;
        //    int majorTickLen = (int)(minDim * majorTickRatio);
        //    int minorTickLen = (int)(minDim * minorTickRatio);
        //    double majorStep = (double)lineLen / (majorDiv + 1);
        //    double minorStep = majorStep / (minorPerMajor + 1);

        //    // ——— 중앙 크로스 그리기 ———
        //    Cv2.Line(image, C.X - lineLen, C.Y, C.X + lineLen, C.Y, color, thickness, LineTypes.AntiAlias);
        //    Cv2.Line(image, C.X, C.Y - lineLen, C.X, C.Y + lineLen, color, thickness, LineTypes.AntiAlias);

        //    // ——— 큰 눈금 그리기 ———
        //    for (int i = 1; i <= majorDiv; i++)
        //    {
        //        int dx = (int)(majorStep * i);
        //        // 수평선 위의 큰 눈금 (수직 방향으로 표시)
        //        Cv2.Line(image,
        //            new OpenCvSharp.Point(C.X - dx, C.Y - majorTickLen),
        //            new OpenCvSharp.Point(C.X - dx, C.Y + majorTickLen),
        //            color, thickness, LineTypes.AntiAlias);
        //        Cv2.Line(image,
        //            new OpenCvSharp.Point(C.X + dx, C.Y - majorTickLen),
        //            new OpenCvSharp.Point(C.X + dx, C.Y + majorTickLen),
        //            color, thickness, LineTypes.AntiAlias);

        //        int dy = (int)(majorStep * i);
        //        // 수직선 위의 큰 눈금 (수평 방향으로 표시)
        //        Cv2.Line(image,
        //            new OpenCvSharp.Point(C.X - majorTickLen, C.Y - dy),
        //            new OpenCvSharp.Point(C.X + majorTickLen, C.Y - dy),
        //            color, thickness, LineTypes.AntiAlias);
        //        Cv2.Line(image,
        //            new OpenCvSharp.Point(C.X - majorTickLen, C.Y + dy),
        //            new OpenCvSharp.Point(C.X + majorTickLen, C.Y + dy),
        //            color, thickness, LineTypes.AntiAlias);
        //    }

        //    // ——— 작은 눈금 그리기 ———
        //    for (int gi = 0; gi <= majorDiv; gi++)
        //    {
        //        for (int sj = 1; sj <= minorPerMajor; sj++)
        //        {
        //            double baseOff = majorStep * (gi + 1);
        //            double off = baseOff - majorStep + minorStep * sj;

        //            int dx = (int)off;
        //            // 수평선 위 작은 눈금
        //            Cv2.Line(image,
        //                new OpenCvSharp.Point(C.X - dx, C.Y - minorTickLen),
        //                new OpenCvSharp.Point(C.X - dx, C.Y + minorTickLen),
        //                color, thickness, LineTypes.AntiAlias);
        //            Cv2.Line(image,
        //                new OpenCvSharp.Point(C.X + dx, C.Y - minorTickLen),
        //                new OpenCvSharp.Point(C.X + dx, C.Y + minorTickLen),
        //                color, thickness, LineTypes.AntiAlias);

        //            int dy = (int)off;
        //            // 수직선 위 작은 눈금
        //            Cv2.Line(image,
        //                new OpenCvSharp.Point(C.X - minorTickLen, C.Y - dy),
        //                new OpenCvSharp.Point(C.X + minorTickLen, C.Y - dy),
        //                color, thickness, LineTypes.AntiAlias);
        //            Cv2.Line(image,
        //                new OpenCvSharp.Point(C.X - minorTickLen, C.Y + dy),
        //                new OpenCvSharp.Point(C.X + minorTickLen, C.Y + dy),
        //                color, thickness, LineTypes.AntiAlias);
        //        }
        //    }
        //}

        //private void DrawOverlayDivideGuide(ref Mat image, int nDivide)
        //{
        //    if (!IsOverlayDivideGuide) return;

        //    int rows = image.Rows;
        //    int cols = image.Cols;

        //    // 선 색상, 두께 설정 (BGR)
        //    Scalar lineColor = new Scalar(0, 255, 255); // 노란색
        //    int thickness = Math.Max(1, (int)(rows * 0.002)); // 이미지 크기 비례

        //    // 1 ~ divisions-1 위치에 선 그리기
        //    for (int i = 1; i < nDivide; i++)
        //    {
        //        int y = rows * i / nDivide;
        //        Cv2.Line(
        //            image,
        //            new OpenCvSharp.Point(0, y),
        //            new OpenCvSharp.Point(cols - 1, y),
        //            lineColor,
        //            thickness,
        //            LineTypes.AntiAlias);
        //    }
        //}

        //private void DrawIrMask(ref Mat image, int radius)
        //{
        //    if (IsMaskOn == false)
        //        return;

        //    if (image == null || image.Empty() || radius <= 0)
        //        return;

        //    try
        //    {
        //        var imageSize = image.Size();

        //        // 1. 마스크 캐시 확인 및 생성 (이미지 크기나 반지름이 변경된 경우에만)
        //        if (_irMaskCache.NeedsUpdate(imageSize, radius))
        //        {
        //            UpdateMaskCache(ref _irMaskCache, imageSize, radius);
        //        }

        //        // 2. 최적화된 마스킹 적용 - SetTo 방식 (가장 빠름)
        //        ApplyMaskOptimized(image, _irMaskCache.InvertedMask);
        //    }
        //    catch (Exception ex)
        //    {
        //        System.Diagnostics.Debug.WriteLine($"마스크 적용 중 오류: {ex.Message}");
        //    }
        //}

        //private void DrawColorMask(ref Mat image, int radius)
        //{
        //    if (IsMaskOn == false)
        //        return;

        //    if (image == null || image.Empty() || radius <= 0)
        //        return;

        //    try
        //    {
        //        var imageSize = image.Size();

        //        // 1. 마스크 캐시 확인 및 생성 (이미지 크기나 반지름이 변경된 경우에만)
        //        if (_colorMaskCache.NeedsUpdate(imageSize, radius))
        //        {
        //            UpdateMaskCache(ref _colorMaskCache, imageSize, radius);
        //        }

        //        // 2. 최적화된 마스킹 적용 - SetTo 방식 (가장 빠름)
        //        ApplyMaskOptimized(image, _colorMaskCache.InvertedMask);
        //    }
        //    catch (Exception ex)
        //    {
        //        System.Diagnostics.Debug.WriteLine($"마스크 적용 중 오류: {ex.Message}");
        //    }
        //}

        //private void ApplyMaskOptimized(Mat image, Mat invertedMask)
        //{
        //    //PixelDepth가 8bit인지 16bit인지에 따라 gray value 지정
        //    var grayValue = image.Depth() == MatType.CV_16U ? 32767 : 128;

        //    // 마스크 영역 외부를 회색으로 직접 설정
        //    var grayColor = image.Channels() == 3 ?
        //        new Scalar(grayValue, grayValue, grayValue) : new Scalar(grayValue);

        //    image.SetTo(grayColor, invertedMask);
        //}

        //private void UpdateMaskCache(ref MaskCache maskCache, OpenCvSharp.Size imageSize, int radius)
        //{
        //    // 기존 캐시 해제
        //    maskCache.Dispose();

        //    // 새 마스크 생성
        //    maskCache.Mask = new Mat(imageSize, MatType.CV_8UC1, Scalar.All(0));
        //    maskCache.InvertedMask = new Mat(imageSize, MatType.CV_8UC1);

        //    // 이미지 중심점 계산
        //    int centerX = imageSize.Width / 2;
        //    int centerY = imageSize.Height / 2;

        //    // 원형 마스크 그리기
        //    Cv2.Circle(maskCache.Mask, new OpenCvSharp.Point(centerX, centerY), radius, Scalar.White, -1);

        //    // 반전 마스크도 미리 생성
        //    Cv2.BitwiseNot(maskCache.Mask, maskCache.InvertedMask);

        //    // 캐시 정보 업데이트
        //    maskCache.Radius = radius;
        //    maskCache.ImageSize = imageSize;

        //    System.Diagnostics.Debug.WriteLine($"마스크 캐시 업데이트: 크기={imageSize}, 반지름={radius}");
        //}

        //// 리소스 정리 - 컨트롤 해제 시 호출
        //public void DisposeIrMaskResources()
        //{
        //    _irMaskCache.Dispose();
        //}

        //public void DisposeColorMaskResources()
        //{
        //    _colorMaskCache.Dispose();
        //}

        //// 마스크 설정 변경 시 캐시 무효화
        //public void InvalidateIrMaskCache()
        //{
        //    _irMaskCache.Invalidate();
        //}

        //public void InvalidateColorMaskCache()
        //{
        //    _colorMaskCache.Invalidate();
        //}

        public void SetLiveMode()
        {
            showDisplayMenu_(PreviewDisplayMode.LIVE);

            ClearReviewImages();
            IsReviewMode = false;
            //IsReviewSliceMode = false;
        }

        public void ClearReviewImages()
        {
            _captureImageList.Clear();
            //_captureSliceImageList.Clear();
            //_captureSliceAcqImageList.Clear();

            //_captureImageAndParamList.Clear();
            //_captureShootImageList.Clear();
        }

        public void makeReviewImage(Mat originMat, int nPixelFormat)
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
                    //eConversionCodes = ColorConversionCodes.2Gray
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

            _captureImageList.Add(_resultMat);
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
                    //eConversionCodes = ColorConversionCodes.2Gray
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

        ///// <summary>
        ///// List를 chunkSize만큼 잘라서 List<List<T>> 형태로 반환하는 메서드
        ///// </summary>
        //static List<List<Tuple<Mat, ImageAdjustParam>>> SplitListBySize(List<Tuple<Mat, ImageAdjustParam>> source, int chunkSize)
        //{
        //    var result = new List<List<Tuple<Mat, ImageAdjustParam>>>();
        //    if (chunkSize <= 0)
        //        throw new ArgumentException("chunkSize는 1 이상의 값이어야 합니다.", nameof(chunkSize));

        //    int count = source.Count;
        //    for (int i = 0; i < count; i += chunkSize)
        //    {
        //        // (i부터 chunkSize 개수)만큼 잘라서 새로운 List로 만든다.
        //        int size = Math.Min(chunkSize, count - i);
        //        List<Tuple<Mat, ImageAdjustParam>> chunk = source.GetRange(i, size);
        //        result.Add(chunk);
        //    }
        //    return result;
        //}

        private void showDisplayMenu_(PreviewDisplayMode mode)
        {
            myGridLive.Visibility = Visibility.Hidden;
            myGridReview.Visibility = Visibility.Hidden;
            //myGridReviewSliceMode.Visibility = Visibility.Hidden;
            //myGridReviewROIMode.Visibility = Visibility.Hidden;
            //myGridReviewOffsetROI.Visibility = Visibility.Hidden;
            //myGridReviewRollSWTrigOverlap.Visibility = Visibility.Hidden;
            //myGridReviewRollSWTrigManual.Visibility = Visibility.Hidden;

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
                //case PreviewDisplayMode.REVIEW_SLICE:
                //    {
                //        myGridReviewSliceMode.Visibility = Visibility.Visible;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROI:
                //    {
                //        myGridReviewROIMode.Visibility = Visibility.Visible;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_OFFSET_ROI:
                //    {
                //        myGridReviewOffsetROI.Visibility = Visibility.Visible;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_OVERLAP:
                //    {
                //        myGridReviewRollSWTrigOverlap.Visibility = Visibility.Visible;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_MANUAL:
                //    {
                //        myGridReviewRollSWTrigManual.Visibility = Visibility.Visible;
                //    }
                //    break;
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
                //case PreviewDisplayMode.REVIEW_SLICE:
                //    {
                //        mySliderReviewSliceImageIndex.Minimum = 1;
                //        mySliderReviewSliceImageIndex.Maximum = _captureSliceAcqImageList.Count;
                //        myTbSliceAcqCurIndex.Text = "1";
                //        myTbSliceAcqTotalCount.Text = _captureSliceAcqImageList.Count.ToString();

                //        mySliderReviewSliceSubImageIndex.Minimum = 1;
                //        mySliderReviewSliceSubImageIndex.Maximum = _captureSliceAcqImageList[0].Count;
                //        myTbSliceSubCurIndex.Text = "1";
                //        myTbSliceSubTotalCount.Text = _captureSliceAcqImageList[0].Count.ToString();
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROI:
                //    {
                //        mySliderReviewROIImageIndex.Minimum = 1;
                //        mySliderReviewROIImageIndex.Maximum = _captureSliceAcqImageList.Count;
                //        myTbROIAcqCurIndex.Text = "1";
                //        myTbROIAcqTotalCount.Text = _captureSliceAcqImageList.Count.ToString();

                //        mySliderReviewROISubImageIndex.Minimum = 1;
                //        mySliderReviewROISubImageIndex.Maximum = _captureSliceAcqImageList[0].Count;
                //        myTbROISubCurIndex.Text = "1";
                //        myTbROISubTotalCount.Text = _captureSliceAcqImageList[0].Count.ToString();
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_OVERLAP:
                //    {
                //        //mySliderReviewRollSWTrigOverlapImageIndex.Minimum = 1;
                //        mySliderReviewRollSWTrigOverlapImageIndex.Maximum = _captureImageList.Count;
                //        myTbReviewRollSWTrigOverlapCurIndex.Text = "1";
                //        myTbReviewRollSWTrigOverlapTotalCount.Text = _captureImageList.Count.ToString();
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_MANUAL:
                //    {
                //    }
                //    break;
            }
        }

        //#region Method - Review

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
                    //// 원본 이미지를 복사하여 조정 적용
                    //Mat adjustedMat = _captureImageAndParamList[0].Item1.Clone();
                    //ImageAdjustParam adjustParam = _captureImageAndParamList[0].Item2;

                    //// 1. 반사광 제거 적용 (원본에서 먼저 적용)
                    //if (adjustParam.IsRemoveReflectedLight == true)
                    //{
                    //    Mat tempMat = applyRemoveReflectedLight(adjustedMat);
                    //    adjustedMat.Dispose();
                    //    adjustedMat = tempMat;
                    //}

                    //// 1. Brightness 조정
                    //if (adjustParam.Brightness != 0)
                    //{
                    //    Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
                    //    adjustedMat.Dispose();
                    //    adjustedMat = tempMat;
                    //}

                    //// 2. Radial Correction 적용 (곡선 기반)
                    //if (adjustParam.IsRadialCorrection == true)
                    //{
                    //    Mat correctedMat;

                    //    // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
                    //    if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
                    //    {
                    //        correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
                    //            adjustedMat, adjustParam.RadialCorrectionCurve);
                    //    }
                    //    else
                    //    {
                    //        correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
                    //            adjustedMat, adjustParam.RadialCorIntensity);
                    //    }

                    //    adjustedMat.Dispose();
                    //    adjustedMat = correctedMat;
                    //}


                    //// 4. 마스크 적용
                    //DrawColorMask(ref adjustedMat, _maskColorRadius);

                    //// 5. UI 업데이트 (UI 스레드에서)
                    //Dispatcher.Invoke(() =>
                    //{
                    //    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
                    //});

                    //adjustedMat.Dispose();

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

        private void UpdateReviewStatusItems(int nWidth, int nHeight)
        {
            string s = string.Format("{0} x {1}", nWidth, nHeight);
            lblImageStatusReview.Content = s;
        }

        //private async Task saveSingleImage_()
        //{
        //    if (_captureImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nIndex = (int)mySliderReviewImageIndex.Value;

        //    await Task.Run(() =>
        //    {
        //        if (nIndex > 0)
        //        {
        //            Mat mat = _captureImageList[nIndex - 1].Clone();

        //            string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";

        //            if (Directory.Exists(strFolderPath) == false)
        //            {
        //                Directory.CreateDirectory(strFolderPath);
        //            }
        //            string strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + $"_Index_{nIndex.ToString()}" + ".png";

        //            DrawColorMask(ref mat, _maskColorRadius);

        //            mat.SaveImage(strFileName);
        //        }
        //    });
        //}

        //private async Task saveAllImages_()
        //{
        //    if (_captureImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    await Task.Run(() =>
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss");

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        for (int i = 0; i < _captureImageList.Count(); ++i)
        //        {
        //            Mat mat = _captureImageList[i].Clone();
        //            string strFileName = strFolderPath + "//" + $"Index_{i + 1}" + ".png";
        //            DrawColorMask(ref mat, _maskColorRadius);
        //            mat.SaveImage(strFileName);
        //            mat.Dispose();
        //        }
        //    });
        //}

        //private async Task saveSingleAdjustImage_()
        //{
        //    if (_captureImageAndParamList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;


        //    int nBright = _captureImageAndParamList[nImageIndex].Item2.Brightness;
        //    bool bRadialCor = _captureImageAndParamList[nImageIndex].Item2.IsRadialCorrection;
        //    float fRadialCorrectionIntensity = _captureImageAndParamList[nImageIndex].Item2.RadialCorIntensity;
        //    double[] radialCorrectionCurve = _captureImageAndParamList[nImageIndex].Item2.RadialCorrectionCurve; // 곡선 데이터 추가
        //    bool bRemoveReflectedLight = _captureImageAndParamList[nImageIndex].Item2.IsRemoveReflectedLight;

        //    await Task.Run(() =>
        //    {
        //        string strFileName;
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        // 원본 이미지를 복사하여 조정 적용
        //        Mat adjustedMat = _captureImageAndParamList[nImageIndex].Item1.Clone();

        //        if (bRemoveReflectedLight == true)
        //        {
        //            Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //            adjustedMat.Dispose();
        //            adjustedMat = tempMat;
        //        }

        //        // 1. Brightness 조정
        //        if (nBright != 0)
        //        {
        //            Mat tempMat = adjustBrightnessWinStyle(adjustedMat, nBright);
        //            adjustedMat.Dispose();
        //            adjustedMat = tempMat;
        //        }

        //        // 2. Radial Correction 적용 (곡선 기반 또는 강도 기반)
        //        if (bRadialCor == true)
        //        {
        //            Mat correctedMat;

        //            // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //            if (radialCorrectionCurve != null && radialCorrectionCurve.Length > 0)
        //            {
        //                correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                    adjustedMat, radialCorrectionCurve);
        //                strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                    $"_Review_Bright_{nBright}_RadialCurve" + ".png";
        //            }
        //            else
        //            {
        //                correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                    adjustedMat, fRadialCorrectionIntensity);
        //                strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                    $"_Review_Bright_{nBright}_Radial_{fRadialCorrectionIntensity}" + ".png";
        //            }

        //            adjustedMat.Dispose();
        //            adjustedMat = correctedMat;
        //        }
        //        else
        //        {
        //            strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                $"_Review_Bright_{nBright}" + ".png";
        //        }

        //        DrawColorMask(ref adjustedMat, _maskColorRadius);

        //        adjustedMat.SaveImage(strFileName);

        //        // IR 영상
        //        if (_frameIrMat != null || _frameIrMat?.Empty() == false)
        //        {
        //            strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + $"_Review_IR" + ".png";
        //            _frameIrMat.SaveImage(strFileName);
        //        }

        //        adjustedMat.Dispose();
        //    });
        //}

        //#endregion Method - Review

        //#region Method - Review Slice

        //public void SetReviewSliceMode()
        //{
        //    showDisplayMenu_(PreviewDisplayMode.REVIEW_SLICE);

        //    if (_captureImageAndParamList.Count > 0)
        //    {
        //        if (SubFrameCount == 1)
        //        {
        //            if (AcqFrameCount == 1)
        //            {
        //                List<Tuple<Mat, ImageAdjustParam>> tempSubFrameList = new List<Tuple<Mat, ImageAdjustParam>>() { _captureImageAndParamList[0] };
        //                _captureSliceAcqImageList.Add(tempSubFrameList);
        //            }
        //            else
        //            {
        //                int nFrameCount = _captureImageList.Count();
        //                for (int i = 0; i < nFrameCount; ++i)
        //                {
        //                    List<Tuple<Mat, ImageAdjustParam>> tempSubFrameList = new List<Tuple<Mat, ImageAdjustParam>>() { _captureImageAndParamList[i] };
        //                    _captureSliceAcqImageList.Add(tempSubFrameList);
        //                }
        //            }
        //        }
        //        else
        //        {
        //            int nAcqFrameCount = AcqFrameCount;
        //            int nSubFrameCount = SubFrameCount;
        //            int nFrameCount = _captureImageAndParamList.Count();

        //            if (nAcqFrameCount * nSubFrameCount != nFrameCount)
        //            {
        //                MessageBox.Show("Total frame count is invalid.");
        //                return;
        //            }

        //            int chunkSize = nFrameCount / nAcqFrameCount; // 한 acq frame 당 요소 개수
        //            List<List<Tuple<Mat, ImageAdjustParam>>> tempAcqFrameList = SplitListBySize(_captureImageAndParamList, chunkSize);

        //            // 3) 기본 이미지 크기 (모두 동일하다고 가정)
        //            int height = _captureImageAndParamList[0].Item1.Rows;
        //            int width = _captureImageAndParamList[0].Item1.Cols;

        //            // 4) 분할 높이 계산 (정수 나누기)
        //            int segH = height / nSubFrameCount;

        //            for (int i = 0; i < tempAcqFrameList.Count; ++i)
        //            {
        //                // 5) 각 이미지에서 해당 분할 영역만큼 잘라내기
        //                List<Tuple<Mat, ImageAdjustParam>> tempSubFrameList = new List<Tuple<Mat, ImageAdjustParam>>();
        //                Mat[] Sliced = new Mat[nSubFrameCount];

        //                for (int j = 0; j < nSubFrameCount; ++j)
        //                {
        //                    int y0 = j * segH;
        //                    int h = (j < nSubFrameCount) ? segH : (height - y0);  // 마지막은 남은 만큼 모두
        //                    OpenCvSharp.Rect roi = new OpenCvSharp.Rect(X: 0, Y: y0, Width: width, Height: h);
        //                    Sliced[j] = new Mat(tempAcqFrameList[i][j].Item1, roi).Clone();  // Clone()으로 메모리 분리
        //                    //tempSubFrameList.Add(Sliced[j]);
        //                    tempSubFrameList.Add(new Tuple<Mat, ImageAdjustParam>(Sliced[j], tempAcqFrameList[i][j].Item2));
        //                }

        //                // 6) 세로 방향으로 붙이기
        //                Mat mergedMat = new Mat();
        //                Cv2.VConcat(Sliced, mergedMat);
        //                Tuple<Mat, ImageAdjustParam> merged = new Tuple<Mat, ImageAdjustParam>(mergedMat, new ImageAdjustParam(ImageAdjustBrightness, 0));
        //                tempSubFrameList.Insert(0, merged);

        //                _captureSliceAcqImageList.Add(tempSubFrameList);
        //            }
        //        }

        //        mySliderReviewSliceImageIndex.Value = 1;
        //        mySliderReviewSliceSubImageIndex.Value = 1;

        //        //imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_captureSliceAcqImageList[0][0].Item1);

        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureSliceAcqImageList[0][0].Item1.Clone();
        //            ImageAdjustParam adjustParam = _captureSliceAcqImageList[0][0].Item2;

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            //_captureImageAndParamList[nImageIndex].Item2.Brightness = adjustParam.Brightness;

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    }

        //    resetDisplayMenu_(PreviewDisplayMode.REVIEW_SLICE, _captureSliceImageList.Count());
        //}

        //private void UpdateReviewSliceStatusItems(int nWidth, int nHeight)
        //{
        //    string s = string.Format("{0} x {1}", nWidth, nHeight);
        //    lblImageStatusReviewSlice.Content = s;
        //}

        //private async Task saveSliceImage_()
        //{
        //    if (_captureSliceAcqImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    await Task.Run(() =>
        //    {
        //        string strFileName;
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_Slice";

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        int nAcqFrameCount = _captureSliceAcqImageList.Count;

        //        if (nAcqFrameCount == 1 && _captureSliceAcqImageList[0].Count == 1) // Image가 1 장일 때
        //        {
        //            int nBright = _captureSliceAcqImageList[0][0].Item2.Brightness;
        //            bool bRadialCor = _captureSliceAcqImageList[0][0].Item2.IsRadialCorrection;
        //            float fRadialCorrectionIntensity = _captureSliceAcqImageList[0][0].Item2.RadialCorIntensity;
        //            double[] radialCorrectionCurve = _captureSliceAcqImageList[0][0].Item2.RadialCorrectionCurve; // 곡선 데이터 추가
        //            bool bRemoveReflectedLight = _captureSliceAcqImageList[0][0].Item2.IsRemoveReflectedLight;

        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureSliceAcqImageList[0][0].Item1.Clone();

        //            if (bRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 1. Brightness 조정
        //            if (nBright != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, nBright);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 2. Radial Correction 적용 (곡선 기반 또는 강도 기반)
        //            if (bRadialCor == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (radialCorrectionCurve != null && radialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, radialCorrectionCurve);
        //                    strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                        $"_Merged_Bright_{nBright}_RadialCurve" + ".png";
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, fRadialCorrectionIntensity);
        //                    strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                        $"_Merged_Bright_{nBright}_Radial_{fRadialCorrectionIntensity}" + ".png";
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;
        //            }
        //            else
        //            {
        //                strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                    $"_Merged_Bright_{nBright}" + ".png";
        //            }

        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            /////////////////////////////
        //            adjustedMat.SaveImage(strFileName);
        //            adjustedMat.Dispose();
        //        }
        //        else
        //        {
        //            for (int nAcqIndex = 0; nAcqIndex < nAcqFrameCount; nAcqIndex++)
        //            {
        //                var SubFrameList = _captureSliceAcqImageList[nAcqIndex];
        //                int nSubFrameCount = SubFrameList.Count;
        //                string strSlisedFileName;

        //                for (int nSubFrameIndex = 1; nSubFrameIndex < nSubFrameCount; nSubFrameIndex++)
        //                {
        //                    strSlisedFileName = strFolderPath + "//" + $"Sliced_{nAcqIndex}_{nSubFrameIndex}" + ".png";
        //                    _captureSliceAcqImageList[nAcqIndex][nSubFrameIndex].Item1.SaveImage(strSlisedFileName);
        //                }

        //                //// Merged Image 
        //                int nBright = _captureSliceAcqImageList[nAcqIndex][0].Item2.Brightness;
        //                bool bRadialCor = _captureSliceAcqImageList[nAcqIndex][0].Item2.IsRadialCorrection;
        //                float fRadialCorrectionIntensity = _captureSliceAcqImageList[nAcqIndex][0].Item2.RadialCorIntensity;
        //                double[] radialCorrectionCurve = _captureSliceAcqImageList[nAcqIndex][0].Item2.RadialCorrectionCurve; // 곡선 데이터 추가
        //                bool bRemoveReflectedLight = _captureSliceAcqImageList[nAcqIndex][0].Item2.IsRemoveReflectedLight;

        //                Mat adjustedMat = _captureSliceAcqImageList[nAcqIndex][0].Item1.Clone();

        //                if (bRemoveReflectedLight == true)
        //                {
        //                    Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                    adjustedMat.Dispose();
        //                    adjustedMat = tempMat;
        //                }

        //                // 1. Brightness 조정
        //                if (nBright != 0)
        //                {
        //                    Mat tempMat = adjustBrightnessWinStyle(adjustedMat, nBright);
        //                    adjustedMat.Dispose();
        //                    adjustedMat = tempMat;
        //                }

        //                // 2. Radial Correction 적용 (곡선 기반 또는 강도 기반)
        //                if (bRadialCor == true)
        //                {
        //                    Mat correctedMat;

        //                    // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                    if (radialCorrectionCurve != null && radialCorrectionCurve.Length > 0)
        //                    {
        //                        correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                            adjustedMat, radialCorrectionCurve);
        //                        strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                            $"_Merged_{nAcqIndex}_Bright_{nBright}_RadialCurve" + ".png";
        //                    }
        //                    else
        //                    {
        //                        correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                            adjustedMat, fRadialCorrectionIntensity);
        //                        strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                            $"_Merged_{nAcqIndex}_Bright_{nBright}_Radial_{fRadialCorrectionIntensity}" + ".png";
        //                    }

        //                    adjustedMat.Dispose();
        //                    adjustedMat = correctedMat;
        //                }
        //                else
        //                {
        //                    strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                        $"_Merged_{nAcqIndex}_Bright_{nBright}" + ".png";
        //                }

        //                DrawColorMask(ref adjustedMat, _maskColorRadius);
        //                adjustedMat.SaveImage(strFileName);
        //                adjustedMat.Dispose();
        //            }
        //        }
        //    });
        //}

        //#endregion Method - Review Slice

        //#region Method - Review ROI

        //public void SetReviewROIMode()
        //{
        //    showDisplayMenu_(PreviewDisplayMode.REVIEW_ROI);

        //    if (_captureImageAndParamList.Count > 0)
        //    {
        //        int nAcqFrameCount = AcqFrameCount;
        //        int nSubFrameCount = SubFrameCount;
        //        int nFrameCount = _captureImageAndParamList.Count();

        //        FrameROIPosition[] frameROIs = FrameROIs;

        //        if (nAcqFrameCount != 1 && nSubFrameCount != 1 && nFrameCount != 2)
        //        {
        //            if (nAcqFrameCount * nSubFrameCount != nFrameCount)
        //            {
        //                MessageBox.Show("Total frame count is invalid.");
        //                return;
        //            }
        //        }

        //        int chunkSize = nFrameCount / nAcqFrameCount; // 한 acq frame 당 요소 개수
        //        List<List<Tuple<Mat, ImageAdjustParam>>> tempAcqFrameList = SplitListBySize(_captureImageAndParamList, chunkSize);

        //        // 3) 기본 이미지 크기 (모두 동일하다고 가정)
        //        int height = _captureImageAndParamList[0].Item1.Rows;
        //        int width = _captureImageAndParamList[0].Item1.Cols;

        //        for (int i = 0; i < tempAcqFrameList.Count; ++i)
        //        {
        //            // 5) 각 이미지에서 해당 분할 영역만큼 잘라내기
        //            List<Tuple<Mat, ImageAdjustParam>> tempSubFrameList = new List<Tuple<Mat, ImageAdjustParam>>();
        //            Mat[] Sliced = new Mat[nSubFrameCount];

        //            for (int j = 0; j < nSubFrameCount; ++j)
        //            {
        //                if (j > frameROIs.Count() - 1) // FrameROIs의 셋팅을 업데이트 하지 않아 현재 SubFrameCount보다 작을 경우는 얻은 프레임을 그대로 적용
        //                {
        //                    Sliced[j] = tempAcqFrameList[i][j].Item1.Clone();
        //                    tempSubFrameList.Add(new Tuple<Mat, ImageAdjustParam>(Sliced[j], tempAcqFrameList[i][j].Item2));
        //                }
        //                else
        //                {
        //                    int startY = frameROIs[j].StartY;
        //                    if (startY < 0)
        //                    {
        //                        startY = 0;
        //                    }
        //                    else if (startY > height)
        //                    {
        //                        startY = height;
        //                    }
        //                    int endY = frameROIs[j].EndY;
        //                    if (endY < 0)
        //                    {
        //                        endY = 0;
        //                    }
        //                    else if (endY > height)
        //                    {
        //                        endY = height;
        //                    }

        //                    OpenCvSharp.Rect roi = new OpenCvSharp.Rect(X: 0, Y: startY, Width: width, Height: endY - startY);
        //                    Sliced[j] = new Mat(tempAcqFrameList[i][j].Item1, roi).Clone();  // Clone()으로 메모리 분리
        //                    tempSubFrameList.Add(new Tuple<Mat, ImageAdjustParam>(Sliced[j], tempAcqFrameList[i][j].Item2));
        //                }

        //            }

        //            //Merge Sliced Image
        //            // 1. base 이미지 준비 (3채널 컬러 이미지 예시)
        //            Mat baseImg = new Mat(height, width, MatType.CV_8UC3, Scalar.All(0));

        //            // 2. 각 ROI 영역에 Sliced 이미지를 복사
        //            for (int index = 0; index < nSubFrameCount; ++index)
        //            {
        //                if (index > frameROIs.Count() - 1) // FrameROIs의 셋팅을 업데이트 하지 않아 현재 SubFrameCount보다 작을 경우는 얻은 프레임을 그대로 적용
        //                {
        //                    Sliced[index].CopyTo(baseImg);
        //                    continue;
        //                }

        //                int startY = frameROIs[index].StartY;
        //                if (startY < 0)
        //                {
        //                    startY = 0;
        //                }
        //                else if (startY > height)
        //                {
        //                    startY = height;
        //                }
        //                int endY = frameROIs[index].EndY;
        //                if (endY < 0)
        //                {
        //                    endY = 0;
        //                }
        //                else if (endY > height)
        //                {
        //                    endY = height;
        //                }
        //                int roiHeight = endY - startY;

        //                if (roiHeight <= 0) continue; // 잘못된 영역 체크

        //                // baseImg에서 ROI 영역 추출 (참조)
        //                OpenCvSharp.Rect roi = new OpenCvSharp.Rect(0, startY, width, roiHeight);
        //                using (Mat baseROI = new Mat(baseImg, roi))
        //                {
        //                    // Sliced[i]는 roiHeight x width 크기여야 함!
        //                    Sliced[index].CopyTo(baseROI);
        //                }
        //            }

        //            Tuple<Mat, ImageAdjustParam> merged = new Tuple<Mat, ImageAdjustParam>(baseImg, new ImageAdjustParam(ImageAdjustBrightness, 0));
        //            tempSubFrameList.Insert(0, merged);

        //            _captureSliceAcqImageList.Add(tempSubFrameList);
        //        }

        //        mySliderReviewROIImageIndex.Value = 1;
        //        mySliderReviewROISubImageIndex.Value = 1;

        //        //imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_captureSliceAcqImageList[0][0].Item1);

        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureSliceAcqImageList[0][0].Item1.Clone();
        //            ImageAdjustParam adjustParam = _captureSliceAcqImageList[0][0].Item2;

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            //_captureImageAndParamList[nImageIndex].Item2.Brightness = adjustParam.Brightness;

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    }

        //    resetDisplayMenu_(PreviewDisplayMode.REVIEW_ROI, _captureSliceAcqImageList.Count());
        //}

        //private void UpdateReviewROIStatusItems(int nWidth, int nHeight)
        //{
        //    string s = string.Format("{0} x {1}", nWidth, nHeight);
        //    lblImageStatusReviewROI.Content = s;
        //}

        //private async Task saveROICaptureImage_()
        //{
        //    if (_captureSliceAcqImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    await Task.Run(() =>
        //    {
        //        string strFileName;
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_ROI";

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        int nAcqFrameCount = _captureSliceAcqImageList.Count;

        //        if (nAcqFrameCount == 1 && _captureSliceAcqImageList[0].Count == 1) // Image가 1 장일 때
        //        {
        //            int nBright = _captureSliceAcqImageList[0][0].Item2.Brightness;
        //            bool bRadialCor = _captureSliceAcqImageList[0][0].Item2.IsRadialCorrection;
        //            float fRadialCorrectionIntensity = _captureSliceAcqImageList[0][0].Item2.RadialCorIntensity;
        //            double[] radialCorrectionCurve = _captureSliceAcqImageList[0][0].Item2.RadialCorrectionCurve; // 곡선 데이터 추가
        //            bool bRemoveReflectedLight = _captureSliceAcqImageList[0][0].Item2.IsRemoveReflectedLight;

        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureSliceAcqImageList[0][0].Item1.Clone();

        //            if (bRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 1. Brightness 조정
        //            if (nBright != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, nBright);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 2. Radial Correction 적용 (곡선 기반 또는 강도 기반)
        //            if (bRadialCor == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (radialCorrectionCurve != null && radialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, radialCorrectionCurve);
        //                    strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                        $"_Merged_Bright_{nBright}_RadialCurve" + ".png";
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, fRadialCorrectionIntensity);
        //                    strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                        $"_Merged_Bright_{nBright}_Radial_{fRadialCorrectionIntensity}" + ".png";
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;
        //            }
        //            else
        //            {
        //                strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                    $"_Merged_Bright_{nBright}" + ".png";
        //            }

        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            /////////////////////////////
        //            adjustedMat.SaveImage(strFileName);
        //            adjustedMat.Dispose();
        //        }
        //        else
        //        {
        //            for (int nAcqIndex = 0; nAcqIndex < nAcqFrameCount; nAcqIndex++)
        //            {
        //                var SubFrameList = _captureSliceAcqImageList[nAcqIndex];
        //                int nSubFrameCount = SubFrameList.Count;

        //                //// Sliced ROI Image
        //                string strSlisedFileName;
        //                for (int nSubFrameIndex = 1; nSubFrameIndex < nSubFrameCount; nSubFrameIndex++)
        //                {
        //                    strSlisedFileName = strFolderPath + "//" + $"Sliced_{nAcqIndex}_{nSubFrameIndex}" + ".png";
        //                    _captureSliceAcqImageList[nAcqIndex][nSubFrameIndex].Item1.SaveImage(strSlisedFileName);
        //                }

        //                //// Merged Image 
        //                int nBright = _captureSliceAcqImageList[nAcqIndex][0].Item2.Brightness;
        //                bool bRadialCor = _captureSliceAcqImageList[nAcqIndex][0].Item2.IsRadialCorrection;
        //                float fRadialCorrectionIntensity = _captureSliceAcqImageList[nAcqIndex][0].Item2.RadialCorIntensity;
        //                double[] radialCorrectionCurve = _captureSliceAcqImageList[nAcqIndex][0].Item2.RadialCorrectionCurve; // 곡선 데이터 추가
        //                bool bRemoveReflectedLight = _captureSliceAcqImageList[nAcqIndex][0].Item2.IsRemoveReflectedLight;

        //                Mat adjustedMat = _captureSliceAcqImageList[nAcqIndex][0].Item1.Clone();

        //                if (bRemoveReflectedLight == true)
        //                {
        //                    Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                    adjustedMat.Dispose();
        //                    adjustedMat = tempMat;
        //                }

        //                // 1. Brightness 조정
        //                if (nBright != 0)
        //                {
        //                    Mat tempMat = adjustBrightnessWinStyle(adjustedMat, nBright);
        //                    adjustedMat.Dispose();
        //                    adjustedMat = tempMat;
        //                }

        //                // 2. Radial Correction 적용 (곡선 기반 또는 강도 기반)
        //                if (bRadialCor == true)
        //                {
        //                    Mat correctedMat;

        //                    // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                    if (radialCorrectionCurve != null && radialCorrectionCurve.Length > 0)
        //                    {
        //                        correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                            adjustedMat, radialCorrectionCurve);
        //                        strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                            $"_Merged_{nAcqIndex}_Bright_{nBright}_RadialCurve" + ".png";
        //                    }
        //                    else
        //                    {
        //                        correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                            adjustedMat, fRadialCorrectionIntensity);
        //                        strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                            $"_Merged_{nAcqIndex}_Bright_{nBright}_Radial_{fRadialCorrectionIntensity}" + ".png";
        //                    }

        //                    adjustedMat.Dispose();
        //                    adjustedMat = correctedMat;
        //                }
        //                else
        //                {
        //                    strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                        $"_Merged_{nAcqIndex}_Bright_{nBright}" + ".png";
        //                }

        //                DrawColorMask(ref adjustedMat, _maskColorRadius);
        //                adjustedMat.SaveImage(strFileName);
        //                adjustedMat.Dispose();
        //            }
        //        }
        //    });
        //}

        //#endregion Method - Review ROI

        //#region Method - Live Seq ROI

        //public void DisplaySeqROIMode()
        //{
        //    if (_captureImageList.Count <= 0)
        //        return;

        //    FrameSeqROIParam[] frameROIs = FrameSeqROIs;

        //    if (frameROIs == null || frameROIs.Length == 0)
        //        return;

        //    // 최대 크기 계산 (모든 이미지의 위치와 크기를 고려하여 전체 결과 이미지 크기 결정)
        //    int maxWidth = SeqTargetImageMaxWidth;
        //    int maxHeight = SeqTargetImageMaxHeight;

        //    //for (int i = 0; i < Math.Min(_captureImageList.Count, frameROIs.Length); i++)
        //    //{
        //    //    var roi = frameROIs[i];
        //    //    int rightEdge = roi.OffsetX + roi.Width;
        //    //    int bottomEdge = roi.OffsetY + roi.Height;

        //    //    if (rightEdge > maxWidth)
        //    //        maxWidth = rightEdge;
        //    //    if (bottomEdge > maxHeight)
        //    //        maxHeight = bottomEdge;
        //    //}

        //    // 결과 Mat 생성 (3채널 컬러 이미지)
        //    Mat resultMat = new Mat(maxHeight, maxWidth, MatType.CV_8UC3, Scalar.All(0));

        //    // 각 이미지를 해당 위치에 배치
        //    for (int i = 0; i < Math.Min(_captureImageList.Count, frameROIs.Length); i++)
        //    {
        //        var sourceMat = _captureImageList[i];
        //        var roi = frameROIs[i];

        //        if (sourceMat.Empty())
        //            continue;

        //        // ROI 영역 유효성 검사
        //        if (roi.OffsetX < 0 || roi.OffsetY < 0 ||
        //            roi.OffsetX + roi.Width > maxWidth ||
        //            roi.OffsetY + roi.Height > maxHeight)
        //            continue;

        //        // 결과 이미지에서 해당 위치의 ROI 영역 추출
        //        OpenCvSharp.Rect targetRect = new OpenCvSharp.Rect(roi.OffsetX, roi.OffsetY, roi.Width, roi.Height);

        //        try
        //        {
        //            using (Mat targetROI = new Mat(resultMat, targetRect))
        //            {
        //                // 소스 이미지가 단일 채널인 경우 3채널로 변환
        //                if (sourceMat.Channels() == 1)
        //                {
        //                    Mat colorMat = new Mat();
        //                    Cv2.CvtColor(sourceMat, colorMat, ColorConversionCodes.GRAY2BGR);
        //                    colorMat.CopyTo(targetROI);
        //                    colorMat.Dispose();
        //                }
        //                else
        //                {
        //                    sourceMat.CopyTo(targetROI);
        //                }
        //            }
        //        }
        //        catch (Exception ex)
        //        {
        //            // ROI 영역이 유효하지 않은 경우 건너뛰기
        //            System.Diagnostics.Debug.WriteLine($"Error copying image {i}: {ex.Message}");
        //        }
        //    }

        //    // 결과를 이미지 뷰포트에 표시
        //    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(resultMat);

        //    // 메모리 해제
        //    resultMat.Dispose();

        //    _captureImageList.Clear();
        //}

        //#endregion Method - Live Seq ROI

        //public void DisplayOffsetROIMode()
        //{
        //    if (_captureImageList.Count <= 0)
        //        return;

        //    FrameOffsetROIParam[] frameROIs = FrameOffsetROIs;

        //    if (frameROIs == null || frameROIs.Length == 0)
        //        return;

        //    // 최대 크기 계산 (모든 이미지의 위치와 크기를 고려하여 전체 결과 이미지 크기 결정)
        //    int maxWidth = OffsetRoiTargetImageMaxWidth;
        //    int maxHeight = OffsetRoiTargetImageMaxHeight;

        //    //for (int i = 0; i < Math.Min(_captureImageList.Count, frameROIs.Length); i++)
        //    //{
        //    //    var roi = frameROIs[i];
        //    //    int rightEdge = roi.OffsetX + roi.Width;
        //    //    int bottomEdge = roi.OffsetY + roi.Height;

        //    //    if (rightEdge > maxWidth)
        //    //        maxWidth = rightEdge;
        //    //    if (bottomEdge > maxHeight)
        //    //        maxHeight = bottomEdge;
        //    //}

        //    // 결과 Mat 생성 (3채널 컬러 이미지)
        //    Mat resultMat = new Mat(maxHeight, maxWidth, MatType.CV_8UC3, Scalar.All(0));

        //    // 각 이미지를 해당 위치에 배치
        //    for (int i = 0; i < Math.Min(_captureImageList.Count, frameROIs.Length); i++)
        //    {
        //        var sourceMat = _captureImageList[i];
        //        var roi = frameROIs[i];

        //        if (sourceMat.Empty())
        //            continue;

        //        // ROI 영역 유효성 검사
        //        if (roi.OffsetX < 0 || roi.OffsetY < 0 ||
        //            roi.OffsetX + OffsetRoiImageWidth > maxWidth ||
        //            roi.OffsetY + OffsetRoiImageHeight > maxHeight)
        //            continue;

        //        // 결과 이미지에서 해당 위치의 ROI 영역 추출
        //        OpenCvSharp.Rect targetRect = new OpenCvSharp.Rect(roi.OffsetX, roi.OffsetY, OffsetRoiImageWidth, OffsetRoiImageHeight);

        //        try
        //        {
        //            using (Mat targetROI = new Mat(resultMat, targetRect))
        //            {
        //                // 소스 이미지가 단일 채널인 경우 3채널로 변환
        //                if (sourceMat.Channels() == 1)
        //                {
        //                    Mat colorMat = new Mat();
        //                    Cv2.CvtColor(sourceMat, colorMat, ColorConversionCodes.GRAY2BGR);
        //                    colorMat.CopyTo(targetROI);
        //                    colorMat.Dispose();
        //                }
        //                else
        //                {
        //                    sourceMat.CopyTo(targetROI);
        //                }
        //            }
        //        }
        //        catch (Exception ex)
        //        {
        //            // ROI 영역이 유효하지 않은 경우 건너뛰기
        //            System.Diagnostics.Debug.WriteLine($"Error copying image {i}: {ex.Message}");
        //        }
        //    }

        //    // 결과를 이미지 뷰포트에 표시
        //    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(resultMat);

        //    // 메모리 해제
        //    resultMat.Dispose();

        //    _captureImageList.Clear();
        //}

        //public void SetOffsetROIMode()
        //{
        //    showDisplayMenu_(PreviewDisplayMode.REVIEW_OFFSET_ROI);

        //    if (_captureImageList.Count <= 0)
        //        return;

        //    FrameOffsetROIParam[] frameROIs = FrameOffsetROIs;

        //    if (frameROIs == null || frameROIs.Length == 0)
        //        return;

        //    // 최대 크기 계산 (모든 이미지의 위치와 크기를 고려하여 전체 결과 이미지 크기 결정)
        //    int maxWidth = OffsetRoiTargetImageMaxWidth;
        //    int maxHeight = OffsetRoiTargetImageMaxHeight;

        //    //for (int i = 0; i < Math.Min(_captureImageList.Count, frameROIs.Length); i++)
        //    //{
        //    //    var roi = frameROIs[i];
        //    //    int rightEdge = roi.OffsetX + roi.Width;
        //    //    int bottomEdge = roi.OffsetY + roi.Height;

        //    //    if (rightEdge > maxWidth)
        //    //        maxWidth = rightEdge;
        //    //    if (bottomEdge > maxHeight)
        //    //        maxHeight = bottomEdge;
        //    //}

        //    // 결과 Mat 생성 (3채널 컬러 이미지)
        //    Mat resultMat = new Mat(maxHeight, maxWidth, MatType.CV_8UC3, Scalar.All(0));

        //    // 각 이미지를 해당 위치에 배치
        //    for (int i = 0; i < Math.Min(_captureImageList.Count, frameROIs.Length); i++)
        //    {
        //        var sourceMat = _captureImageList[i];
        //        var roi = frameROIs[i];

        //        if (sourceMat.Empty())
        //            continue;

        //        // ROI 영역 유효성 검사
        //        if (roi.OffsetX < 0 || roi.OffsetY < 0 ||
        //            roi.OffsetX + OffsetRoiImageWidth > maxWidth ||
        //            roi.OffsetY + OffsetRoiImageHeight > maxHeight)
        //            continue;

        //        // 결과 이미지에서 해당 위치의 ROI 영역 추출
        //        OpenCvSharp.Rect targetRect = new OpenCvSharp.Rect(roi.OffsetX, roi.OffsetY, OffsetRoiImageWidth, OffsetRoiImageHeight);

        //        try
        //        {
        //            using (Mat targetROI = new Mat(resultMat, targetRect))
        //            {
        //                // 소스 이미지가 단일 채널인 경우 3채널로 변환
        //                if (sourceMat.Channels() == 1)
        //                {
        //                    Mat colorMat = new Mat();
        //                    Cv2.CvtColor(sourceMat, colorMat, ColorConversionCodes.GRAY2BGR);
        //                    colorMat.CopyTo(targetROI);
        //                    colorMat.Dispose();
        //                }
        //                else
        //                {
        //                    sourceMat.CopyTo(targetROI);
        //                }
        //            }
        //        }
        //        catch (Exception ex)
        //        {
        //            // ROI 영역이 유효하지 않은 경우 건너뛰기
        //            System.Diagnostics.Debug.WriteLine($"Error copying image {i}: {ex.Message}");
        //        }
        //    }

        //    _offsetROIMergedMat = resultMat.Clone();

        //    // 결과를 이미지 뷰포트에 표시
        //    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(resultMat);

        //    // 메모리 해제
        //    resultMat.Dispose();

        //    //_captureImageList.Clear();
        //}

        //private async Task saveOffsetROIImage_()
        //{
        //    if (_captureImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    await Task.Run(() =>
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_OffsetROI";

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        int nAcqFrameCount = _captureImageList.Count;

        //        string strSlisedFileName;

        //        for (int nAcqIndex = 0; nAcqIndex < nAcqFrameCount; nAcqIndex++)
        //        {
        //            var ROIFrame = _captureImageList[nAcqIndex];
        //            strSlisedFileName = strFolderPath + "//" + $"OffsetROI_{nAcqIndex}" + ".png";
        //            _captureImageList[nAcqIndex].SaveImage(strSlisedFileName);
        //        }

        //        if (_offsetROIMergedMat != null)
        //        {
        //            strSlisedFileName = strFolderPath + "//" + $"Merged" + ".png"; // 각 acq의 첫번째 이미지는 Merged Image
        //            _offsetROIMergedMat.SaveImage(strSlisedFileName);
        //        }
        //    });
        //}

        //#region Method - Live, Review Roll SWTrig Overlap
        //public void DisplayRollSWTrigOverlapMode()
        //{
        //    if (_captureImageList.Count <= 0)
        //        return;

        //    // 첫 번째 이미지의 크기와 타입을 기준으로 평균 이미지 생성
        //    Mat firstImage = _captureImageList[0];
        //    int width = firstImage.Width;
        //    int height = firstImage.Height;

        //    // 첫 번째 이미지의 타입 확인
        //    bool is16Bit = firstImage.Depth() == MatType.CV_16U;
        //    int channels = firstImage.Channels();

        //    // 평균 계산을 위한 누적 이미지 (32비트 float로 생성)
        //    MatType accumulatedType = channels == 3 ? MatType.CV_32FC3 : MatType.CV_32FC1;
        //    Mat accumulatedMat = new Mat(height, width, accumulatedType, Scalar.All(0));

        //    try
        //    {
        //        // 모든 이미지를 32비트 float로 변환하고 누적
        //        for (int i = 0; i < _captureImageList.Count; i++)
        //        {
        //            Mat currentMat = _captureImageList[i];
        //            Mat floatMat = new Mat();

        //            // 16UC3/16UC1 또는 8UC3/8UC1 → 32FC3/32FC1로 변환
        //            currentMat.ConvertTo(floatMat, accumulatedType);

        //            // 누적 합계에 추가
        //            Cv2.Add(accumulatedMat, floatMat, accumulatedMat);

        //            floatMat.Dispose();
        //        }

        //        // 평균 계산 - 직접 나누기
        //        Mat averagedMat = new Mat();
        //        accumulatedMat.ConvertTo(averagedMat, accumulatedType, 1.0 / _captureImageList.Count);

        //        // 표시용으로 적절한 타입으로 변환
        //        Mat resultMat = new Mat();
        //        if (is16Bit)
        //        {
        //            // 16비트 이미지는 8비트로 변환하여 표시 (0-65535 → 0-255)
        //            MatType displayType = channels == 3 ? MatType.CV_8UC3 : MatType.CV_8UC1;
        //            averagedMat.ConvertTo(resultMat, displayType, 1.0 / 256.0);
        //        }
        //        else
        //        {
        //            // 8비트 이미지는 그대로 변환
        //            MatType displayType = channels == 3 ? MatType.CV_8UC3 : MatType.CV_8UC1;
        //            averagedMat.ConvertTo(resultMat, displayType);
        //        }

        //        // 단일 채널인 경우 3채널로 변환 (표시용)
        //        Mat displayMat = new Mat();
        //        if (channels == 1)
        //        {
        //            Cv2.CvtColor(resultMat, displayMat, ColorConversionCodes.GRAY2BGR);
        //        }
        //        else
        //        {
        //            displayMat = resultMat.Clone();
        //        }

        //        // 결과를 이미지 뷰포트에 표시
        //        DrawColorMask(ref displayMat, _maskColorRadius);
        //        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(displayMat);

        //        // 메모리 해제
        //        displayMat.Dispose();
        //        resultMat.Dispose();
        //        averagedMat.Dispose();
        //    }
        //    catch (Exception ex)
        //    {
        //        // 오류 발생 시 첫 번째 이미지만 표시
        //        System.Diagnostics.Debug.WriteLine($"Error in averaging images: {ex.Message}");

        //        // 16비트 이미지인 경우 8비트로 변환하여 표시
        //        Mat displayMat = new Mat();
        //        if (is16Bit)
        //        {
        //            if (channels == 3)
        //            {
        //                firstImage.ConvertTo(displayMat, MatType.CV_8UC3, 1.0 / 256.0);
        //            }
        //            else
        //            {
        //                Mat tempMat = new Mat();
        //                firstImage.ConvertTo(tempMat, MatType.CV_8UC1, 1.0 / 256.0);
        //                Cv2.CvtColor(tempMat, displayMat, ColorConversionCodes.GRAY2BGR);
        //                tempMat.Dispose();
        //            }
        //        }
        //        else
        //        {
        //            if (channels == 1)
        //            {
        //                Cv2.CvtColor(firstImage, displayMat, ColorConversionCodes.GRAY2BGR);
        //            }
        //            else
        //            {
        //                displayMat = firstImage.Clone();
        //            }
        //        }

        //        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(displayMat);
        //        displayMat.Dispose();
        //    }
        //    finally
        //    {
        //        // 메모리 해제
        //        accumulatedMat.Dispose();
        //    }

        //    _captureImageList.Clear();
        //}

        //public void SetReviewRollSWTrigOverlapMode(int nTotalFrameCount)
        //{
        //    showDisplayMenu_(PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_OVERLAP);

        //    // 첫 번째 이미지의 크기와 타입을 기준으로 평균 이미지 생성
        //    Mat firstImage = _captureImageList[0];
        //    int width = firstImage.Width;
        //    int height = firstImage.Height;

        //    // 첫 번째 이미지의 타입 확인
        //    bool is16Bit = firstImage.Depth() == MatType.CV_16U;
        //    int channels = firstImage.Channels();

        //    // 평균 계산을 위한 누적 이미지 (32비트 float로 생성)
        //    MatType accumulatedType = channels == 3 ? MatType.CV_32FC3 : MatType.CV_32FC1;
        //    Mat accumulatedMat = new Mat(height, width, accumulatedType, Scalar.All(0));

        //    try
        //    {
        //        // 모든 이미지를 32비트 float로 변환하고 누적
        //        for (int i = 0; i < _captureImageList.Count; i++)
        //        {
        //            Mat currentMat = _captureImageList[i];
        //            Mat floatMat = new Mat();

        //            // 16UC3 또는 8UC3 → 32FC3로 변환
        //            // 16UC1 또는 8UC1 → 32FC1로 변환
        //            currentMat.ConvertTo(floatMat, accumulatedType);

        //            // 누적 합계에 추가
        //            Cv2.Add(accumulatedMat, floatMat, accumulatedMat);

        //            floatMat.Dispose();
        //        }

        //        // 평균 계산 - 직접 나누기
        //        Mat averagedMat = new Mat();
        //        accumulatedMat.ConvertTo(averagedMat, accumulatedType, 1.0 / _captureImageList.Count);

        //        // 원본 타입으로 다시 변환하여 표시
        //        Mat resultMat = new Mat();
        //        if (is16Bit)
        //        {
        //            // 16비트로 변환
        //            MatType resultType = channels == 3 ? MatType.CV_16UC3 : MatType.CV_16UC1;
        //            averagedMat.ConvertTo(resultMat, resultType);
        //        }
        //        else
        //        {
        //            // 8비트로 변환
        //            MatType resultType = channels == 3 ? MatType.CV_8UC3 : MatType.CV_8UC1;
        //            averagedMat.ConvertTo(resultMat, resultType);
        //        }

        //        // 단일 채널인 경우 3채널로 변환 (표시용)
        //        Mat displayMat = new Mat();
        //        if (channels == 1)
        //        {
        //            ColorConversionCodes conversionCode = is16Bit ?
        //                ColorConversionCodes.GRAY2BGR : ColorConversionCodes.GRAY2BGR;
        //            Cv2.CvtColor(resultMat, displayMat, conversionCode);
        //        }
        //        else
        //        {
        //            displayMat = resultMat.Clone();
        //        }

        //        // 메모리 해제
        //        _captureImageList.Insert(0, displayMat);
        //        resultMat.Dispose();
        //        averagedMat.Dispose();
        //    }
        //    catch (Exception ex)
        //    {
        //        // 오류 발생 시 첫 번째 이미지만 표시
        //        System.Diagnostics.Debug.WriteLine($"Error in averaging images: {ex.Message}");
        //    }
        //    finally
        //    {
        //        // 메모리 해제
        //        accumulatedMat.Dispose();
        //    }

        //    resetDisplayMenu_(PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_OVERLAP, nTotalFrameCount);

        //    if (_captureImageList.Count > 0)
        //    {
        //        mySliderReviewImageIndex.Value = 1;
        //        Mat matDisplay = _captureImageList[0].Clone();
        //        DrawColorMask(ref matDisplay, _maskColorRadius);
        //        imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(matDisplay);
        //        setSliderBrightnessRange_(ref mySliderReviewRollSWTrigOverlapBrightness, matDisplay.Depth());
        //        mySliderReviewRollSWTrigOverlapBrightness.Value = ReviewRollSWTrigOverlapBrightness;
        //        matDisplay.Dispose();
        //    }
        //}

        //private void setSliderBrightnessRange_(ref Slider sliderBright, int ImagePixelDepth)
        //{
        //    if (ImagePixelDepth == 2) // 16bit
        //    {
        //        sliderBright.Minimum = -32767;
        //        sliderBright.Maximum = 32767;
        //    } 
        //    else // 8bit
        //    {
        //        sliderBright.Minimum = -127;
        //        sliderBright.Maximum = 127;
        //    }
        //}

        //private async Task saveAllReviewRollSWTrigOverlapImage_()
        //{
        //    if (_captureImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    await Task.Run(() =>
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture" + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + "_Roll_SWTrig_Overlap";

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        int nFrameCount = _captureImageList.Count;

        //        string strFileName;
        //        Mat maskMat;

        //        for (int nIndex = 1; nIndex < nFrameCount; nIndex++)
        //        {
        //            strFileName = strFolderPath + "//" + $"{nIndex}" + ".png";
        //            maskMat = _captureImageList[nIndex].Clone();
        //            DrawColorMask(ref maskMat, _maskColorRadius);
        //            maskMat.SaveImage(strFileName);
        //        }

        //        maskMat = _captureImageList[0].Clone();
        //        DrawColorMask(ref maskMat, _maskColorRadius);
        //        strFileName = strFolderPath + "//" + $"Overlaped" + ".png"; // 각 acq의 첫번째 이미지는 Merged Image
        //        maskMat.SaveImage(strFileName);

        //        maskMat.Dispose();
        //    });
        //}

        //private async Task saveReviewRollSWTrigOverlapBrightImage_()
        //{
        //    if (_captureImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nIndex = (int)mySliderReviewRollSWTrigOverlapImageIndex.Value;
        //    int nBright = (int)mySliderReviewRollSWTrigOverlapBrightness.Value;

        //    await Task.Run(() =>
        //    {
        //        if (nIndex > 0)
        //        {
        //            Mat matBright = adjustBrightnessWinStyle(_captureImageList[nIndex - 1], nBright);
        //            DrawColorMask(ref matBright, _maskColorRadius);

        //            string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";

        //            if (Directory.Exists(strFolderPath) == false)
        //            {
        //                Directory.CreateDirectory(strFolderPath);
        //            }

        //            if (nIndex == 1)
        //            {
        //                string strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + $"_Overlaped_Bright_{nBright}" + ".png";
        //                matBright.SaveImage(strFileName);
        //            }
        //            else
        //            {
        //                string strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + $"_Index_{nIndex}_Bright_{nBright}" + ".png";
        //                matBright.SaveImage(strFileName);
        //            }

        //            matBright.Dispose();
        //        }
        //    });
        //}

        //public static Mat AdjustBrightness(Mat source, int brightness)
        //{
        //    if (brightness == 0)
        //    {
        //        return source.Clone();
        //    }

        //    if (source.Empty())
        //    {
        //        throw new ArgumentException("Source image is empty.");
        //    }

        //    Mat result = new Mat();

        //    // 16비트 이미지인지 확인
        //    bool is16Bit = source.Depth() == MatType.CV_16U;

        //    if (is16Bit)
        //    {
        //        // 16비트 이미지 처리
        //        Mat normalized = new Mat();
        //        Mat processed = new Mat();

        //        try
        //        {
        //            // 16비트를 8비트로 정규화 (0-65535 -> 0-255)
        //            source.ConvertTo(normalized, MatType.CV_8UC3, 1.0 / 256.0);

        //            // HSV 변환 및 밝기 조정
        //            Mat hsv = new Mat();
        //            Cv2.CvtColor(normalized, hsv, ColorConversionCodes.RGB2HSV);

        //            Mat[] channels = Cv2.Split(hsv);
        //            Mat vChannel = channels[2];

        //            double alpha = 1.0 + brightness / 100.0;
        //            vChannel.ConvertTo(vChannel, -1, alpha);

        //            Cv2.Merge(channels, hsv);
        //            Cv2.CvtColor(hsv, processed, ColorConversionCodes.HSV2RGB);

        //            // 다시 16비트로 변환 (0-255 -> 0-65535)
        //            processed.ConvertTo(result, MatType.CV_16UC3, 256.0);

        //            // 메모리 해제
        //            hsv.Dispose();
        //            foreach (var channel in channels) { channel.Dispose(); }
        //            normalized.Dispose();
        //            processed.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            // 오류 발생 시 원본 반환
        //            System.Diagnostics.Debug.WriteLine($"16비트 이미지 밝기 조정 중 오류: {ex.Message}");
        //            normalized?.Dispose();
        //            processed?.Dispose();
        //            return source.Clone();
        //        }
        //    }
        //    else
        //    {
        //        // 기존 8비트 이미지 처리 로직
        //        Mat hsv = source.EmptyClone();
        //        Cv2.CvtColor(source, hsv, ColorConversionCodes.RGB2HSV);

        //        Mat[] channels = Cv2.Split(hsv);
        //        Mat vChannel = channels[2];

        //        double alpha = 1.0 + brightness / 100.0;
        //        vChannel.ConvertTo(vChannel, -1, alpha);

        //        Cv2.Merge(channels, hsv);
        //        Cv2.CvtColor(hsv, result, ColorConversionCodes.HSV2RGB);

        //        // 메모리 해제
        //        hsv.Dispose();
        //        foreach (var channel in channels) { channel.Dispose(); }
        //    }

        //    return result;
        //}

        ////private Mat adjustBrightnessWinStyle(Mat source, int brightness)
        ////{
        ////    if (brightness == 0)
        ////    {
        ////        return source.Clone();
        ////    }

        ////    if (source.Empty())
        ////    {
        ////        throw new ArgumentException("Source image is empty.");
        ////    }

        ////    // 16비트 이미지인지 확인
        ////    bool is16Bit = source.Depth() == MatType.CV_16U;
        ////    double maxRange = is16Bit ? 65535.0 : 255.0;

        ////    // 현재 이미지의 실제 min/max 찾기
        ////    source.MinMaxLoc(out double minVal, out double maxVal);

        ////    // ImageJ 방식: display range 조정
        ////    double newMin = minVal - brightness;
        ////    double newMax = maxVal - brightness;

        ////    // contrast > 1.0: 대비 증가, contrast < 1.0: 대비 감소
        ////    newMin += brightness;
        ////    newMax -= brightness;

        ////    // 범위가 0이 되는 것을 방지
        ////    if (Math.Abs(newMax - newMin) < 1e-6)
        ////    {
        ////        newMax = newMin + 1.0;
        ////    }

        ////    // 16비트와 8비트에 따른 적절한 스케일링
        ////    Mat result = new Mat();
        ////    double scale, offset;

        ////    if (is16Bit)
        ////    {
        ////        // 16비트 이미지: 원본 범위 유지
        ////        scale = maxRange / (newMax - newMin);
        ////        offset = -newMin * scale;

        ////        // 16비트 범위로 클리핑
        ////        source.ConvertTo(result, -1, scale, offset);

        ////        // 16비트 범위 클리핑 (0-65535)
        ////        Cv2.Threshold(result, result, 65535, 65535, ThresholdTypes.Trunc);
        ////        Cv2.Threshold(result, result, 0, 0, ThresholdTypes.Tozero);
        ////    }
        ////    else
        ////    {
        ////        // 8비트 이미지: 기존 로직
        ////        scale = 255.0 / (newMax - newMin);
        ////        offset = -newMin * scale;
        ////        source.ConvertTo(result, -1, scale, offset);
        ////    }

        ////    // 밝기 증가 시 denoising 적용
        ////    if (brightness > 10) // 밝기가 10 이상 증가할 때만 denoising 적용
        ////    {
        ////        Mat denoisedResult = ApplyDenoising(result, brightness);
        ////        result.Dispose();
        ////        return denoisedResult;
        ////    }

        ////    return result;
        ////}

        //private Mat adjustBrightnessWinStyle(Mat source, int brightness)
        //{
        //    const int BrightnessMin = -127;
        //    const int BrightnessMax = 127;
        //    int boundedBrightness = Math.Clamp(brightness, BrightnessMin, BrightnessMax);
        //    if (boundedBrightness == 0)
        //        return source.Clone();

        //    if (source.Empty())
        //        throw new ArgumentException("Source image is empty.");

        //    bool is16Bit = source.Depth() == MatType.CV_16U;
        //    double maxRange = is16Bit ? 65535.0 : 255.0;

        //    source.MinMaxLoc(out double minVal, out double maxVal);

        //    double normalizedBrightness = boundedBrightness / 127.0;
        //    double center = (minVal + maxVal) / 2.0;
        //    double halfRange = (maxVal - minVal) / 2.0;

        //    double contrastFactor = 1.0 - normalizedBrightness * 1.0; // [0.5, 1.5]
        //    double newHalfRange = halfRange * contrastFactor;
        //    double newCenter = center - normalizedBrightness * halfRange;

        //    double newMin = newCenter - newHalfRange;
        //    double newMax = newCenter + newHalfRange;

        //    if (Math.Abs(newMax - newMin) < 1e-6)
        //        newMax = newMin + 1.0;

        //    Mat result = new Mat();
        //    double scale = maxRange / (newMax - newMin);
        //    double offset = -newMin * scale;

        //    source.ConvertTo(result, -1, scale, offset);

        //    if (is16Bit)
        //    {
        //        Cv2.Threshold(result, result, 65535, 65535, ThresholdTypes.Trunc);
        //        Cv2.Threshold(result, result, 0, 0, ThresholdTypes.Tozero);
        //    }

        //    if (boundedBrightness > 10)
        //    {
        //        Mat denoisedResult = ApplyDenoising(result, boundedBrightness);
        //        result.Dispose();
        //        return denoisedResult;
        //    }

        //    return result;
        //}

        //private Mat ApplyDenoising(Mat source, int brightness)
        //{
        //    if (source.Empty())
        //    {
        //        return source.Clone();
        //    }

        //    Mat result = new Mat();

        //    // 16비트 이미지인지 확인
        //    bool is16Bit = source.Depth() == MatType.CV_16U;

        //    try
        //    {
        //        if (is16Bit)
        //        {
        //            // 16비트 이미지 처리: 8비트로 변환 후 처리하고 다시 16비트로 복원
        //            Mat normalized = new Mat();
        //            Mat processed = new Mat();

        //            // 16비트를 8비트로 정규화 (0-65535 -> 0-255)
        //            source.ConvertTo(normalized, MatType.CV_8UC3, 1.0 / 256.0);

        //            // 8비트 이미지에 denoising 적용
        //            ApplyDenoisingTo8Bit(normalized, processed, brightness);

        //            // 다시 16비트로 변환 (0-255 -> 0-65535)
        //            processed.ConvertTo(result, MatType.CV_16UC3, 256.0);

        //            // 메모리 해제
        //            normalized.Dispose();
        //            processed.Dispose();
        //        }
        //        else
        //        {
        //            // 8비트 이미지 직접 처리
        //            ApplyDenoisingTo8Bit(source, result, brightness);
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        System.Diagnostics.Debug.WriteLine($"Denoising 적용 중 오류: {ex.Message}");
        //        result?.Dispose();
        //        return source.Clone(); // 오류 시 원본 반환
        //    }

        //    return result;
        //}

        //private void ApplyDenoisingTo8Bit(Mat source, Mat result, int brightness)
        //{
        //    // 밝기 값에 따른 denoising 방법 선택
        //    if (brightness <= 30)
        //    {
        //        // 약한 밝기 증가: 가벼운 가우시안 블러
        //        double sigma = 0.8 + (brightness / 100.0) * 0.4; // 0.8 ~ 1.2 범위
        //        Cv2.GaussianBlur(source, result, new OpenCvSharp.Size(3, 3), sigma);

        //        System.Diagnostics.Debug.WriteLine($"가우시안 블러 적용: sigma={sigma:F2}");
        //    }
        //    else
        //    {
        //        // 중간 밝기 증가: Bilateral Filter (8비트에서만 안전하게 동작)
        //        int d = 7;
        //        double sigmaColor = 40.0 + (brightness / 100.0) * 40.0; // 40 ~ 80 범위
        //        double sigmaSpace = 40.0 + (brightness / 100.0) * 40.0; // 40 ~ 80 범위

        //        try
        //        {
        //            Cv2.BilateralFilter(source, result, d, sigmaColor, sigmaSpace);
        //            System.Diagnostics.Debug.WriteLine($"Bilateral Filter 적용: d={d}, sigmaColor={sigmaColor:F1}, sigmaSpace={sigmaSpace:F1}");
        //        }
        //        catch (Exception ex)
        //        {
        //            // BilateralFilter 실패 시 가우시안 블러로 대체
        //            System.Diagnostics.Debug.WriteLine($"BilateralFilter 실패, 가우시안 블러로 대체: {ex.Message}");
        //            double sigma = 1.0 + (brightness / 100.0) * 0.5;
        //            Cv2.GaussianBlur(source, result, new OpenCvSharp.Size(5, 5), sigma);
        //        }
        //    }
        //}

        //#endregion

        //#region Method - Review Roll SWTrig Manual

        //public void SetReviewRollSWTrigManualMode(int nTotalFrameCount)
        //{
        //    showDisplayMenu_(PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_MANUAL);
        //    resetDisplayMenu_(PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_MANUAL, nTotalFrameCount);

        //    if (_captureShootImageList.Count > 0)
        //    {
        //        mySliderReviewImageIndex.Value = 1;
        //        //Mat matDisplay = _captureShootImageList[0].Item1.Clone();
        //        //DrawColorMask(ref matDisplay, _maskColorRadius);
        //        //imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(matDisplay);
        //        //matDisplay.Dispose();

        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureShootImageList[0].Item1.Clone();
        //            ImageAdjustParam adjustParam = _captureShootImageList[0].Item2;

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    }
        //}

        //private async Task saveReviewRollSWTrigManualBrightImage_()
        //{
        //    if (_captureShootImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nBright = _captureShootImageList[0].Item2.Brightness;
        //    bool bRadialCor = _captureShootImageList[0].Item2.IsRadialCorrection;
        //    float fRadialCorrectionIntensity = _captureShootImageList[0].Item2.RadialCorIntensity;
        //    double[] radialCorrectionCurve = _captureShootImageList[0].Item2.RadialCorrectionCurve; // 곡선 데이터 추가
        //    bool bRemoveReflectedLight = _captureShootImageList[0].Item2.IsRemoveReflectedLight;

        //    await Task.Run(() =>
        //    {
        //        string strFileName;
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";

        //        if (Directory.Exists(strFolderPath) == false)
        //        {
        //            Directory.CreateDirectory(strFolderPath);
        //        }

        //        // 원본 이미지를 복사하여 조정 적용
        //        Mat adjustedMat = _captureShootImageList[0].Item1.Clone();

        //        if (bRemoveReflectedLight == true)
        //        {
        //            Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //            adjustedMat.Dispose();
        //            adjustedMat = tempMat;
        //        }

        //        // 1. Brightness 조정
        //        if (nBright != 0)
        //        {
        //            Mat tempMat = adjustBrightnessWinStyle(adjustedMat, nBright);
        //            adjustedMat.Dispose();
        //            adjustedMat = tempMat;
        //        }

        //        // 2. Radial Correction 적용 (곡선 기반 또는 강도 기반)
        //        if (bRadialCor == true)
        //        {
        //            Mat correctedMat;

        //            // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //            if (radialCorrectionCurve != null && radialCorrectionCurve.Length > 0)
        //            {
        //                correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                    adjustedMat, radialCorrectionCurve);
        //                strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                    $"_Manual_Bright_{nBright}_RadialCurve" + ".png";
        //            }
        //            else
        //            {
        //                correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                    adjustedMat, fRadialCorrectionIntensity);
        //                strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                    $"_Manual_Bright_{nBright}_Radial_{fRadialCorrectionIntensity}" + ".png";
        //            }

        //            adjustedMat.Dispose();
        //            adjustedMat = correctedMat;
        //        }
        //        else
        //        {
        //            strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") +
        //                $"_Manual_Bright_{nBright}" + ".png";
        //        }

        //        DrawColorMask(ref adjustedMat, _maskColorRadius);

        //        adjustedMat.SaveImage(strFileName);

        //        // IR 영상
        //        if (_frameIrMat != null || _frameIrMat?.Empty() == false)
        //        {
        //            strFileName = strFolderPath + "//" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + $"_Manual_IR" + ".png";
        //            _frameIrMat.SaveImage(strFileName);
        //        }

        //        adjustedMat.Dispose();
        //    });
        //}

        //private Mat applyRemoveReflectedLight(Mat source)
        //{
        //    string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_ReflectedLight";
        //    if (Directory.Exists(strFolderPath) == false)
        //    {
        //        Directory.CreateDirectory(strFolderPath);
        //    }

        //    // 폴더에서 첫 번째 파일 찾기
        //    string[] files = Directory.GetFiles(strFolderPath);
        //    if (files.Length == 0)
        //    {
        //        return source.Clone();
        //    }

        //    Mat reflectedMat = Cv2.ImRead(files[0], ImreadModes.Color);

        //    // 이미지 로드 실패 체크
        //    if (reflectedMat == null || reflectedMat.Empty())
        //    {
        //        if (reflectedMat != null)
        //            reflectedMat.Dispose();
        //        return source.Clone();
        //    }

        //    // 이미지 사이즈 체크
        //    if (source.Width != reflectedMat.Width || source.Height != reflectedMat.Height)
        //    {
        //        reflectedMat.Dispose();
        //        return source.Clone();
        //    }

        //    // 원본에서 반사광 빼기
        //    Mat resultMat = new Mat();
        //    Cv2.Subtract(source, reflectedMat, resultMat);

        //    // 반사광 이미지 메모리 해제
        //    reflectedMat.Dispose();

        //    return resultMat;
        //}

        //#endregion Method - Review Roll SWTrig Manual

        //#region EventHandler - Review

        //private void mySliderReviewImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (_captureImageAndParamList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;

        //    // UI 스레드에서 실행되므로 비동기 처리
        //    _ = Task.Run(() =>
        //    {
        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureImageAndParamList[nImageIndex].Item1.Clone();
        //            ImageAdjustParam adjustParam = _captureImageAndParamList[nImageIndex].Item2;

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            //_captureImageAndParamList[nImageIndex].Item2.Brightness = adjustParam.Brightness;

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    });
        //}

        //private void myBtReviewSaveSingle_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveSingleAdjustImage_();
        //}

        //private void myBtReviewSaveAll_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveAllImages_();
        //}

        //private void myBtReviewImageAdjust_Click(object sender, RoutedEventArgs e)
        //{
        //    if (_captureImageAndParamList.Count > 0)
        //    {
        //        int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;
        //        AdjustmentWindow.SetDefaultValues(_captureImageAndParamList[nImageIndex].Item2);
        //    }
        //    else
        //    {
        //        return;
        //    }

        //    AdjustmentWindow.SetAdjustmentCallback(OnReviewImageAdjustmentChanged);
        //    // Modeless Window 표시
        //    if (!AdjustmentWindow.IsVisible)
        //    {
        //        AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);  // 부모 윈도우 설정
        //        //_adjustmentWindow.ResetValues();
        //        AdjustmentWindow.Show();
        //    }
        //    else
        //    {
        //        AdjustmentWindow.Activate();  // 이미 열려있으면 활성화
        //    }
        //}

        //private void OnReviewImageAdjustmentChanged(ImageAdjustParam adjustParam)
        //{
        //    if (_captureImageAndParamList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;

        //    // UI 스레드에서 실행되므로 비동기 처리
        //    _ = Task.Run(() =>
        //    {
        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureImageAndParamList[nImageIndex].Item1.Clone();

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //                _captureImageAndParamList[nImageIndex].Item2.IsRemoveReflectedLight = true;
        //            }
        //            else
        //            {
        //                _captureImageAndParamList[nImageIndex].Item2.IsRemoveReflectedLight = false;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            _captureImageAndParamList[nImageIndex].Item2.Brightness = adjustParam.Brightness;

        //            //// 2. Contrast 조정
        //            //if (Math.Abs(contrast - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = new Mat();
        //            //    adjustedMat.ConvertTo(tempMat, -1, contrast, 0);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            //// 3. Gamma 조정
        //            //if (Math.Abs(gamma - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = AdjustGamma(adjustedMat, gamma);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;

        //                _captureImageAndParamList[nImageIndex].Item2.IsRadialCorrection = true;
        //                _captureImageAndParamList[nImageIndex].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureImageAndParamList[nImageIndex].Item2.RadialCorrectionCurve = adjustParam.RadialCorrectionCurve;
        //            }
        //            else
        //            {
        //                _captureImageAndParamList[nImageIndex].Item2.IsRadialCorrection = false;
        //                _captureImageAndParamList[nImageIndex].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureImageAndParamList[nImageIndex].Item2.RadialCorrectionCurve = null;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();

        //            ImageAdjustPreset imageAdjustPreset = new ImageAdjustPreset();
        //            imageAdjustPreset.Brightness = adjustParam.Brightness;
        //            Configuration.SubmitImageAdjustPreset(imageAdjustPreset);
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    });
        //}


        //#endregion EventHandler - Review

        //#region EventHandler - Review Slice

        //private void myBtReviewSliceSaveAll_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveSliceImage_();
        //}

        //private void myBtReviewSliceImageAdjust_Click(object sender, RoutedEventArgs e)
        //{
        //    if (_captureSliceAcqImageList.Count > 0)
        //    {
        //        int nAcqImageIndex = (int)mySliderReviewSliceImageIndex.Value - 1;
        //        int nSubImageIndex = (int)mySliderReviewSliceSubImageIndex.Value - 1;
        //        AdjustmentWindow.SetDefaultValues(_captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2);
        //    }
        //    else
        //    {
        //        return;
        //    }

        //    AdjustmentWindow.SetAdjustmentCallback(OnReviewSliceImageAdjustmentChanged);
        //    // Modeless Window 표시
        //    if (!AdjustmentWindow.IsVisible)
        //    {
        //        AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);  // 부모 윈도우 설정
        //        //_adjustmentWindow.ResetValues();
        //        AdjustmentWindow.Show();
        //    }
        //    else
        //    {
        //        AdjustmentWindow.Activate();  // 이미 열려있으면 활성화
        //    }
        //}

        //private void OnReviewSliceImageAdjustmentChanged(ImageAdjustParam adjustParam)
        //{
        //    if (_captureSliceAcqImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nAcqImageIndex = (int)mySliderReviewSliceImageIndex.Value - 1;
        //    int nSubImageIndex = (int)mySliderReviewSliceSubImageIndex.Value - 1;

        //    // UI 스레드에서 실행되므로 비동기 처리
        //    _ = Task.Run(() =>
        //    {
        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item1.Clone();

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRemoveReflectedLight = true;
        //            }
        //            else
        //            {
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRemoveReflectedLight = false;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.Brightness = adjustParam.Brightness;

        //            //// 2. Contrast 조정
        //            //if (Math.Abs(contrast - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = new Mat();
        //            //    adjustedMat.ConvertTo(tempMat, -1, contrast, 0);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            //// 3. Gamma 조정
        //            //if (Math.Abs(gamma - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = AdjustGamma(adjustedMat, gamma);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;

        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRadialCorrection = true;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorrectionCurve = adjustParam.RadialCorrectionCurve;
        //            }
        //            else
        //            {
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRadialCorrection = false;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorrectionCurve = null;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    });
        //}

        //private void mySliderReviewSliceImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (sender is Slider slider)
        //    {
        //        int nIndex = (int)slider.Value;
        //        if (nIndex > 0)
        //        {
        //            if (_captureSliceAcqImageList.Count() > 0)
        //            {
        //                int oldSubImageIndex = (int)mySliderReviewSliceSubImageIndex.Value;

        //                mySliderReviewSliceSubImageIndex.Value = mySliderReviewSliceSubImageIndex.Minimum;

        //                if (oldSubImageIndex == mySliderReviewSliceSubImageIndex.Minimum)
        //                {
        //                    var args = new RoutedPropertyChangedEventArgs<double>(1, 1)
        //                    {
        //                        RoutedEvent = Slider.ValueChangedEvent
        //                    };

        //                    // RaiseEvent로 이벤트를 강제로 발생시킨다.
        //                    mySliderReviewSliceSubImageIndex.RaiseEvent(args);
        //                }
        //            }
        //        }
        //    }
        //}

        //private void mySliderReviewSliceSubImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (sender is Slider slider)
        //    {
        //        int nIndex = (int)slider.Value;
        //        if (nIndex > 0)
        //        {
        //            if (_captureSliceAcqImageList.Count() > 0)
        //            {
        //                int nAcqFrameIndex = (int)mySliderReviewSliceImageIndex.Value;

        //                if (nIndex == 1)
        //                {
        //                    myBtReviewSliceImageAdjust.IsEnabled = true;

        //                    _ = Task.Run(() =>
        //                    {
        //                        try
        //                        {
        //                            // 원본 이미지를 복사하여 조정 적용
        //                            Mat adjustedMat = _captureSliceAcqImageList[nAcqFrameIndex-1][nIndex - 1].Item1.Clone();
        //                            ImageAdjustParam adjustParam = _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item2;

        //                            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //                            if (adjustParam.IsRemoveReflectedLight == true)
        //                            {
        //                                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                                adjustedMat.Dispose();
        //                                adjustedMat = tempMat;
        //                            }

        //                            // 1. Brightness 조정
        //                            if (adjustParam.Brightness != 0)
        //                            {
        //                                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                                adjustedMat.Dispose();
        //                                adjustedMat = tempMat;
        //                            }

        //                            // 2. Radial Correction 적용 (곡선 기반)
        //                            if (adjustParam.IsRadialCorrection == true)
        //                            {
        //                                Mat correctedMat;

        //                                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                                {
        //                                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                                }
        //                                else
        //                                {
        //                                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                                        adjustedMat, adjustParam.RadialCorIntensity);
        //                                }

        //                                adjustedMat.Dispose();
        //                                adjustedMat = correctedMat;
        //                            }


        //                            // 4. 마스크 적용
        //                            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //                            // 5. UI 업데이트 (UI 스레드에서)
        //                            Dispatcher.Invoke(() =>
        //                            {
        //                                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //                            });

        //                            adjustedMat.Dispose();
        //                        }
        //                        catch (Exception ex)
        //                        {
        //                            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //                        }
        //                    });
        //                }
        //                else
        //                {
        //                    myBtReviewSliceImageAdjust.IsEnabled = false;

        //                    UpdateReviewSliceStatusItems(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1.Width, _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1.Height);
        //                    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1);
        //                }
        //            }
        //        }
        //    }
        //}

        //#endregion EventHandler - Review Slice

        //#region EventHandler - Review ROI

        //private void mySliderReviewROIImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (sender is Slider slider)
        //    {
        //        int nIndex = (int)slider.Value;
        //        if (nIndex > 0)
        //        {
        //            if (_captureSliceAcqImageList.Count() > 0)
        //            {
        //                int oldSubImageIndex = (int)mySliderReviewROISubImageIndex.Value;

        //                mySliderReviewROISubImageIndex.Value = mySliderReviewROISubImageIndex.Minimum;

        //                if (oldSubImageIndex == mySliderReviewROISubImageIndex.Minimum)
        //                {
        //                    var args = new RoutedPropertyChangedEventArgs<double>(1, 1)
        //                    {
        //                        RoutedEvent = Slider.ValueChangedEvent
        //                    };

        //                    // RaiseEvent로 이벤트를 강제로 발생시킨다.
        //                    mySliderReviewROISubImageIndex.RaiseEvent(args);
        //                }
        //            }
        //        }
        //    }
        //}

        //private void mySliderReviewROISubImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (sender is Slider slider)
        //    {
        //        int nIndex = (int)slider.Value;
        //        if (nIndex > 0)
        //        {
        //            if (_captureSliceAcqImageList.Count() > 0)
        //            {
        //                int nAcqFrameIndex = (int)mySliderReviewROIImageIndex.Value;

        //                if (nIndex == 1)
        //                {
        //                    myBtReviewROIImageAdjust.IsEnabled = true;

        //                    _ = Task.Run(() =>
        //                    {
        //                        try
        //                        {
        //                            // 원본 이미지를 복사하여 조정 적용
        //                            Mat adjustedMat = _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1.Clone();
        //                            ImageAdjustParam adjustParam = _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item2;

        //                            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //                            if (adjustParam.IsRemoveReflectedLight == true)
        //                            {
        //                                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                                adjustedMat.Dispose();
        //                                adjustedMat = tempMat;
        //                            }

        //                            // 1. Brightness 조정
        //                            if (adjustParam.Brightness != 0)
        //                            {
        //                                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                                adjustedMat.Dispose();
        //                                adjustedMat = tempMat;
        //                            }

        //                            // 2. Radial Correction 적용 (곡선 기반)
        //                            if (adjustParam.IsRadialCorrection == true)
        //                            {
        //                                Mat correctedMat;

        //                                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                                {
        //                                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                                }
        //                                else
        //                                {
        //                                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                                        adjustedMat, adjustParam.RadialCorIntensity);
        //                                }

        //                                adjustedMat.Dispose();
        //                                adjustedMat = correctedMat;
        //                            }


        //                            // 4. 마스크 적용
        //                            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //                            // 5. UI 업데이트 (UI 스레드에서)
        //                            Dispatcher.Invoke(() =>
        //                            {
        //                                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //                            });

        //                            adjustedMat.Dispose();
        //                        }
        //                        catch (Exception ex)
        //                        {
        //                            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //                        }
        //                    });
        //                }
        //                else
        //                {
        //                    myBtReviewROIImageAdjust.IsEnabled = false;

        //                    UpdateReviewROIStatusItems(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1.Width, _captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1.Height);
        //                    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_captureSliceAcqImageList[nAcqFrameIndex - 1][nIndex - 1].Item1);
        //                }
        //            }
        //        }
        //    }
        //}

        //private void myBtReviewROISaveAll_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveROICaptureImage_();
        //}

        //private void myBtReviewROIImageAdjust_Click(object sender, RoutedEventArgs e)
        //{
        //    if (_captureSliceAcqImageList.Count > 0)
        //    {
        //        int nAcqImageIndex = (int)mySliderReviewROIImageIndex.Value - 1;
        //        int nSubImageIndex = (int)mySliderReviewROISubImageIndex.Value - 1;
        //        AdjustmentWindow.SetDefaultValues(_captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2);
        //    }
        //    else
        //    {
        //        return;
        //    }

        //    AdjustmentWindow.SetAdjustmentCallback(OnReviewROIImageAdjustmentChanged);
        //    // Modeless Window 표시
        //    if (!AdjustmentWindow.IsVisible)
        //    {
        //        AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);  // 부모 윈도우 설정
        //        //_adjustmentWindow.ResetValues();
        //        AdjustmentWindow.Show();
        //    }
        //    else
        //    {
        //        AdjustmentWindow.Activate();  // 이미 열려있으면 활성화
        //    }
        //}

        //private void OnReviewROIImageAdjustmentChanged(ImageAdjustParam adjustParam)
        //{
        //    if (_captureSliceAcqImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    int nAcqImageIndex = (int)mySliderReviewROIImageIndex.Value - 1;
        //    int nSubImageIndex = (int)mySliderReviewROISubImageIndex.Value - 1;

        //    // UI 스레드에서 실행되므로 비동기 처리
        //    _ = Task.Run(() =>
        //    {
        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item1.Clone();

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRemoveReflectedLight = true;
        //            }
        //            else
        //            {
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRemoveReflectedLight = false;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.Brightness = adjustParam.Brightness;

        //            //// 2. Contrast 조정
        //            //if (Math.Abs(contrast - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = new Mat();
        //            //    adjustedMat.ConvertTo(tempMat, -1, contrast, 0);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            //// 3. Gamma 조정
        //            //if (Math.Abs(gamma - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = AdjustGamma(adjustedMat, gamma);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;

        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRadialCorrection = true;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorrectionCurve = adjustParam.RadialCorrectionCurve;
        //            }
        //            else
        //            {
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.IsRadialCorrection = false;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureSliceAcqImageList[nAcqImageIndex][nSubImageIndex].Item2.RadialCorrectionCurve = null;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    });
        //}

        //#endregion EventHandler - Review ROI

        //#region EventHandler - Review Offset ROI

        //private void myBtSaveReviewOffsetROI_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveOffsetROIImage_();
        //}

        //#endregion EventHandler - Review Offset ROI

        //#region EventHandler - Review Roll SWTrig Overlap

        //private void mySliderReviewRollSWTrigOverlapImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (sender is Slider slider)
        //    {
        //        int nIndex = (int)slider.Value;
        //        if (nIndex > 0)
        //        {
        //            if (_captureImageList.Count() > 0)
        //            {
        //                Mat matBright = adjustBrightnessWinStyle(_captureImageList[nIndex - 1], ReviewRollSWTrigOverlapBrightness);
        //                DrawColorMask(ref matBright, _maskColorRadius);
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(matBright);
        //                mySliderReviewRollSWTrigOverlapBrightness.Value = ReviewRollSWTrigOverlapBrightness;

        //                matBright.Dispose();
        //            }
        //        }
        //    }
        //}

        //private void myBtReviewRollSWTrigOverlapSaveAll_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveAllReviewRollSWTrigOverlapImage_();
        //}

        //private void myBtReviewRollSWTrigOverlapSaveCurrentSingle_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveReviewRollSWTrigOverlapBrightImage_();
        //}

        //private void mySliderReviewRollSWTrigOverlapBrightness_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        //{
        //    if (sender is Slider slider)
        //    {
        //        int nIndex = (int)mySliderReviewRollSWTrigOverlapImageIndex.Value;
        //        int nBrightness = (int)slider.Value;
        //        if (_captureImageList.Count() > 0)
        //        {
        //            Mat matBright = adjustBrightnessWinStyle(_captureImageList[nIndex - 1], nBrightness);
        //            DrawColorMask(ref matBright, _maskColorRadius);
        //            imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(matBright);
        //            matBright.Dispose();
        //        }
        //    }
        //}

        //private void myTbReviewRollSWTrigOverlapBrightness_KeyDown(object sender, KeyEventArgs e)
        //{
        //    if (e.Key == Key.Enter)
        //    {
        //        int value;
        //        if (int.TryParse(myTbReviewRollSWTrigOverlapBrightness.Text, out value))
        //        {
        //            mySliderReviewRollSWTrigOverlapBrightness.Value = value;
        //        }
        //        else
        //        {
        //            mySliderReviewRollSWTrigOverlapBrightness.Value = 0;
        //        }
        //    }
        //}

        //#endregion EventHandler - Review Roll SWTrig Overlap

        //#region EventHandler - Review Roll SWTrig Manual

        //private void myBtReviewRollSWTrigManualSaveCurrentSingle_Click(object sender, RoutedEventArgs e)
        //{
        //    if ((Keyboard.Modifiers & ModifierKeys.Control) == ModifierKeys.Control)
        //    {
        //        string strFolderPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture";
        //        Process.Start("explorer.exe", strFolderPath);
        //        return;
        //    }

        //    _ = saveReviewRollSWTrigManualBrightImage_();
        //}

        //private void myBtReviewRollSWTrigManualAdjust_Click(object sender, RoutedEventArgs e)
        //{
        //    if (_captureShootImageList.Count > 0)
        //    {
        //        AdjustmentWindow.SetDefaultValues(_captureShootImageList[0].Item2);
        //    }

        //    AdjustmentWindow.SetAdjustmentCallback(OnReviewRollSWTrigManualImageAdjustmentChanged);
        //    // Modeless Window 표시
        //    if (!AdjustmentWindow.IsVisible)
        //    {
        //        AdjustmentWindow.Owner = System.Windows.Window.GetWindow(this);  // 부모 윈도우 설정
        //        //_adjustmentWindow.ResetValues();
        //        AdjustmentWindow.Show();
        //    }
        //    else
        //    {
        //        AdjustmentWindow.Activate();  // 이미 열려있으면 활성화
        //    }
        //}

        //private void OnReviewRollSWTrigManualImageAdjustmentChanged(ImageAdjustParam adjustParam)
        //{
        //    if (_captureShootImageList.Count() <= 0)
        //    {
        //        return;
        //    }

        //    // UI 스레드에서 실행되므로 비동기 처리
        //    _ = Task.Run(() =>
        //    {
        //        try
        //        {
        //            // 원본 이미지를 복사하여 조정 적용
        //            Mat adjustedMat = _captureShootImageList[0].Item1.Clone();

        //            // 1. 반사광 제거 적용 (원본에서 먼저 적용)
        //            if (adjustParam.IsRemoveReflectedLight == true)
        //            {
        //                Mat tempMat = applyRemoveReflectedLight(adjustedMat);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //                _captureShootImageList[0].Item2.IsRemoveReflectedLight = true;
        //            }
        //            else
        //            {
        //                _captureShootImageList[0].Item2.IsRemoveReflectedLight = false;
        //            }

        //            // 1. Brightness 조정
        //            if (adjustParam.Brightness != 0)
        //            {
        //                Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
        //                adjustedMat.Dispose();
        //                adjustedMat = tempMat;
        //            }

        //            _captureShootImageList[0].Item2.Brightness = adjustParam.Brightness;

        //            //// 2. Contrast 조정
        //            //if (Math.Abs(contrast - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = new Mat();
        //            //    adjustedMat.ConvertTo(tempMat, -1, contrast, 0);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            //// 3. Gamma 조정
        //            //if (Math.Abs(gamma - 1.0) > 0.01)
        //            //{
        //            //    Mat tempMat = AdjustGamma(adjustedMat, gamma);
        //            //    adjustedMat.Dispose();
        //            //    adjustedMat = tempMat;
        //            //}

        //            // 2. Radial Correction 적용 (곡선 기반)
        //            if (adjustParam.IsRadialCorrection == true)
        //            {
        //                Mat correctedMat;

        //                // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
        //                if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
        //                {
        //                    correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
        //                        adjustedMat, adjustParam.RadialCorrectionCurve);
        //                }
        //                else
        //                {
        //                    correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
        //                        adjustedMat, adjustParam.RadialCorIntensity);
        //                }

        //                adjustedMat.Dispose();
        //                adjustedMat = correctedMat;

        //                _captureShootImageList[0].Item2.IsRadialCorrection = true;
        //                _captureShootImageList[0].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureShootImageList[0].Item2.RadialCorrectionCurve = adjustParam.RadialCorrectionCurve;
        //            }
        //            else
        //            {
        //                _captureShootImageList[0].Item2.IsRadialCorrection = false;
        //                _captureShootImageList[0].Item2.RadialCorIntensity = adjustParam.RadialCorIntensity;
        //                _captureShootImageList[0].Item2.RadialCorrectionCurve = null;
        //            }


        //            // 4. 마스크 적용
        //            DrawColorMask(ref adjustedMat, _maskColorRadius);

        //            // 5. UI 업데이트 (UI 스레드에서)
        //            Dispatcher.Invoke(() =>
        //            {
        //                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
        //            });

        //            adjustedMat.Dispose();

        //            ImageAdjustPreset imageAdjustPreset = new ImageAdjustPreset();
        //            imageAdjustPreset.Brightness = adjustParam.Brightness;
        //            Configuration.SubmitImageAdjustPreset(imageAdjustPreset);
        //        }
        //        catch (Exception ex)
        //        {
        //            System.Diagnostics.Debug.WriteLine($"Image adjustment error: {ex.Message}");
        //        }
        //    });
        //}

        //#endregion EventHandler - Review Roll SWTrig Manual

        //#region Callback

        //public void CallbackLsoScanFrameImage(byte[] data, int width, int height, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //        _imageMat = mat;


        //    //if (IsHorizontalFlip)
        //    //{
        //    //    Cv2.Flip(_imageMat, _imageMat, FlipMode.Y);
        //    //}
        //    //if (IsVerticalFlip)
        //    //{
        //    //    Cv2.Flip(_imageMat, _imageMat, FlipMode.X);
        //    //}

        //    UpdatePreviewStatusItems();
        //    UpdateColorFrameImage(nPixelFormat);
        //}

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

            if (frameCount == totalFrameCount - 1)
            {
                //_adjustmentWindow?.ForceClose();
                //_adjustmentWindow = null;

                //if (IsReviewSliceMode == true)
                //{
                //    SetReviewSliceMode();
                //}
                //else if (IsReviewROIMode == true)
                //{
                //    SetReviewROIMode();
                //}
                //else
                //{
                //    UpdateReviewStatusItems(width, height);
                //    SetReviewMode(totalFrameCount);
                //}

                UpdateReviewStatusItems(width, height);
                SetReviewMode(totalFrameCount);
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
                    //// 원본 이미지를 복사하여 조정 적용
                    //Mat adjustedMat = _captureImageAndParamList[nImageIndex].Item1.Clone();
                    //ImageAdjustParam adjustParam = _captureImageAndParamList[nImageIndex].Item2;

                    //// 1. 반사광 제거 적용 (원본에서 먼저 적용)
                    //if (adjustParam.IsRemoveReflectedLight == true)
                    //{
                    //    Mat tempMat = applyRemoveReflectedLight(adjustedMat);
                    //    adjustedMat.Dispose();
                    //    adjustedMat = tempMat;
                    //}

                    //// 1. Brightness 조정
                    //if (adjustParam.Brightness != 0)
                    //{
                    //    Mat tempMat = adjustBrightnessWinStyle(adjustedMat, adjustParam.Brightness);
                    //    adjustedMat.Dispose();
                    //    adjustedMat = tempMat;
                    //}

                    ////_captureImageAndParamList[nImageIndex].Item2.Brightness = adjustParam.Brightness;

                    //// 2. Radial Correction 적용 (곡선 기반)
                    //if (adjustParam.IsRadialCorrection == true)
                    //{
                    //    Mat correctedMat;

                    //    // 곡선 데이터가 있으면 곡선 기반 보정, 없으면 기존 강도 기반 보정
                    //    if (adjustParam.RadialCorrectionCurve != null && adjustParam.RadialCorrectionCurve.Length > 0)
                    //    {
                    //        correctedMat = RadialCorrection.ApplyCustomCurveRadialCorrection(
                    //            adjustedMat, adjustParam.RadialCorrectionCurve);
                    //    }
                    //    else
                    //    {
                    //        correctedMat = RadialCorrection.ApplyRadialBrightnessCorrection(
                    //            adjustedMat, adjustParam.RadialCorIntensity);
                    //    }

                    //    adjustedMat.Dispose();
                    //    adjustedMat = correctedMat;
                    //}


                    //// 4. 마스크 적용
                    //DrawColorMask(ref adjustedMat, _maskColorRadius);

                    //// 5. UI 업데이트 (UI 스레드에서)
                    //Dispatcher.Invoke(() =>
                    //{
                    //    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(adjustedMat);
                    //});

                    //adjustedMat.Dispose();

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

        //public void CallbackLsoScanSeqLiveFrameImage(byte[] data, int width, int height, int offsetX, int offsetY, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //    {
        //        makeReviewImage(mat, nPixelFormat);
        //    }

        //    if (frameCount == totalFrameCount - 1)
        //    {
        //        DisplaySeqROIMode();
        //        UpdateFramerate();
        //        UpdatePreviewStatusItems();
        //    }
        //}

        //public void CallbackLsoScanOffsetLiveFrameImage(byte[] data, int width, int height, int offsetX, int offsetY, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //    {
        //        makeReviewImage(mat, nPixelFormat);
        //    }

        //    if (frameCount == totalFrameCount - 1)
        //    {
        //        DisplayOffsetROIMode();
        //        UpdateFramerate();
        //        UpdatePreviewStatusItems();
        //    }
        //}

        //public void CallbackLsoScanOffsetCaptureFrameImage(byte[] data, int width, int height, int offsetX, int offsetY, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //    {
        //        makeReviewImage(mat, nPixelFormat);
        //    }

        //    if (frameCount == totalFrameCount - 1)
        //    {
        //        SetOffsetROIMode();
        //        //UpdateFramerate();
        //        UpdateReviewOffsetROIStatusItems();
        //    }
        //}

        //public void CallbackLsoScanRollSWTrigOverlapLiveFrameImage(byte[] data, int width, int height, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //    {
        //        makeReviewImage(mat, nPixelFormat);
        //    }

        //    if (frameCount == totalFrameCount - 1)
        //    {
        //        DisplayRollSWTrigOverlapMode();
        //        UpdateFramerate();
        //        UpdatePreviewStatusItems();
        //    }
        //}

        //public void CallbackLsoScanRollSWTrigOverlapCaptureFrameImage(byte[] data, int width, int height, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //    {
        //        makeReviewImage(mat, nPixelFormat);
        //    }

        //    if (frameCount == totalFrameCount - 1)
        //    {
        //        SetReviewRollSWTrigOverlapMode(totalFrameCount);
        //        //DisplayRollSWTrigOverlapMode();
        //        //UpdateFramerate();
        //        //UpdatePreviewStatusItems();
        //    }
        //}

        //public void CallbackLsoScanRollSWTrigManualCaptureFrameImage(byte[] data, int width, int height, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;
        //    _imageChannels = channels;
        //    _imageQuality = quality;

        //    MatType matType = -1;

        //    switch (nBytesPerPixel)
        //    {
        //        case 1:
        //            matType = MatType.CV_8UC1;
        //            break;
        //        case 2:
        //            matType = MatType.CV_16UC1;
        //            break;
        //        case 3:
        //            matType = MatType.CV_8UC3;
        //            break;
        //        default:
        //            return;
        //    }

        //    // 1) 새 Mat 생성 (언매니지드 메모리 할당)
        //    var mat = new Mat(height, width, matType);

        //    // 2) managed 배열 → Mat.Data(IntPtr)로 복사

        //    int byteCount = height * width * channels * sizeof(byte) * nBytesPerPixel;

        //    // CV_16U 이므로 ushort(2바이트) 사용
        //    Marshal.Copy(data, 0, mat.Data, byteCount);

        //    // 3) 스레드 안전하게 교체
        //    lock (_matLock)
        //    {
        //        makeReviewImage(ref _captureShootImageList, mat, nPixelFormat);
        //    }

        //    if (frameCount == totalFrameCount - 1)
        //    {
        //        _adjustmentWindow?.ForceClose();
        //        _adjustmentWindow = null;
        //        SetReviewRollSWTrigManualMode(totalFrameCount);
        //    }
        //}

        //public void CallbackIrCameraFrame(byte[] data, int width, int height)
        //{
        //    _imageWidth = width;
        //    _imageHeight = height;

        //    UpdatePreviewStatusItems();

        //    _imageIrMat = Mat.FromPixelData(height, width, MatType.CV_8UC1, data);
        //    UpdateIrFrameImage();
        //}


        //#endregion Callback


    }
}
