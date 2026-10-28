using System;
using System.Threading.Tasks;
using System.Windows;
using WsoNativeLib.Imaging;
using WsoNativeLib.Presets;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoDomain;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit
{
    // Draft postprocessing entry points of the scan-test screen: the button that opens the review
    // window, and the run that puts the processed image on screen after a capture. The capture path
    // itself is untouched; the frame it hands over is the one the review sees.
    public partial class LsoScanTestWindow
    {
        private LsoPostProcessingWindow? _postProcessingWindow;

        private void myBtPostProcess_Click(object sender, RoutedEventArgs e)
        {
            if (_postProcessingWindow == null)
            {
                _postProcessingWindow = new LsoPostProcessingWindow { Owner = this };
                _postProcessingWindow.Closed += (_, _) => _postProcessingWindow = null;
            }
            _postProcessingWindow.Show();
            _postProcessingWindow.Activate();
        }

        // Runs on the UI continuation of the capture completion, after processCapturedFrames_.
        // A successful single-frame Color capture is processed with the review window's settings
        // (or the defaults) and the final image replaces the review - the operator sees what the
        // pipeline makes of the frame, not the raw sweep.
        private void postProcessAfterCapture_(ColorCaptureStage stage, int framesAcquired)
        {
            if (stage != ColorCaptureStage.Ok) return;
            if (_scanTestModel.ModePresets.CaptureImageMode != LsoImageMode.Color || framesAcquired != 1)
            {
                System.Console.WriteLine($"[LSO-POST] not processed: mode={_scanTestModel.ModePresets.CaptureImageMode} "
                    + $"frames={framesAcquired} (Color, single frame only)");
                return;
            }
            _ = processCaptureForReviewAsync_();
        }

        private async Task processCaptureForReviewAsync_()
        {
            PostProcessInput? input = LsoPostProcessingWindow.BuildLatestInput();
            if (input == null)
            {
                System.Console.WriteLine("[LSO-POST] not processed: no Bayer frame kept for the last capture.");
                return;
            }

            LsoPostProcessingWindow? window = _postProcessingWindow;
            PostProcessOptions options = window?.CurrentOptions ?? new PostProcessOptions();
            PostProcessResult result;
            try
            {
                result = await Task.Run(() => LsoFundusPostProcessor.Run(input, options));
            }
            catch (Exception ex)
            {
                System.Console.WriteLine($"[LSO-POST] failed: {ex.Message}");
                return;
            }

            myColorPreview.Image.SetProcessedColor(result.Final.Display,
                $"Post processed ({result.Stages.Count} stages)");

            // The window owns the result when it asked for it; otherwise it is disposed here.
            if (window != null && window.IsVisible)
            {
                window.AdoptInput(input);
                if (window.ProcessAfterCapture)
                {
                    window.ShowExternalResult(result, input);
                    return;
                }
            }
            result.Dispose();
        }
    }
}
