using System;
using System.Runtime.InteropServices;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoCallback;
    using static WsoNativeLib.WsoDevice;
    using static WsoNativeLib.WsoDomain;
    using static WsoNativeLib.WsoEyeAlign;

    /// <summary>
    /// Auto alignment of the optical head to the eye (native EyeAlign module through
    /// WsoSystem). Everything that happens during a run arrives through the single
    /// <see cref="EyeAlignEventOccurred"/> callback; the pupil position is polled with
    /// <see cref="GetPupilPosition"/>.
    /// <para>
    /// Callback contract: the delegate is invoked on a native dispatcher thread. Keep the
    /// delegate alive in a field, marshal to the Dispatcher and return immediately. A
    /// measurement started on <c>FocusAligned</c> must call <see cref="HoldAlignment"/>
    /// first, run the capture on a Task, and call <see cref="ResumeAlignment"/> when done.
    /// Never call <see cref="CancelAlignment"/> synchronously from inside the callback.
    /// </para>
    /// </summary>
    public static class EyeAlign
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        private static extern int startEyeAlignment(int mode, int eyeSide, EyeAlignEventOccurred clbEvent);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cancelEyeAlignment();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool holdEyeAlignment();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool resumeEyeAlignment();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isEyeAlignmentHeld();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isEyeAligning();

        [DllImport(LibraryName)]
        private static extern int getEyeAlignMode();

        [DllImport(LibraryName)]
        private static extern int getEyeAlignPhase();

        [DllImport(LibraryName)]
        private static extern int getEyeAlignResult();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool fetchEyeAlignStatus(ref EyeAlignStatus status);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isPupilDetected(int camId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getPupilPosition(int camId, out float cx, out float cy, out float radius, out float score);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool fetchPupilDetectResult(int camId, ref PupilDetectResult result);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool detectPupilInImage(nint gray, int width, int height, int camId, ref PupilDetectResult result);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setPupilDetectOverride(int camId, [MarshalAs(UnmanagedType.I1)] bool found, float cx, float cy, float radius);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool clearPupilDetectOverride(int camId);

        [DllImport(LibraryName)]
        private static extern int getEyeAlignStatusStructSize();

        [DllImport(LibraryName)]
        private static extern int getEyeAlignConfigStructSize();


        private static bool _structSizesVerified = false;

        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////

        /// <summary>
        /// Asserts that the C# mirrors of the native structs have the native size. Called
        /// once by <see cref="StartAlignment"/>; throws on a mismatch so a layout drift is
        /// caught before any pointer is handed across the boundary.
        /// </summary>
        public static void VerifyStructSizes()
        {
            if (_structSizesVerified)
            {
                return;
            }
            int nativeStatus = getEyeAlignStatusStructSize();
            int nativeConfig = getEyeAlignConfigStructSize();
            int managedStatus = Marshal.SizeOf<EyeAlignStatus>();
            int managedConfig = Marshal.SizeOf<EyeAlignConfigParam>();
            if (nativeStatus != managedStatus)
            {
                throw new InvalidOperationException($"EyeAlignStatus size mismatch: native {nativeStatus}, managed {managedStatus}");
            }
            if (nativeConfig != managedConfig)
            {
                throw new InvalidOperationException($"EyeAlignConfigParam size mismatch: native {nativeConfig}, managed {managedConfig}");
            }
            _structSizesVerified = true;
        }

        /// <summary>
        /// Starts an alignment run. Returns <see cref="EyeAlignResult.None"/> on success,
        /// otherwise the refusal reason (busy, preview not running, motors not ready, ...).
        /// SINGLE and MANUAL use <paramref name="eyeSide"/>; FULL always starts with OD.
        /// </summary>
        public static EyeAlignResult StartAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback)
        {
            VerifyStructSizes();
            return (EyeAlignResult)startEyeAlignment((int)mode, (int)eyeSide, callback);
        }

        /// <summary>Stops all axes and ends the run. Blocks until the run has finished.</summary>
        public static bool CancelAlignment()
        {
            return cancelEyeAlignment();
        }

        /// <summary>Freezes the motors (detection and events continue). Call before a measurement.</summary>
        public static bool HoldAlignment()
        {
            return holdEyeAlignment();
        }

        /// <summary>Leaves the held state; following resumes from the Z-centring step.</summary>
        public static bool ResumeAlignment()
        {
            return resumeEyeAlignment();
        }

        public static bool IsHeld()
        {
            return isEyeAlignmentHeld();
        }

        public static bool IsAligning()
        {
            return isEyeAligning();
        }

        public static EyeAlignMode GetMode()
        {
            return (EyeAlignMode)getEyeAlignMode();
        }

        public static EyeAlignPhase GetPhase()
        {
            return (EyeAlignPhase)getEyeAlignPhase();
        }

        public static EyeAlignResult GetResult()
        {
            return (EyeAlignResult)getEyeAlignResult();
        }

        public static bool FetchStatus(out EyeAlignStatus status)
        {
            status = new EyeAlignStatus();
            return fetchEyeAlignStatus(ref status);
        }

        public static bool IsPupilDetected(CameraType camera)
        {
            return isPupilDetected((int)camera);
        }

        /// <summary>
        /// Latest pupil position of a cornea camera in image pixels. Returns false when no
        /// pupil is detected or the last result is stale.
        /// </summary>
        public static bool GetPupilPosition(CameraType camera, out float cx, out float cy, out float radius, out float score)
        {
            return getPupilPosition((int)camera, out cx, out cy, out radius, out score);
        }

        public static bool FetchPupilDetectResult(CameraType camera, out PupilDetectResult result)
        {
            result = new PupilDetectResult();
            return fetchPupilDetectResult((int)camera, ref result);
        }

        /// <summary>Runs the detector on a caller-owned 8-bit grayscale buffer (offline harness).</summary>
        public static bool DetectPupilInImage(nint gray, int width, int height, CameraType camera, out PupilDetectResult result)
        {
            result = new PupilDetectResult();
            return detectPupilInImage(gray, width, height, (int)camera, ref result);
        }

        /// <summary>Test hook: substitutes the detector output of a camera while set.</summary>
        public static bool SetPupilDetectOverride(CameraType camera, bool found, float cx, float cy, float radius)
        {
            return setPupilDetectOverride((int)camera, found, cx, cy, radius);
        }

        public static bool ClearPupilDetectOverride(CameraType camera)
        {
            return clearPupilDetectOverride((int)camera);
        }

        public static void ClearAllPupilDetectOverrides()
        {
            clearPupilDetectOverride((int)CameraType.IrCorneaLeft);
            clearPupilDetectOverride((int)CameraType.IrCorneaRight);
            clearPupilDetectOverride((int)CameraType.IrCorneaLower);
        }

        /// <summary>Readable name of an event for logs and the tool's event list.</summary>
        public static string EventName(EyeAlignEvent evt)
        {
            return evt.ToString();
        }

        public static bool IsTerminalEvent(EyeAlignEvent evt)
        {
            return evt == EyeAlignEvent.AlignCancelled || evt == EyeAlignEvent.AlignFailed;
        }
    }
}
