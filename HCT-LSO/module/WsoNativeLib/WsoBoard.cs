
using System.Runtime.InteropServices;
using System.Security.RightsManagement;

namespace WsoNativeLib
{
    public static class  WsoBoard
    {
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct DispersionCalibration
        {
            public double a2;
            public double a3;
            public double a4;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct SpectrometerCalibration
        {
            public double a0;
            public double a1;
            public double a2;
            public double a3;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctGalvanometerCalibration
        {
            public float offsetX;
            public float offsetY;
            public float rangeX;
            public float rangeY;
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
        public struct LedCalibration
        {
            public ushort whiteIntensity;
            public ushort retinaIrIntensity;
            public ushort anteriorIrIntensity1;
            public ushort anteriorIrIntensity2;
            public ushort blueIntensity;
            public ushort greenIntensity;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct DiopterCalibration
        {
            public int octFocusZeroPos;
            public int lsoFocusZeroPos;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct FilterCalibration
        {
            public int inPos;
            public int outPos;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsoFilterCalibration
        {
            public int colorModePos;
            public int blueFafPos;
            public int greenFafPos;
            public int icgaPos;
            public int reserved;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct MotorCalibration
        {
            public int referRetinaOriginPos;
            public int referCorneaOriginPos;
            public int polarOriginPos;
            public int lsoFocusZeroDioptPos;
            public int octFocusZeroDioptPos;

            public FilterCalibration returnMirror;
            public FilterCalibration octAnteriorLens;
            public LsoFilterCalibration lsoFilter;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct SystemCalibration
        {
            public DispersionCalibration dispersionRetina;
            public DispersionCalibration dispersionCornea;
            public SpectrometerCalibration spectrometer;
            public OctGalvanometerCalibration octGalvano;
            public SldCalibration sldParam;
            public LedCalibration ledParam;
            public MotorCalibration motorParam;
        }
    }
    
}