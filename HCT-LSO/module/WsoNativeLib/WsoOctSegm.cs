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

    public static class WsoOctSegm
    {
        // Structure Definitions
        /////////////////////////////////////////////////////////////////////////////////////////////
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctRetinaBsegmDescriptor
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] ilms;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] nfls;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] ipls;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] opls;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] ioss;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] rpes;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] brms;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] oprs;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] datums;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] inners;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] outers;

            public int width;
            public int height;
            public float range;

            public int foveaX1;
            public int foveaX2;
            public int opticDiscX1;
            public int opticDiscX2;
            public int opticCupX1;
            public int opticCupX2;
            public int opticDiscPixelSize;
            public int opticCupPixelSize;

            [MarshalAs(UnmanagedType.I1)]
            public bool isFoveaValid;
            [MarshalAs(UnmanagedType.I1)]
            public bool isOpticDiscValid;
            [MarshalAs(UnmanagedType.I1)]
            public bool isOpticCupValid;

            public void InitializeObject()
            {
                ilms = new int[SEGM_IMAGE_WIDTH_MAX];
                nfls = new int[SEGM_IMAGE_WIDTH_MAX];
                ipls = new int[SEGM_IMAGE_WIDTH_MAX];
                opls = new int[SEGM_IMAGE_WIDTH_MAX];
                ioss = new int[SEGM_IMAGE_WIDTH_MAX];
                rpes = new int[SEGM_IMAGE_WIDTH_MAX];
                brms = new int[SEGM_IMAGE_WIDTH_MAX];
                oprs = new int[SEGM_IMAGE_WIDTH_MAX];

                datums = new int[SEGM_IMAGE_WIDTH_MAX];
                inners = new int[SEGM_IMAGE_WIDTH_MAX];
                outers = new int[SEGM_IMAGE_WIDTH_MAX];

                width = 0;
                height = 0;
                range = 0.0f;

                foveaX1 = 0;
                foveaX2 = 0;
                opticDiscX1 = 0;
                opticDiscX2 = 0;
                opticCupX1 = 0;
                opticCupX2 = 0;
                opticDiscPixelSize = 0;
                opticCupPixelSize = 0;

                isFoveaValid = false;
                isOpticDiscValid = false;
                isOpticCupValid = false;
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctBsegmLayerPoints
        {
            public OcularLayerType type;

            public int width;
            public int height;
            public float range;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] data;

            public void InitializeObject()
            {
                type = OcularLayerType.UNKNOWN;

                width = 0;
                height = 0;
                range = 0.0f;

                data = new int[SEGM_IMAGE_WIDTH_MAX];
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctRetinaBsegmTraits
        {
            public int width;
            public int height;
            public float range;

            public int foveaX1;
            public int foveaX2;
            public int opticDiscX1;
            public int opticDiscX2;
            public int opticCupX1;
            public int opticCupX2;
            public int opticDiscPixelSize;
            public int opticCupPixelSize;

            [MarshalAs(UnmanagedType.I1)]
            public bool isFoveaValid;
            [MarshalAs(UnmanagedType.I1)]
            public bool isOpticDiscValid;
            [MarshalAs(UnmanagedType.I1)]
            public bool isOpticCupValid;

            public void InitializeObject()
            {
                width = 0;
                height = 0;
                range = 0.0f;

                foveaX1 = 0;
                foveaX2 = 0;
                opticDiscX1 = 0;
                opticDiscX2 = 0;
                opticCupX1 = 0;
                opticCupX2 = 0;
                opticDiscPixelSize = 0;
                opticCupPixelSize = 0;

                isFoveaValid = false;
                isOpticDiscValid = false;
                isOpticCupValid = false;
            }
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctCorneaBsegmDescriptor
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] epis;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] bows;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] ends;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] datums;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] inners;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public int[] outers;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public float[] curveRadius;

            public int width;
            public int height;
            public float range;

            public int corenaTopX;
            public int corenaTopY;

            public void InitializeObject()
            {
                epis = new int[SEGM_IMAGE_WIDTH_MAX];
                bows = new int[SEGM_IMAGE_WIDTH_MAX];
                ends = new int[SEGM_IMAGE_WIDTH_MAX];

                datums = new int[SEGM_IMAGE_WIDTH_MAX];
                inners = new int[SEGM_IMAGE_WIDTH_MAX];
                outers = new int[SEGM_IMAGE_WIDTH_MAX];

                curveRadius = new float[SEGM_IMAGE_WIDTH_MAX];

                width = 0;
                height = 0;
                range = 0.0f;

                corenaTopX = 0;
                corenaTopY = 0;
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctCorneaBsegmTraits
        {
            public int width;
            public int height;
            public float range;

            public int corenaTopX;
            public int corenaTopY;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R4, SizeConst = SEGM_IMAGE_WIDTH_MAX)]
            public float[] curveRadius;
            public void InitializeObject()
            {
                width = 0;
                height = 0;
                range = 0.0f;

                corenaTopX = 0;
                corenaTopY = 0;

                curveRadius = new float[SEGM_IMAGE_WIDTH_MAX];
            }
        }
    }
}
