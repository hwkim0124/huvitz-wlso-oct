using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;
using System.Runtime.InteropServices;


namespace WsoNativeLib2
{
    public static class WsoAnalytics
    {
        const string LibraryName = "./system_libs/wsoanalytics.dll";

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool initializeWsoAnalytics();


        public static bool InitializeWsoAnalytics()
        {
            DumpDllSearchContext();
            initializeWsoAnalytics();
            return true;
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
                Path.Combine(AppContext.BaseDirectory, "wsoanalytics.dll"),
                Path.Combine(AppContext.BaseDirectory, "system_libs", "wsoanalytics.dll"),
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)!, "wsoanalytics.dll"),
            };
            foreach (var c in candidates)
                Console.WriteLine($"{(File.Exists(c) ? "FOUND " : "miss  ")} {c}");
        }


    }
}
