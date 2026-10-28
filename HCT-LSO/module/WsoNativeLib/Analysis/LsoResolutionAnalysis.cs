using System;
using System.Collections.Generic;
using System.Linq;

namespace WsoNativeLib.Analysis
{
    // DevKit LsoImageAnalysisForm 2027-2623; literal numerical routines.
    public static class LsoResolutionAnalysis
    {
        public static UsafDetectionResult Detect(float[] values, double distance, float thresholdPercent = 50, bool automatic = true, double[]? offsets = null)
        {
            var stats = LsoImageAnalysis.GetStatistics(values);
            float threshold = stats.Min + thresholdPercent / 100f * (stats.Max - stats.Min);
            UsafDetectionResult result;
            if (!automatic || !DetectUsafLinePairs(values, distance, thresholdPercent, out result))
                AnalyzeCrossingsAtThreshold(values, distance, threshold, out result);
            if (result.Success && offsets != null)
            {
                result.Bar1Start = Math.Clamp(result.Bar1Start + offsets[0], 0, distance);
                result.Bar1End = Math.Clamp(result.Bar1End + offsets[1], 0, distance);
                if (result.DetectedBarCount >= 2) {
                    result.Bar2Start = Math.Clamp(result.Bar2Start + offsets[2], 0, distance);
                    result.Bar2End = Math.Clamp(result.Bar2End + offsets[3], 0, distance);
                }
                if (result.DetectedBarCount >= 3) {
                    result.Bar3Start = Math.Clamp(result.Bar3Start + offsets[4], 0, distance);
                    result.Bar3End = Math.Clamp(result.Bar3End + offsets[5], 0, distance);
                }
            }
            return result;
        }

        public static string RejectionStatus(double db) => db >= 18.0 ? "PASS" : db >= 10.0 ? "WARN" : "FAIL";
        public readonly record struct Rejection(double Db, string Status, float Peak);
        public static Rejection FafRejection(LsoImageAnalysis.Profile profile, int mode)
        {
            float meanG = profile.G.Average(), meanB = profile.B.Average(), meanR = profile.R.Average();
            double db = mode == 2
                ? 20.0 * Math.Log10(Math.Max(1.0, meanG) / Math.Max(0.5, meanB))
                : mode == 3
                ? 20.0 * Math.Log10(Math.Max(1.0, (meanG + meanR) * 0.5) / Math.Max(0.5, meanB))
                : 20.0 * Math.Log10(Math.Max(1.0, meanR) / Math.Max(0.5, meanG));
            return new Rejection(db, RejectionStatus(db), mode == 2 ? profile.G.Max() : mode == 3 ? profile.Y.Max() : profile.R.Max());
        }
        public static void Calculate10To90EdgeRise(float[] yVals, double rayLength, out double edgeRisePx, out string edgeType)
        {
            edgeRisePx = 0;
            edgeType = "N/A";
            if (yVals.Length < 3 || rayLength < 1.0) return;

            float minV = float.MaxValue;
            float maxV = float.MinValue;
            for (int i = 0; i < yVals.Length; i++)
            {
                if (yVals[i] < minV) minV = yVals[i];
                if (yVals[i] > maxV) maxV = yVals[i];
            }

            float range = maxV - minV;
            if (range < 5.0f)
            {
                edgeType = "Low Ctr";
                return;
            }

            // Find steepest slope segment
            int maxSlopeIdx = 0;
            float maxAbsSlope = 0;
            bool isRising = true;
            for (int i = 0; i < yVals.Length - 1; i++)
            {
                float slope = yVals[i + 1] - yVals[i];
                if (Math.Abs(slope) > maxAbsSlope)
                {
                    maxAbsSlope = Math.Abs(slope);
                    maxSlopeIdx = i;
                    isRising = (slope >= 0);
                }
            }

            if (maxAbsSlope < 0.001f) return;

            float t10 = minV + 0.10f * range;
            float t90 = minV + 0.90f * range;

            double s10 = -1, s90 = -1;

            if (isRising)
            {
                edgeType = "Rise";
                for (int i = maxSlopeIdx; i >= 0; i--)
                {
                    if (yVals[i] <= t10)
                    {
                        float denom = (i < yVals.Length - 1 && yVals[i + 1] != yVals[i]) ? (yVals[i + 1] - yVals[i]) : 1f;
                        s10 = i + (t10 - yVals[i]) / denom;
                        break;
                    }
                }
                if (s10 < 0) s10 = 0;

                for (int i = maxSlopeIdx; i < yVals.Length; i++)
                {
                    if (yVals[i] >= t90)
                    {
                        float denom = (i > 0 && yVals[i] != yVals[i - 1]) ? (yVals[i] - yVals[i - 1]) : 1f;
                        s90 = (i - 1) + (t90 - yVals[i - 1]) / denom;
                        break;
                    }
                }
                if (s90 < 0) s90 = yVals.Length - 1;
            }
            else
            {
                edgeType = "Fall";
                for (int i = maxSlopeIdx; i >= 0; i--)
                {
                    if (yVals[i] >= t90)
                    {
                        float denom = (i < yVals.Length - 1 && yVals[i + 1] != yVals[i]) ? (yVals[i + 1] - yVals[i]) : 1f;
                        s90 = i + (t90 - yVals[i]) / denom;
                        break;
                    }
                }
                if (s90 < 0) s90 = 0;

                for (int i = maxSlopeIdx; i < yVals.Length; i++)
                {
                    if (yVals[i] <= t10)
                    {
                        float denom = (i > 0 && yVals[i] != yVals[i - 1]) ? (yVals[i] - yVals[i - 1]) : 1f;
                        s10 = (i - 1) + (t10 - yVals[i - 1]) / denom;
                        break;
                    }
                }
                if (s10 < 0) s10 = yVals.Length - 1;
            }

            double sampleStep = rayLength / (yVals.Length - 1);
            edgeRisePx = Math.Abs(s90 - s10) * sampleStep;
        }

