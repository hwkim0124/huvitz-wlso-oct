using System;
using System.Collections.Generic;

namespace WsoNativeLib.Presets
{
    // One slit-band measurement, shared by the scan-test window's Auto button and by
    // lsotune align. The two cannot disagree: Rec Exp written from one of them is what
    // the other one has to park the slit against.
    //
    // EquivalentRows is the light-bearing width (background-subtracted area over the
    // peak), not the 2-click full width. Rec Exp = slit rows x line time has to use
    // that width: the 2-click full width measured about 2x wider on 2026-09-20, which
    // would ask the scan for about twice the exposure the slit actually delivers.
    public readonly record struct LsoSlitBandResult(
        bool Found,
        double CenterRow,
        int FwhmRows,
        double EquivalentRows,
        double TopRow,
        double BottomRow,
        double Peak,
        double Baseline,
        string Reason);

    public static class LsoSlitBand
    {
        // Align refuses a band narrower than this, and a shorter profile cannot hold one.
        // Measuring it would report a centroid of the noise floor.
        private const int MinProfileRows = 8;

        // Rows dimmer than this fraction of the peak are the floor, not the slit. The
        // cutoff still keeps essentially all of a flat-top's light, and of a Gaussian's
        // (sigma * sqrt(2 * pi)), which is the width Rec Exp is built from.
        private const double EquivalentFootprintFraction = 0.02;

        // Mean of each image row. Bayer stays un-debayered: the band tools judge a
        // horizontal slit, and a per-channel debayer would move the row means they share.
        public static double[] RowProfile(ReadOnlySpan<byte> raw, int width, int height, int bytesPerPixel)
        {
            if (width <= 0)
            {
                throw new ArgumentException("width must be positive.", nameof(width));
            }
            if (height <= 0)
            {
                throw new ArgumentException("height must be positive.", nameof(height));
            }
            if (bytesPerPixel is not (1 or 2 or 3 or 4))
            {
                throw new ArgumentException("bytesPerPixel must be 1, 2, 3 or 4.", nameof(bytesPerPixel));
            }

            long needed = (long)width * height * bytesPerPixel;
            if (needed < 0 || raw.Length < needed)
            {
                throw new ArgumentException(
                    "raw is shorter than width*height*bytesPerPixel.", nameof(raw));
            }

            var profile = new double[height];
            int stride = width * bytesPerPixel;
            for (int y = 0; y < height; y++)
            {
                long sum = 0;
                int row = y * stride;
                if (bytesPerPixel == 1)
                {
                    for (int x = 0; x < width; x++)
                    {
                        sum += raw[row + x];
                    }
                    profile[y] = (double)sum / width;
                }
                else if (bytesPerPixel == 2)
                {
                    for (int x = 0; x < width; x++)
                    {
                        sum += raw[row + 2 * x] | (raw[row + 2 * x + 1] << 8); // little endian
                    }
                    profile[y] = (double)sum / width;
                }
                else
                {
                    // Packed RGB/BGR. The fourth byte of BGRA is padding, not light, so the
                    // row mean is the mean of the three colour bytes.
                    for (int x = 0; x < width; x++)
                    {
                        int pixel = row + bytesPerPixel * x;
                        sum += raw[pixel] + raw[pixel + 1] + raw[pixel + 2];
                    }
                    profile[y] = (double)sum / (3.0 * width);
                }
            }
            return profile;
        }

        // Rows are image rows of the profile passed in (0 = first element). Found is false,
        // with a reason, when there is no band to measure.
        public static LsoSlitBandResult Measure(IReadOnlyList<double> rowProfile)
        {
            if (rowProfile == null)
            {
                throw new ArgumentNullException(nameof(rowProfile));
            }
            int n = rowProfile.Count;
            if (n < MinProfileRows)
            {
                return NotFound("profile has fewer than " + MinProfileRows + " rows");
            }

            // De-comb first. On a Bayer sensor the row means alternate between the RG and GB
            // row types, and that alternation cuts the half-max crossing short: the SAME
            // parked slit measured 75 rows off the raw profile and 112 after averaging
            // adjacent rows. The centroid survived it, the width did not - and the width is
            // what Rec Exp is built from. Averaging two neighbours cancels a 2-row
            // alternation exactly and barely touches a real edge.
            var paired = new double[n];
            for (int i = 0; i < n; i++)
            {
                paired[i] = i + 1 < n ? 0.5 * (rowProfile[i] + rowProfile[i + 1]) : rowProfile[i];
            }

            double baseline = Percentile(paired, 10);
            var signal = new double[n];
            int peak = 0;
            for (int i = 0; i < n; i++)
            {
                signal[i] = paired[i] - baseline;
                if (signal[i] > signal[peak])
                {
                    peak = i;
                }
            }
            if (signal[peak] <= 0.0)
            {
                return NotFound(
                    "background-subtracted peak is not above the baseline",
                    signal[peak], baseline);
            }

            double half = signal[peak] * 0.5;
            int lo = peak;
            int hi = peak;
            while (lo > 0 && signal[lo - 1] > half)
            {
                lo--;
            }
            while (hi < n - 1 && signal[hi + 1] > half)
            {
                hi++;
            }

            double weight = 0.0;
            double moment = 0.0;
            for (int i = lo; i <= hi; i++)
            {
                weight += signal[i];
                moment += signal[i] * i;
            }

            // The light-bearing footprint, not the half-max window: a flat-top's area over
            // its peak is its width, and a Gaussian's is sigma * sqrt(2 * pi). Rows below
            // 2% of the peak are the floor and must not inflate that width.
            double floor = signal[peak] * EquivalentFootprintFraction;
            int elo = peak;
            int ehi = peak;
            while (elo > 0 && signal[elo - 1] > floor)
            {
                elo--;
            }
            while (ehi < n - 1 && signal[ehi + 1] > floor)
            {
                ehi++;
            }
            double area = 0.0;
            for (int i = elo; i <= ehi; i++)
            {
                if (signal[i] > 0.0)
                {
                    area += signal[i];
                }
            }

            // paired[i] holds the value at image row i+0.5, so every reported row comes
            // back half a row low unless that half row is put back.
            double centerIndex = weight > 0.0 ? moment / weight : peak;
            return new LsoSlitBandResult(
                true,
                centerIndex + 0.5,
                hi - lo + 1,
                area / signal[peak],
                lo + 0.5,
                hi + 0.5,
                signal[peak],
                baseline,
                string.Empty);
        }

        // numpy's linear interpolation, so the baseline matches the validated reference.
        private static double Percentile(double[] values, double percent)
        {
            if (values.Length == 0)
            {
                return 0.0;
            }
            var sorted = (double[])values.Clone();
            Array.Sort(sorted);
            double index = percent / 100.0 * (sorted.Length - 1);
            int lo = (int)Math.Floor(index);
            int hi = (int)Math.Ceiling(index);
            return lo == hi ? sorted[lo] : sorted[lo] + (sorted[hi] - sorted[lo]) * (index - lo);
        }

        private static LsoSlitBandResult NotFound(string reason, double peak = 0.0, double baseline = 0.0)
        {
            return new LsoSlitBandResult(false, 0.0, 0, 0.0, 0.0, 0.0, peak, baseline, reason);
        }
    }
}
