using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;


namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoDomain;
    using static WsoNativeLib.WsoDevice;

    public static class Fixation
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOnInternalFixation(int row, int col);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOnInternalFixationWithTarget(int side, int target);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool turnOffInternalFixation();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getCurrentInternalFixation(out int row, out int col);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setupInternalFixation(InternalFixationParam param);

        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool TurnOnInternalFixation(int row, int col)
        {
            return turnOnInternalFixation(row, col);
        }

        public static bool TurnOnInternalFixationWithTarget(EyeSide side, FixationTarget target)
        {
            return turnOnInternalFixationWithTarget((int)side, (int)target);
        }

        public static bool TurnOffInternalFixation()
        {
            return turnOffInternalFixation();
        }

        public static bool GetCurrentInternalFixation(out int row, out int col)
        {
            return getCurrentInternalFixation(out row, out col);
        }

        public static bool SetupInternalFixation(InternalFixationParam param)
        {
            return setupInternalFixation(param);
        }
    }
}
