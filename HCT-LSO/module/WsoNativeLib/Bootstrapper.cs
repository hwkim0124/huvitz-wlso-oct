using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;


namespace WsoNativeLib
{
    using static WsoNativeLib.WsoCallback;
    using static WsoNativeLib.LibraryConfig;

    public static class Bootstrapper
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool initializeWsoSystem(WsoLogMsgCallback clb, [MarshalAs(UnmanagedType.I1)] bool trace_mode);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isWsoSystemInitialized();

        [DllImport(LibraryName)]
        private static extern void releaseWsoSystem();

        [DllImport(LibraryName)]
        private static extern void connectWsoLogMsgCallback(WsoLogMsgCallback clb);

        [DllImport(LibraryName)]
        private static extern void releaseWsoLogMsgCallback();


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool InitializeWsoSystem(WsoLogMsgCallback clb, bool trace_mode)
        {
            releaseWsoLogMsgCallback();
            return initializeWsoSystem(clb, trace_mode);
        }

        public static bool IsWsoSystemInitialized()
        {
            return isWsoSystemInitialized();
        }

        public static void ReleaseWsoSystem()
        {
            releaseWsoSystem();
        }

        public static void ConnectWsoLogMsgCallback(WsoLogMsgCallback clb)
        {
            connectWsoLogMsgCallback(clb);
        }

        public static void ReleaseWsoLogMsgCallback()
        {
            releaseWsoLogMsgCallback();
        }
    }
}
