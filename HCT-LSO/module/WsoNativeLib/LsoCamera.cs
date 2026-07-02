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
    using static WsoNativeLib.WsoCallback;

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
        private static extern void setColorCameraSettingParam(ref LsoColorCameraSettingParam param);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///
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

        public static void SetCameraParameters(ref LsoColorCameraSettingParam param)
        {
            setColorCameraSettingParam(ref param);
            return;
        }

        public static void GetCameraParameters(out LsoColorCameraSettingParam param)
        {
            getColorCameraSettingParam(out param);
            return;
        }
    }
}