        public class UsafDetectionResult
        {
            public bool Success { get; set; }
            public bool IsBrightBars { get; set; } = true;
            public int DetectedBarCount { get; set; } = 3;

            public double Bar1Start { get; set; }
            public double Bar1End { get; set; }
            public double Bar2Start { get; set; }
            public double Bar2End { get; set; }
            public double Bar3Start { get; set; }
            public double Bar3End { get; set; }

            public double Space1Start => Bar1End;
            public double Space1End => Bar2Start;
            public double Space2Start => Bar2End;
            public double Space2End => Bar3Start;

            public double W1 => Math.Max(0.01, Bar1End - Bar1Start);
            public double W2 => Math.Max(0.01, Bar2End - Bar2Start);
            public double W3 => Math.Max(0.01, Bar3End - Bar3Start);
            public double S1 => Math.Max(0.01, Space1End - Space1Start);
            public double S2 => Math.Max(0.01, Space2End - Space2Start);

            public double MeanBarWidth
            {
                get
                {
                    if (DetectedBarCount == 3) return (W1 + W2 + W3) / 3.0;
                    if (DetectedBarCount == 2) return (W1 + W2) / 2.0;
                    return W1;
                }
            }

            public double MeanSpaceWidth
            {
                get
                {
                    if (DetectedBarCount == 3) return (S1 + S2) / 2.0;
                    if (DetectedBarCount == 2) return S1;
                    return 0;
                }
            }

            public double Pitch
            {
                get
                {
                    if (DetectedBarCount >= 2) return MeanBarWidth + MeanSpaceWidth;
                    return MeanBarWidth * 2.0;
                }
            }

            public double SpatialFrequency => (Pitch > 0.001) ? (1.0 / Pitch) : 0.0; // lp/px

            public double DutyCycle => (Pitch > 0.001) ? (MeanBarWidth / Pitch) * 100.0 : 50.0;

            public float PeakIntensity { get; set; }
            public float ValleyIntensity { get; set; }
            public float ThresholdLevel { get; set; }
            public float TripletContrast => (PeakIntensity + ValleyIntensity > 0.001f)
                ? (PeakIntensity - ValleyIntensity) / (PeakIntensity + ValleyIntensity) : 0f;

            public double[] EdgeDistances
            {
                get
                {
                    if (DetectedBarCount == 3)
                        return new double[] { Bar1Start, Bar1End, Bar2Start, Bar2End, Bar3Start, Bar3End };
                    if (DetectedBarCount == 2)
                        return new double[] { Bar1Start, Bar1End, Bar2Start, Bar2End };
                    if (DetectedBarCount == 1)
                        return new double[] { Bar1Start, Bar1End };
                    return Array.Empty<double>();
                }
            }
        }

