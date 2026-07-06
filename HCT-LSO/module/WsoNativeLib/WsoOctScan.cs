using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;


namespace WsoNativeLib
{
    using static WsoDomain;
    using static WsoOctDefs;


    public static class WsoOctScan
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////
        /// Constant Definitions
        /// 
        public const int OCT_GCC_CHART_SECTION_SIZE = 6;
        public const float OCT_GCC_CHART_INNER_RANGE = 1.0f;
        public const float OCT_GCC_CHART_OUTER_RANGE = 4.0f;

        public const int OCT_ETDRS_CHART_SECTION_SIZE = 9;
        public const int OCT_ETDRS_CHART_SIDE_SIZE = 4;

        public const int OCT_RNFL_CHART_SECTION_SIZE = 12;
        public const int OCT_RNFL_CHART_QUAD_SIZE = 4;
        public const int OCT_RNFL_TSNIT_GRAPH_SIZE = 360;

        public const int OCT_ANGIO_FLOWS_SECTION_SIZE = 4;


        ////////////////////////////////////////////////////////////////////////////////////////////////
        /// Structure Definitions
        /// 
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctScanPattern
        {
            public OctPatternCode patternCode;
            public OctPatternType patternType;
            public EyeRegion eyeRegion;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            public string patternName;

            public bool IsAngioScan()
            {
                return (patternType == OctPatternType.ANGIO);
            }

            public bool IsCubeScan()
            {
                return (patternType == OctPatternType.CUBE);
            }

            public bool Is3DScan()
            {
                return IsAngioScan() || IsCubeScan();
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctProtocolInitParam
        {
            public EyeSide eyeSide;
            public float diopter;

            public OctPatternCode patternCode;
            public OctPatternType previewType;

            public int scanPoints;
            public int scanLines;
            public int scanOverlaps;

            public float scanRangeX;
            public float scanRangeY;
            public float lineSpace;

            public OctScanDirection scanDirection;
            public OctScanSpeed scanSpeed;

            public float scanOffsetX;
            public float scanOffsetY;
            public float scanScaleX;
            public float scanScaleY;
            public float scanAngle;

            public int enfacePoints;
            public int enfaceLines;
            public float enfaceRangeX;
            public float enfaceRangeY;

            [MarshalAs(UnmanagedType.I1)]
            public bool isFirstScanOnSide;
            [MarshalAs(UnmanagedType.I1)]
            public bool isPhasingEnface;
            [MarshalAs(UnmanagedType.I1)]
            public bool isNotFocusAlign;
            [MarshalAs(UnmanagedType.I1)]
            public bool isNotImageGrabbing;

            public OctProtocolInitParam()
            {
                eyeSide = EyeSide.OD;
                diopter = 0.0f;

                patternCode = OctPatternCode.MACULAR_LINE;
                previewType = OctPatternType.LINE;

                scanPoints = 1024;
                scanLines = 1;
                scanRangeX = 6.0f;
                scanRangeY = 6.0f;
                lineSpace = 0.0f;

                scanOverlaps = 1;
                scanDirection = OctScanDirection.X_TO_Y;
                scanSpeed = OctScanSpeed.FAST;

                scanOffsetX = 0.0f;
                scanOffsetY = 0.0f;
                scanScaleX = 1.0f;
                scanScaleY = 1.0f;
                scanAngle = 0.0f;

                enfacePoints = 256;
                enfaceLines = 128;
                enfaceRangeX = 6.0f;
                enfaceRangeY = 6.0f;

                isFirstScanOnSide = true;
                isPhasingEnface = false;
                isNotFocusAlign = false;
                isNotImageGrabbing = false;
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctProtocolDescript
        {
            public EyeSide eyeSide;
            public EyeRegion eyeRegion;
            public float diopter;

            public OctPatternCode patternCode;
            public OctPatternType patternType;
            public OctPatternType previewType;

            public int scanPoints;
            public int scanLines;
            public int scanOverlaps;
            public float scanRangeX;
            public float scanRangeY;

            public OctScanDirection scanDirection;
            public OctScanSpeed scanSpeed;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            public string protocolName;

            public void copyFromInitParam(OctProtocolInitParam param)
            {
                eyeSide = param.eyeSide;
                diopter = param.diopter;

                patternCode = param.patternCode;
                previewType = param.previewType;

                scanPoints = param.scanPoints;
                scanLines = param.scanLines;
                scanOverlaps = param.scanOverlaps;
                scanRangeX = param.scanRangeX;
                scanRangeY = param.scanRangeY;

                scanDirection = param.scanDirection;
                scanSpeed = param.scanSpeed;
            }
        }


        //////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Image Descriptors
        /// 
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctScanImageDescript
        {
            public IntPtr data;
            public int width;
            public int height;
            public float quality;
            public float snrRatio;
            public int refPoint;

            public int index;
            public float scanRange;

            [MarshalAs(UnmanagedType.I1)]
            public bool isReversed;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctEnfaceImageDescript
        {
            public IntPtr data;
            public int width;
            public int height;
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctCorneaImageDescript
        {
            public IntPtr data;
            public int width;
            public int height;
        }


        //////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Layers Descriptors
        /// 
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctStratumParam
        {
            public OcularLayerType upperType;
            public OcularLayerType lowerType;
            public float upperOffset;
            public float lowerOffset;
            public float chartCenterX;
            public float chartCenterY;

            public void InitAsGccAnalysis()
            {
                upperType = OcularLayerType.ILM;
                lowerType = OcularLayerType.IPL;
            }

            public void InitAsMacularAnalysis()
            {
                upperType = OcularLayerType.ILM;
                lowerType = OcularLayerType.RPE;
            }

            public void InitAsRnflAnalysis()
            {
                upperType = OcularLayerType.ILM;
                lowerType = OcularLayerType.NFL;
            }

            public void InitAsCorneaAnalysis()
            {
                upperType = OcularLayerType.EPI;
                lowerType = OcularLayerType.END;
            }

            public void InitAsAngioFlowAnalysis()
            {
                upperType = OcularLayerType.NFL;
                lowerType = OcularLayerType.OPL;
            }

            public bool IsGccAnalysis()
            {
                return (upperType == OcularLayerType.ILM && lowerType == OcularLayerType.IPL);
            }

            public bool IsRnflAnalysis()
            {
                return (upperType == OcularLayerType.ILM && lowerType == OcularLayerType.NFL);
            }

            public bool IsAngioFlowAnalysis()
            {
                return (upperType == OcularLayerType.NFL && lowerType == OcularLayerType.OPL);
            }
        }


        //////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Thickness Map & Chart Descriptors
        /// 
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctThicknessMapDescript
        {
            public IntPtr data;
            public int width;
            public int height;
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctETDRSChartDescript
        {
            public float centerThick;
            public float centerPercent;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ETDRS_CHART_SIDE_SIZE)]
            public float[] innerThicks;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ETDRS_CHART_SIDE_SIZE)]
            public float[] outerThicks;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ETDRS_CHART_SIDE_SIZE)]
            public float[] innerPercents;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ETDRS_CHART_SIDE_SIZE)]
            public float[] outerPercents;

