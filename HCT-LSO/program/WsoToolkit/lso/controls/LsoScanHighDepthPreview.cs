using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using OpenCvSharp;
using WsoNativeLib;
using WsoNativeLib.Imaging;
using WsoNativeLib.Presets;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit.controls
{
    public partial class LsoScanImagePreview
    {
        private readonly List<CameraHighDepthData?> _highDepthFrames = new();
        public IReadOnlyList<CameraHighDepthData?> HighDepthFrames => _highDepthFrames;
        // Beside _highDepthFrames, one per frame: what it was captured with and, when a correction
        // ran on it, the sensor frame from before. Saved as <stem>_capture.json / _uncorrected16.tiff.
        private readonly List<(LsoCaptureRecord? Record, byte[]? Uncorrected)> _captureContexts = new();
        private (LsoCaptureRecord? Record, byte[]? Uncorrected) _pendingContext;

        // Called on the same dispatcher turn, right before ReceiveCameraCapture for that frame.
        public void AttachCaptureContext(LsoCaptureRecord? record, byte[]? uncorrected)
            => _pendingContext = (record, uncorrected);

        // Kept in this partial so the preview/layout worker owns its original files.
        public void EnableHighDepthSaving()
        {
            myBtReviewSaveAll.Click -= myBtReviewSaveAll_Click;
            myBtReviewSaveAll.Click += saveHighDepthAll_Click;
            myBtReviewSaveOne.Click -= myBtReviewSaveOne_Click;
            myBtReviewSaveOne.Click += saveHighDepthOne_Click;
        }

        private async void saveHighDepthAll_Click(object sender, RoutedEventArgs e)
        {
            if (!_highDepthFrames.Any(f => f != null) && !IsFafCapture_())
            {
                myBtReviewSaveAll_Click(sender, e);
                return;
            }
            using var dialog = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select the folder to save the review images into",
                UseDescriptionForTitle = true,
                SelectedPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture",
            };
            if (dialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                return;
            string stem = $"{CapturePresetName}_{DateTime.Now:yyyyMMdd_HHmmss}";
            // Snapshot both lists before yielding: a new capture must not change this save.
            Mat[] images = _captureImageList.Select(m => m.Clone()).ToArray();
            CameraHighDepthData?[] high = _highDepthFrames.ToArray();
            var contexts = _captureContexts.ToArray();
            // The DevKit saves the displayed FAF gray and keeps the color frame next to it.
            bool fafGray = IsFafCapture_() && !_showFafColor;
            LsoModeProcessing? fafRule = IsFafCapture_()
                ? LsoImageModes.Processing(CaptureImageMode!.Value, FafOption)
                : null;
            try
            {
                await Task.Run(() =>
                {
                    Directory.CreateDirectory(dialog.SelectedPath);
                    for (int i = 0; i < images.Length; ++i)
                    {
                        string path = Path.Combine(dialog.SelectedPath, $"{stem}_{i + 1}.png");
                        CameraHighDepthData? highFrame = i < high.Length ? high[i] : null;
                        if (fafGray && fafRule != null)
                        {
                            using Mat faf = LsoFafImage.ExtractDisplay8(highFrame, images[i], fafRule);
                            faf.SaveImage(path);
                            string colorRef = Path.ChangeExtension(path, null) + "_ColorRef.png";
                            images[i].SaveImage(colorRef);
                        }
                        else
                        {
                            images[i].SaveImage(path);
                        }
                        highFrame?.SaveTiffCompanions(path);
                        writeCaptureRecord_(path, highFrame, i < contexts.Length ? contexts[i] : default);
                    }
                });
            }
            finally
            {
                foreach (Mat image in images) image.Dispose();
            }
        }

        private void saveHighDepthOne_Click(object sender, RoutedEventArgs e)
        {
            if (!_highDepthFrames.Any(f => f != null) && !IsFafCapture_())
            {
                myBtReviewSaveOne_Click(sender, e);
                return;
            }
            int index = (int)mySliderReviewImageIndex.Value - 1;
            if (index < 0) return;
            Mat source;
            string stem;
            switch (_reviewView)
            {
                case ReviewViewMode.Processed:
                    if (_processedMat == null) return;
                    source = _processedMat;
                    stem = "processed";
                    break;
                case ReviewViewMode.Stitched:
                    if (index >= _stitchedList.Count) return;
                    source = _stitchedList[index];
                    stem = $"stitched{index + 1}";
                    break;
                case ReviewViewMode.SubBands:
                    if (_bandList.Count == 0 || _bandList[0].Count == 0) return;
                    int count = _bandList[0].Count;
                    int acq = index / count;
                    int band = index % count;
                    if (acq >= _bandList.Count) return;
                    source = _bandList[acq][band];
                    stem = $"acq{acq + 1}_band{band + 1}";
                    break;
                default:
                    if (index >= _captureImageList.Count) return;
                    source = _captureImageList[index];
                    stem = $"{index + 1}";
                    break;
            }
            using var dialog = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select the folder to save this image into",
                UseDescriptionForTitle = true,
                SelectedPath = AppDomain.CurrentDomain.BaseDirectory + "_Capture",
            };
            if (dialog.ShowDialog() != System.Windows.Forms.DialogResult.OK) return;
            Directory.CreateDirectory(dialog.SelectedPath);
            string path = Path.Combine(dialog.SelectedPath, $"{CapturePresetName}_{DateTime.Now:yyyyMMdd_HHmmss}_{stem}.png");
            using Mat image = source.Clone();
            bool fafGray = IsFafCapture_() && !_showFafColor
                && _reviewView != ReviewViewMode.Processed;
            if (fafGray && TryExtractFaf8_(index, source, out Mat? faf) && faf != null)
            {
                using (faf)
                {
                    faf.SaveImage(path);
                }

                // The DevKit keeps the color frame next to the extracted FAF image.
                string colorRef = Path.ChangeExtension(path, null) + "_ColorRef.png";
                image.SaveImage(colorRef);
            }
            else
            {
                image.SaveImage(path);

                // The processed view is a FAF mean; keep the color frame it came from.
                if (IsFafCapture_() && _reviewView == ReviewViewMode.Processed
                    && _captureImageList.Count > 0)
                {
                    string colorRef = Path.ChangeExtension(path, null) + "_ColorRef.png";
                    using Mat color = _captureImageList[0].Clone();
                    color.SaveImage(colorRef);
                }
            }

            if (_reviewView == ReviewViewMode.FullFrames)
                saveHighDepthFrame_(path, index);
            else
            {
                // A processed/stitch/band PNG is a display derivative. Export its authentic
                // sensor sources explicitly; never label an 8-bit reconstruction as 12-bit.
                for (int i = 0; i < _highDepthFrames.Count; ++i)
                    saveHighDepthFrame_(Path.ChangeExtension(path, null) + $"_source{i + 1}.png", i);
            }
        }

        public void ReceiveCameraFrame(byte[] data, int width, int height, int channels,
            float quality, int format, int bytesPerPixel)
        {
            if (format is (int)ColorPixelFormat.BayerRG12Packed
                or (int)ColorPixelFormat.BayerRG16 or (int)ColorPixelFormat.Mono16)
            {
                bool mono = format == (int)ColorPixelFormat.Mono16;
                data = new CameraHighDepthData(data, width, height,
                    format == (int)ColorPixelFormat.BayerRG12Packed ? 12 : 16, mono).ToDisplay8();
                format = (int)(mono ? ColorPixelFormat.Mono8 : ColorPixelFormat.BayerRG8);
                bytesPerPixel = 1;
            }
            CallbackLsoScanFrameImage(data, width, height, channels, quality, format, bytesPerPixel);
        }

        public void ReceiveCameraCapture(byte[] data, int width, int height, int frameCount,
            int totalFrameCount, int channels, float quality, int format, int bytesPerPixel)
        {
            // The review list is the lifecycle authority (ClearReviewImages may run directly).
            if (_captureImageList.Count == 0)
            {
                _highDepthFrames.Clear();
                _captureContexts.Clear();
            }
            CameraHighDepthData? high = null;
            if (format is (int)ColorPixelFormat.BayerRG12Packed
                or (int)ColorPixelFormat.BayerRG16 or (int)ColorPixelFormat.Mono16)
            {
                bool mono = format == (int)ColorPixelFormat.Mono16;
                high = new CameraHighDepthData(data, width, height,
                    format == (int)ColorPixelFormat.BayerRG12Packed ? 12 : 16, mono);
                data = high.ToDisplay8();
                format = (int)(mono ? ColorPixelFormat.Mono8 : ColorPixelFormat.BayerRG8);
                bytesPerPixel = 1;
            }
            _highDepthFrames.Add(high);
            _captureContexts.Add(_pendingContext);
            _pendingContext = default;
            CallbackLsoScanCaptureFrameImage(data, width, height, frameCount, totalFrameCount,
                channels, quality, format, bytesPerPixel);
        }

        private void saveHighDepthFrame_(string pngPath, int frameIndex)
        {
            if (frameIndex >= 0 && frameIndex < _highDepthFrames.Count)
            {
                _highDepthFrames[frameIndex]?.SaveTiffCompanions(pngPath);
                writeCaptureRecord_(pngPath, _highDepthFrames[frameIndex],
                    frameIndex < _captureContexts.Count ? _captureContexts[frameIndex] : default);
            }
        }

        private static void writeCaptureRecord_(string pngPath, CameraHighDepthData? frame,
            (LsoCaptureRecord? Record, byte[]? Uncorrected) context)
        {
            if (frame != null && context.Record != null)
                LsoCaptureRecord.Write(pngPath, context.Record, frame, context.Uncorrected);
        }
    }
}
