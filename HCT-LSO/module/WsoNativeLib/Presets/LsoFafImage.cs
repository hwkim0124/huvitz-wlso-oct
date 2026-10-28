using System;
using System.Runtime.InteropServices;
using OpenCvSharp;

namespace WsoNativeLib.Presets
{
    // The DevKit's FAF review path (CreateFafGrayscaleBitmap): extract the emission from the
    // sensor buffer BEFORE the 8-bit display conversion when the high-depth frame is
    // available, then scale once to 8-bit gray. Without a high-depth frame the same weighted
    // composite is taken from the demosaiced 8-bit review image.
    public static class LsoFafImage
    {
        // high: this frame's CameraHighDepthData, or null when the camera delivered 8-bit.
        // color8: the review image (RGB CV_8UC3, or an already-gray CV_8UC1/CV_16UC1).
        // A high-depth Bayer frame yields half the input resolution (one pixel per 2x2 cell),
        // the same geometry LsoModeImageAccumulator produces.
        public static Mat ExtractDisplay8(CameraHighDepthData? high, Mat color8, LsoModeProcessing rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException(nameof(rule));
            }

            if (color8 == null)
            {
                throw new ArgumentNullException(nameof(color8));
            }

            if (high != null)
            {
                float[] gray = LsoModeImageAccumulator.ToGray(high.Raw16, high.Width, high.Height, rule);
                return ToDisplay8(gray, high.Width / 2, high.Height / 2, high.MaxDepthValue);
            }

            return ExtractFromImage(color8, rule);
        }

        private static Mat ToDisplay8(float[] gray, int width, int height, int maxValue)
        {
            double scale = 255.0 / Math.Max(1, maxValue);
            var output = new Mat(height, width, MatType.CV_8UC1);
            int step = (int)output.Step();
            var row = new byte[width];
            for (int y = 0; y < height; y++)
            {
                int offset = y * width;
                for (int x = 0; x < width; x++)
                {
                    double value = gray[offset + x] * scale;
                    row[x] = (byte)(value <= 0 ? 0 : value >= 255 ? 255 : (int)(value + 0.5));
                }
                Marshal.Copy(row, 0, output.Data + y * step, width);
            }
            return output;
        }

        private static Mat ExtractFromImage(Mat source, LsoModeProcessing rule)
        {
            int total = rule.WeightR + 2 * rule.WeightG + rule.WeightB;
            // The cell rule counts both G pixels; a demosaiced pixel has one, so its share
            // doubles and the composite stays 0.6 G + 0.4 R for (4,3,0).
            double wr = rule.WeightR / (double)total;
            double wg = 2.0 * rule.WeightG / total;
            double wb = rule.WeightB / total;

            var output = new Mat(source.Rows, source.Cols, MatType.CV_8UC1);
            int rows = source.Rows;
            int cols = source.Cols;
            int channels = source.Channels();
            bool sixteen = source.Depth() == MatType.CV_16U;
            int sampleBytes = sixteen ? 2 : 1;
            int rowBytes = cols * channels * sampleBytes;
            var line = new byte[rowBytes];
            var grayLine = new byte[cols];
            double sourceScale = sixteen ? 1.0 / 257.0 : 1.0;
            int sourceStep = (int)source.Step();
            int outputStep = (int)output.Step();

            for (int y = 0; y < rows; y++)
            {
                Marshal.Copy(source.Data + y * sourceStep, line, 0, rowBytes);
                for (int x = 0; x < cols; x++)
                {
                    double value;
                    if (channels >= 3)
                    {
                        int index = x * channels * sampleBytes;
                        double r = Sample(line, index, sixteen);
                        double g = Sample(line, index + sampleBytes, sixteen);
                        double b = Sample(line, index + 2 * sampleBytes, sixteen);
                        value = (wr * r + wg * g + wb * b) * sourceScale;
                    }
                    else
                    {
                        value = Sample(line, x * sampleBytes, sixteen) * sourceScale;
                    }

                    grayLine[x] = (byte)(value <= 0 ? 0 : value >= 255 ? 255 : (int)(value + 0.5));
                }
                Marshal.Copy(grayLine, 0, output.Data + y * outputStep, cols);
            }
            return output;
        }

        private static int Sample(byte[] line, int index, bool sixteen)
        {
            return sixteen ? line[index] | (line[index + 1] << 8) : line[index];
        }
    }
}
