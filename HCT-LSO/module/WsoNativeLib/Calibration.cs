using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using static WsoNativeLib.WsoBoard;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;

    public static class Calibration
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        //////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        private static extern bool fetchSystemCalibration(out SystemCalibration calib, [MarshalAs(UnmanagedType.I1)] bool reload);

        [DllImport(LibraryName)]
        private static extern bool applySystemCalibration(ref SystemCalibration calib, [MarshalAs(UnmanagedType.I1)] bool write);


        // Public methods
        //////////////////////////////////////////////////////////////////////////
        public static bool FetchSystemCalibration(out SystemCalibration calib, bool reload)
        {
            return fetchSystemCalibration(out calib, reload);
        }

        public static bool ApplySystemCalibration(ref SystemCalibration calib, bool write)
        {
            return applySystemCalibration(ref calib, write);
        }
    }
}
