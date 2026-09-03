using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Printing;
using System.Security.RightsManagement;

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
        private static extern float getCorneaCameraExposureTime(int type);


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraAgain(int type, float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraDgain(int type, float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraExposureTime(int type, float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraOutputMode(int type, int mode, int pattern, int option);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setCorneaCameraMirrorMode(int type, int horzFlip, int vertFlip);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getCorneaCameraMirrorMode(int type, out int horzFlip, out int vertFlip);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getCorneaCameraOutputMode(int type, out int mode, out int pattern, out int option);


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

        public static float GetExposureTime(CameraType type)
        {
            return getCorneaCameraExposureTime((int)type);
        }

        public static bool SetAnalogGain(CameraType type, float value)
        {
            return setCorneaCameraAgain((int)type, value);
        }

        public static bool SetDigitalGain(CameraType type, float value)
        {
            return setCorneaCameraDgain((int)type, value);
        }

        public static bool SetExposureTime(CameraType type, float value)
        {
            return setCorneaCameraExposureTime((int)type, value);
        }

        public static bool SetOutputMode(CameraType type, IrCameraOutputMode mode, IrCameraTestPattern pattern, int option)
        {
            return setCorneaCameraOutputMode((int)type, (int)mode, (int)pattern, option);
        }

        public static bool SetMirrorMode(CameraType type, IrCameraMirrorMode horzFlip, IrCameraMirrorMode vertFlip)
        {
            return setCorneaCameraMirrorMode((int)type, (int)horzFlip, (int)vertFlip);
        }

        public static bool GetMirrorMode(CameraType type, out IrCameraMirrorMode horzFlip, out IrCameraMirrorMode vertFlip)
        {
            int horz, vert;
            bool result = getCorneaCameraMirrorMode((int)type, out horz, out vert);
            horzFlip = (IrCameraMirrorMode)horz;
            vertFlip = (IrCameraMirrorMode)vert;
            return result;
        }

        public static bool GetOutputMode(CameraType type, out IrCameraOutputMode mode, out IrCameraTestPattern pattern, out int option)
        {
            int m, p, o;
            bool result = getCorneaCameraOutputMode((int)type, out m, out p, out o);
            mode = (IrCameraOutputMode)m;
            pattern = (IrCameraTestPattern)p;
            option = (int)o;
            return result;
        }

    }
}
