using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace WsoNativeLib
{
    using static WsoNativeLib.WsoDomain;

    public static class WsoLsoScan
    {
        // Constant Definitions 
        ////////////////////////////////////////////////////////////////////////////////////////////
        public const int LSO_SCANNER_SAMPLE_SIZE_MAX = 8192;
        public const int LSO_SCANNER_PATTERN_NUM_MAX = 8;
        public const int LSO_CAPTURE_FRAME_ROI_MAX = 255;


        // Enum Definitions
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Which entity starts a scan frame (DevKit LsoPatternForm "Scan Sequence Control").
        // Board-global: the command that sets it carries no pattern id.
        public enum LsoScanSequenceMode : int
        {
            // Closed loop: DAC playback starts on the EXPOSURE_ACTIVE rising edge, so the
            // sensor's blanking rows are absorbed while the mirror accelerates. No trigger
            // timing to calibrate - this is the recommended mode.
            TrigFirst = 0,

            // Open loop: the mirror pre-moves and the board fires the camera trigger after
            // PreMoveDelayUs. Needs the camera trigger delay to be calibrated.
            GalvanoFirst = 1,
        }

        // Structure Definitions
        ////////////////////////////////////////////////////////////////////////////////////////////
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoScannerControlParam
        {
            public float timeStepUs;         // galvano scanner control sample time.
            public float exposureTimeUs;     // camera exposure time in us.

            public ushort triggerSource;   // camera flash out source 0: ColorCamera,1:IR camera. 
            public ushort acquisitionMode; // 0:continuous, 1:fixed frame Mode.
            public int acqFrameSize;      // valid on Fixed FrameMode

            public ushort sampleSize;      // galvano control pattern sample size
            public ushort subFrameSize;    // subframe size of a Full frame.
            public short prescanPosRewindOffset;   // gavano rewind offset of subframe
            public ushort ledOnPosIndex;   // led on sample index of gavano pattern.
            public ushort ledOffPosIndex;  // led off sample index of galvano pattern.

            public short yGalvoStartPos;
            public short yGalvoEndPos;

            // Ramp samples before start / after end (PC-side only, not in the board struct).
            // 0/0 = plain linear sweep. Mirror of wso_domain::LsoScannerControlParam.
            public ushort leadInSamples;
            public ushort leadOutSamples;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoScannerCaptureParam
        {
            public uint controlStatus;
            public uint acquisitionCount;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoColorCameraSettingParam
        {
            public uint roiMaxWidth;
            public uint roiMaxHeight;
            public uint roiXWidth;
            public uint roiYHeight;
            public uint roiXOffset;
            public uint roiYOffset;
            public uint exposureTime;
            public uint acquisitionMode;
            public uint acquisitionFrameCount;
            public uint pixelFormat;
            public uint binningHorizontal;
            public uint binningVertical;
            public float gain;
            public uint adcDepthIndex;
        }

        // --- A-1 extended camera contract (mirror of wso_domain::ColorCameraExtendedParam) --
        // The 14-field LsoColorCameraSettingParam above is frozen; everything else the
        // DevKit ColorCameraConfig carries travels here, in a separate size- and
        // version-stamped block.
        //
        // Enum values are GenApi SYMBOLIC names, not SDK integers: the numbers differ per
        // model and per SDK release, the names are the SFNC contract the DevKit configs are
        // already written in, and this way nothing on the managed side has to claim to know
        // which entries a camera implements.
        // An EMPTY name - or a negative number - means "this caller does not state this
        // node", and the native side then leaves it exactly as the device has it.
        public const int LSO_CAMERA_LINE_COUNT = 4;
        public const int LSO_CAMERA_NAME_MAX = 32;
        public const int LSO_CAMERA_FIELD_MAX = 40;
        public const uint LSO_CAMERA_CONFIG_VERSION = 1;

        // Native sizeof(), pinned here and asserted by program/TestCameraPreset against
        // Marshal.SizeOf. The same numbers are static_assert-ed in LsoCameraStruct.h, so a
        // field added on one side cannot pass unnoticed on the other.
        public const int NativeSizeOfLineConfig = 68;
        public const int NativeSizeOfExtendedParam = 696;
        public const int NativeSizeOfFieldResult = 100;
        public const int NativeSizeOfApplyResult = 4016;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoColorCameraLineConfig
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)]
            public string mode;         // LineMode symbolic, "" = not stated

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)]
            public string source;       // LineSource symbolic, "" = not stated

            public int inverter;        // 0 / 1, negative = not stated
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoColorCameraExtendedParam
        {
            public uint structSize;
            public uint version;
            public long acquisitionBurstFrameCount;   // negative = not stated
            public long deviceLinkThroughput;         // DeviceLinkThroughputLimit, negative = not stated
            public double triggerDelayUs;             // TriggerDelay in us, negative = not stated
            public int ispEnable;                     // 0 / 1, negative = not stated
            public int reserved0;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string exposureAuto;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string binningSelector;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string binningHorizontalMode;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string binningVerticalMode;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string testPattern;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string sensorShutterMode;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string triggerMode;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string triggerSource;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string triggerActivation;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string triggerSelector;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string triggerOverlap;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string lineSelector;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = LSO_CAMERA_LINE_COUNT, ArraySubType = UnmanagedType.Struct)]
            public LsoColorCameraLineConfig[] lines;

            // A block that states nothing. Writing it changes no node, which is what lets a
            // caller with no A-1 configuration keep the behaviour it had before A-1 existed.
            // ByValTStr marshals a null string as an empty buffer, but the four line rows
            // are a fixed-size array the marshaller will not allocate for us.
            public static LsoColorCameraExtendedParam Create()
            {
                var lines = new LsoColorCameraLineConfig[LSO_CAMERA_LINE_COUNT];
                for (int i = 0; i < lines.Length; ++i)
                {
                    lines[i] = new LsoColorCameraLineConfig
                    {
                        mode = string.Empty,
                        source = string.Empty,
                        inverter = -1,
                    };
                }

                return new LsoColorCameraExtendedParam
                {
                    structSize = (uint)Marshal.SizeOf<LsoColorCameraExtendedParam>(),
                    version = LSO_CAMERA_CONFIG_VERSION,
                    acquisitionBurstFrameCount = -1,
                    deviceLinkThroughput = -1,
                    triggerDelayUs = -1.0,
                    ispEnable = -1,
                    reserved0 = 0,
                    exposureAuto = string.Empty,
                    binningSelector = string.Empty,
                    binningHorizontalMode = string.Empty,
                    binningVerticalMode = string.Empty,
                    testPattern = string.Empty,
                    sensorShutterMode = string.Empty,
                    triggerMode = string.Empty,
                    triggerSource = string.Empty,
                    triggerActivation = string.Empty,
                    triggerSelector = string.Empty,
                    triggerOverlap = string.Empty,
                    lineSelector = string.Empty,
                    lines = lines,
                };
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoColorCameraEnumValue
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)]
            public string name;
        }

        // What actually happened to one node. `status` is LsoColorCameraFieldStatus - a
        // setter that returned without throwing is not reported as success anywhere.
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoColorCameraFieldResult
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string node;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string requested;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = LSO_CAMERA_NAME_MAX)] public string actual;
            public int status;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoColorCameraApplyResult
        {
            public uint structSize;
            public uint version;
            public int fieldCount;
            public int failedCount;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = LSO_CAMERA_FIELD_MAX, ArraySubType = UnmanagedType.Struct)]
            public LsoColorCameraFieldResult[] fields;

            public static LsoColorCameraApplyResult Create()
            {
                return new LsoColorCameraApplyResult
                {
                    structSize = (uint)Marshal.SizeOf<LsoColorCameraApplyResult>(),
                    version = LSO_CAMERA_CONFIG_VERSION,
                    fieldCount = 0,
                    failedCount = 0,
                    fields = new LsoColorCameraFieldResult[LSO_CAMERA_FIELD_MAX],
                };
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoCaptureFrameROI
        {
            public int width;
            public int height;
            public int offsetX;
            public int offsetY;
            public int startY;
            public int endY ;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoCaptureFrameSeqROIPreset
        {
            public int frameCount;
            public LsoCaptureFrameROI[] frameROIs;

            public LsoCaptureFrameSeqROIPreset()
            {
                frameCount = 0;
                frameROIs = new LsoCaptureFrameROI[LSO_CAPTURE_FRAME_ROI_MAX];
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoCaptureFrameOffsetROIPreset
        {
            public int frameCount;
            public int roiWidth;
            public int roiHeight;
            public LsoCaptureFrameROI[] frameROIs;

            public LsoCaptureFrameOffsetROIPreset()
            {
                frameCount = 0;
                roiWidth = 0;
                roiHeight = 0;
                frameROIs = new LsoCaptureFrameROI[LSO_CAPTURE_FRAME_ROI_MAX];
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoCaptureFrameRollSwTrigOverlap
        {
            public int frameWidth ;
            public int frameHeight;
            public int offsetX ;
            public int offsetY ;
            public int overlapFrameCount ;
            public int triggerIntervalMs ;
            public int brightness ;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoCaptureFrameRollSwTrigOverlapPreset
        {
            public LsoCaptureFrameRollSwTrigOverlap overlap;
        }
    }
}   
