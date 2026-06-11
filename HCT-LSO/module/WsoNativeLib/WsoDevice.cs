

using System;
using System.Runtime.InteropServices;
using System.Security.Permissions;

namespace WsoNativeLib
{
    using static WsoNativeLib.WsoDomain;


    public static class WsoDevice
    {
        // Enum definition
        /////////////////////////////////////////////////////////////////////////////////////
        public enum MotorType : int
        {
            OctFocus = 0,
            OctRefer = 1,
            OctPolar = 2,
            SloFocus = 3,
            IcgaFilter = 4,
            StageY = 5,
            FixFocus = 6,
            Tilt = 7,
            Swing = 8,
            OctMirror = 9
        }

        public enum LightType : int
        {
            SLO_RED = 0,
            SLO_GREEN = 1,
            SLO_BLUE = 2,
            SLO_IR = 3,
            OCT_SLD = 4
        }

        public enum QldApcMode : int
        {
            APC_MODE = 0,
            ACC_MODE = 1
        }

        public const int FIXATION_ROW_MIN = 0;      // -199;
        public const int FIXATION_ROW_MAX = +199;
        public const int FIXATION_COL_MIN = 0;      // -199;
        public const int FIXATION_COL_MAX = +199;


        // Callback function type 
        //////////////////////////////////////////////////////////////////////////////////////
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void StepMotorPositionChanged(int pos, float value);




        // Structure Definitions
        /////////////////////////////////////////////////////////////////////////////////////////////
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct StepMotorStatus
        {
            public int rangeMin;
            public int rangeMax;
            public int currPos;
            public int maxSpeed;
            public int minSpeed;
            public int accelStep;
            public int sliderStepSize;
            public int sliderPageSize;
            public float rangeMinValue;
            public float rangeMaxValue;
        }

        // Struct type uses sequential layout as default, so we don't need to specify it explicitly. 
        // Sequential layout is the only layout that can be used in unmanaged code.
        // But we need to specify the CharSet explicitly, otherwise the default CharSet is CharSet.Ansi.
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctSldCalibParam
        {
            public int highCode;
            public int lowCode1;
            public int lowCode2;
            public int rsiCode;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct OctSldStatusParam
        {
            public OctSldCalibParam calibParam;

            public float sldCurrent;
            public float ipdCurrent;
            public int externalPd;
            public float temperature;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct AdcSensorStatus
        {
            public float cpuTempature;
            public int externalMonitorPd;
            public int lensHallSensor1;
            public int lensHallSensor2;
            public int odOs;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct InternalFixationParam
        {
            public int brightness;
            public int blinkMode;
            public int blinkPeriod;
            public int blinkOnTime;
            public int fixationType;
        }
    }
}