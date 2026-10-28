using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using OpenCvSharp;

namespace WsoNativeLib
{
    // Sensor values stay right-aligned (0..4095), not scaled to the TIFF container range.
    public sealed class CameraHighDepthData
    {
        public int Width { get; }
        public int Height { get; }
        public int BitDepth { get; }
        // Full-scale sensor value of this frame (4095 for the 12-bit formats).
        public int MaxDepthValue => (1 << BitDepth) - 1;
        public bool IsMono { get; }
        public ushort[] Raw16 { get; }

        public CameraHighDepthData(byte[] unpacked, int width, int height, int bitDepth = 12, bool isMono = false)
        {
            Width = width;
            Height = height;
            BitDepth = bitDepth;
            IsMono = isMono;
            Raw16 = new ushort[checked(width * height)];
            if (unpacked.Length != checked(Raw16.Length * 2))
                throw new ArgumentException("Expected a tightly packed uint16 camera frame.", nameof(unpacked));
            Buffer.BlockCopy(unpacked, 0, Raw16, 0, unpacked.Length);
        }

        // FLIR/Spinnaker BayerRG12Packed, NOT BayerRG12p. A trailing odd pixel
        // uses B0 + the high nibble of B1; rows may have SDK padding.
        public static ushort[] Unpack12Packed(byte[] source, int width, int height, int stride)
        {
            int rowBytes = checked((width * 12 + 7) / 8);
            if (width <= 0 || height <= 0 || stride < rowBytes
                || source.Length < checked((height - 1) * stride + rowBytes))
                throw new ArgumentException("Invalid packed camera geometry.");
            var result = new ushort[checked(width * height)];
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    int b = y * stride + (x / 2) * 3;
                    result[y * width + x] = (ushort)((x & 1) == 0
                        ? (source[b] << 4) | (source[b + 1] >> 4)
                        : (source[b + 2] << 4) | (source[b + 1] & 15));
                }
            }
            return result;
        }

        public byte[] ToDisplay8()
        {
            var display = new byte[Raw16.Length];
            for (int i = 0; i < display.Length; ++i)
                display[i] = (byte)(Raw16[i] >> (BitDepth - 8));
            return display;
        }

        public ushort[] DebayerRgb48()
        {
            using var raw = new Mat(Height, Width, MatType.CV_16UC1);
            var bytes = new byte[Raw16.Length * 2];
            Buffer.BlockCopy(Raw16, 0, bytes, 0, bytes.Length);
            Marshal.Copy(bytes, 0, raw.Data, bytes.Length);
            using var rgb = new Mat();
            // GenICam RGGB is OpenCV BayerBG (same mapping as the Agent).
            Cv2.CvtColor(raw, rgb, ColorConversionCodes.BayerBG2RGB);
            var rgbBytes = new byte[Raw16.Length * 6];
            Marshal.Copy(rgb.Data, rgbBytes, 0, rgbBytes.Length);
            var result = new ushort[Raw16.Length * 3];
            Buffer.BlockCopy(rgbBytes, 0, result, 0, rgbBytes.Length);
            return result;
        }

        public void SaveTiffCompanions(string pngPath)
        {
            string stem = Path.Combine(Path.GetDirectoryName(pngPath)!, Path.GetFileNameWithoutExtension(pngPath));
            SaveTiff(stem + "_raw16.tiff", Width, Height, Raw16, false);
            if (!IsMono)
                SaveTiff(stem + "_rgb48.tiff", Width, Height, DebayerRgb48(), true);
        }

        public static void SaveTiff(string path, int width, int height, ushort[] pixels, bool rgb)
        {
            int channels = rgb ? 3 : 1;
            if (pixels.Length != checked(width * height * channels))
                throw new ArgumentException("TIFF dimensions do not match the pixels.", nameof(pixels));
            BitmapSource bitmap = BitmapSource.Create(width, height, 96, 96,
                rgb ? PixelFormats.Rgb48 : PixelFormats.Gray16, null, pixels, checked(width * channels * 2));
            var encoder = new TiffBitmapEncoder { Compression = TiffCompressOption.None };
            encoder.Frames.Add(BitmapFrame.Create(bitmap));
            using var stream = File.Create(path);
            encoder.Save(stream);
        }
    }
}
