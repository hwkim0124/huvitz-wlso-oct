using System;
using System.Threading.Tasks;

namespace WsoNativeLib.Analysis
{
    // DevKit LsoImageAnalysisForm: 1093-1218, 1230-1599, 1680-1712,
    // 1952-2031, 2625-2658, 2759-2905. UI/bitmap access is kept outside this class.
    public static class LsoImageAnalysis
    {
        public sealed class Frame
        {
            public int Width { get; }
            public int Height { get; }
            public int BitDepth { get; }
            public bool IsMono { get; }
            public float[] Rgb { get; }
            public float Maximum => (1 << BitDepth) - 1;
            public Frame(int width, int height, int bitDepth, bool mono, float[] rgb)
            {
                if (width <= 0 || height <= 0 || bitDepth < 8 || bitDepth > 16
                    || rgb.Length != checked(width * height * 3))
                    throw new ArgumentException("Invalid analysis frame geometry.");
                Width = width; Height = height; BitDepth = bitDepth; IsMono = mono; Rgb = rgb;
            }
        }

        public enum Direction { Horizontal, Vertical, Cross, Diagonal }
        // DevKit CameraHighDepthData.GetRgbPixel, lines 266-331: clamped RGGB neighbors,
        // including fractional channel values that must not be rounded by an 8/16-bit demosaicer.
        public static Frame FromSensor(ushort[] raw, int width, int height, int bitDepth, bool mono)
        {
            if (raw.Length != checked(width * height)) throw new ArgumentException("Invalid sensor frame.");
            var rgb = new float[checked(width * height * 3)];
            // Rows are independent (each writes only its own pixels); a full sensor is ~20 M pixels.
            Parallel.For(0, height, y =>
            {
                for (int x = 0; x < width; ++x)
                {
                    int i = (y * width + x) * 3;
                    ushort c = raw[y * width + x];
                    if (mono) { rgb[i] = rgb[i + 1] = rgb[i + 2] = c; continue; }
                    int ym1 = y > 0 ? y - 1 : 0, yp1 = y < height - 1 ? y + 1 : height - 1;
                    int xm1 = x > 0 ? x - 1 : 0, xp1 = x < width - 1 ? x + 1 : width - 1;
                    ushort n = raw[ym1 * width + x], s = raw[yp1 * width + x];
                    ushort w = raw[y * width + xm1], e = raw[y * width + xp1];
                    ushort nw = raw[ym1 * width + xm1], ne = raw[ym1 * width + xp1];
                    ushort sw = raw[yp1 * width + xm1], se = raw[yp1 * width + xp1];
                    if ((y & 1) == 0)
                    {
                        if ((x & 1) == 0) { rgb[i] = c; rgb[i + 1] = (n + s + w + e) * 0.25f; rgb[i + 2] = (nw + ne + sw + se) * 0.25f; }
                        else { rgb[i] = (w + e) * 0.5f; rgb[i + 1] = c; rgb[i + 2] = (n + s) * 0.5f; }
                    }
                    else
                    {
                        if ((x & 1) == 0) { rgb[i] = (n + s) * 0.5f; rgb[i + 1] = c; rgb[i + 2] = (w + e) * 0.5f; }
                        else { rgb[i] = (nw + ne + sw + se) * 0.25f; rgb[i + 1] = (n + s + w + e) * 0.25f; rgb[i + 2] = c; }
                    }
                }
            });
            return new Frame(width, height, bitDepth, mono, rgb);
        }

        public readonly record struct Statistics(float Min, float Max, float Mean, float Std, float Uniformity);
        public sealed record Profile(float[] R, float[] G, float[] B, float[] Y, double Distance)
        {
            public float[] Channel(int channel) => channel switch { 1 => G, 2 => R, 3 => B, _ => Y };
        }

