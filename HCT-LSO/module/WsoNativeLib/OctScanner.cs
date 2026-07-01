using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Net.Mail;
using WsoNativeLib;

namespace WsoNativeLib
{
    using static LibraryConfig;
    using static WsoOctDefs;
    using static WsoOctCalib;


    public static class OctScanner
    {
        const string LibraryName = WsoSystemDllPath;
        private const int BufferSize = 64;

        static private float[] _floatBuff = new float[BufferSize]; 
        static private double[] _detectBuff = new double[LINE_CAMERA_CCD_PIXELS];


        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        private static extern float getOctLineCameraExposureTime();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctLineCameraExposureTime(float exp_time);

        [DllImport(LibraryName)]
        private static extern float getOctGalvanoTriggerTimeStep();

        [DllImport(LibraryName)]
        private static extern int getOctGalvanoTriggerTimeDelay();

        [DllImport(LibraryName)]
        private static extern int getOctGalvanoTriggerForePaddings(OctScanSpeed speed);

        [DllImport(LibraryName)]
        private static extern int getOctGalvanoTriggerPostPaddings(OctScanSpeed speed);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctGalvanoTriggerTimeStep(float time_step);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctGalvanoTriggerTimeDelay(int time_delay);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctGalvanoTriggerForePaddings(OctScanSpeed speed, int padds);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctGalvanoTriggerPostPaddings(OctScanSpeed speed, int padds);


        [DllImport(LibraryName)]
        private static extern int getOctScanPatternCount([MarshalAs(UnmanagedType.I1)] bool not_point);

        [DllImport(LibraryName)]
        private static extern int getOctScanPatternList([Out] WsoOctScan.OctScanPattern[] patterns, int buff_size, [MarshalAs(UnmanagedType.I1)] bool not_point);

        [DllImport(LibraryName)]
        private static extern int getOctScanPatternAscanList([Out] int[] ascans, int buff_size, int code);

        [DllImport(LibraryName)]
        private static extern int getOctScanPatternBscanList([Out] int[] bscans, int buff_size, int code, int ascans);

        [DllImport(LibraryName)]
        private static extern int getOctScanPatternOverlapList([Out] int[] overlaps, int buff_size, int code, int ascans, int bscans);

        [DllImport(LibraryName)]
        private static extern int getOctScanPatternRangeXList([Out] float[] ranges, int buff_size, int code);

        [DllImport(LibraryName)]
        private static extern int getOctScanPatternRangeYList([Out] float[] ranges, int buff_size, int code);

        [DllImport(LibraryName)]
        private static extern int getOctDetectorWavelengths([Out] double[] wlens);

        [DllImport(LibraryName)]
        private static extern double getOctAxialPixelResolution(double fwhm, double ref_index);

