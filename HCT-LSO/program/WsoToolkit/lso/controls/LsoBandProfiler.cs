using System;
using System.Collections.Generic;
using MathNet.Numerics;
using OpenCvSharp;

namespace WsoToolkit.controls
{
    /// <summary>
    /// What one captured subframe's illuminated band actually looks like, measured from the
    /// frame instead of assumed from the subframe arithmetic.
    /// </summary>
    /// <remarks>
    /// Measured on a 40-frame slit scan (2026-09-17): the band walks 41..99 rows between
    /// frames (not the nominal 91), is 80..135 rows wide depending on where it sits, is bowed
    /// by up to 40 rows across the width, carries 4.8x different peak brightness, and at the
    /// top and bottom of the field only covers part of the columns. None of that is
    /// expressible as "cut height/N rows", which is why the merge needs this.
    /// </remarks>
    public sealed class LsoBandProfile
    {
        public LsoBandProfile(int frameIndex, int width, int frameHeight)
        {
            FrameIndex = frameIndex;
            Width = width;
            FrameHeight = frameHeight;
            Center = new float[width];
            Amplitude = new float[width];
            Valid = new bool[width];
        }

        public int FrameIndex { get; }

        public int Width { get; }

        public int FrameHeight { get; }

        /// <summary>Band centre row per column, smoothed; only meaningful where Valid.</summary>
        public float[] Center { get; }

        /// <summary>Total band signal per column, smoothed; the illumination's strength.</summary>
        public float[] Amplitude { get; }

        /// <summary>Columns the band actually reaches (the field is a disc, not a square).</summary>
        public bool[] Valid { get; }

        /// <summary>
        /// The band's cross-section, learnt from the frame itself (median over columns after
        /// aligning them on Center), normalised to sum 1. Index = row offset + ShapeRadius.
        /// </summary>
        public float[] Shape { get; private set; } = Array.Empty<float>();

        public int ShapeRadius { get; private set; }

        public int FirstValidColumn { get; private set; } = -1;

        public int LastValidColumn { get; private set; } = -1;

        public int ValidColumnCount { get; private set; }

        /// <summary>Centre row at mid width - the single number that says where the band is.</summary>
        public double CenterAtMid { get; private set; }

        /// <summary>Width at half maximum of the measured cross-section, in rows.</summary>
        public double FwhmRows { get; private set; }

        /// <summary>Centre at mid width minus the average of both ends: the band's bow.</summary>
        public double BowRows { get; private set; }

        public double TotalFlux { get; private set; }

        public bool IsMeasured => ValidColumnCount > 0 && Shape.Length > 0;

        /// <summary>
        /// The illumination this frame contributes at (x, y): amplitude scaled by the measured
        /// cross-section, shifted to this column's centre. Zero outside the band and outside
        /// the columns the band reaches - which is exactly the weight the merge needs.
        /// </summary>
        public double WeightAt(int x, int y)
        {
            if (x < 0 || x >= Width || !Valid[x] || Shape.Length == 0)
            {
                return 0.0;
            }

            double offset = y - Center[x] + ShapeRadius;
            if (offset < 0 || offset >= Shape.Length - 1)
            {
                return 0.0;
            }

            int i = (int)offset;
            double frac = offset - i;
            return Amplitude[x] * ((Shape[i] * (1.0 - frac)) + (Shape[i + 1] * frac));
        }

        internal void SetColumns(int first, int last, int count, double totalFlux)
        {
            FirstValidColumn = first;
            LastValidColumn = last;
            ValidColumnCount = count;
            TotalFlux = totalFlux;
        }

        internal void SetShape(float[] shape, int radius)
        {
            Shape = shape;
            ShapeRadius = radius;

            float peak = 0f;
            foreach (float v in shape)
            {
                peak = Math.Max(peak, v);
            }

            int half = 0;
            foreach (float v in shape)
            {
                if (v >= peak / 2f)
                {
                    half++;
                }
            }

            FwhmRows = half;
        }

