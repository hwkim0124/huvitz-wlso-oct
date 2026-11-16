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


        /// <summary>
        /// Full range (complex-conjugate resolved) scanning parameters: the reference phase ramp
        /// that puts a carrier across the A-lines, and the lateral filter that separates the image
        /// from its mirror twin. Enabled per scan by OctProtocolInitParam.isFullRangeDomain.
        /// Mirrors native wso_domain::OctFullRangeParam (WsoDomain/OctScanStructs.h); the size is
        /// checked at run time by <see cref="OctScanner.VerifyStructSizes"/>.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        public struct OctFullRangeParam
        {
            /// <summary>Phase DAC code at the start of the ramp, within the int16 range.</summary>
            public int phaseBase;
            /// <summary>Phase DAC codes per trigger, non-zero within the int16 range. Sets the carrier.</summary>
            public int phaseInc;
            /// <summary>Hold time at the end of the ramp, in microseconds.</summary>
            public int endPhaseWaitTimeUs;
            /// <summary>Time to slew back to the base code, in microseconds, within the uint16 range.</summary>
            public int phaseOriginSlewTimeUs;
            /// <summary>Phase trigger delay, 1 count: 10 ns.</summary>
            public int trigDelayCount;
            /// <summary>Lowest lateral frequencies removed, in percent of the A-lines, from 0 to below 50.</summary>
            public float lateralCutoffPercent;
            /// <summary>1 to keep the negative lateral band, which mirrors the image; otherwise 0.</summary>
            public int flipSideband;

            public OctFullRangeParam()
            {
                phaseBase = -32767;
                phaseInc = 4;
                endPhaseWaitTimeUs = 0;
                phaseOriginSlewTimeUs = 5000;
                trigDelayCount = 1;
                lateralCutoffPercent = 3.125f;
                flipSideband = 0;
            }
        }

    }
}