        public static float ApplyBoost(float val, float gain, int mode, float maxVal = 255f)
        {
            if (gain <= 1.001f && mode == 0) return Math.Clamp(val, 0f, maxVal);
            switch (mode)
            {
                case 0:
                    float linearVal = val * gain;
                    float kneeT = maxVal * (200.0f / 255.0f);
                    if (linearVal <= kneeT) return linearVal;
                    float span = maxVal - kneeT;
                    float excess = linearVal - kneeT;
                    return kneeT + span * (1.0f - (float)Math.Exp(-excess / span));
                case 2:
                    float norm = Math.Clamp(val / maxVal, 0f, 1f);
                    return Math.Clamp(maxVal * (float)Math.Pow(norm, 0.5) * gain, 0f, maxVal);
                default:
                    return Math.Clamp(val * gain, 0f, maxVal);
            }
        }

        public static float Luminance(float r, float g, float b, bool mono, bool rec709, int fafMode = 0)
            => fafMode switch
            {
                2 => g,
                3 => 0.6f * g + 0.4f * r,
                4 => r,
                _ => mono ? b : rec709 ? 0.2126f * r + 0.7152f * g + 0.0722f * b : (r + g + b) / 3f
            };

        public static void Sample(Frame image, float x, float y, out float r, out float g, out float b)
        {
            int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y);
            int x1 = x0 + 1, y1 = y0 + 1;
            float fx = x - x0, fy = y - y0;
            x0 = Math.Clamp(x0, 0, image.Width - 1); x1 = Math.Clamp(x1, 0, image.Width - 1);
            y0 = Math.Clamp(y0, 0, image.Height - 1); y1 = Math.Clamp(y1, 0, image.Height - 1);
            float w00 = (1f - fx) * (1f - fy), w10 = fx * (1f - fy), w01 = (1f - fx) * fy, w11 = fx * fy;
            int i00 = (y0 * image.Width + x0) * 3, i10 = (y0 * image.Width + x1) * 3;
            int i01 = (y1 * image.Width + x0) * 3, i11 = (y1 * image.Width + x1) * 3;
            float[] p = image.Rgb;
            r = p[i00] * w00 + p[i10] * w10 + p[i01] * w01 + p[i11] * w11;
            g = p[i00 + 1] * w00 + p[i10 + 1] * w10 + p[i01 + 1] * w01 + p[i11 + 1] * w11;
            b = p[i00 + 2] * w00 + p[i10 + 2] * w10 + p[i01 + 2] * w01 + p[i11 + 2] * w11;
        }

        public static Profile Extract(Frame image, Direction direction, int row, int column, int thickness,
            float x1 = 0, float y1 = 0, float x2 = 0, float y2 = 0, bool rec709 = true, float gain = 1, int mode = 0,
            int fafMode = 0)
        {
            bool diagonal = direction == Direction.Diagonal, horizontal = direction != Direction.Vertical;
            row = Math.Clamp(row, 0, image.Height - 1); column = Math.Clamp(column, 0, image.Width - 1);
            int half = Math.Max(1, thickness) / 2;
            float dx = x2 - x1, dy = y2 - y1;
            double distance = Math.Sqrt(dx * dx + dy * dy);
            int length = diagonal ? Math.Max(64, (int)Math.Ceiling(distance * 4) + 1) : horizontal ? image.Width : image.Height;
            float ux = distance > 0.001 ? dx / (length - 1) : 0;
            float uy = distance > 0.001 ? dy / (length - 1) : 0;
            float nx = distance > 0.001 ? (float)(-dy / distance) : 0;
            float ny = distance > 0.001 ? (float)(dx / distance) : 1;
            var result = new Profile(new float[length], new float[length], new float[length], new float[length],
                diagonal ? distance : length - 1);
            for (int s = 0; s < length; ++s)
            {
                float sr = 0, sg = 0, sb = 0;
                int count = 0;
                int low = diagonal ? -half : Math.Max(0, (horizontal ? row : column) - half);
                int high = diagonal ? half : Math.Min((horizontal ? image.Height : image.Width) - 1, (horizontal ? row : column) + half);
                for (int k = low; k <= high; ++k)
                {
                    float x = diagonal ? x1 + s * ux + k * nx : horizontal ? s : k;
                    float y = diagonal ? y1 + s * uy + k * ny : horizontal ? k : s;
                    Sample(image, x, y, out float r, out float g, out float b);
                    sr += r; sg += g; sb += b; ++count;
                }
                float rr = sr / count, gg = sg / count, bb = sb / count;
                result.R[s] = ApplyBoost(rr, gain, mode, image.Maximum);
                result.G[s] = ApplyBoost(gg, gain, mode, image.Maximum);
                result.B[s] = ApplyBoost(bb, gain, mode, image.Maximum);
                result.Y[s] = ApplyBoost(Luminance(rr, gg, bb, image.IsMono, rec709, fafMode), gain, mode, image.Maximum);
            }
            return result;
        }