        [DllImport(LibraryName)]
        private static extern double getOctWavelengthAtPixelPosition(float index);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainOctSpectrometerParam(ref OctSpectrometerParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool submitOctSpectrometerParam(OctSpectrometerParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainOctDispersionParam(ref OctDispersionParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool submitOctDispersionParam(OctDispersionParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctFFTZeroPaddingEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable, int multi);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctRefreshPhaseShiftParamEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctBackgroundSubtractionEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctDispersionCompensationEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctFixedNoiseReductionEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctAdaptiveGrayScalingEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctFFTWindowingEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctKLinearResamplingEnabled([MarshalAs(UnmanagedType.I1)] bool isset, [MarshalAs(UnmanagedType.I1)] bool enable);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static float GetLineCameraExposureTime()
        {
            return getOctLineCameraExposureTime();
        }

        public static bool SetLineCameraExposureTime(float exp_time)
        {
            return setOctLineCameraExposureTime(exp_time);
        }

        public static float GetGalvanoTriggerTimeStep()
        {
            return getOctGalvanoTriggerTimeStep();
        }

        public static int GetGalvanoTriggerTimeDelay()
        {
            return getOctGalvanoTriggerTimeDelay();
        }

        public static int GetGalvanoTriggerForePaddings(OctScanSpeed speed)
        {
            return getOctGalvanoTriggerForePaddings(speed);
        }

        public static int GetGalvanoTriggerPostPaddings(OctScanSpeed speed)
        {
            return getOctGalvanoTriggerPostPaddings(speed);
        }

        public static bool SetGalvanoTriggerTimeStep(float time_step)
        {
            return setOctGalvanoTriggerTimeStep(time_step);
        }
        
        public static bool SetGalvanoTriggerTimeDelay(int time_delay)
        {
            return setOctGalvanoTriggerTimeDelay(time_delay);
        }

        public static bool SetGalvanoTriggerForePaddings(OctScanSpeed speed, int padds)
        {
            return setOctGalvanoTriggerForePaddings(speed, padds);
        }

        public static bool SetGalvanoTriggerPostPaddings(OctScanSpeed speed, int padds)
        {
            return setOctGalvanoTriggerPostPaddings(speed, padds);
        }

        
        public static int GetOctScanPatternCount(bool not_point)
        {
            return getOctScanPatternCount(not_point);
        }

        public static int GetOctScanPatternList(WsoOctScan.OctScanPattern[] patterns, int buff_size, bool not_point)
        {
            return getOctScanPatternList(patterns, buff_size, not_point);
        }

        public static int GetOctScanPatternAscanList(int[] ascans, int buff_size, OctPatternCode code)
        {
            return getOctScanPatternAscanList(ascans, buff_size, (int)code);
        }

        public static int GetOctScanPatternBscanList(int[] bscans, int buff_size, OctPatternCode code, int ascans)
        {
            return getOctScanPatternBscanList(bscans, buff_size, (int)code, ascans);
        }

        public static int GetOctScanPatternOverlapList(int[] overlaps, int buff_size, OctPatternCode code, int ascans, int bscans)
        {
            return getOctScanPatternOverlapList(overlaps, buff_size, (int)code, ascans, bscans);
        }

        public static int GetOctScanPatternRangeXList(out float[] ranges, OctPatternCode code)
        {
            int count = getOctScanPatternRangeXList(_floatBuff, BufferSize, (int)code);
            ranges = new float[count];
            Array.Copy(_floatBuff, ranges, count);
            return count;
        }

        public static int GetOctScanPatternRangeYList(out float[] ranges, OctPatternCode code)
        {
            int count = getOctScanPatternRangeYList(_floatBuff, BufferSize, (int)code);
            ranges = new float[count];
            Array.Copy(_floatBuff, ranges, count);
            return count;
        }

        public static int GetOctDetectorWavelengths(out double[] wlens)
        {
            int count = getOctDetectorWavelengths(_detectBuff);
            wlens = new double[count];
            Array.Copy(_detectBuff, wlens, count);
            return count;
        }

        public static double GetOctAxialPixelResolution(double fwhm, double ref_index)
        {
            return getOctAxialPixelResolution(fwhm, ref_index);
        }

        public static double GetOctWavelengthAtPixelPosition(float index)
        {
            return getOctWavelengthAtPixelPosition(index);
        }

        public static bool ObtainOctSpectrometerParam(ref OctSpectrometerParam param)
        {
            return obtainOctSpectrometerParam(ref param);
        }

        public static bool SubmitOctSpectrometerParam(OctSpectrometerParam param)
        {
            return submitOctSpectrometerParam(param);
        }

        public static bool ObtainOctDispersionParam(ref OctDispersionParam param)
        {
            return obtainOctDispersionParam(ref param);
        }

        public static bool SubmitOctDispersionParam(OctDispersionParam param)
        {
            return submitOctDispersionParam(param);
        }

        public static bool IsFFTZeroPaddingEnabled(bool isset = false, bool enable = false, int multi = 1)
        {
            return isOctFFTZeroPaddingEnabled(isset, enable, multi);
        }

        public static bool IsRefreshPhaseShiftParamEnabled(bool isset = false, bool enable = false)
        {
            return isOctRefreshPhaseShiftParamEnabled(isset, enable);
        }

        public static bool IsBackgroundSubtractionEnabled(bool isset = false, bool enable = false)
        {
            return isOctBackgroundSubtractionEnabled(isset, enable);
        }
        public static bool IsDispersionCompensationEnabled(bool isset = false, bool enable = false)
        {
            return isOctDispersionCompensationEnabled(isset, enable);
        }

        public static bool IsFixedNoiseReductionEnabled(bool isset = false, bool enable = false)
        {
            return isOctFixedNoiseReductionEnabled(isset, enable);
        }
        public static bool IsAdaptiveGrayScalingEnabled(bool isset = false, bool enable = false)
        {
            return isOctAdaptiveGrayScalingEnabled(isset, enable);
        }
        public static bool IsFFTWindowingEnabled(bool isset = false, bool enable = false)
        {
            return isOctFFTWindowingEnabled(isset, enable);
        }
        public static bool IsKLinearResamplingEnabled(bool isset = false, bool enable = false)
        {
            return isOctKLinearResamplingEnabled(isset, enable);
        }
    }
}
