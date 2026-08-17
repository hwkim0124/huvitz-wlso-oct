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
        private static extern bool setCorneaCameraViewPreset(int preset);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startCorneaCameraPreview(CorneaCameraFrameCaptured? clbLeft, CorneaCameraFrameCaptured? clbRight, CorneaCameraFrameCaptured? clbLower);

        [DllImport(LibraryName)]
        private static extern void closeCorneaCameraPreview();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isCorneaCameraPreviewing();

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
        public static bool SetCameraViewPreset(IrCameraViewPreset preset)
        {
            return setCorneaCameraViewPreset((int)preset);
        }

        public static bool StartPreview(CorneaCameraFrameCaptured? clbLeft, CorneaCameraFrameCaptured? clbRight, CorneaCameraFrameCaptured? clbLower)
        {
            return startCorneaCameraPreview(clbLeft, clbRight, clbLower);
        }

        public static void ClosePreview()
        {
            closeCorneaCameraPreview();
        }

        public static bool IsPreviewing()
        {
            return isCorneaCameraPreviewing();
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
