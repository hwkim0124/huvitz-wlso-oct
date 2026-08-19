

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
            OctFocus = 0x00,
            OctRefer = 0x01,
            OctPolar = 0x02,
            OctRefNd = 0x03,
            LsoFocus = 0x04,
            RetMirror = 0x05,
            OctAntLens = 0x06,
            LsoFilter = 0x07, 
            Ap2 = 0x08, 
            Swing = 0x09,
            StageX = 0x0A,
            StageY = 0x0B, 
            StageZ = 0x0C, 
            ChinRest = 0x0F,
        }

        public enum LightType : int
        {
            LsoWhite = 0x00,
            RetinaIr = 0x01,
            CorneaIrLeft = 0x02, 
            CorneaIrRight = 0x03,
            ExtFixation = 0x04, 
            LsoBlue = 0x05, 
            LsoGreen = 0x06
        }

        public enum LightMode : int
        {
            Trigger = 0x00, 
            Continuous = 0x01,
            Unknown = 0xFF
        }

        public enum LaserType : int
        {
            OctSld = 0x04,
        }

        public enum CameraType : int
        {
            IrRetina = 0x00,
            IrCorneaLower = 0x01,
            IrCorneaLeft = 0x02,
            IrCorneaRight = 0x03,
            ColorFundus = 0x04 
        }

        public enum IrCameraViewPreset : int
        {
            Default = 0x00,
            LowerOnly = 0x01,
            LeftOnly = 0x02,
            LowerLeft = 0x03,
            RightOnly = 0x04,
            LowerRight = 0x05,
            LeftRight = 0x06,
            LowerLeftRight = 0x07
        }

        public enum IrCameraMirrorMode : int
        {
            MirrorOff = 0x00,
            MirrorOn = 0x01
        }

        public enum IrCameraOutputMode : int
        {
            Normal = 0x00,
            TestPattern = 0x01
        }

        public enum IrCameraTestPattern : int
        {
            Standard = 0x00,
            TopBottomBar = 0x04, 
            LeftRightBar = 0x08,
            Random = 0x01,
            Square = 0x12, 
            Black = 0x03
        }

        public enum IrCameraTestOption : int
        {
            None = 0x00,    
            RollingBar = 0x40,
            Transparent = 0x80 
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

            public int motorWait;
            public int piStatus;
            public int piHitRefPos;
            public int piHitMargin;
            public int piHitLastPos;
            public int piHitPosError;

            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = 2)]
            public int[] limitRange;
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I4, SizeConst = 2)]
            public int[] limitStatus;

            public int sliderStepSize;
            public int sliderPageSize;
            public float rangeMinValue;
            public float rangeMaxValue;

            public StepMotorStatus() 
            {
                rangeMin = 0;
                rangeMax = 0;
                currPos = 0;
                maxSpeed = 0;
                minSpeed = 0;
                accelStep = 0;

                motorWait = 0;
                piStatus = 0;
                piHitRefPos = 0;
                piHitMargin = 0;
                piHitLastPos = 0;
                piHitPosError = 0;

                limitRange = new int[2];
                limitStatus = new int[2];

                sliderStepSize = 0;
                sliderPageSize = 0;
                rangeMinValue = 0.0f;
                rangeMaxValue = 0.0f;
            }
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