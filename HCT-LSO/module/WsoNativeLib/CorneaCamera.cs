using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Printing;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoCallback;
    using static WsoNativeLib.WsoDevice;

    public static class CorneaCamera
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startCorneaCameraPreview(int type, CorneaCameraFrameCaptured clb);

        [DllImport(LibraryName)]
        private static extern void closeCorneaCameraPreview(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isCorneaCameraPreviewing(int type);

        [DllImport(LibraryName)]
        private static extern float getCorneaCameraAgain(int type);

        [DllImport(LibraryName)]
        private static extern float getCorneaCameraDgain(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraAgain(int type, float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraDgain(int type, float value);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool StartPreview(CameraType type, CorneaCameraFrameCaptured clb)
        {
            return startCorneaCameraPreview((int)type, clb);
        }

        public static void ClosePreview(CameraType type)
        {
            closeCorneaCameraPreview((int)type);
        }

        public static bool IsPreviewing(CameraType type)
        {
            return isCorneaCameraPreviewing((int)type);
        }

        public static float GetAnalogGain(CameraType type)
        {
            return getCorneaCameraAgain((int)type);
        }

        public static float GetDigitalGain(CameraType type)
        {
            return getCorneaCameraDgain((int)type);
        }

        public static bool SetAnalogGain(CameraType type, float value)
        {
            return setCorneaCameraAgain((int)type, value);
        }

        public static bool SetDigitalGain(CameraType type, float value)
        {
            return setCorneaCameraDgain((int)type, value);
        }
    }
}
