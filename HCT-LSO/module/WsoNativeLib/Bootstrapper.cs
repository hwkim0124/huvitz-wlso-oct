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
            DumpDllSearchContext();
            releaseWsoLogMsgCallback();
            return initializeWsoSystem(clb, trace_mode);
        }

        static void DumpDllSearchContext()
        {
            Console.WriteLine($"Process:           {Environment.ProcessPath}");
            Console.WriteLine($"Process arch:      {RuntimeInformation.ProcessArchitecture}");
            Console.WriteLine($"AppContext.BaseDir: {AppContext.BaseDirectory}");
            Console.WriteLine($"CurrentDirectory:  {Environment.CurrentDirectory}");
            Console.WriteLine($"Assembly location: {Assembly.GetExecutingAssembly().Location}");
            Console.WriteLine($"Assembly dir:      {Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)}");
            Console.WriteLine();
            Console.WriteLine("PATH entries:");
            foreach (var p in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(';'))
                Console.WriteLine($"  {p}");
            Console.WriteLine();
            // Check the obvious candidate locations for the actual file
            var candidates = new[]
            {
                Path.Combine(AppContext.BaseDirectory, "wsosystem.dll"),
                Path.Combine(AppContext.BaseDirectory, "system_libs", "wsosystem.dll"),
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)!, "wsosystem.dll"),
                WsoSystemDllPath
            };
            foreach (var c in candidates)
                Console.WriteLine($"{(File.Exists(c) ? "FOUND " : "miss  ")} {c}");
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
