using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoBoard;
    using static WsoNativeLib.WsoConfig;

    public static class Configuration
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainInternalFixationPreset(ref InternalFixationPreset param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainCorneaCameraConfigParam(ref CorneaCameraConfigParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainOctGalvanoConfigParam(ref OctGalvanoConfigParam param);

        [DllImport(LibraryName)]
        private static extern void submitInternalFixationPreset(InternalFixationPreset param);

        [DllImport(LibraryName)]
        private static extern void submitCorneaCameraConfigParam(CorneaCameraConfigParam param);

        [DllImport(LibraryName)]
        private static extern void submitOctGalvanoConfigParam(OctGalvanoConfigParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool loadSystemConfigFile();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool saveSystemConfigFile();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool loadSystemConfiguration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool saveSystemConfiguration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool applySystemConfiguration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool applySystemCalibration();


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool ObtainInternalFixationPreset(out InternalFixationPreset param)
        {
            param = new InternalFixationPreset();
            return obtainInternalFixationPreset(ref param);
        }

        public static bool ObtainCorneaCameraConfig(out CorneaCameraConfigParam param)
        {
            param = new CorneaCameraConfigParam();
            return obtainCorneaCameraConfigParam(ref param);
        }

        public static bool ObtainOctGalvanoConfig(out OctGalvanoConfigParam param)
        {
            param = new OctGalvanoConfigParam();
            return obtainOctGalvanoConfigParam(ref param);
        }

        public static void SubmitInternalFixationPreset(InternalFixationPreset param)
        {
            submitInternalFixationPreset(param);
        }
        public static void SubmitCorneaCameraConfig(CorneaCameraConfigParam param)
        {
            submitCorneaCameraConfigParam(param);
        }
        
        public static void SubmitOctGalvanoConfig(OctGalvanoConfigParam param)
        {
            submitOctGalvanoConfigParam(param);
        }

        public static bool LoadSystemConfigFile()
        {
            return loadSystemConfigFile();
        }

        public static bool SaveSystemConfigFile()
        {
            return saveSystemConfigFile();
        }

        public static bool LoadSystemConfiguration()
        {
            return loadSystemConfiguration();
        }

        public static bool SaveSystemConfiguration()
        {
            return saveSystemConfiguration();
        }

        public static bool ApplySystemConfiguration()
        {
            return applySystemConfiguration();
        }

        public static bool ApplySystemCalibration()
        {
            return applySystemCalibration();
        }
    }
}
