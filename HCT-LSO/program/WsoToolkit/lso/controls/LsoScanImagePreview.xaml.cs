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
        private const double LiveClaheClipLimit = 2.0;
        private const int LiveClaheTileGridSize = 8;

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
            return CloneAdjustParam(_defaultAdjustPreset);
        }

        private static ImageAdjustParam CloneAdjustParam(ImageAdjustParam source)
        {
            return new ImageAdjustParam(source.Brightness)
            {
                Contrast = source.Contrast,
                Gamma = source.Gamma,
                Sharpness = source.Sharpness,
                Saturation = source.Saturation,
                ChannelView = source.ChannelView,
                IsRadialCorrection = source.IsRadialCorrection,
                RadialCorIntensity = source.RadialCorIntensity,
                RadialCorrectionCurve = source.RadialCorrectionCurve?.ToArray(),
            };
        }

        private void PersistAdjustPreset_(ImageAdjustParam adjustParam)
        {
            _defaultAdjustPreset = CloneAdjustParam(adjustParam);
            ImageAdjustBrightness = adjustParam.Brightness;
            AdjustPresetChanged?.Invoke(CloneAdjustParam(adjustParam));
        }

        // Review view: one capture, up to four ways of looking at it. The stitch and its bands
        // only exist when the scan really ran more than one subframe per acquisition; Processed
        // only exists for a Gray image mode, once its frames have been averaged.
        public enum ReviewViewMode
        {
            Stitched = 0,
            SubBands = 1,
            FullFrames = 2,
            Processed = 3,
        }

        private ReviewViewMode _reviewView = ReviewViewMode.FullFrames;
        private readonly List<Mat> _stitchedList = new();
        private readonly List<List<Mat>> _bandList = new();
        private readonly List<ImageAdjustParam> _stitchedAdjustParams = new();

        // CV_16UC1, half the frame size: LsoModeImageAccumulator's mean, kept in its own units
        // so Save writes exactly what the Agent will store.
        private Mat? _processedMat;
        private int _processedMaxValue;
        private string _processedLabel = string.Empty;
        // The colour pipeline's final 8-bit image (post processing). Already display-ready, so it is
        // kept as a frozen BitmapSource instead of a Mat.
        private BitmapSource? _processedColor;

        private bool HasProcessed => _processedMat != null || _processedColor != null;

        /// <summary>
        /// How the subframes are cut and blended. Edited from the Stitch settings window;
        /// call <see cref="Restitch"/> after changing it.
        /// </summary>
        public LsoStitchParams StitchParams { get; } = new();

        // Where each band came from, for the acquisitions in review. Every acquisition has the
        // same geometry, so one list is enough - and the Bands view reads it instead of
        // recomputing band positions from Mat heights.
        private List<LsoStitchParams.BandGeometry> _bandGeom = new();

        private bool HasStitched => _stitchedList.Count > 0;

        public LsoScanImagePreview()
        {
            InitializeComponent();

            mySlitOverlay.Viewport = imageViewport;

            // The viewport grid is click-through by design; it is opened only for the duration
            // of a slit measurement so the two clicks can land.
            mySlitOverlay.MeasuringChanged += measuring => myViewportHost.IsHitTestVisible = measuring;
        }

        /// <summary>Two-point slit height measurement drawn over the live image.</summary>
        public SlitMeasureOverlay SlitMeasure => mySlitOverlay;

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

            // The guide marks the image centre, so under the preview zoom the centre rides on the
            // image and the tick pitch scales with it, while the strokes stay 1 px. At x1 this is
            // the viewport centre and the original pitch.
            System.Windows.Point centre = myZoomSurface.TranslatePoint(
                new System.Windows.Point(myZoomSurface.ActualWidth / 2, myZoomSurface.ActualHeight / 2), canvas);
            double zoom = myZoomScale.ScaleX;
            double cx = centre.X;
            double cy = centre.Y;

            // 주 십자선 (viewport 전체)
            AddGuideLine(brush, cx, 0, cx, h);
            AddGuideLine(brush, 0, cy, w, cy);

            // Major tick
            double unit = 80 * zoom, size = 7;
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
            unit = 10 * zoom; size = 3;
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
                    using (var enhancedMat = new Mat())
                    using (var clahe = Cv2.CreateCLAHE(
                        LiveClaheClipLimit,
                        new OpenCvSharp.Size(LiveClaheTileGridSize, LiveClaheTileGridSize)))
                    {
                        clahe.Apply(_imageMat, enhancedMat);
                        Cv2.CvtColor(enhancedMat, _frameMat, ColorConversionCodes.GRAY2RGB);
                    }
                    imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(_frameMat);
                    return;
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
                            if (eFormat == ColorPixelFormat.BayerRG8)
                            {
                                using var enhancedMat = new Mat();
                                using var clahe = Cv2.CreateCLAHE(
                                    LiveClaheClipLimit,
                                    new OpenCvSharp.Size(LiveClaheTileGridSize, LiveClaheTileGridSize));
                                clahe.Apply(grayMat, enhancedMat);
                                Cv2.CvtColor(enhancedMat, _frameMat, ColorConversionCodes.GRAY2RGB);
                            }
                            else
                            {
                                Cv2.CvtColor(grayMat, _frameMat, ColorConversionCodes.GRAY2RGB);
                            }
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
            refreshFafControls_();
            drawFixationOverlay_();
        }

        public void ClearReviewImages()
        {
            _captureImageList.Clear();
            _captureImageAdjustParams.Clear();
            clearStitched_();
            _processedMat?.Dispose();
            _processedMat = null;
            _processedMaxValue = 0;
            _processedColor = null;
            _showFafColor = false;
            refreshFafControls_();
        }

        // The Gray-mode result of the capture in review. A result that arrives after the view
        // left review (a new live start) belongs to no capture on screen and is dropped.
        // maxValue is the sensor's full scale (4095 for 12-bit, 65535 for 16-bit); 0 keeps
        // the historical min..max stretch for callers that do not state it.
        public void SetProcessedImage(ushort[] gray16, int width, int height, string label,
            int maxValue = 0)
        {
            if (gray16 == null)
            {
                throw new ArgumentNullException(nameof(gray16));
            }

            if (width <= 0 || height <= 0 || gray16.Length < width * height)
            {
                throw new ArgumentException($"The processed image does not hold {width}x{height}.", nameof(gray16));
            }

            if (!IsReviewMode)
            {
                return;
            }

            var mat = new Mat(height, width, MatType.CV_16UC1);
            short[] pixels = MemoryMarshal.Cast<ushort, short>(gray16.AsSpan(0, width * height)).ToArray();
            Marshal.Copy(pixels, 0, mat.Data, pixels.Length);

            _processedMat?.Dispose();
            _processedMat = null;
            _processedMaxValue = maxValue;
            _processedLabel = label;
            _processedColor = null;
            _reviewView = ReviewViewMode.Processed;
            applyReviewView_();
        }

        // The post-processing pipeline's final 8-bit BGR image, shown as the review's Processed
        // view straight after a capture. ToBitmapSource reads a 3-channel Mat as Bgr24, so the BGR
        // image goes in unconverted (an RGB Mat here swaps red and blue).
        public void SetProcessedColor(Mat bgr8, string label)
        {
            if (bgr8 == null || bgr8.Empty()) return;
            if (!IsReviewMode) return;

            BitmapSource bitmap = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(bgr8);
            bitmap.Freeze();

            _processedMat?.Dispose();
            _processedMat = null;
            _processedMaxValue = 0;
            _processedColor = bitmap;
            _processedLabel = label;
            _reviewView = ReviewViewMode.Processed;
            applyReviewView_();
        }

        // A stated full scale (the sensor bit depth) is the DevKit's fixed display scale. A
        // caller that does not state one keeps the historical min..max stretch.
        private void renderProcessed_(Mat gray16)
        {
            using var display = new Mat();
            if (_processedMaxValue > 0)
            {
                double scale = 255.0 / _processedMaxValue;
                gray16.ConvertTo(display, MatType.CV_8UC1, scale);
            }
            else
            {
                Cv2.MinMaxLoc(gray16, out double min, out double max);
                double scale = max > min ? 255.0 / (max - min) : 1.0;
                gray16.ConvertTo(display, MatType.CV_8UC1, scale, -min * scale);
            }
            imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(display);
        }

        // The stitch and its bands are Mats this control cloned itself, so it disposes them.
        private void clearStitched_()
        {
            foreach (Mat stitched in _stitchedList)
            {
                stitched.Dispose();
            }

            foreach (List<Mat> bands in _bandList)
            {
                foreach (Mat band in bands)
                {
                    band.Dispose();
                }
            }

            _stitchedList.Clear();
            _bandList.Clear();
            _stitchedAdjustParams.Clear();
            _bandGeom = new List<LsoStitchParams.BandGeometry>();
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
                default:
                    {
                        myGridLive.Visibility = Visibility.Visible;
                    }
                    break;
            }
        }

        public void SetReviewMode()
        {
            showDisplayMenu_(PreviewDisplayMode.REVIEW);

            // A capture with more than one subframe opens on the stitch, because that is the
            // image the operator pressed Capture for.
            _reviewView = HasStitched ? ReviewViewMode.Stitched : ReviewViewMode.FullFrames;
            applyReviewView_();
            drawFixationOverlay_();
        }

        // Frame j of an acquisition holds band j of the field - the galvo walks one band per
        // frame - so band j is cut out and the bands are laid back on top of each other:
        // frame 1 -> top, frame 2 -> next, and so on.
        //
        // The cut is taken with a margin on both sides (StitchParams.BandMarginRatio), so two
        // neighbours share rows instead of meeting at a hard edge, and those shared rows are
        // cross-faded. The weights are normalised per row before they are used, so the merge
        // is a plain accumulation - no full-size division buffer, and a row nothing covers
        // stays black instead of being divided by zero.
        private void buildStitchedFromSubFrames_()
        {
            clearStitched_();

            int subFrames = SubFrameCount;
            int acqFrames = Math.Max(1, AcqFrameCount);
            if (subFrames <= 1 || _captureImageList.Count != acqFrames * subFrames)
            {
                return;   // nothing to stitch - review stays on the raw frames
            }

            int height = _captureImageList[0].Rows;
            int width = _captureImageList[0].Cols;

            _bandGeom = StitchParams.Plan(height, subFrames);
            if (_bandGeom.Count != subFrames)
            {
                return;
            }

            for (int acq = 0; acq < acqFrames; ++acq)
            {
                Mat merged = mergeBands_(acq, subFrames, height, width, out List<Mat> bands);
                _stitchedList.Add(merged);
                _bandList.Add(bands);
                _stitchedAdjustParams.Add(CreateAdjustParamFromDefault(merged.Depth()));
            }
        }

        // One acquisition's bands blended into one image. The band Mats handed back are the
        // cut-outs WITH their margins, because that is what the Bands view has to show.
        private Mat mergeBands_(int acq, int subFrames, int height, int width, out List<Mat> bands)
        {
            bands = new List<Mat>(subFrames);

            Mat first = _captureImageList[acq * subFrames];
            int channels = first.Channels();

            // Weights depend on the row only, so the per-row total is known up front and each
            // band's weight can be normalised before it is added.
            double[] rowTotal = new double[height];
            for (int j = 0; j < subFrames; ++j)
            {
                LsoStitchParams.BandGeometry g = _bandGeom[j];
                for (int y = g.Src0; y < g.Src1; ++y)
                {
                    rowTotal[y] += StitchParams.WeightAt(_bandGeom, j, y);
                }
            }

            using Mat accumulator = new Mat(height, width, MatType.CV_32FC(channels), Scalar.All(0));

            for (int j = 0; j < subFrames; ++j)
            {
                LsoStitchParams.BandGeometry g = _bandGeom[j];
                var roi = new OpenCvSharp.Rect(0, g.Src0, width, g.SrcHeight);

                Mat slice = new Mat(_captureImageList[acq * subFrames + j], roi).Clone();
                bands.Add(slice);

                using Mat slice32 = new Mat();
                slice.ConvertTo(slice32, MatType.CV_32FC(channels));

                using Mat scaleColumn = new Mat(g.SrcHeight, 1, MatType.CV_32FC1);
                for (int r = 0; r < g.SrcHeight; ++r)
                {
                    int y = g.Src0 + r;
                    double weight = StitchParams.WeightAt(_bandGeom, j, y);
                    double share = rowTotal[y] > 1e-6 ? weight / rowTotal[y] : 0.0;
                    scaleColumn.Set(r, 0, (float)(share * StitchParams.GainAt(g, y)));
                }

                using Mat scale = new Mat();
                Cv2.Repeat(scaleColumn, 1, width, scale);
                using Mat scaleN = expandChannels_(scale, channels);
                using Mat contribution = slice32.Mul(scaleN);
                using Mat target = new Mat(accumulator, roi);
                Cv2.Add(target, contribution, target);
            }

            Mat merged = new Mat();
            accumulator.ConvertTo(merged, first.Type());
            return merged;
        }

        private static Mat expandChannels_(Mat single, int channels)
        {
            if (channels <= 1)
            {
                return single.Clone();
            }

            Mat[] planes = new Mat[channels];
            for (int i = 0; i < channels; ++i)
            {
                planes[i] = single;
            }

            Mat merged = new Mat();
            Cv2.Merge(planes, merged);
            return merged;
        }

        /// <summary>
        /// Rebuilds the stitch from the frames already in review with the current
        /// <see cref="StitchParams"/> and redraws whatever view is open.
        /// </summary>
        public void Restitch()
        {
            if (_captureImageList.Count == 0)
            {
                return;
            }

            buildStitchedFromSubFrames_();

            if (!HasStitched && _reviewView != ReviewViewMode.FullFrames)
            {
                _reviewView = ReviewViewMode.FullFrames;
            }

            applyReviewView_();
        }

        private int reviewImageCount_()
        {
            return _reviewView switch
            {
                ReviewViewMode.Stitched => _stitchedList.Count,
                ReviewViewMode.SubBands => _bandList.Count == 0 ? 0 : _bandList.Count * _bandList[0].Count,
                ReviewViewMode.Processed => HasProcessed ? 1 : 0,
                _ => _captureImageList.Count,
            };
        }

        // The one slider means a different thing per view, so its range and the readout are
        // rebuilt whenever the view changes.
        private void applyReviewView_()
        {
            myBtReviewViewMode.IsEnabled = HasStitched || HasProcessed;
            myBtReviewViewMode.Content = _reviewView switch
            {
                ReviewViewMode.Stitched => "View : Stitched",
                ReviewViewMode.SubBands => "View : Bands",
                ReviewViewMode.Processed => "View : Processed",
                _ => "View : Frames",
            };

            int count = reviewImageCount_();
            myTbTotalCount.Text = count.ToString();
            mySliderReviewImageIndex.Minimum = 1;
            mySliderReviewImageIndex.Maximum = Math.Max(1, count);

            if ((int)mySliderReviewImageIndex.Value == 1)
            {
                // Already at 1, so no ValueChanged will come - render it here.
                showReviewIndex_(1);
            }
            else
            {
                mySliderReviewImageIndex.Value = 1;
            }

            refreshFafControls_();
        }

        // Stitched = acquisition index, Bands = acquisition x band flattened, Frames = the
        // raw capture order.
        private void showReviewIndex_(int nOneBasedIndex)
        {
            int index = nOneBasedIndex - 1;
            if (index < 0)
            {
                return;
            }

            Mat source;
            ImageAdjustParam? adjust = null;
            string label;
            int guideBand = -1;

            switch (_reviewView)
            {
                case ReviewViewMode.Processed:
                    if (_processedColor != null)
                    {
                        lblImageStatusReview.Content = $"Processed ({_processedLabel})   {_processedColor.PixelWidth} x {_processedColor.PixelHeight}";
                        imageViewport.Source = _processedColor;
                        return;
                    }
                    if (_processedMat == null)
                    {
                        return;
                    }
                    lblImageStatusReview.Content = $"Processed ({_processedLabel})   {_processedMat.Width} x {_processedMat.Height}";
                    renderProcessed_(_processedMat);
                    return;

                case ReviewViewMode.Stitched:
                    if (index >= _stitchedList.Count)
                    {
                        return;
                    }
                    source = _stitchedList[index];
                    adjust = getStitchedAdjustParam_(index);
                    label = $"Stitched {nOneBasedIndex}/{_stitchedList.Count}";
                    break;

                case ReviewViewMode.SubBands:
                    {
                        if (_bandList.Count == 0)
                        {
                            return;
                        }

                        int bandCount = _bandList[0].Count;
                        int acq = index / bandCount;
                        int band = index % bandCount;
                        if (acq >= _bandList.Count)
                        {
                            return;
                        }

                        // The frame this band came from, with the cut drawn on it, so the
                        // band can be judged against the part of the frame that was lit.
                        // The geometry comes from the stitch itself - with margins the band
                        // Mat's height no longer says where it sits.
                        source = _captureImageList[acq * bandCount + band];
                        guideBand = band < _bandGeom.Count ? band : -1;
                        label = $"Acq {acq + 1} / Band {band + 1}";
                    }
                    break;

                default:
                    if (index >= _captureImageList.Count)
                    {
                        return;
                    }
                    source = _captureImageList[index];
                    adjust = GetAdjustParam(index);
                    label = $"Frame {nOneBasedIndex}/{_captureImageList.Count}";
                    break;
            }

            lblImageStatusReview.Content = $"{label}   {source.Width} x {source.Height}";

            // A FAF capture opens on its extracted gray image (DevKit FAF View); the color
            // frame is one press away.
            if (TryRenderFafView_(index, source, label))
            {
                return;
            }

            if (guideBand >= 0)
            {
                renderBandGuide_(source, _bandGeom[guideBand]);
                return;
            }

            renderReviewMat_(source, adjust);
        }

        // Bands are shown as captured (they carry no adjustment of their own); the stitch and
        // the raw frames go through the adjustment pipeline Review always used.
        private void renderReviewMat_(Mat source, ImageAdjustParam? adjustParam)
        {
            if (adjustParam == null)
            {
                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(source);
                return;
            }

            _ = Task.Run(() =>
            {
                try
                {
                    Mat adjustedMat = ApplyImageAdjustments(source, adjustParam);

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

        // Draws where this band is cut and how far it is blended, into a copy of the frame:
        //   solid yellow  = the rows this band alone owns (core)
        //   dashed cyan   = how far the cut actually reaches (core + margin)
        // Between the two lines the band is cross-faded with its neighbour. Lines go into the
        // image itself rather than a canvas overlay, so no viewport-to-image mapping is
        // needed and what is saved is what is seen.
        private void renderBandGuide_(Mat frame, LsoStitchParams.BandGeometry band)
        {
            using Mat marked = frame.Clone();

            double max = marked.Depth() == MatType.CV_16U ? 65535.0 : 255.0;
            var core = new Scalar(max, max, 0.0);          // yellow (these review Mats are RGB)
            var margin = new Scalar(0.0, max, max);        // cyan
            int thickness = Math.Max(2, marked.Rows / 500);

            drawGuideLine_(marked, band.Core0, core, thickness, 0);
            drawGuideLine_(marked, band.Core1 - 1, core, thickness, 0);

            if (band.Margin > 0)
            {
                int dash = Math.Max(8, marked.Cols / 60);
                drawGuideLine_(marked, band.Src0, margin, thickness, dash);
                drawGuideLine_(marked, band.Src1 - 1, margin, thickness, dash);
            }

            imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(marked);
        }

        // dash = 0 draws a solid line; otherwise dash-long segments with a dash-long gap,
        // which OpenCV has no line style for.
        private static void drawGuideLine_(Mat image, int y, Scalar color, int thickness, int dash)
        {
            int row = Math.Clamp(y, 0, image.Rows - 1);
            int right = image.Cols - 1;

            if (dash <= 0)
            {
                Cv2.Line(image, new OpenCvSharp.Point(0, row), new OpenCvSharp.Point(right, row), color, thickness);
                return;
            }

            for (int x = 0; x < right; x += dash * 2)
            {
                int end = Math.Min(right, x + dash);
                Cv2.Line(image, new OpenCvSharp.Point(x, row), new OpenCvSharp.Point(end, row), color, thickness);
            }
        }

        private ImageAdjustParam getStitchedAdjustParam_(int nIndex)
        {
            while (_stitchedAdjustParams.Count <= nIndex)
            {
                _stitchedAdjustParams.Add(CreateAdjustParamFromDefault());
            }

            return _stitchedAdjustParams[nIndex];
        }

        // The stitch tuning window. Owned by the host window, so it goes away with it; the
        // field only exists to avoid opening a second copy.
        private LsoStitchSettingWindow? _stitchWindow;

        private void myBtStitchSetting_Click(object sender, RoutedEventArgs e)
        {
            if (_stitchWindow != null)
            {
                _stitchWindow.Activate();
                return;
            }

            _stitchWindow = new LsoStitchSettingWindow(StitchParams, Restitch, stitchSummary_)
            {
                Owner = System.Windows.Window.GetWindow(this),
            };
            _stitchWindow.Closed += (_, _) => _stitchWindow = null;
            _stitchWindow.Show();
        }

        private string stitchSummary_()
        {
            if (_captureImageList.Count == 0)
            {
                return "Nothing in review yet - capture first, then the numbers below apply to it.";
            }

            int subFrames = Math.Max(1, SubFrameCount);
            int height = _captureImageList[0].Rows;
            int segH = StitchParams.EffectiveBandHeight(height, subFrames);
            int margin = StitchParams.EffectiveMargin(height, subFrames);

            string line = $"Frame {height} rows / {subFrames} subframes"
                + $"  ->  band {segH}, margin {margin} (overlap {margin * 2}), {segH + (margin * 2)} rows cut per band.";

            if (!HasStitched)
            {
                line += "  This capture has nothing to stitch (one subframe per acquisition).";
            }
            else if (segH * subFrames != height)
            {
                line += $"  Band x subframes = {segH * subFrames}, which is {Math.Abs((segH * subFrames) - height)} rows"
                    + (segH * subFrames < height ? " short of" : " past") + " the frame.";
            }

            return line;
        }

        private void myBtReviewViewMode_Click(object sender, RoutedEventArgs e)
        {
            if (!HasStitched && !HasProcessed)
            {
                return;
            }

            _reviewView = _reviewView switch
            {
                ReviewViewMode.Stitched => ReviewViewMode.SubBands,
                ReviewViewMode.SubBands => ReviewViewMode.FullFrames,
                ReviewViewMode.FullFrames when HasProcessed => ReviewViewMode.Processed,
                _ => HasStitched ? ReviewViewMode.Stitched : ReviewViewMode.FullFrames,
            };

            applyReviewView_();
        }

        // Which Capture preset produced the images now in review; the host sets it when the
        // capture starts, so the file names say what they were shot with.
        public string CapturePresetName { get; set; } = "Capture";

        private async Task saveAllImages_(string strFolderPath)
        {
            if (_captureImageList.Count <= 0)
            {
                return;
            }

            string strPreset = CapturePresetName;
            string strStamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            // Taken on the UI thread: the per-frame adjustment the review shows for each frame.
            var adjustParams = new List<ImageAdjustParam>();
            for (int i = 0; i < _captureImageList.Count; ++i)
            {
                adjustParams.Add(GetAdjustParam(i));
            }

            await Task.Run(() =>
            {
                if (!Directory.Exists(strFolderPath))
                {
                    Directory.CreateDirectory(strFolderPath);
                }

                for (int i = 0; i < _captureImageList.Count; ++i)
                {
                    using Mat mat = _captureImageList[i].Clone();
                    string strFileName = System.IO.Path.Combine(strFolderPath, $"{strPreset}_{strStamp}_{i + 1}.png");
                    mat.SaveImage(strFileName);
                    using Mat adjusted = ApplyImageAdjustments(mat, adjustParams[i]);
                    adjusted.SaveImage(System.IO.Path.Combine(strFolderPath, $"{strPreset}_{strStamp}_{i + 1}_adjusted.png"));
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

            if (frameCount == totalFrameCount - 1 && IsReviewMode) // Capture
            {
                buildStitchedFromSubFrames_();
                SetReviewMode();
            }
        }

        private void mySliderReviewImageIndex_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            showReviewIndex_((int)mySliderReviewImageIndex.Value);
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

            ApplyNativeAdjust(adjustedMat, adjustParam);

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

        private void ApplyNativeAdjust(Mat target, ImageAdjustParam adjustParam)
        {
            if (target.Empty())
            {
                return;
            }

            LsoImageAdjustParam nativeParam = adjustParam.ToNativeParam();
            if (LsoImageAdjust.IsNeutral(ref nativeParam))
            {
                return;
            }

            int nStride = (int)target.Step();

            // The review cache is built with BayerRG2RGB, so this buffer is RGB - the Agent
            // passes Bgr for the same call.
            ImageAdjustResult eResult = LsoImageAdjust.Adjust(
                target.Data, target.Data,
                target.Width, target.Height, nStride, nStride,
                target.Channels(), (int)target.ElemSize1(),
                LsoChannelOrder.Rgb, ref nativeParam);

            if (eResult != ImageAdjustResult.Ok)
            {
                // Runs on a background task, so leave the un-adjusted image rather than throw.
                System.Diagnostics.Debug.WriteLine(
                    $"Native image adjust failed ({eResult}): {LsoImageAdjust.GetLastError()}");
            }
        }

        public void OpenImageAdjustment()
        {
            if (myGridReview.Visibility != Visibility.Visible)
            {
                MessageBox.Show("Image Adjust is available in Review mode.");
                return;
            }

            if (_reviewView == ReviewViewMode.SubBands || _reviewView == ReviewViewMode.Processed)
            {
                MessageBox.Show("Image Adjust is available for the stitched image or a full frame.");
                return;
            }

            OpenReviewImageAdjustment_();
        }

        private void OpenReviewImageAdjustment_()
        {
            if (reviewImageCount_() <= 0)
            {
                return;
            }

            int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;
            AdjustmentWindow.SetDefaultValues(_reviewView == ReviewViewMode.Stitched
                ? getStitchedAdjustParam_(nImageIndex)
                : GetAdjustParam(nImageIndex));

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
            int nImageIndex = (int)mySliderReviewImageIndex.Value - 1;
            if (nImageIndex < 0)
            {
                return;
            }

            Mat source;
            ImageAdjustParam stored;

            if (_reviewView == ReviewViewMode.Stitched)
            {
                if (nImageIndex >= _stitchedList.Count)
                {
                    return;
                }

                source = _stitchedList[nImageIndex];
                stored = getStitchedAdjustParam_(nImageIndex);
            }
            else
            {
                if (nImageIndex >= _captureImageList.Count)
                {
                    return;
                }

                source = _captureImageList[nImageIndex];
                while (_captureImageAdjustParams.Count <= nImageIndex)
                {
                    _captureImageAdjustParams.Add(CreateAdjustParamFromDefault());
                }

                stored = _captureImageAdjustParams[nImageIndex];
            }

            PersistAdjustPreset_(adjustParam);

            stored = CloneAdjustParam(adjustParam);
            if (_reviewView == ReviewViewMode.Stitched)
            {
                _stitchedAdjustParams[nImageIndex] = stored;
            }
            else
            {
                _captureImageAdjustParams[nImageIndex] = stored;
            }

            renderReviewMat_(source, stored);
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

            if (_captureImageList.Count <= 0)
            {
                return;
            }

            using var dialog = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select the folder to save the review images into",
                UseDescriptionForTitle = true,
                SelectedPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture",
            };
            if (dialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
            {
                return;
            }

            _ = saveAllImages_(dialog.SelectedPath);
        }

        // Saves exactly what the viewport shows: the stitch, one band, or one raw frame.
        private void myBtReviewSaveOne_Click(object sender, RoutedEventArgs e)
        {
            int index = (int)mySliderReviewImageIndex.Value - 1;
            if (index < 0)
            {
                return;
            }

            Mat source;
            string strStem;
            ImageAdjustParam? adjust = null;

            switch (_reviewView)
            {
                case ReviewViewMode.Processed:
                    if (_processedMat == null)
                    {
                        return;
                    }
                    source = _processedMat;
                    strStem = "processed";
                    break;

                case ReviewViewMode.Stitched:
                    if (index >= _stitchedList.Count)
                    {
                        return;
                    }
                    source = _stitchedList[index];
                    adjust = getStitchedAdjustParam_(index);
                    strStem = $"stitched{index + 1}";
                    break;

                case ReviewViewMode.SubBands:
                    {
                        if (_bandList.Count == 0)
                        {
                            return;
                        }

                        int bandCount = _bandList[0].Count;
                        int acq = index / bandCount;
                        int band = index % bandCount;
                        if (acq >= _bandList.Count)
                        {
                            return;
                        }

                        source = _bandList[acq][band];
                        strStem = $"acq{acq + 1}_band{band + 1}";
                    }
                    break;

                default:
                    if (index >= _captureImageList.Count)
                    {
                        return;
                    }
                    source = _captureImageList[index];
                    adjust = GetAdjustParam(index);
                    strStem = $"{index + 1}";
                    break;
            }

            using var dialog = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select the folder to save this image into",
                UseDescriptionForTitle = true,
                SelectedPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture",
            };
            if (dialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
            {
                return;
            }

            if (!Directory.Exists(dialog.SelectedPath))
            {
                Directory.CreateDirectory(dialog.SelectedPath);
            }

            string strFileName = System.IO.Path.Combine(dialog.SelectedPath,
                $"{CapturePresetName}_{DateTime.Now:yyyyMMdd_HHmmss}_{strStem}.png");
            using Mat mat = source.Clone();
            mat.SaveImage(strFileName);
            // Also the image as Review shows it (Image Adjust applied); bands and the processed
            // view carry no adjustment of their own.
            if (adjust != null)
            {
                using Mat adjusted = ApplyImageAdjustments(mat, adjust);
                adjusted.SaveImage(System.IO.Path.ChangeExtension(strFileName, null) + "_adjusted.png");
            }
        }

        #endregion EventHandler - Save
    }
}