        private static bool AnalyzeCrossingsAtThreshold(float[] yVals, double rayLength, float thresholdLevel, out UsafDetectionResult result)
        {
            result = new UsafDetectionResult { ThresholdLevel = thresholdLevel };
            if (yVals == null || yVals.Length < 4 || rayLength < 0.5) return false;

            int n = yVals.Length;
            double sampleStep = rayLength / Math.Max(1, n - 1);

            // 1. Find all sub-pixel crossings of yVals with thresholdLevel
            var crossings = new List<(double posPx, bool isRising, int index)>();

            for (int i = 1; i < n; i++)
            {
                float y0 = yVals[i - 1];
                float y1 = yVals[i];

                bool isRising = (y0 < thresholdLevel && y1 >= thresholdLevel);
                bool isFalling = (y0 >= thresholdLevel && y1 < thresholdLevel);

                if (isRising || isFalling)
                {
                    float denom = (y1 != y0) ? (y1 - y0) : 1f;
                    double frac = Math.Clamp((thresholdLevel - y0) / denom, 0.0, 1.0);
                    double posPx = (i - 1 + frac) * sampleStep;
                    crossings.Add((posPx, isRising, i));
                }
            }

            if (crossings.Count < 2) return false;

            // 2. Identify segments (bars) above threshold (bright bars)
            var bars = new List<(double startPx, double endPx, float peakVal)>();
            for (int i = 0; i < crossings.Count - 1; i++)
            {
                if (crossings[i].isRising && !crossings[i + 1].isRising)
                {
                    double bStart = crossings[i].posPx;
                    double bEnd = crossings[i + 1].posPx;
                    int iA = Math.Max(0, crossings[i].index - 1);
                    int iB = Math.Min(n - 1, crossings[i + 1].index);
                    float pk = thresholdLevel;
                    for (int s = iA; s <= iB; s++)
                    {
                        if (yVals[s] > pk) pk = yVals[s];
                    }
                    bars.Add((bStart, bEnd, pk));
                    i++; // skip falling edge
                }
            }

            bool isBrightBars = true;
            if (bars.Count == 0) // try dark bars (segments below threshold)
            {
                for (int i = 0; i < crossings.Count - 1; i++)
                {
                    if (!crossings[i].isRising && crossings[i + 1].isRising)
                    {
                        double bStart = crossings[i].posPx;
                        double bEnd = crossings[i + 1].posPx;
                        int iA = Math.Max(0, crossings[i].index - 1);
                        int iB = Math.Min(n - 1, crossings[i + 1].index);
                        float vl = thresholdLevel;
                        for (int s = iA; s <= iB; s++)
                        {
                            if (yVals[s] < vl) vl = yVals[s];
                        }
                        bars.Add((bStart, bEnd, vl));
                        i++;
                    }
                }
                if (bars.Count > 0) isBrightBars = false;
            }

            if (bars.Count == 0) return false;

            // 3. Find valleys (or spaces) between consecutive bars
            var spaces = new List<(double startPx, double endPx, float valleyVal)>();
            for (int b = 0; b < bars.Count - 1; b++)
            {
                double sStart = bars[b].endPx;
                double sEnd = bars[b + 1].startPx;
                int iA = (int)Math.Floor(sStart / sampleStep);
                int iB = (int)Math.Ceiling(sEnd / sampleStep);
                iA = Math.Clamp(iA, 0, n - 1);
                iB = Math.Clamp(iB, 0, n - 1);

                float vl = thresholdLevel;
                for (int s = iA; s <= iB; s++)
                {
                    if (isBrightBars ? (yVals[s] < vl) : (yVals[s] > vl))
                        vl = yVals[s];
                }
                spaces.Add((sStart, sEnd, vl));
            }

            result.Success = true;
            result.IsBrightBars = isBrightBars;

            if (bars.Count >= 3)
            {
                result.DetectedBarCount = 3;
                result.Bar1Start = bars[0].startPx;
                result.Bar1End   = bars[0].endPx;
                result.Bar2Start = bars[1].startPx;
                result.Bar2End   = bars[1].endPx;
                result.Bar3Start = bars[2].startPx;
                result.Bar3End   = bars[2].endPx;

                result.PeakIntensity = (bars[0].peakVal + bars[1].peakVal + bars[2].peakVal) / 3f;
                result.ValleyIntensity = (spaces.Count >= 2) ? (spaces[0].valleyVal + spaces[1].valleyVal) / 2f
                                       : ((spaces.Count == 1) ? spaces[0].valleyVal : thresholdLevel);
            }
            else if (bars.Count == 2)
            {
                result.DetectedBarCount = 2;
                result.Bar1Start = bars[0].startPx;
                result.Bar1End   = bars[0].endPx;
                result.Bar2Start = bars[1].startPx;
                result.Bar2End   = bars[1].endPx;

                result.PeakIntensity = (bars[0].peakVal + bars[1].peakVal) / 2f;
                result.ValleyIntensity = (spaces.Count >= 1) ? spaces[0].valleyVal : thresholdLevel;
            }
            else
            {
                result.DetectedBarCount = 1;
                result.Bar1Start = bars[0].startPx;
                result.Bar1End   = bars[0].endPx;

                result.PeakIntensity = bars[0].peakVal;
                float minOut = yVals.Min();
                result.ValleyIntensity = (minOut < thresholdLevel) ? minOut : thresholdLevel;
            }

            return true;
        }

