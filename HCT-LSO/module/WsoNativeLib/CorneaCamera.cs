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

    public static class CorneaCamera
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startCorneaCameraPreview(CorneaCameraFrameCaptured clb);

        [DllImport(LibraryName)]
        private static extern void closeCorneaCameraPreview();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isCorneaCameraPreviewing();

        [DllImport(LibraryName)]
        private static extern float getCorneaCameraAgain();

        [DllImport(LibraryName)]
        private static extern float getCorneaCameraDgain();

        [DllImport(LibraryName)]
        private static extern int getWorkingDotIntensity(int index);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraAgain(float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraDgain(float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setWorkingDotIntensity(int index, int value);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool StartPreview(CorneaCameraFrameCaptured clb)
        {
            return startCorneaCameraPreview(clb);
        }

        public static void ClosePreview()
        {
            closeCorneaCameraPreview();
        }

        public static bool IsPreviewing()
        {
            return isCorneaCameraPreviewing();
        }

        public static float GetAnalogGain()
        {
            return getCorneaCameraAgain();
        }

        public static float GetDigitalGain()
        {
            return getCorneaCameraDgain();
        }

        public static bool SetAnalogGain(float value)
        {
            return setCorneaCameraAgain(value);
        }

        public static bool SetDigitalGain(float value)
        {
            return setCorneaCameraDgain(value);
        }
        public static int GetWorkingDotIntensity(int index)
        {
            return getWorkingDotIntensity(index);
        }
        public static bool SetWorkingDotIntensity(int index, int value)
        {
            return setWorkingDotIntensity(index, value);
        }
    }
}
