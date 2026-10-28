using System;
using System.Runtime.InteropServices;
using OpenCvSharp;

namespace WsoNativeLib.Presets
{
    // One Gray-mode capture in, one averaged gray image out:
    //   1. each raw Bayer frame (flat-field already applied) is reduced to half resolution,
    //      one pixel per 2x2 cell = the weighted mean of the planes LsoModeProcessing names;
    //   2. every frame after the first is aligned to the first by phase correlation
    //      (translation only - the eye drifts between the sweeps of a burst);
    //   3. the aligned frames are averaged.
    // The result is 16-bit: 8-bit input is scaled x257 first, so the mean keeps the precision
    // averaging buys instead of being rounded back to 256 levels.
    //
    // Each Add runs an FFT, so callers feed it off the native acquisition thread and off the
    // UI thread. Not thread-safe; one instance per capture.
    public sealed class LsoModeImageAccumulator : IDisposable
    {
        private readonly LsoModeProcessing _processing;
        private Mat? _reference;
        private Mat? _window;
        private double[]? _sum;
        private int _width;
        private int _height;

        public LsoModeImageAccumulator(LsoModeProcessing processing)
        {
            if (processing == null)
            {
                throw new ArgumentNullException(nameof(processing));
            }

            if (!processing.Gray
                || processing.WeightR < 0 || processing.WeightG < 0 || processing.WeightB < 0
                || processing.WeightR + 2 * processing.WeightG + processing.WeightB <= 0)
            {
                throw new ArgumentException("A gray processing rule with a positive plane weight is required.",
                    nameof(processing));
            }

            _processing = processing;
        }

        public int Count { get; private set; }

        // Shift of the last added frame against the first, in output pixels, measured before it
        // was aligned. Callers log it so a burst that drifted is visible.
        public Point2d LastShift { get; private set; }

        // rawBayer: BayerRG (R G / G B), 1 byte per pixel or 2 bytes little-endian.
        public void Add(byte[] rawBayer, int width, int height, int bytesPerPixel)
        {
            if (rawBayer == null)
            {
                throw new ArgumentNullException(nameof(rawBayer));
            }

            if (bytesPerPixel != 1 && bytesPerPixel != 2)
            {
                throw new ArgumentException($"{bytesPerPixel} bytes per pixel is not a Bayer format.", nameof(bytesPerPixel));
            }

            if (width < 2 || height < 2 || rawBayer.Length < width * height * bytesPerPixel)
            {
                throw new ArgumentException($"The frame buffer does not hold {width}x{height}x{bytesPerPixel}.", nameof(rawBayer));
            }

            int outWidth = width / 2;
            int outHeight = height / 2;
            if (_sum != null && (outWidth != _width || outHeight != _height))
            {
                throw new ArgumentException($"Frame {Count + 1} is {width}x{height}; one capture cannot change size.",
                    nameof(rawBayer));
            }

            float[] gray = ToGray(rawBayer, width, height, bytesPerPixel, _processing);
            using var frame = new Mat(outHeight, outWidth, MatType.CV_32FC1);
            Marshal.Copy(gray, 0, frame.Data, gray.Length);

            if (_sum == null)
            {
                _width = outWidth;
                _height = outHeight;
                _sum = new double[gray.Length];
                _reference = frame.Clone();
                _window = new Mat();
                Cv2.CreateHanningWindow(_window, new Size(outWidth, outHeight), MatType.CV_32FC1);
                AddToSum(gray);
                LastShift = new Point2d(0, 0);
                Count = 1;
                return;
            }

            // PhaseCorrelate reports how far this frame sits from the reference; moving it back
            // by that amount lays it on the reference. It is given copies: when the size is
            // already an optimal DFT size OpenCV multiplies its inputs by the window IN PLACE,
            // which would darken this frame's edges and re-window the reference every call.
            Point2d shift;
            using (Mat reference = _reference!.Clone())
            using (Mat moving = frame.Clone())
            {
                shift = Cv2.PhaseCorrelate(reference, moving, _window!, out _);
            }
            if (double.IsNaN(shift.X) || double.IsNaN(shift.Y))
            {
                shift = new Point2d(0, 0); // nothing to correlate (blank sweep): average it where it lies
            }
            LastShift = shift;

            using var move = new Mat(2, 3, MatType.CV_64FC1);
            Marshal.Copy(new double[] { 1, 0, -shift.X, 0, 1, -shift.Y }, 0, move.Data, 6);
            using var aligned = new Mat();
            Cv2.WarpAffine(frame, aligned, move, frame.Size(), InterpolationFlags.Linear, BorderTypes.Replicate);

            var values = new float[gray.Length];
            Marshal.Copy(aligned.Data, values, 0, values.Length);
            AddToSum(values);
            Count++;
        }

