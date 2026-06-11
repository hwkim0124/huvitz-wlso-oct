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

    public static class WsoOctCalib
    {
        // Structure Definitions
        /////////////////////////////////////////////////////////////////////////////////////////////
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctSpectrometerParam
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R8, SizeConst = OCT_SPECTROMETER_COEFFS_SIZE)]
            public double[] wfCoeffs;

            public OctSpectrometerParam()
            {
                wfCoeffs = new double[OCT_SPECTROMETER_COEFFS_SIZE];
            }
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctDispersionParam
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R8, SizeConst = OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE)]
            public double[] retinaCoeffs;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.R8, SizeConst = OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE)]
            public double[] corneaCoeffs;

            public OctDispersionParam()
            {
                retinaCoeffs = new double[OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE];
                corneaCoeffs = new double[OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE];
            }
        }

    }
}
