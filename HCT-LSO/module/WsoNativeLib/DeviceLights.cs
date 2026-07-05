using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoDevice;
    using static WsoNativeLib.WsoLights;

    public static class DeviceLights
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOnLight(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOffLight(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isLightOn(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOnOctSld();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOffOctSld();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctSldOn();


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainAdcSensorStatus(ref AdcSensorStatus status, [MarshalAs(UnmanagedType.I1)] bool reload);


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainOctSldStatusParam(ref OctSldStatusParam status, [MarshalAs(UnmanagedType.I1)] bool reload);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainOctSldCalibParam(ref OctSldCalibParam param, [MarshalAs(UnmanagedType.I1)] bool reload);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctSldHighCode(int value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctSldLowCode1(int value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctSldLowCode2(int value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setOctSldRsiCode(int value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool saveOctSldCalibration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool loadOctSldCalibration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setLightMode(int type, ushort mode);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setLightIntensity(int type, ushort value);

        [DllImport(LibraryName)]
        private static extern ushort getLightMode(int type);

        [DllImport(LibraryName)]
        private static extern ushort getLightIntensity(int type);


        public static bool TurnOnLight(LightType type)
        {
            return turnOnLight((int)type);
        }

        public static bool TurnOffLight(LightType type)
        {
            return turnOffLight((int)type);
        }

        public static bool IsLightOn(LightType type)
        {
            return isLightOn((int)type);
        }

        public static bool TurnOnOctSld()
        {
            return turnOnOctSld();
        }

        public static bool TurnOffOctSld()
        {
            return turnOffOctSld();
        }

        public static bool IsOctSldOn()
        {
            return isOctSldOn();
        }

        public static bool ObtainAdcSensorStatus(ref AdcSensorStatus status, bool reload = false)
        {
            return obtainAdcSensorStatus(ref status, reload);
        }

        public static bool ObtainOctSldStatusParam(ref OctSldStatusParam status, bool reload)
        {
            return obtainOctSldStatusParam(ref status, reload);
        }

        public static bool ObtainOctSldCalibParam(ref OctSldCalibParam param, bool reload)
        {
            return obtainOctSldCalibParam(ref param, reload);
        }

        public static bool SetOctSldHighCode(int value)
        {
            return setOctSldHighCode(value);
        }

        public static bool SetOctSldLowCode1(int value)
        {
            return setOctSldLowCode1(value);
        }

        public static bool SetOctSldLowCode2(int value)
        {
            return setOctSldLowCode2(value);
        }

        public static bool SetOctSldRsiCode(int value)
        {
            return setOctSldRsiCode(value);
        }

        public static bool SaveOctSldCalibration()
        {
            return saveOctSldCalibration();
        }

        public static bool LoadOctSldCalibration()
        {
            return loadOctSldCalibration();
        }

        public static bool SetLightMode(LightType type, LightMode mode)
        {
            return setLightMode((int)type, (ushort)mode);
        }
        public static bool SetLightIntensity(LightType type, ushort value)
        {
            return setLightIntensity((int)type, value);
        }

        public static LightMode GetLightMode(LightType type)
        {
            return (LightMode)getLightMode((int)type);
        }

        public static ushort GetLightIntensity(LightType type)
        {
            return getLightIntensity((int)type);
        }
    }
}