        private static bool DetectUsafLinePairs(float[] yVals, double rayLength, float threshPct, out UsafDetectionResult result)
        {
            result = new UsafDetectionResult();
            if (yVals == null || yVals.Length < 10 || rayLength < 5.0) return false;

            int n = yVals.Length;
            double sampleStep = rayLength / Math.Max(1, n - 1);

            // 1. Smooth profile with 3-point moving average to filter sensor noise
            float[] sm = new float[n];
            for (int i = 0; i < n; i++)
            {
                int i0 = Math.Max(0, i - 1);
                int i1 = Math.Min(n - 1, i + 1);
                sm[i] = (yVals[i0] + yVals[i] + yVals[i1]) / 3.0f;
            }

            float globalMin = float.MaxValue;
            float globalMax = float.MinValue;
            for (int i = 0; i < n; i++)
            {
                if (sm[i] < globalMin) globalMin = sm[i];
                if (sm[i] > globalMax) globalMax = sm[i];
            }
            float dynRange = globalMax - globalMin;
            if (dynRange < 6.0f) return false; // Insufficient dynamic range

            // 2. Find local peaks and valleys
            var peaks = new List<int>();
            var valleys = new List<int>();

            for (int i = 1; i < n - 1; i++)
            {
                if (sm[i] >= sm[i - 1] && sm[i] >= sm[i + 1] && (sm[i] > sm[i - 1] || sm[i] > sm[i + 1]))
                {
                    if (peaks.Count == 0 || i - peaks[^1] > 1)
                        peaks.Add(i);
                }
                else if (sm[i] <= sm[i - 1] && sm[i] <= sm[i + 1] && (sm[i] < sm[i - 1] || sm[i] < sm[i + 1]))
                {
                    if (valleys.Count == 0 || i - valleys[^1] > 1)
                        valleys.Add(i);
                }
            }

            if (peaks.Count < 2 && valleys.Count < 2) return false;

            List<int> allPeaks = peaks, allValleys = valleys;
            peaks = Strongest(peaks, sm, highest: true);
            valleys = Strongest(valleys, sm, highest: false);
            int[,] valleyBetweenPeaks = BestBetween(peaks, allValleys, sm, wantMax: false);
            int[,] peakBetweenValleys = BestBetween(valleys, allPeaks, sm, wantMax: true);

            float minProm = 0.05f * dynRange;

            // 3. Search for 3-bar triplet candidate
            bool bestFound = false;
            bool bestIsBright = true;
            int bIdx1 = 0, bIdx2 = 0, bIdx3 = 0;
            int vIdx1 = 0, vIdx2 = 0;
            float bestScore = -1f;

            // Test Bright Bars: 3 peaks with 2 intervening valleys
            for (int p1 = 0; p1 < peaks.Count - 2; p1++)
            {
                int ip1 = peaks[p1];
                for (int p2 = p1 + 1; p2 < peaks.Count - 1; p2++)
                {
                    int ip2 = peaks[p2];
                    int iv1 = valleyBetweenPeaks[p1, p2];
                    if (iv1 < 0) continue;

                    for (int p3 = p2 + 1; p3 < peaks.Count; p3++)
                    {
                        int ip3 = peaks[p3];
                        int iv2 = valleyBetweenPeaks[p2, p3];
                        if (iv2 < 0) continue;

                        double d1 = ip2 - ip1;
                        double d2 = ip3 - ip2;
                        if (d1 < 2.0 || d2 < 2.0) continue;

                        double maxD = Math.Max(d1, d2);
                        double regularity = Math.Abs(d1 - d2) / maxD;
                        if (regularity > 0.45) continue;

                        float avgPk = (sm[ip1] + sm[ip2] + sm[ip3]) / 3f;
                        float avgVl = (sm[iv1] + sm[iv2]) / 2f;
                        float amp = avgPk - avgVl;
                        if (amp < minProm) continue;

                        float contrast = (avgPk + avgVl > 0.001f) ? amp / (avgPk + avgVl) : 0f;
                        float score = contrast * (float)(1.0 - regularity);

                        if (score > bestScore)
                        {
                            bestScore = score;
                            bestFound = true;
                            bestIsBright = true;
                            bIdx1 = ip1; bIdx2 = ip2; bIdx3 = ip3;
                            vIdx1 = iv1; vIdx2 = iv2;
                        }
                    }
                }
            }

            // Test Dark Bars: 3 valleys with 2 intervening peaks
            for (int v1 = 0; v1 < valleys.Count - 2; v1++)
            {
                int iv1 = valleys[v1];
                for (int v2 = v1 + 1; v2 < valleys.Count - 1; v2++)
                {
                    int iv2 = valleys[v2];
                    int ip1 = peakBetweenValleys[v1, v2];
                    if (ip1 < 0) continue;

                    for (int v3 = v2 + 1; v3 < valleys.Count; v3++)
                    {
                        int iv3 = valleys[v3];
                        int ip2 = peakBetweenValleys[v2, v3];
                        if (ip2 < 0) continue;

                        double d1 = iv2 - iv1;
                        double d2 = iv3 - iv2;
                        if (d1 < 2.0 || d2 < 2.0) continue;

                        double maxD = Math.Max(d1, d2);
                        double regularity = Math.Abs(d1 - d2) / maxD;
                        if (regularity > 0.45) continue;

                        float avgPk = (sm[ip1] + sm[ip2]) / 2f;
                        float avgVl = (sm[iv1] + sm[iv2] + sm[iv3]) / 3f;
                        float amp = avgPk - avgVl;
                        if (amp < minProm) continue;

                        float contrast = (avgPk + avgVl > 0.001f) ? amp / (avgPk + avgVl) : 0f;
                        float score = contrast * (float)(1.0 - regularity);

                        if (score > bestScore)
                        {
                            bestScore = score;
                            bestFound = true;
                            bestIsBright = false;
                            bIdx1 = iv1; bIdx2 = iv2; bIdx3 = iv3;
                            vIdx1 = ip1; vIdx2 = ip2;
                        }
                    }
                }
            }

            if (!bestFound || bestScore < 0.03f) return false;

            // 4. Calculate threshold level
            float pkVal = bestIsBright ? ((sm[bIdx1] + sm[bIdx2] + sm[bIdx3]) / 3f) : ((sm[vIdx1] + sm[vIdx2]) / 2f);
            float vlVal = bestIsBright ? ((sm[vIdx1] + sm[vIdx2]) / 2f) : ((sm[bIdx1] + sm[bIdx2] + sm[bIdx3]) / 3f);
            float tLevel = vlVal + (threshPct / 100.0f) * (pkVal - vlVal);

            // 5. Segment bar boundaries using sub-pixel interpolation
            double c1 = -1, c2 = -1, c3 = -1, c4 = -1, c5 = -1, c6 = -1;

            if (bestIsBright)
            {
                c1 = FindCrossingBackward(yVals, bIdx1, 0, tLevel, isRising: true);
                c2 = FindCrossingForward(yVals, bIdx1, vIdx1, tLevel, isRising: false);

                c3 = FindCrossingBackward(yVals, bIdx2, vIdx1, tLevel, isRising: true);
                c4 = FindCrossingForward(yVals, bIdx2, vIdx2, tLevel, isRising: false);

                c5 = FindCrossingBackward(yVals, bIdx3, vIdx2, tLevel, isRising: true);
                c6 = FindCrossingForward(yVals, bIdx3, n - 1, tLevel, isRising: false);
            }
            else
            {
                c1 = FindCrossingBackward(yVals, bIdx1, 0, tLevel, isRising: false);
                c2 = FindCrossingForward(yVals, bIdx1, vIdx1, tLevel, isRising: true);

                c3 = FindCrossingBackward(yVals, bIdx2, vIdx1, tLevel, isRising: false);
                c4 = FindCrossingForward(yVals, bIdx2, vIdx2, tLevel, isRising: true);

                c5 = FindCrossingBackward(yVals, bIdx3, vIdx2, tLevel, isRising: false);
                c6 = FindCrossingForward(yVals, bIdx3, n - 1, tLevel, isRising: true);
            }

            if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0 || c5 < 0 || c6 < 0) return false;
            if (!(c1 < c2 && c2 <= c3 && c3 < c4 && c4 <= c5 && c5 < c6)) return false;

