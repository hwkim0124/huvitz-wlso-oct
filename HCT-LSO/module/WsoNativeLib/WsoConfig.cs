using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WsoNativeLib
{
    public static class WsoConfig
    {
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct CorneaCameraConfigParam
        {
            public float again;
            public float dgain;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = 2)]
            public int[] wdots;

            public CorneaCameraConfigParam()
            {
                again = 0.0f;
                dgain = 0.0f;
                wdots = new int[2];
            }
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctGalvanoConfigParam
        {
            public float xoffset;
            public float yoffset;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = 8)]
            public float[] xscales;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = 8)]
            public float[] yscales;

            public OctGalvanoConfigParam()
            {
                xoffset = 0.0f;
                yoffset = 0.0f;
                xscales = new float[8];
                yscales = new float[8];
            }
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct FixationPoint
        {
            public int x;
            public int y;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct InternalFixationPreset
        {
            [MarshalAs(UnmanagedType.I1)]
            public bool useLcdFix;
            [MarshalAs(UnmanagedType.I1)]
            public bool useBlinkMode;

            public int blinkPeriod;
            public int blinkOnTime;
            public int brightness;
            public int fixationType;

            public FixationPoint centerOD;
            public FixationPoint fundusOD;
            public FixationPoint scanDiscOD;
            public FixationPoint leftCenterOD;
            public FixationPoint leftTopOD;
            public FixationPoint leftBottomOD;
            public FixationPoint rightCenterOD;
            public FixationPoint rightTopOD;
            public FixationPoint rightBottomOD;

            public FixationPoint centerOS;
            public FixationPoint fundusOS;
            public FixationPoint scanDiscOS;
            public FixationPoint leftCenterOS;
            public FixationPoint leftTopOS;
            public FixationPoint leftBottomOS;
            public FixationPoint rightCenterOS;
            public FixationPoint rightTopOS;
            public FixationPoint rightBottomOS;
        }
    }
}
