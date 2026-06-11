
using System.Runtime.InteropServices;

namespace WsoNativeLib
{
    public static class  WsoBoard
    {
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct DispersionCalibration
        {
            public double a1;
            public double a2;
            public double a3;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct SpectrometerCalibration
        {
            public double a1;
            public double a2;
            public double a3;
            public double a4;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct GalvanometerCalibration
        {
            public float offset;
            public float range;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct SldCalibration
        {
            public ushort pdCurrMax;
            public ushort pdCurrMin;
            public ushort sldCurrMax;
            public ushort sldCurrMin;
            public ushort refEpdMax;
            public ushort refEpdMin;

            public ushort rmonHighCode;
            public ushort rmonLowCode1;
            public ushort rmonLowCode2;
            public ushort rmonRsiCode;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct DiopterCalibration
        {
            public int octFocusZeroPos;
            public int sloFocusZeroPos;
            public int fixFocusZeroPos;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct FilterCalibration
        {
            public short inPos;
            public short outPos;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct ApdCalibration
        {
            public float vbr;
            public float vr;
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct HbsSysCalibration
        {
            public int referPosRetina;
            public int referPosCornea;
            public int polarPos;

            public DispersionCalibration dispersionRetina;
            public DispersionCalibration dispersionCornea;
            public SpectrometerCalibration spectrometer;
            public GalvanometerCalibration galvanometerX;
            public GalvanometerCalibration galvanometerY;
            public SldCalibration sldParam;
            public DiopterCalibration diopterParam;
            public FilterCalibration icgaFilter;
            public FilterCalibration octMirror;

            public ulong checksum;
        }
    }
    
}