            result.Success = true;
            result.IsBrightBars = bestIsBright;
            result.Bar1Start = c1 * sampleStep;
            result.Bar1End = c2 * sampleStep;
            result.Bar2Start = c3 * sampleStep;
            result.Bar2End = c4 * sampleStep;
            result.Bar3Start = c5 * sampleStep;
            result.Bar3End = c6 * sampleStep;
            result.PeakIntensity = pkVal;
            result.ValleyIntensity = vlVal;
            result.ThresholdLevel = tLevel;

            return true;
        }

        // ponytail: the triplet search is O(P^3) in the number of candidate extrema; on a long noisy
        // profile that is thousands and it never returned. Above this many candidates only the
        // strongest are searched (the valleys between them are still taken from every extremum).
        private const int MaxCandidates = 256;

        private static List<int> Strongest(List<int> indices, float[] sm, bool highest)
        {
            if (indices.Count <= MaxCandidates) return indices;
            var ranked = highest ? indices.OrderByDescending(i => sm[i]) : indices.OrderBy(i => sm[i]);
            return ranked.Take(MaxCandidates).OrderBy(i => i).ToList();
        }

        // table[i, j]: the inner index strictly between outer[i] and outer[j] with the lowest (or
        // highest) value, the earliest on ties; -1 when there is none.
        private static int[,] BestBetween(List<int> outer, List<int> inner, float[] sm, bool wantMax)
        {
            var table = new int[outer.Count, outer.Count];
            for (int i = 0; i < outer.Count; i++)
            {
                int best = -1;
                int next = 0;
                while (next < inner.Count && inner[next] <= outer[i]) next++;
                for (int j = i + 1; j < outer.Count; j++)
                {
                    for (; next < inner.Count && inner[next] < outer[j]; next++)
                    {
                        if (best < 0 || (wantMax ? sm[inner[next]] > sm[best] : sm[inner[next]] < sm[best]))
                            best = inner[next];
                    }
                    table[i, j] = best;
                }
            }
            return table;
        }

