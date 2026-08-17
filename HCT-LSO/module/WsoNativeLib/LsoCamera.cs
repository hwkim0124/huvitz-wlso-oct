using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace WsoNativeLib
{
    using static LibraryConfig;
    using static WsoCallback;
    using static WsoLsoDefs;
    using static WsoLsoScan;

    public static class LsoCamera
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        private static extern void startColorCameraLive(ColorCameraFrameCaptured clbFrame);

        [DllImport(LibraryName)]
        private static extern void pauseColorCameraLive();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isColorCameraLive();

        [DllImport(LibraryName)]
        private static extern void startColorCameraSingleFrameLive(ColorCameraFrameCaptured clbFrame);

        [DllImport(LibraryName)]
        private static extern void startColorCameraSeqROILive(ColorCameraSeqROIFrameCaptured clbFrame, ref LsoCaptureFrameSeqROIPreset preset);

        [DllImport(LibraryName)]
        private static extern void startColorCameraSwTriggerOffsetROILive(ColorCameraOffsetROIFrameCaptured clbFrame, ref LsoCaptureFrameOffsetROIPreset preset);

        [DllImport(LibraryName)]
        private static extern void startColorCameraSwTriggerOffsetROICapture(ColorCameraOffsetROIFrameCaptured clbFrame, ref LsoCaptureFrameOffsetROIPreset preset);

        [DllImport(LibraryName)]
        private static extern void startColorCameraRollSwTrigOverlapLive(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, ref LsoCaptureFrameRollSwTrigOverlapPreset preset);

        [DllImport(LibraryName)]
        private static extern void startColorCameraRollSwTrigOverlapCapture(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, ref LsoCaptureFrameRollSwTrigOverlapPreset preset);

        [DllImport(LibraryName)]
        private static extern void startColorCameraSwTriggerLive(ColorCameraFrameCaptured clbFrame);

        [DllImport(LibraryName)]
        private static extern void stopColorCameraSwTriggerLive();

        [DllImport(LibraryName)]
        private static extern void shootColorCameraSwTriggerLive();

        [DllImport(LibraryName)]
        private static extern void startColorCameraHwTriggerLive(ColorCameraImageCaptured clbFrame);

        [DllImport(LibraryName)]
        private static extern void stopColorCameraHwTriggerLive();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isColorCameraIsStreaming();

        [DllImport(LibraryName)]
        private static extern void setupColorCameraHWTriggerLive();

        [DllImport(LibraryName)]
        private static extern void startColorCameraOriginal(ColorCameraImageCaptured clbImage);

        [DllImport(LibraryName)]
        private static extern void pauseColorCameraOriginal();

        [DllImport(LibraryName)]
        private static extern void getColorCameraSettingParam(out LsoColorCameraSettingParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setColorCameraSettingParam(ref LsoColorCameraSettingParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool squareColorCameraRoi();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startColorCameraMonoPreview(int mode, ColorCameraFrameCaptured cb);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopColorCameraMonoPreview();

        [DllImport(LibraryName)]
        private static extern int startColorCameraColorCapture(ref LsoColorCameraSettingParam param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

        [DllImport(LibraryName)]
        private static extern int startColorCameraColorCaptureComposed(ref LsoColorCameraSettingParam param, int scanPatternId, int subFrameCount, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopColorCameraColorCapture();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isColorCameraCapturing();

        [DllImport(LibraryName)]
        private static extern int getLastColorCaptureError(StringBuilder buf, int len);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static void StartLiveMode(ColorCameraFrameCaptured clbFrame)
        {
            startColorCameraLive(clbFrame);
            return;
        }

        public static void PauseLiveMode()
        {
            pauseColorCameraLive();
            return;
        }

        public static bool IsLiveMode()
        {
            return isColorCameraLive();
        }


        public static void StartSingleFrameLiveMode(ColorCameraFrameCaptured clbFrame)
        {
            startColorCameraSingleFrameLive(clbFrame);
            return;
        }

        public static void StartSeqROILiveMode(ColorCameraSeqROIFrameCaptured clbFrame, ref LsoCaptureFrameSeqROIPreset preset)
        {
            startColorCameraSeqROILive(clbFrame, ref preset);
            return;
        }

        public static void StartSwTriggerOffsetROILiveMode(ColorCameraOffsetROIFrameCaptured clbFrame, ref LsoCaptureFrameOffsetROIPreset preset)
        {
            startColorCameraSwTriggerOffsetROILive(clbFrame, ref preset);
            return;
        }

        // Global Shutter, SW Trigger (Auto), Capture
        public static void StartColorCameraCaptureSwTriggerOffsetROI(ColorCameraOffsetROIFrameCaptured clbFrame, ref LsoCaptureFrameOffsetROIPreset preset)
        {
            startColorCameraSwTriggerOffsetROICapture(clbFrame, ref preset);
            return;
        }

        public static void StartRollSwTrigOverlapLiveMode(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, ref LsoCaptureFrameRollSwTrigOverlapPreset preset)
        {
            startColorCameraRollSwTrigOverlapLive(clbFrame, ref preset);
            return;
        }

        public static void StartRollSwTrigOverlapCaptureMode(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, ref LsoCaptureFrameRollSwTrigOverlapPreset preset)
        {
            startColorCameraRollSwTrigOverlapCapture(clbFrame, ref preset);
            return;
        }

        public static void StartColorCameraSwTriggerMode(ColorCameraFrameCaptured clbFrame)
        {
            startColorCameraSwTriggerLive(clbFrame);
            return;
        }

        public static void StopColorCameraSwTriggerLive()
        {
            stopColorCameraSwTriggerLive();
            return;
        }

        public static void ShootColorCameraSwTriggerLive()
        {
            shootColorCameraSwTriggerLive();
            return;
        }

        public static void StartColorCameraHwTriggerLive(ColorCameraImageCaptured clbFrame)
        {
            startColorCameraHwTriggerLive(clbFrame);
            return;
        }

        public static void StopColorCameraHwTriggerLive()
        {
            stopColorCameraHwTriggerLive();
            return;
        }

        public static bool IsColorCameraStreaming()
        {
            return isColorCameraIsStreaming();
        }

        public static void StartOriginalMode(ColorCameraImageCaptured clbImage)
        {
            startColorCameraOriginal(clbImage);
            return;
        }

        public static void PauseOriginalMode()
        {
            pauseColorCameraLive();
            return;
        }

        // Applies camera settings. While a live/capture session is active, structural fields
        // are camera-locked: tunables (exposure/gain) still apply, structural fields are
        // ignored and false is returned. Stop the session before structural changes.
        public static bool SetCameraParameters(ref LsoColorCameraSettingParam param)
        {
            return setColorCameraSettingParam(ref param);
        }

        public static void GetCameraParameters(out LsoColorCameraSettingParam param)
        {
            getColorCameraSettingParam(out param);
            return;
        }

        // Crops the current ROI to a centered square (side = min(width, height)).
        // Applied while acquisition is stopped; returns false if the camera is absent.
        public static bool SquareRoi()
        {
            return squareColorCameraRoi();
        }

        // One-call mono square preview: native sets params (Mono8/Bayer) -> square ROI -> IR LED ->
        // scanner IR pattern -> live. Caller keeps 'cb' alive in a long-lived field (GC).
        public static bool StartColorMonoPreview(ColorMonoPreviewMode mode, ColorCameraFrameCaptured cb)
        {
            return startColorCameraMonoPreview((int)mode, cb);
        }

        // Stops the mono square preview: live pause + IR off + scanner pause + restore saved params.
        public static bool StopColorMonoPreview()
        {
            return stopColorCameraMonoPreview();
        }

        // One-call color capture: native runs IR off -> preset apply(checked) -> scanner grab
        // -> worker acquisition -> scanner pause -> completedCallback(result, framesAcquired).
        // Ok means the worker started; completion is reported exactly once via the callback.
        // Keep BOTH delegates alive in long-lived fields (P/Invoke GC rule).
        public static ColorCaptureStage StartColorCapture(ref LsoColorCameraSettingParam param,
            ColorCameraImageCaptured frameCallback, ColorCaptureCompleted completedCallback,
            int scanPatternId = (int)LsoScannerPatternId.COLOR)
        {
            return (ColorCaptureStage)startColorCameraColorCapture(ref param, scanPatternId, frameCallback, completedCallback);
        }

        // One-call color capture with native subframe composition: the consumer receives ONE
        // composed image (frameCount=0, totalFrameCount=1) just before the completion callback,
        // and only when the capture completed every band - a cancelled or failed capture reports
        // through the completion callback alone.
        // subFrameCount <= 0 derives the band count from the acquired frame count.
        // Keep BOTH delegates alive in long-lived fields (P/Invoke GC rule).
        public static ColorCaptureStage StartColorCaptureComposed(ref LsoColorCameraSettingParam param,
            ColorCameraImageCaptured composedCallback, ColorCaptureCompleted completedCallback,
            int subFrameCount = 0, int scanPatternId = (int)LsoScannerPatternId.COLOR)
        {
            return (ColorCaptureStage)startColorCameraColorCaptureComposed(ref param, scanPatternId, subFrameCount, composedCallback, completedCallback);
        }

        // Cancels a running capture and joins the native worker (partial frames are reported
        // to the completion callback as Cancelled). Never call this from the frame/completed
        // callback itself.
        public static bool StopColorCapture()
        {
            return stopColorCameraColorCapture();
        }

        public static bool IsCapturing()
        {
            return isColorCameraCapturing();
        }

        public static string GetLastCaptureError()
        {
            var buf = new StringBuilder(256);
            getLastColorCaptureError(buf, buf.Capacity);
            return buf.ToString();
        }
    }
}
