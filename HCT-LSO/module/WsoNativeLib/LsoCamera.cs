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
        private static extern bool setColorCameraExtendedParam(ref LsoColorCameraExtendedParam param, ref LsoColorCameraApplyResult result);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setColorCameraFullSettingParam(ref LsoColorCameraSettingParam baseParam, ref LsoColorCameraExtendedParam ext, ref LsoColorCameraApplyResult result);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getColorCameraExtendedParam(ref LsoColorCameraExtendedParam param);

        [DllImport(LibraryName)]
        private static extern int getColorCameraEnumEntries([MarshalAs(UnmanagedType.LPStr)] string nodeName,
            [Out] LsoColorCameraEnumValue[] buffer, int maxCount);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getColorCameraNodeRange([MarshalAs(UnmanagedType.LPStr)] string nodeName,
            out double minValue, out double maxValue, out double increment,
            [MarshalAs(UnmanagedType.I1)] out bool writable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getColorCameraNodeValue([MarshalAs(UnmanagedType.LPStr)] string nodeName,
            out double value);

        [DllImport(LibraryName)]
        private static extern int validateLsoCameraStartContract(ref LsoColorCameraExtendedParam param,
            [MarshalAs(UnmanagedType.I1)] bool hwTriggered);

        [DllImport(LibraryName)]
        private static extern int describeLsoCameraStartContract(int contract, StringBuilder buf, int len);

        [DllImport(LibraryName)]
        private static extern uint getLsoCameraStructSize(int which);

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
        private static extern int startColorCameraColorCaptureEx(ref LsoColorCameraSettingParam param, int scanPatternId, int triggerMode, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

        [DllImport(LibraryName)]
        private static extern int startColorCameraColorCaptureComposed(ref LsoColorCameraSettingParam param, int scanPatternId, int subFrameCount, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

        [DllImport(LibraryName)]
        private static extern int startColorCameraColorCaptureComposedEx(ref LsoColorCameraSettingParam param, int scanPatternId, int subFrameCount, int triggerMode, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopColorCameraColorCapture();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isColorCameraCapturing();

        [DllImport(LibraryName)]
        private static extern int getLastColorCaptureError(StringBuilder buf, int len);

        [DllImport(LibraryName)]
        private static extern int startLsoFundusCapture(ref LsoFundusCaptureRequest request, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startLsoFundusPreview(ref LsoFundusPreviewRequest request, ColorCameraFrameCaptured cb);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopLsoFundusPreview();

        [DllImport(LibraryName)]
        private static extern int validateLsoFundusCaptureRequest(ref LsoFundusCaptureRequest request, out uint totalFrameCount);

        [DllImport(LibraryName)]
        private static extern int validateLsoFundusPreviewRequest(ref LsoFundusPreviewRequest request);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setLsoCaptureBarrier(int stepId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool waitLsoCaptureBarrier(int timeoutMs);

        [DllImport(LibraryName)]
        private static extern void releaseLsoCaptureBarrier();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool waitLsoCaptureCancelLatched(int timeoutMs);


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

        // --- A-1 extended camera configuration ------------------------------------------
        // Values are GenApi symbolic names; an empty name (or a negative number) means the
        // caller does not state that node and it is left as the device has it.
        //
        // True only when every STATED field was written AND read back equal - the native
        // side never treats a setter that did not throw as success. `result` comes back
        // with the per-field requested/actual/status table, which is what an editor shows
        // instead of a bare "failed".
        public static bool SetExtendedParameters(ref LsoColorCameraExtendedParam param,
            out LsoColorCameraApplyResult result)
        {
            result = LsoColorCameraApplyResult.Create();
            return setColorCameraExtendedParam(ref param, ref result);
        }

        // The whole camera contract in node-dependency order (TriggerMode Off -> sensor and
        // binning -> the 14 structural fields -> GPIO -> trigger -> TriggerMode). True only
        // when the structural verify AND every stated extended field succeeded, which is
        // the answer the Toolkit saves a preset on.
        public static bool SetFullCameraParameters(ref LsoColorCameraSettingParam baseParam,
            ref LsoColorCameraExtendedParam ext, out LsoColorCameraApplyResult result)
        {
            result = LsoColorCameraApplyResult.Create();
            return setColorCameraFullSettingParam(ref baseParam, ref ext, ref result);
        }

        // Device read-back of the same nodes. Nodes that cannot be read come back unstated,
        // so a snapshot never invents a value. False with no camera attached.
        public static bool GetExtendedParameters(out LsoColorCameraExtendedParam param)
        {
            param = LsoColorCameraExtendedParam.Create();
            return getColorCameraExtendedParam(ref param);
        }

        // Capability: the symbolic entries this camera actually implements for an enum
        // node. Empty with no camera attached - the caller then has to say so rather than
        // present a hardcoded list as if the device had confirmed it.
        public static IReadOnlyList<string> GetEnumEntries(string nodeName)
        {
            if (string.IsNullOrEmpty(nodeName))
            {
                return Array.Empty<string>();
            }

            var buffer = new LsoColorCameraEnumValue[64];
            int count = getColorCameraEnumEntries(nodeName, buffer, buffer.Length);
            if (count <= 0)
            {
                return Array.Empty<string>();
            }

            var names = new List<string>(count);
            for (int i = 0; i < count; ++i)
            {
                names.Add(buffer[i].name ?? string.Empty);
            }
            return names;
        }

        // Capability: range and increment of an integer or float node. False when the node
        // is absent on this camera.
        public static bool GetNodeRange(string nodeName, out double minValue, out double maxValue,
            out double increment, out bool writable)
        {
            minValue = 0.0;
            maxValue = 0.0;
            increment = 0.0;
            writable = false;
            if (string.IsNullOrEmpty(nodeName))
            {
                return false;
            }
            return getColorCameraNodeRange(nodeName, out minValue, out maxValue, out increment, out writable);
        }

        // Current value of an integer or float node - what the camera holds right now, which
        // GetNodeRange cannot say. False when the camera is not connected or the node is not
        // implemented; `value` is 0 then, never a stale reading.
        public static bool GetNodeValue(string nodeName, out double value)
        {
            value = 0.0;
            if (string.IsNullOrEmpty(nodeName))
            {
                return false;
            }
            return getColorCameraNodeValue(nodeName, out value);
        }

        // Device-free: whether a stated configuration can drive an acquisition on this
        // instrument's wiring. Ok means nothing conflicts; anything else names the field,
        // so the caller can refuse the start BEFORE anything is written instead of quietly
        // replacing the operator's value with a constant.
        public static CameraStartContract ValidateStartContract(ref LsoColorCameraExtendedParam param, bool hwTriggered)
        {
            return (CameraStartContract)validateLsoCameraStartContract(ref param, hwTriggered);
        }

        public static string DescribeStartContract(CameraStartContract contract)
        {
            var buf = new StringBuilder(256);
            describeLsoCameraStartContract((int)contract, buf, buf.Capacity);
            return buf.ToString();
        }

        // Native sizeof() of an ABI struct, for a mirror check against the shipped DLL.
        public static uint GetNativeStructSize(LsoCameraStructId which)
        {
            return getLsoCameraStructSize((int)which);
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

        public static ColorCaptureStage StartColorCapture(ref LsoColorCameraSettingParam param,
            ColorCaptureTriggerMode triggerMode, ColorCameraImageCaptured frameCallback,
            ColorCaptureCompleted completedCallback, int scanPatternId = (int)LsoScannerPatternId.COLOR)
        {
            return (ColorCaptureStage)startColorCameraColorCaptureEx(ref param, scanPatternId,
                (int)triggerMode, frameCallback, completedCallback);
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

        public static ColorCaptureStage StartColorCaptureComposed(ref LsoColorCameraSettingParam param,
            ColorCaptureTriggerMode triggerMode, ColorCameraImageCaptured composedCallback,
            ColorCaptureCompleted completedCallback, int subFrameCount = 0,
            int scanPatternId = (int)LsoScannerPatternId.COLOR)
        {
            return (ColorCaptureStage)startColorCameraColorCaptureComposedEx(ref param, scanPatternId,
                subFrameCount, (int)triggerMode, composedCallback, completedCallback);
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


        // One-call fundus capture: native owns the whole sequence - request validation ->
        // board-global scan sequence -> camera preset (HW: continuous, Normal: mode/count
        // derived from the profile) -> measured exposure into the profile -> profile upload ->
        // capture light mode/intensity/ON -> camera
        // begin -> HW arm wait -> board capture on -> bounded acquisition -> teardown of
        // exactly what was acquired, restore light mode/intensity/ON -> completedCallback.
        // The caller must NOT start/pause the scanner, set the lights or upload the profile
        // itself; doing any of that again double-drives the board.
        // Ok means the worker started and completion is reported exactly once; any other
        // stage means nothing was armed and no completion callback follows.
        // Keep BOTH delegates alive in long-lived fields (P/Invoke GC rule).
        public static ColorCaptureStage StartFundusCapture(ref LsoFundusCaptureRequest request,
            ColorCameraImageCaptured frameCallback, ColorCaptureCompleted completedCallback)
        {
            return (ColorCaptureStage)startLsoFundusCapture(ref request, frameCallback, completedCallback);
        }

        // One-call fundus (aiming) preview, hardware-triggered like the DevKit preview path:
        // scan sequence -> preset camera params + the request's own exposure/gain -> profile upload ->
        // preview light mode/intensity/ON -> frame callback -> camera trigger setup (FrameStart, Line3,
        // rising edge, ReadOut, trigger delay) + acquisition begin + bounded arm wait ->
        // board capture on. The board is started LAST, once the camera is armed.
        // A failure at any step rolls back exactly what was acquired, in reverse, so false
        // means nothing is running. Stop reverses the same ledger and applies the restore
        // illumination. The caller must not start/pause the scanner itself.
        // Keep 'cb' alive in a long-lived field.
        public static bool StartFundusPreview(ref LsoFundusPreviewRequest request, ColorCameraFrameCaptured cb)
        {
            return startLsoFundusPreview(ref request, cb);
        }

        public static bool StopFundusPreview()
        {
            return stopLsoFundusPreview();
        }

        // Request gates only - no device access, so these answer with an instrument absent.
        public static CaptureValidationReason ValidateFundusCaptureRequest(ref LsoFundusCaptureRequest request, out uint totalFrameCount)
        {
            return (CaptureValidationReason)validateLsoFundusCaptureRequest(ref request, out totalFrameCount);
        }

        public static CaptureValidationReason ValidateFundusPreviewRequest(ref LsoFundusPreviewRequest request)
        {
            return (CaptureValidationReason)validateLsoFundusPreviewRequest(ref request);
        }

        // Test-only race seam. Arm a step BEFORE starting the capture, wait for the worker
        // to park there, then drive the case under test (e.g. call StopColorCapture from a
        // dedicated thread) and release. WaitCaptureCancelLatched is the only supported way
        // to observe that the sequence saw the stop.
        public static bool SetCaptureBarrier(LsoCaptureStep step)
        {
            return setLsoCaptureBarrier((int)step);
        }

        public static bool WaitCaptureBarrier(int timeoutMs)
        {
            return waitLsoCaptureBarrier(timeoutMs);
        }

        public static void ReleaseCaptureBarrier()
        {
            releaseLsoCaptureBarrier();
        }

        public static bool WaitCaptureCancelLatched(int timeoutMs)
        {
            return waitLsoCaptureCancelLatched(timeoutMs);
        }
    }
}
