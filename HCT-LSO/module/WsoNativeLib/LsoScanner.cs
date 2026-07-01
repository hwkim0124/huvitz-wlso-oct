using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Net.Mail;

namespace WsoNativeLib
{
    using static LibraryConfig;
    using static WsoLsoDefs;
    using static WsoLsoScan;

    public static class LsoScanner
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainLsoScannerControlParam(int patternId, ref LsoScannerControlParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool submitLsoScannerControlParam(int patternId, LsoScannerControlParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool obtainLsoScannerCaptureParam(int patternId, ref LsoScannerCaptureParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool submitLsoScannerCaptureParam(int patternId, LsoScannerCaptureParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveLsoScannerYposition(int ypos);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool controlLsoScannerCapture(int patternId, int onOff);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setLsoScannerTriggerMode(int onOff);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startLsoScannerGrabbing(int patternId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool pauseLsoScannerGrabbing(int patternId);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool ObtainLsoScannerControlParam(int patternId, ref LsoScannerControlParam param)
        {
            return obtainLsoScannerControlParam(patternId, ref param);
        }

        public static bool SubmitLsoScannerControlParam(int patternId, LsoScannerControlParam param)
        {
            return submitLsoScannerControlParam(patternId, param);
        }

        public static bool ObtainLsoScannerCaptureParam(int patternId, ref LsoScannerCaptureParam param)
        {
            return obtainLsoScannerCaptureParam(patternId, ref param);
        }

        public static bool SubmitLsoScannerCaptureParam(int patternId, LsoScannerCaptureParam param)
        {
            return submitLsoScannerCaptureParam(patternId, param);
        }

        public static bool MoveLsoScannerYposition(int ypos)
        {
            return moveLsoScannerYposition(ypos);
        }

        public static bool ControlLsoScannerCapture(int patternId, int onOff)
        {
            return controlLsoScannerCapture(patternId, onOff);
        }

        public static bool SetLsoScannerTriggerMode(int onOff)
        {
            return setLsoScannerTriggerMode(onOff);
        }

        public static bool StartLsoScannerGrabbing(int patternId)
        {
            return startLsoScannerGrabbing(patternId);
        }

        public static bool PauseLsoScannerGrabbing(int patternId)
        {
            return pauseLsoScannerGrabbing(patternId);
        }
    }
}