            public float superiorThick;
            public float inferiorThick;
            public float averageThick;

            public EyeSide eyeSide;
            public float centerX;
            public float centerY;

            OctStratumParam stratumParam;

            public OctETDRSChartDescript(EyeSide side = EyeSide.OD)
            {
                innerThicks = new float[OCT_ETDRS_CHART_SIDE_SIZE];
                outerThicks = new float[OCT_ETDRS_CHART_SIDE_SIZE];
                innerPercents = new float[OCT_ETDRS_CHART_SIDE_SIZE];
                outerPercents = new float[OCT_ETDRS_CHART_SIDE_SIZE];

                centerThick = 0.0f;
                centerPercent = 0.0f;
                superiorThick = 0.0f;
                inferiorThick = 0.0f;
                averageThick = 0.0f;

                eyeSide = side; // EyeSide.OD;
                centerX = 0.0f;
                centerY = 0.0f;

                stratumParam = new OctStratumParam();
                stratumParam.InitAsMacularAnalysis();
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctGCCThickChartDescript
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_GCC_CHART_SECTION_SIZE)]
            public float[] sectionThicks;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_GCC_CHART_SECTION_SIZE)]
            public float[] sectionPercents;

            public float superiorThick;
            public float inferiorThick;
            public float averageThick;

            public float superiorPercent;
            public float inferiorPercent;

            public float innerDiam;
            public float outerDiam;

            public EyeSide eyeSide;
            public float centerX;
            public float centerY;

            OctStratumParam stratumParam;

            public OctGCCThickChartDescript(EyeSide side = EyeSide.OD)
            {
                sectionThicks = new float[OCT_GCC_CHART_SECTION_SIZE];
                sectionPercents = new float[OCT_GCC_CHART_SECTION_SIZE];

                superiorThick = 0.0f;
                inferiorThick = 0.0f;
                averageThick = 0.0f;

                superiorPercent = 0.0f;
                inferiorPercent = 0.0f;

                innerDiam = OCT_GCC_CHART_INNER_RANGE;
                outerDiam = OCT_GCC_CHART_OUTER_RANGE;

                eyeSide = side; // EyeSide.OD;
                centerX = 0.0f;
                centerY = 0.0f;

                stratumParam = new OctStratumParam();
                stratumParam.InitAsGccAnalysis();
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctClockChartDescript
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_CHART_SECTION_SIZE)]
            public float[] clockThicks;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_CHART_SECTION_SIZE)]
            public float[] clockPercents;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_CHART_QUAD_SIZE)]
            public float[] quadThicks;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_CHART_QUAD_SIZE)]
            public float[] quadPercents;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_TSNIT_GRAPH_SIZE)]
            public float[] graphTSNIT;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_TSNIT_GRAPH_SIZE)]
            public float[] graphNormal;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_TSNIT_GRAPH_SIZE)]
            public float[] graphBorder;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_RNFL_TSNIT_GRAPH_SIZE)]
            public float[] graphOutside;

            public EyeSide eyeSide;
            public float centerX;
            public float centerY;

            public OctStratumParam stratumParam;

            public OctClockChartDescript(EyeSide side = EyeSide.OD)
            {
                clockThicks = new float[OCT_RNFL_CHART_SECTION_SIZE];
                clockPercents = new float[OCT_RNFL_CHART_SECTION_SIZE];
                quadThicks = new float[OCT_RNFL_CHART_QUAD_SIZE];
                quadPercents = new float[OCT_RNFL_CHART_QUAD_SIZE];
                graphTSNIT = new float[OCT_RNFL_TSNIT_GRAPH_SIZE];
                graphNormal = new float[OCT_RNFL_TSNIT_GRAPH_SIZE];
                graphBorder = new float[OCT_RNFL_TSNIT_GRAPH_SIZE];
                graphOutside = new float[OCT_RNFL_TSNIT_GRAPH_SIZE];

                eyeSide = side; // EyeSide.OD;
                centerX = 0.0f;
                centerY = 0.0f;

                stratumParam = new OctStratumParam();
                stratumParam.InitAsRnflAnalysis();
            }
        }


        //////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Report Summary Descriptors
        /// 
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctMacularSummaryDescriptor
        {
            public EyeSide eyeSide;
            public float diopter;

            public float foveaCenterX;
            public float foveaCenterY;
            public float foveaCenterThick;
            public int foveaCenterLine;
            public int foveaCenterXpos;

            public bool isFoveaValid;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctOpticDiscSummaryDescriptor
        {
            public EyeSide eyeSide;
            public float diopter;

            public int discStartLine;
            public int discCloseLine;
            public int discCenterLine;
            public int discCenterXpos;
            public int discXposMin;
            public int discXposMax;

            public float discArea;
            public float discVolume;

            public int cupStartLine;
            public int cupCloseLine;
            public int cupCenterLine;
            public int cupCenterXpos;
            public int cupXposMin;
            public int cupXposMax;

            public float cupArea;
            public float cupVolume;
            public float rimArea;
            public float rimVolume;

            public int nerveHeadCenterLine;
            public int nerveHeadCenterXpos;
            public float nerveHeadCenterX;
            public float nerveHeadCenterY;

            public float cupDiscHorzRatio;
            public float cupDiscVertRatio;
            public float cupDiscAreaRatio;

            public bool isOpticCupValid;
            public bool isOpticDiscValid;
            public bool isNerveHeadCenterValid;
            public bool isCupDiscRatioValid;
        }


    }

}
