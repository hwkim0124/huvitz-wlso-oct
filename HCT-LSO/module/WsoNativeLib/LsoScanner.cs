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
        private static extern bool setLsoScannerTrajectory(int patternId, short[]? pos, int count);

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
        private static extern bool setLsoScanSequence(int seqMode, int preMoveOffset, int preMoveDelayUs);

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

        // Installs (or with null/empty clears) the per-slot pos[] override the native side
        // copies into the board profile on every submit instead of the linear sweep. The
        // length has to equal the profile's sampleSize when that submit happens.
        public static bool SetLsoScannerTrajectory(int patternId, short[]? pos)
        {
            return setLsoScannerTrajectory(patternId, pos, pos?.Length ?? 0);
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

        // Which entity starts a scan frame. The board keeps ONE setting - the command has no
        // pattern id - so the caller that starts an acquisition sends the sequence of the mode
        // it is about to run, rather than assuming the board still holds it.
        // preMoveOffset / preMoveDelayUs only apply to GALVANO_FIRST; the native side zeroes
        // them for TRIG_FIRST.
        public static bool SetLsoScanSequence(LsoScanSequenceMode mode, short preMoveOffset, ushort preMoveDelayUs)
        {
            return setLsoScanSequence((int)mode, preMoveOffset, preMoveDelayUs);
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
