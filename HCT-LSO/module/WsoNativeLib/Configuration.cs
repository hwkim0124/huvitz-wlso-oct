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
    using static WsoNativeLib.WsoEyeAlign;

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
        private static extern bool obtainRetinaCameraConfigParam(ref RetinaCameraConfigParam param);

        [DllImport(LibraryName)]
        private static extern void submitInternalFixationPreset(InternalFixationPreset param);

        [DllImport(LibraryName)]
        private static extern void submitCorneaCameraConfigParam(CorneaCameraConfigParam param);

        [DllImport(LibraryName)]
        private static extern void submitRetinaCameraConfigParam(RetinaCameraConfigParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainEyeAlignConfigParam(ref EyeAlignConfigParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool submitEyeAlignConfigParam(in EyeAlignConfigParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool importSystemConfigFile();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool exportSystemConfigFile();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool loadSystemConfiguration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool saveSystemConfiguration();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool applySystemConfiguration();


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

        public static bool ObtainRetinaCameraConfig(out RetinaCameraConfigParam param)
        {
            param = new RetinaCameraConfigParam();
            return obtainRetinaCameraConfigParam(ref param);
        }

        public static void SubmitInternalFixationPreset(InternalFixationPreset param)
        {
            submitInternalFixationPreset(param);
        }
        public static void SubmitCorneaCameraConfig(CorneaCameraConfigParam param)
        {
            submitCorneaCameraConfigParam(param);
        }

        public static void SubmitRetinaCameraConfig(RetinaCameraConfigParam param)
        {
            submitRetinaCameraConfigParam(param);
        }

        public static bool ObtainEyeAlignConfig(out EyeAlignConfigParam param)
        {
            param = new EyeAlignConfigParam();
            return obtainEyeAlignConfigParam(ref param);
        }

        public static bool SubmitEyeAlignConfig(in EyeAlignConfigParam param)
        {
            return submitEyeAlignConfigParam(in param);
        }

        public static bool ImportSystemConfigFile()
        {
            return importSystemConfigFile();
        }

        public static bool ExportSystemConfigFile()
        {
            return exportSystemConfigFile();
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

    }
}
