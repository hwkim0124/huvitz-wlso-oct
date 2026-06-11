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
    using static WsoOctScan;

    public static class WsoOctAngio
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Structure Definitions
        /// 
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctAngioImageDescript
        {
            public IntPtr data;
            public int width;
            public int height;
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctAngioChartDescript
        {
            public float centerFlows;
            public float centerDensity;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ANGIO_FLOWS_SECTION_SIZE)]
            public float[] innerFlows;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ANGIO_FLOWS_SECTION_SIZE)]
            public float[] outerFlows;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ANGIO_FLOWS_SECTION_SIZE)]
            public float[] innerDensity;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = OCT_ANGIO_FLOWS_SECTION_SIZE  )]
            public float[] outerDensity;

            public float superiorFlows;
            public float inferiorFlows;
            public float averageFlows;

            public float superiorDensity;
            public float inferiorDensity;
            public float averageDensity;

            public EyeSide eyeSide;
            public float centerX;
            public float centerY;

            public OctStratumParam stratumParam;

            public OctAngioChartDescript(EyeSide side = EyeSide.OD)
            {
                innerFlows = new float[OCT_ANGIO_FLOWS_SECTION_SIZE];
                outerFlows = new float[OCT_ANGIO_FLOWS_SECTION_SIZE];
                innerDensity = new float[OCT_ANGIO_FLOWS_SECTION_SIZE];
                outerDensity = new float[OCT_ANGIO_FLOWS_SECTION_SIZE];

                centerFlows = 0.0f;
                centerDensity = 0.0f;
                superiorFlows = 0.0f;
                inferiorFlows = 0.0f;
                averageFlows = 0.0f;
                superiorDensity = 0.0f;
                inferiorDensity = 0.0f;
                averageDensity = 0.0f;

                eyeSide = side;
                centerX = 0.0f;
                centerY = 0.0f;

                stratumParam = new OctStratumParam();
                stratumParam.InitAsAngioFlowAnalysis();
            }
        };
    }
}
