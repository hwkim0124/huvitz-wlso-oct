using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Threading.Tasks;


namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoCallback;
    using static WsoNativeLib.WsoDevice;

    public static class RetinaCamera
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startRetinaCameraPreview(RetinaCameraFrameCaptured? clbFrame);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool closeRetinaCameraPreview();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isRetinaCameraPreviewing();

        [DllImport(LibraryName)]
        private static extern float getRetinaCameraAgain();

        [DllImport(LibraryName)]
        private static extern float getRetinaCameraDgain();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setRetinaCameraAgain(float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setRetinaCameraDgain(float value);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool StartPreview(RetinaCameraFrameCaptured? clbFrame)
        {
            return startRetinaCameraPreview(clbFrame);
        }

        public static bool ClosePreview()
        {
            return closeRetinaCameraPreview();
        }

        public static bool IsPreviewing()
        {
            return isRetinaCameraPreviewing();
        }

        public static float getAnalogGain()
        {
            return getRetinaCameraAgain();
        }

        public static float getDigitalGain()
        {
            return getRetinaCameraDgain();
        }
        
        public static bool setAnalogGain(float value)
        {
            return setRetinaCameraAgain(value);
        }
        
        public static bool setDigitalGain(float value)
        {
            return setRetinaCameraDgain(value);
        }
    }
}