        // The mean of every frame added so far, row-major, width x height = half the sensor ROI.
        public ushort[] Result(out int width, out int height)
        {
            if (_sum == null || Count == 0)
            {
                throw new InvalidOperationException("No frame was added.");
            }

            width = _width;
            height = _height;
            var result = new ushort[_sum.Length];
            for (int i = 0; i < _sum.Length; i++)
            {
                double value = Math.Round(_sum[i] / Count);
                result[i] = (ushort)(value < 0 ? 0 : value > ushort.MaxValue ? ushort.MaxValue : value);
            }
            return result;
        }

        public void Dispose()
        {
            _reference?.Dispose();
            _reference = null;
            _window?.Dispose();
            _window = null;
        }

        private void AddToSum(float[] values)
        {
            double[] sum = _sum!;
            for (int i = 0; i < sum.Length; i++)
            {
                sum[i] += values[i];
            }
        }

        // Same plane layout LsoFlatField uses: plane (row & 1, col & 1), (0,0) = R.
        internal static float[] ToGray(byte[] raw, int width, int height, int bytesPerPixel, LsoModeProcessing rule)
        {
            int outWidth = width / 2;
            int outHeight = height / 2;
            int weightR = rule.WeightR;
            int weightG = rule.WeightG;
            int weightB = rule.WeightB;
            double scale = (bytesPerPixel == 1 ? 257.0 : 1.0) / (weightR + 2 * weightG + weightB);

            var gray = new float[outWidth * outHeight];
            for (int y = 0; y < outHeight; y++)
            {
                int row0 = 2 * y * width;
                int row1 = row0 + width;
                int outRow = y * outWidth;
                for (int x = 0; x < outWidth; x++)
                {
                    int column = 2 * x;
                    int r = Pixel(raw, row0 + column, bytesPerPixel);
                    int g1 = Pixel(raw, row0 + column + 1, bytesPerPixel);
                    int g2 = Pixel(raw, row1 + column, bytesPerPixel);
                    int b = Pixel(raw, row1 + column + 1, bytesPerPixel);
                    gray[outRow + x] = (float)((weightR * r + weightG * (g1 + g2) + weightB * b) * scale);
                }
            }
            return gray;
        }

        private static int Pixel(byte[] raw, int index, int bytesPerPixel)
        {
            return bytesPerPixel == 1
                ? raw[index]
                : raw[2 * index] | (raw[2 * index + 1] << 8);
        }

        // Same cell mean as ToGray, but from the 16-bit sensor buffer the review keeps: the
        // values stay in the sensor's own units (0..MaxDepthValue), so the caller scales once
        // for display or storage. Half the input resolution, one pixel per 2x2 cell.
        internal static float[] ToGray(ushort[] raw, int width, int height, LsoModeProcessing rule)
        {
            int outWidth = width / 2;
            int outHeight = height / 2;
            int weightR = rule.WeightR;
            int weightG = rule.WeightG;
            int weightB = rule.WeightB;
            double scale = 1.0 / (weightR + 2 * weightG + weightB);

            var gray = new float[outWidth * outHeight];
            for (int y = 0; y < outHeight; y++)
            {
                int row0 = 2 * y * width;
                int row1 = row0 + width;
                int outRow = y * outWidth;
                for (int x = 0; x < outWidth; x++)
                {
                    int column = 2 * x;
                    int r = raw[row0 + column];
                    int g1 = raw[row0 + column + 1];
                    int g2 = raw[row1 + column];
                    int b = raw[row1 + column + 1];
                    gray[outRow + x] = (float)((weightR * r + weightG * (g1 + g2) + weightB * b) * scale);
                }
            }
            return gray;
        }
    }
}
