using System;
using System.IO;
using System.Windows.Media.Imaging;

namespace WsoToolkit.utils
{
    internal static class LsoRoiImageWriter
    {
        internal static void Save(BitmapSource crop, string path)
        {
            string extension = Path.GetExtension(path);
            BitmapEncoder encoder = extension.Equals(".tif", StringComparison.OrdinalIgnoreCase)
                || extension.Equals(".tiff", StringComparison.OrdinalIgnoreCase)
                ? new TiffBitmapEncoder() : new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(crop));
            using var stream = File.Create(path);
            encoder.Save(stream);
        }
    }
}
