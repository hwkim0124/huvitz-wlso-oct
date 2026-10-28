using System;
using System.Windows.Controls;
using WsoNativeLib;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoToolkit.controls
{
    /// <summary>Mono aiming-preview drive mode.</summary>
    public enum ColorPreviewMonoMode : int
    {
        /// <summary>Camera Mono8 + binning 2x2. Half pixel resolution (Mono8 forces binning>=2).
        /// PlayPreview then crops the ROI to a centered square.</summary>
        SensorMono8 = 0,
        /// <summary>Camera BayerRG8 + binning 1x1 (full resolution), grayscale applied only in the
        /// live display. PlayPreview then crops the ROI to a centered square.</summary>
        BayerSwGray = 1,
    }

    /// <summary>
    /// Active preview control for the color (fundus) camera.
    ///
    /// - Live(aiming): shows a mono aiming image using the IR LED + IR scanner pattern.
    ///   Mono drive mode is selected via <see cref="ColorPreviewMonoMode"/>.
    /// - Capture(Bayer color) reuses the host's existing path. White LED is not driven
    ///   in software (left to scan-pattern gating / manual control).
    /// - Rendering / review / image-adjust / save are delegated to the inner
    ///   <see cref="LsoScanImagePreview"/> (composition). Custom-typed members are
    ///   reached through the <see cref="Image"/> property.
    /// </summary>
    public partial class ColorCameraPreview : UserControl
    {
        private bool _aiming = false;

        public ColorCameraPreview()
        {
            InitializeComponent();
            myImagePreview.EnableHighDepthSaving();

            // Clear the inner preview's fixed size so it fills this control.
            myImagePreview.Width = double.NaN;
            myImagePreview.Height = double.NaN;
        }

        /// <summary>Inner control exposing render/review/adjust features.</summary>
        public LsoScanImagePreview Image => myImagePreview;

        /// <summary>Live(aiming) frame callback. Assigned by the host, which keeps the
        /// original delegate in a long-lived field to prevent GC.</summary>
        public WsoCallback.ColorCameraFrameCaptured? LiveCallback { get; set; } = null;

        /// <summary>Whether an aiming session is active. Returns true whenever the session
        /// is open (IR LED on + params changed) even if the camera live start silently
        /// failed, so the stop path (StopPreview) is always reachable.</summary>
        public bool IsPreviewing()
        {
            return _aiming;
        }

        /// <summary>
        /// Start the mono aiming live. The native one-call owns the full sequence (save params ->
        /// Mono8/Bayer + full FOV -> centered-square ROI -> IR LED -> IR scanner pattern -> live);
        /// this control only selects display-side grayscale and provides the frame callback.
        /// </summary>
        public void PlayPreview(ColorPreviewMonoMode mode, LsoFundusPreviewRequest request)
        {
            var callback = LiveCallback;
            if (callback == null)
            {
                return;
            }
            if (_aiming || LsoCamera.IsLiveMode())
            {
                return;
            }

            // Display-side grayscale only for BayerSwGray (Mono8 already delivers gray).
            myImagePreview.IsForceGrayLiveDisplay = (mode == ColorPreviewMonoMode.BayerSwGray);

            myImagePreview.SetLiveMode();

            // Single source of truth for the orchestration - do not duplicate the
            // param/ROI/profile/LED/scanner sequence here, and in particular do not start
            // the scanner as well: the one-call already does, and doing both sends the
            // board an extra off/on pair per transition.
            if (!LsoCamera.StartFundusPreview(ref request, callback))
            {
                // Nothing was started (params/profile restored by the callee) - do not enter the
                // aiming state, or a silent failure looks exactly like a dark, non-sweeping preview.
                myImagePreview.StopTimer();
                System.Windows.MessageBox.Show($"Preview start failed: {LsoCamera.GetLastCaptureError()}");
                return;
            }

            myImagePreview.StartTimer();
            _aiming = true;
        }

        /// <summary>Stop the mono aiming live. The native one-call pauses live + the scan
        /// pattern, applies the request's restore illumination, and restores camera params.</summary>
        public void StopPreview()
        {
            if (!_aiming)
            {
                return;
            }

            LsoCamera.StopFundusPreview();
            myImagePreview.StopTimer();
            myImagePreview.IsForceGrayLiveDisplay = false;
            _aiming = false;
        }

        // ---- Forwarding to the inner LsoScanImagePreview (keeps host call sites unchanged) ----

        public void CloseAdjustmentWindow() => myImagePreview.CloseAdjustmentWindow();
        public void ClearReviewImages() => myImagePreview.ClearReviewImages();
        public void OpenImageAdjustment() => myImagePreview.OpenImageAdjustment();
        public void SetLiveMode() => myImagePreview.SetLiveMode();
        public void StartTimer() => myImagePreview.StartTimer();
        public void StopTimer() => myImagePreview.StopTimer();
        public void UpdateFramerate() => myImagePreview.UpdateFramerate();

        public void CallbackLsoScanFrameImage(byte[] data, int width, int height, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
            => myImagePreview.ReceiveCameraFrame(data, width, height, channels, quality, nPixelFormat, nBytesPerPixel);

        public void CallbackLsoScanCaptureFrameImage(byte[] data, int width, int height, int frameCount, int totalFrameCount, int channels, float quality, int nPixelFormat, int nBytesPerPixel)
            => myImagePreview.ReceiveCameraCapture(data, width, height, frameCount, totalFrameCount, channels, quality, nPixelFormat, nBytesPerPixel);

        public bool IsForceGrayLiveDisplay
        {
            get => myImagePreview.IsForceGrayLiveDisplay;
            set => myImagePreview.IsForceGrayLiveDisplay = value;
        }

        public bool IsOverlayAlignGuide
        {
            get => myImagePreview.IsOverlayAlignGuide;
            set => myImagePreview.IsOverlayAlignGuide = value;
        }

        public bool IsReviewMode
        {
            get => myImagePreview.IsReviewMode;
            set => myImagePreview.IsReviewMode = value;
        }

        public int AcqFrameCount
        {
            get => myImagePreview.AcqFrameCount;
            set => myImagePreview.AcqFrameCount = value;
        }

        public int SubFrameCount
        {
            get => myImagePreview.SubFrameCount;
            set => myImagePreview.SubFrameCount = value;
        }
    }
}
