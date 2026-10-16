using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace WsoNativeLib
{
    using static WsoNativeLib.WsoDomain;
    using static WsoNativeLib.WsoLsoScan;

    public static class WsoLsoDefs
    {
        // Enum Definitions
        /////////////////////////////////////////////////////////////////////////////////////////////
        public enum LsoScannerPatternId : int
        {
            COLOR = 0, 
            IR = 1,
        }

        public enum LsoScannerTriggerMode : int
        {
            RollingShutter = 0,
            GlobalShutter = 1,
        }

        public enum ColorPixelFormat : int
        {
            Mono8 = 0,      // 8 bit
            Mono16 = 1,     // 16 bit
            RGB8Packed = 2, // 24 bit
            BayerRG8 = 4,   // 8 bit
            BayerRG16 = 8,  // 16 bit
            BGR8 = 27,       // 24 bit
            UNKNOWN = -1
        }
        public enum ColorAdcBitDepth : int
        {
            Bit10 = 1,      // 10 bit
            Bit12 = 2,      // 12 bit
            UNKNOWN = -1
        }

        // Mono aiming-preview drive mode for the color camera one-call preview API.
        public enum ColorMonoPreviewMode : int
        {
            SensorMono8 = 0,  // camera Mono8 + binning 2x2 (half pixel resolution)
            BayerSwGray = 1,  // camera BayerRG8 + binning 1x1 (full resolution), grayscale at display side
        }

        public enum ColorCaptureTriggerMode : int
        {
            Normal = 0,
            HWTrigger = 1,
        }

        // Result/stage codes for the color capture one-call (mirror of native ColorCaptureStage).
        // Start return: Ok/Busy/Param/Scanner (no completion callback follows a non-Ok start).
        // Completion callback: Ok/Camera/Cancelled/Fault (how the acquisition ended).
        public enum ColorCaptureStage : int
        {
            Ok = 0,
            Busy = 1,
            Param = 2,
            Scanner = 3,
            Camera = 4,
            Cancelled = 5,
            Fault = 9,
        }

        // Steps of the fundus capture sequence, in execution order (mirror of native
        // LsoCaptureStep). Also the step id the diagnostic barrier takes.
        public enum LsoCaptureStep : int
        {
            None = -1,
            Validate = 0,
            ApplyCamera = 1,
            UploadProfile = 2,
            CaptureLight = 3,
            BeginCamera = 4,
            WaitArm = 5,
            StartScanner = 6,
            Acquire = 7,
            Teardown = 8,
        }

        // Which request gate rejected a fundus request (mirror of native
        // CaptureValidationReason). Returned by the validate-only entry points.
        public enum CaptureValidationReason : int
        {
            Valid = 0,
            Null = 1,
            StructSize = 2,
            TriggerMode = 3,
            MonoMode = 4,
            PatternId = 5,
            SampleSizeZero = 6,
            AcqFrameSizeZero = 7,
            SubFrameSizeZero = 8,
            FrameCountRange = 9,
            SubFrameCompose = 10,
            CaptureLight = 11,
            RestoreLight = 12,
            PreviewAcqMode = 13,
            PreviewExposure = 14,
            CameraConfig = 15,
            CameraContract = 16,
            SequenceMode = 17,
            WorkingLightMode = 18,
            RestoreLightMode = 19,
        }

        // What happened to one A-1 camera node (mirror of native ColorCameraFieldStatus).
        // The four failure states are distinct because they need different answers from the
        // operator: a node this camera does not have is not the same problem as a value the
        // device quietly changed.
        public enum ColorCameraFieldStatus : int
        {
            NotRequested = 0,
            Applied = 1,
            Unsupported = 2,
            Locked = 3,
            WriteFailed = 4,
            Mismatch = 5,
            // Written successfully, and the device chose a nearby legal value because the
            // request was outside its range or off its increment. Reported with the
            // requested and actual values, but not treated as a failure - refusing a start
            // over a benign quantisation would break a configuration the camera can run.
            Adjusted = 6,
        }

        // Why a Preview/Capture start refuses a stated camera configuration (mirror of
        // native CameraStartContract). Apply is NOT gated by these: tuning may write any
        // value the device accepts, and only starting an acquisition needs the board's
        // trigger contract to hold.
        public enum CameraStartContract : int
        {
            Ok = 0,
            StructSize = 1,
            Version = 2,
            TriggerMode = 3,
            TriggerSource = 4,
            TriggerActivation = 5,
            TriggerSelector = 6,
            TriggerOverlap = 7,
            TriggerLineMode = 8,
            ExposureAuto = 9,
            BurstCount = 10,
            LineSelector = 11,
            LineName = 12,
            // The trigger group is all-or-nothing: a preset either owns TriggerSource,
            // TriggerSelector, TriggerActivation and TriggerOverlap together - and then the
            // start writes none of them - or states none of them, and the start's own
            // hardware trigger setup owns all of them. Stating only part would let the
            // constants silently fill the rest back in.
            TriggerGroup = 13,
        }

        // Which ABI struct getLsoCameraStructSize() reports (mirror of LsoCameraStructId).
        public enum LsoCameraStructId : int
        {
            CameraSetting = 0,
            CameraExtended = 1,
            CameraLine = 2,
            CameraFieldResult = 3,
            CameraApplyResult = 4,
            FundusCaptureRequest = 5,
            FundusPreviewRequest = 6,
        }

        // The board wiring the hardware-triggered paths depend on - the values
        // setupHwTriggerSetting() has always written. Naming them lets a preset state them
        // explicitly instead of having them replaced after the fact, and lets the editor
        // pre-check a preset with no camera attached.
        public static class ColorCameraHwTrigger
        {
            public const string Mode = "On";
            public const string Source = "Line3";
            public const string Activation = "RisingEdge";
            public const string Selector = "FrameStart";
            public const string Overlap = "ReadOut";
            public const string LineMode = "Input";
            public const int LineIndex = 3;
        }

        // Mirror of native LsoFundusCaptureRequest. Field order and types must match the
        // native struct exactly; structSize is checked by the callee, so a mirror that
        // drifts is rejected instead of read past. Use Create() to fill it.
        // HW trigger forces camera acquisitionMode to continuous but retains its frame count.
        // Normal trigger derives both from profile.acqFrameSize * profile.subFrameSize.
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoFundusCaptureRequest
        {
            public uint structSize;
            public int triggerMode;                 // ColorCaptureTriggerMode
            public int scanPatternId;               // 0 .. LSO_SCANNER_PATTERN_NUM_MAX-1
            public int sequenceMode;                // LsoScanSequenceMode
            public short preMoveOffset;             // GalvanoFirst only; native normalizes otherwise
            public ushort preMoveDelayUs;           // GalvanoFirst only; native normalizes otherwise
            public int captureLight;                // LightType applied for the capture
            public int captureLightMode;            // LightMode applied before turn-on
            public int restoreLight;                // LightType restored on every exit path
            public int restoreLightMode;            // applied before a different restore light turns on
            // 0 = leave the intensity the board already has. GetLightIntensity() reports a
            // cache that starts at 0, so passing that 0 through must not darken the LED.
            public ushort captureLightIntensity;
            public ushort restoreLightIntensity;
            public uint subFrameCompose;            // 0 = per-frame, else == profile.subFrameSize
            public LsoScannerControlParam profile;  // uploaded to the board; owns the frame count
            public LsoColorCameraSettingParam camera;
            // The rest of the A-1 camera configuration. Applied together with `camera` by
            // one checked apply, in node-dependency order, BEFORE the camera is armed - and
            // the nodes it states are not written again afterwards, so a stated trigger or
            // GPIO value survives the start instead of being replaced by the hardcoded
            // trigger setup. Fields it leaves unstated stay owned by that start path.
            public LsoColorCameraExtendedParam cameraExt;

            public static LsoFundusCaptureRequest Create()
            {
                return new LsoFundusCaptureRequest
                {
                    structSize = (uint)Marshal.SizeOf<LsoFundusCaptureRequest>(),
                    cameraExt = LsoColorCameraExtendedParam.Create(),
                };
            }
        }

        // Mirror of native LsoFundusPreviewRequest.
        // The preview is hardware-triggered by contract (TriggerMode On / FrameStart /
        // Line3 / RisingEdge / ReadOut, like the DevKit BfsCameraConfigPreview.json), so
        // there is no trigger on/off field: exposureTimeUs, gain and triggerDelayUs are the
        // camera side of that config, and they are restored when the preview stops.
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoFundusPreviewRequest
        {
            public uint structSize;
            public int monoMode;                    // ColorMonoPreviewMode
            public int scanPatternId;
            public int sequenceMode;                // LsoScanSequenceMode
            public short preMoveOffset;             // GalvanoFirst only; native normalizes otherwise
            public ushort preMoveDelayUs;           // GalvanoFirst only; native normalizes otherwise
            public int previewLight;
            public int previewLightMode;             // LightMode applied before turn-on
            public int restoreLight;
            public int restoreLightMode;             // applied before a different restore light turns on
            public ushort previewLightIntensity;    // 0 = leave the configured intensity
            public ushort restoreLightIntensity;    // 0 = leave the configured intensity
            public uint exposureTimeUs;             // required (> 0) - the preview's own exposure
            public float gain;                      // camera gain in dB; <= 0 keeps the current gain
            public uint triggerDelayUs;             // FrameStart trigger delay; 0 is a legal delay
            public LsoScannerControlParam profile;  // acquisitionMode must be 0 (continuous)
            // Camera preset the preview installs: pixel format, binning, ROI, acquisition
            // mode and ADC depth come from here instead of being hardcoded natively.
            // exposureTime / gain inside it are NOT inputs - exposureTimeUs and gain above
            // are the validated fields and own the tunables; roiMax* are device-reported.
            // A zero-sized ROI means "no window in this preset": the native side then keeps
            // the monoMode full-FOV geometry it used before presets existed.
            public LsoColorCameraSettingParam camera;
            // Same contract as the capture request. triggerDelayUs above stays the
            // validated top-level field, so the Toolkit leaves cameraExt.triggerDelayUs
            // unstated and one value never has two owners.
            public LsoColorCameraExtendedParam cameraExt;

            public static LsoFundusPreviewRequest Create()
            {
                return new LsoFundusPreviewRequest
                {
                    structSize = (uint)Marshal.SizeOf<LsoFundusPreviewRequest>(),
                    cameraExt = LsoColorCameraExtendedParam.Create(),
                };
            }
        }
    }
}