        private static double FindCrossingBackward(float[] yVals, int startIdx, int limitIdx, float thresh, bool isRising)
        {
            for (int i = startIdx; i > limitIdx; i--)
            {
                bool crosses = isRising ? (yVals[i] >= thresh && yVals[i - 1] <= thresh)
                                        : (yVals[i] <= thresh && yVals[i - 1] >= thresh);
                if (crosses)
                {
                    float yA = yVals[i - 1];
                    float yB = yVals[i];
                    float denom = (yB != yA) ? (yB - yA) : 1f;
                    return (i - 1) + Math.Clamp((thresh - yA) / denom, 0f, 1f);
                }
            }
            for (int i = startIdx; i > limitIdx; i--)
            {
                if ((yVals[i] - thresh) * (yVals[i - 1] - thresh) <= 0)
                {
                    float yA = yVals[i - 1];
                    float yB = yVals[i];
                    float denom = (yB != yA) ? (yB - yA) : 1f;
                    return (i - 1) + Math.Clamp((thresh - yA) / denom, 0f, 1f);
                }
            }
            return -1;
        }

        private static double FindCrossingForward(float[] yVals, int startIdx, int limitIdx, float thresh, bool isRising)
        {
            for (int i = startIdx; i < limitIdx; i++)
            {
                bool crosses = isRising ? (yVals[i] <= thresh && yVals[i + 1] >= thresh)
                                        : (yVals[i] >= thresh && yVals[i + 1] <= thresh);
                if (crosses)
                {
                    float yA = yVals[i];
                    float yB = yVals[i + 1];
                    float denom = (yB != yA) ? (yB - yA) : 1f;
                    return i + Math.Clamp((thresh - yA) / denom, 0f, 1f);
                }
            }
            for (int i = startIdx; i < limitIdx; i++)
            {
                if ((yVals[i] - thresh) * (yVals[i + 1] - thresh) <= 0)
                {
                    float yA = yVals[i];
                    float yB = yVals[i + 1];
                    float denom = (yB != yA) ? (yB - yA) : 1f;
                    return i + Math.Clamp((thresh - yA) / denom, 0f, 1f);
                }
            }
            return -1;
        }

    }
}