        public static Statistics GetStatistics(float[] values)
        {
            float min = float.MaxValue, max = float.MinValue;
            double sum = 0, sumSq = 0;
            foreach (float v in values) { min = Math.Min(min, v); max = Math.Max(max, v); sum += v; }
            float mean = (float)(sum / values.Length);
            foreach (float v in values) { float diff = v - mean; sumSq += diff * diff; }
            float uniformity = (max + min > 0) ? (1.0f - ((max - min) / (max + min))) * 100.0f : 100.0f;
            return new Statistics(min, max, mean, (float)Math.Sqrt(sumSq / values.Length), uniformity);
        }

        public static float[,] UniformityMap(Frame image, int blockSize, int metric, bool rec709 = true, float gain = 1, int mode = 0)
        {
            int width = image.Width / blockSize, height = image.Height / blockSize;
            var means = new float[width, height];
            var map = new float[width, height];
            if (width == 0 || height == 0) return map;
            float Value(int x, int y)
            {
                int i = (y * image.Width + x) * 3;
                return ApplyBoost(Luminance(image.Rgb[i], image.Rgb[i + 1], image.Rgb[i + 2], image.IsMono, rec709), gain, mode, image.Maximum);
            }
            Parallel.For(0, height, by =>
            {
                for (int bx = 0; bx < width; ++bx)
                {
                    double sum = 0;
                    for (int y = 0; y < blockSize; ++y)
                        for (int x = 0; x < blockSize; ++x) sum += Value(bx * blockSize + x, by * blockSize + y);
                    means[bx, by] = (float)(sum / (blockSize * blockSize));
                }
            });
            float center = means[width / 2, height / 2];
            if (center <= 0) center = 1;
            Parallel.For(0, height, by =>
            {
                for (int bx = 0; bx < width; ++bx)
                {
                    float mu = means[bx, by], val;
                    if (metric == 0 || metric == 2)
                    {
                        double sum = 0;
                        for (int y = 0; y < blockSize; ++y)
                            for (int x = 0; x < blockSize; ++x)
                            { float diff = Value(bx * blockSize + x, by * blockSize + y) - mu; sum += diff * diff; }
                        float std = (float)Math.Sqrt(sum / (blockSize * blockSize));
                        val = metric == 0 ? std : mu > 0 ? (std / mu) * 100f : 0;
                    }
                    else val = metric == 1 ? mu : (mu / center) * 100f;
                    map[bx, by] = val;
                }
            });
            return map;
        }

        public static int SnapToPeak(Frame image, bool horizontal)
        {
            float max = -1;
            int target = 0;
            for (int line = 0; line < (horizontal ? image.Height : image.Width); line += 4)
            {
                float sum = 0;
                for (int s = 0; s < (horizontal ? image.Width : image.Height); s += 8)
                {
                    int i = ((horizontal ? line : s) * image.Width + (horizontal ? s : line)) * 3;
                    sum += (image.Rgb[i] + image.Rgb[i + 1] + image.Rgb[i + 2]) / 3f;
                }
                if (sum > max) { max = sum; target = line; }
            }
            return target;
        }

        public static float AutoGain(Frame image)
        {
            float peak = 1;
            for (int y = 0; y < image.Height; y += 16)
                for (int x = 0; x < image.Width; x += 16)
                {
                    int i = (y * image.Width + x) * 3;
                    float luma = (image.Rgb[i] + image.Rgb[i + 1] + image.Rgb[i + 2]) / 3f * 255f / image.Maximum;
                    if (luma > peak) peak = luma;
                }
            return (float)Math.Round(Math.Clamp(230f / peak, 1f, 10f), 1);
        }
    }
}