        internal void SetGeometry(double centerAtMid, double bow)
        {
            CenterAtMid = centerAtMid;
            BowRows = bow;
        }
    }

    public static class LsoBandProfiler
    {
        /// <summary>
        /// Measures the lit band in one frame. searchCenterRow &lt; 0 lets it find the band
        /// itself (brightest row); pass the expected row when the scan geometry is known, so a
        /// second bright feature elsewhere in the frame cannot pull the search away.
        /// Returns null when there is no band to measure.
        /// </summary>
        public static LsoBandProfile? Measure(Mat frame, int searchCenterRow, LsoStitchParams parameters, int frameIndex = 0)
        {
            if (frame == null || frame.Empty() || parameters == null)
            {
                return null;
            }

            using Mat gray = toGray32_(frame);
            int height = gray.Rows;
            int width = gray.Cols;

            using Mat rowMeanMat = new Mat();
            Cv2.Reduce(gray, rowMeanMat, ReduceDimension.Column, ReduceTypes.Avg, MatType.CV_32F);
            rowMeanMat.GetArray(out float[] rowMean);

            // Dark level: the frame is mostly unlit, so a low percentile of the row means is
            // the background. Subtracting it keeps the centroid from drifting toward the
            // middle of the frame.
            float dark = percentile_(rowMean, 0.05);

            int center = searchCenterRow;
            if (center < 0 || center >= height)
            {
                center = argMax_(rowMean);
            }

            int radius = Math.Max(16, parameters.ProfileSearchRadiusRows);
            int top = Math.Max(0, center - radius);
            int bottom = Math.Min(height, center + radius);
            if (bottom - top < 8)
            {
                return null;
            }

            using Mat window = new Mat(gray, new Rect(0, top, width, bottom - top)).Clone();
            window.GetArray(out float[] pixels);
            int rows = bottom - top;

            var profile = new LsoBandProfile(frameIndex, width, height);

            // Per column: background-suppressed centroid and flux inside the window.
            double[] flux = new double[width];
            double[] centroid = new double[width];
            double windowPeak = 0.0;
            for (int i = 0; i < pixels.Length; ++i)
            {
                windowPeak = Math.Max(windowPeak, pixels[i] - dark);
            }

            if (windowPeak <= 0.0)
            {
                return null;
            }

            double cut = dark + (windowPeak * 0.10);
            for (int x = 0; x < width; ++x)
            {
                double sum = 0.0;
                double weighted = 0.0;
                for (int r = 0; r < rows; ++r)
                {
                    double v = pixels[(r * width) + x] - cut;
                    if (v <= 0.0)
                    {
                        continue;
                    }

                    sum += v;
                    weighted += v * (top + r);
                }

                flux[x] = sum;
                centroid[x] = sum > 0.0 ? weighted / sum : double.NaN;
            }

            double maxFlux = 0.0;
            foreach (double f in flux)
            {
                maxFlux = Math.Max(maxFlux, f);
            }

            if (maxFlux <= 0.0)
            {
                return null;
            }

            double validCut = maxFlux * Math.Clamp(parameters.ValidColumnPercent, 0.1, 50.0) / 100.0;
            var xs = new List<double>();
            var cs = new List<double>();
            var fs = new List<double>();
            int first = -1;
            int last = -1;
            double totalFlux = 0.0;
            for (int x = 0; x < width; ++x)
            {
                if (flux[x] < validCut || double.IsNaN(centroid[x]))
                {
                    continue;
                }

                profile.Valid[x] = true;
                xs.Add(x);
                cs.Add(centroid[x]);
                fs.Add(flux[x]);
                totalFlux += flux[x];
                first = first < 0 ? x : first;
                last = x;
            }

            if (xs.Count < 64)
            {
                return null;   // the band barely shows in this frame - let the caller fall back
            }

            profile.SetColumns(first, last, xs.Count, totalFlux);

            // A polynomial in x smooths the per-column noise away and gives sub-pixel centres;
            // it also expresses the bow, which a straight cut cannot.
            double[] centerFit = Fit.Polynomial(xs.ToArray(), cs.ToArray(), Math.Clamp(parameters.CenterFitDegree, 1, 6));
            double[] amplitudeFit = Fit.Polynomial(xs.ToArray(), fs.ToArray(), Math.Clamp(parameters.AmplitudeFitDegree, 1, 6));

            for (int x = 0; x < width; ++x)
            {
                profile.Center[x] = (float)Polynomial.Evaluate(x, centerFit);
                profile.Amplitude[x] = (float)Math.Max(0.0, Polynomial.Evaluate(x, amplitudeFit));
            }

            // The cross-section itself, learnt from the frame: align every sampled column on
            // its own centre, normalise by its flux, take the median. No Gaussian assumption -
            // fitting a Gaussian to this slit measured WORSE than the plain row profile.
            int stride = Math.Max(1, parameters.ShapeStrideColumns);
            var samples = new List<int>();
            for (int x = first; x <= last; x += stride)
            {
                if (profile.Valid[x] && profile.Amplitude[x] > 0f)
                {
                    samples.Add(x);
                }
            }

            if (samples.Count < 8)
            {
                return null;
            }

            int shapeRadius = radius;
            int shapeLength = (shapeRadius * 2) + 1;
            float[] shape = new float[shapeLength];
            float[] bucket = new float[samples.Count];
            for (int u = 0; u < shapeLength; ++u)
            {
                int n = 0;
                foreach (int x in samples)
                {
                    int y = (int)Math.Round(profile.Center[x]) + u - shapeRadius;
                    if (y < top || y >= bottom)
                    {
                        continue;
                    }

                    bucket[n++] = (float)((pixels[((y - top) * width) + x] - dark) / profile.Amplitude[x]);
                }

                shape[u] = n > 0 ? median_(bucket, n) : 0f;
            }

            double area = 0.0;
            for (int u = 0; u < shapeLength; ++u)
            {
                shape[u] = Math.Max(0f, shape[u]);
                area += shape[u];
            }

            if (area <= 0.0)
            {
                return null;
            }

            for (int u = 0; u < shapeLength; ++u)
            {
                shape[u] = (float)(shape[u] / area);
            }

            profile.SetShape(shape, shapeRadius);

            int mid = width / 2;
            double midCenter = profile.Center[Math.Clamp(mid, first, last)];
            double ends = (profile.Center[first] + profile.Center[last]) / 2.0;
            profile.SetGeometry(midCenter, midCenter - ends);
            return profile;
        }

        private static Mat toGray32_(Mat frame)
        {
            Mat gray = new Mat();
            if (frame.Channels() == 1)
            {
                frame.ConvertTo(gray, MatType.CV_32F);
                return gray;
            }

            using Mat mono = new Mat();
            Cv2.CvtColor(frame, mono, ColorConversionCodes.RGB2GRAY);
            mono.ConvertTo(gray, MatType.CV_32F);
            return gray;
        }

        private static float percentile_(float[] values, double fraction)
        {
            float[] copy = (float[])values.Clone();
            Array.Sort(copy);
            int index = Math.Clamp((int)Math.Round(fraction * (copy.Length - 1)), 0, copy.Length - 1);
            return copy[index];
        }

        private static int argMax_(float[] values)
        {
            int best = 0;
            for (int i = 1; i < values.Length; ++i)
            {
                if (values[i] > values[best])
                {
                    best = i;
                }
            }

            return best;
        }

        private static float median_(float[] buffer, int count)
        {
            float[] copy = new float[count];
            Array.Copy(buffer, copy, count);
            Array.Sort(copy);
            return (count % 2) == 1
                ? copy[count / 2]
                : (copy[(count / 2) - 1] + copy[count / 2]) / 2f;
        }
    }
}
