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
            public float again_left ;
            public float again_right;
            public float again_lower;
            public float dgain_left;
            public float dgain_right;
            public float dgain_lower;

            public CorneaCameraConfigParam()
            {
                again_left = 1.0f;
                again_right = 1.0f;
                again_lower = 1.0f;
                dgain_left = 1.0f;
                dgain_right = 1.0f;
                dgain_lower = 1.0f;
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
