using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;


namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoCallback;
    using static WsoNativeLib.WsoDevice;
    using static WsoNativeLib.WsoDomain;

    public static class BoardDevice
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        private static extern void connectJoystickButtonPressed(JoystickButtonPressed clb);

        [DllImport(LibraryName)]
        private static extern void connectOptimizeButtonPressed(OptimizeButtonPressed clb);

        [DllImport(LibraryName)]
        private static extern void connectAdapterLensAttached(AdapterLensAttached clb);

        [DllImport(LibraryName)]
        private static extern void releaseJoystickButtonPressed();

        [DllImport(LibraryName)]
        private static extern void releaseOptimizeButtonPressed();

        [DllImport(LibraryName)]
        private static extern void releaseAdapterLensAttached();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getDeviceSideOnPatient(ref int side);

        [DllImport(LibraryName)]
        private static extern int getCurrentEyeSide();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOdOsStatusFlagSet();


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static void ConnectJoystickButtonPressed(JoystickButtonPressed clb)
        {
            connectJoystickButtonPressed(clb);
        }

        public static void ConnectOptimizeButtonPressed(OptimizeButtonPressed clb)
        {
            connectOptimizeButtonPressed(clb);
        }

        public static void ConnectAdapterLensAttached(AdapterLensAttached clb)
        {
            connectAdapterLensAttached(clb);
        }

        public static void ReleaseJoystickButtonPressed()
        {
            releaseJoystickButtonPressed();
        }

        public static void ReleaseOptimizeButtonPressed()
        {
            releaseOptimizeButtonPressed();
        }

        public static void ReleaseAdapterLensAttached()
        {
            releaseAdapterLensAttached();
        }

        public static EyeSide GetDeviceSideOnPatient()
        {
            return IsEyeSideOD() ? EyeSide.OD : EyeSide.OS;
        }

        public static EyeSide GetCurrentEyeSide()
        {
            return (EyeSide)getCurrentEyeSide();
        }

        public static bool IsEyeSideOD()
        {
            if (GetCurrentEyeSide() == EyeSide.OD)
            {
                return true;
            }
            return false;
        }

        public static bool IsOdOsStatusFlagSet()
        {
            return isOdOsStatusFlagSet();
        }
    }
}
