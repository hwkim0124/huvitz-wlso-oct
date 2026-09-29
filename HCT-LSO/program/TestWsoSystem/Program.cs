using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using WsoNativeLib;


namespace HelloWorld
{
    using static WsoNativeLib.WsoDevice;

    class Program
    {
        static WsoCallback.WsoLogMsgCallback logMsgCallback = new WsoCallback.WsoLogMsgCallback(CallbackLogMsg);

        static int Main(string[] args)
        {
            if (args.Length > 0 && args[0] == "--pupil")
            {
                // Offline pupil-detector harness: no hardware, no Bootstrapper.
                string dir = args.Length > 1 ? args[1] : ".\\cornea";
                return RunPupilHarness(dir);
            }

            Console.WriteLine("Hello World!");
            Bootstrapper.InitializeWsoSystem(clb: logMsgCallback, true);
            Bootstrapper.ReleaseWsoSystem();
            return 0;
        }

        static void CallbackLogMsg(string msg, int type)
        {
            Console.WriteLine($"WsoSystem Log: {msg}");
        }

        // Runs EyeAlign.DetectPupilInImage over every PNG in dir. Files saved by the
        // toolkit's CorneaCameraPreview.SaveFrameImage() are named
        // "<Left|Right|Lower>_yyMMdd_HHmmss.png", which selects the camera. While the
        // detector is a stub every result reads found=0; the harness is the fixture the
        // real algorithm will be validated with.
        static int RunPupilHarness(string dir)
        {
            if (!Directory.Exists(dir))
            {
                Console.Error.WriteLine($"Directory not found: {dir}");
                return 2;
            }

            var files = Directory.GetFiles(dir, "*.png");
            Array.Sort(files, StringComparer.OrdinalIgnoreCase);
            if (files.Length == 0)
            {
                Console.Error.WriteLine($"No PNG files in {dir}");
                return 2;
            }

            Console.WriteLine($"{"file",-40} {"camera",-14} {"found",-5} {"cx",8} {"cy",8} {"r",7} {"score",6}");
            int failures = 0;
            foreach (var file in files)
            {
                var camera = CameraFromFileName(Path.GetFileName(file));
                byte[] gray = LoadGray8(file, out int width, out int height);

                GCHandle handle = GCHandle.Alloc(gray, GCHandleType.Pinned);
                try
                {
                    if (!EyeAlign.DetectPupilInImage(handle.AddrOfPinnedObject(), width, height, camera, out var result))
                    {
                        Console.WriteLine($"{Path.GetFileName(file),-40} {camera,-14} call failed");
                        failures++;
                        continue;
                    }
                    Console.WriteLine($"{Path.GetFileName(file),-40} {camera,-14} {result.found,-5} {result.cx,8:F1} {result.cy,8:F1} {result.radius,7:F1} {result.score,6:F2}");
                }
                finally
                {
                    handle.Free();
                }
            }
            return failures == 0 ? 0 : 1;
        }

        static CameraType CameraFromFileName(string name)
        {
            if (name.StartsWith("Left", StringComparison.OrdinalIgnoreCase)) return CameraType.IrCorneaLeft;
            if (name.StartsWith("Right", StringComparison.OrdinalIgnoreCase)) return CameraType.IrCorneaRight;
            return CameraType.IrCorneaLower;
        }

        // Decodes a PNG into a tightly packed 8-bit grayscale buffer (stride == width).
        static byte[] LoadGray8(string path, out int width, out int height)
        {
            using var stream = File.OpenRead(path);
            var decoder = new PngBitmapDecoder(stream, BitmapCreateOptions.PreservePixelFormat, BitmapCacheOption.OnLoad);
            BitmapSource frame = decoder.Frames[0];
            if (frame.Format != PixelFormats.Gray8)
            {
                frame = new FormatConvertedBitmap(frame, PixelFormats.Gray8, null, 0);
            }
            width = frame.PixelWidth;
            height = frame.PixelHeight;
            var pixels = new byte[width * height];
            frame.CopyPixels(pixels, width, 0);
            return pixels;
        }
    }
}
