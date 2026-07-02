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
