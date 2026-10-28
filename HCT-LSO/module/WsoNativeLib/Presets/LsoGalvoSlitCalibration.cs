using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace WsoNativeLib.Presets
{
    // Where the slit lands on the sensor for a galvo DAC, measured point by point, and the
    // fitted inverse of it - DAC(row) - that a calibrated FOV sweep needs to move the slit
    // across the sensor at a constant row rate (DevKit "Calibrated FOV Curve").
    //
    // The file is the DevKit's GalvoSlitCalibration*.json (LSO_DevKit LsoGalvoSlitCalibration),
    // so a table measured on either side loads on the other: Points[{SensorRow, GalvoDac,
    // MechanicalAngleDeg}] and SelectedModelType (1..5 = polynomial order). Every other key
    // (ScannerSettings, ...) is carried through untouched. The fit is never read from the file:
    // it is recomputed from the points on every load, exactly as the DevKit does.
    //
    // Device-free, like the rest of the preset stack, so program/TestCameraPreset can check it.
    public sealed class LsoGalvoSlitCalibration
    {
        public const string DefaultFileName = "GalvoSlitCalibration.json";
        public const int MaxPolynomialOrder = 5;

        // DevKit GalvoFitModelType.CubicSpline. Read so a DevKit file is recognised, refused by
        // TryFit: a polynomial is what both generators have been validated with.
        public const int CubicSplineModel = 10;

        public sealed class Point
        {
            public Point(double galvoDac, double? sensorRow, double? mechanicalAngleDeg = null)
            {
                GalvoDac = galvoDac;
                SensorRow = sensorRow;
                MechanicalAngleDeg = mechanicalAngleDeg;
            }

            public double GalvoDac { get; set; }

            // Null = this DAC was listed but its slit has not been measured yet.
            public double? SensorRow { get; set; }

            public double? MechanicalAngleDeg { get; set; }

            // GalvoDac - fit(SensorRow) after the last TryFit; NaN when not part of the fit.
            public double Residual { get; internal set; } = double.NaN;
        }

        private JsonObject _root = new JsonObject();

        private JsonObject ScannerSettings
        {
            get
            {
                if (_root["ScannerSettings"] is not JsonObject settings)
                {
                    settings = new JsonObject();
                    _root["ScannerSettings"] = settings;
                }
                return settings;
            }
        }

        public double OpticalCenterRow
        {
            get => ScannerSettings["OpticalCenterRow"]?.GetValue<double>() ?? 1824.0;
            set => ScannerSettings["OpticalCenterRow"] = value;
        }

        public string CameraType => ScannerSettings["CameraType"]?.GetValue<string>() ?? "BFS";

        public string CameraFileName => CameraType.Contains("Atlas", StringComparison.OrdinalIgnoreCase)
            || CameraType.Contains("ATX", StringComparison.OrdinalIgnoreCase)
            ? "GalvoSlitCalibration_Atlas.json"
            : CameraType.Contains("BFS", StringComparison.OrdinalIgnoreCase)
                ? "GalvoSlitCalibration_BFS.json" : DefaultFileName;

        // DevKit CreateDefaultCalibration: candidate rows are nominal, not measurements.
        public void ResetNominal(int totalRows)
        {
            double maxAngle = ScannerSettings["MechanicalAngleMaxDeg"]?.GetValue<double>() ?? 7.5;
            double scale = ScannerSettings["DacPerMechanicalDeg"]?.GetValue<double>() ?? 32767.0 / 7.5;
            double offset = ScannerSettings["DacOffset"]?.GetValue<double>() ?? 0.0;
            double minDac = ScannerSettings["MinDacNegative"]?.GetValue<double>() ?? short.MinValue;
            double maxDac = ScannerSettings["MaxDacPositive"]?.GetValue<double>() ?? short.MaxValue;
            ScannerSettings["SensorPhysicalRows"] = totalRows;
            if (totalRows == 3648 || totalRows == 5680 || totalRows == 5588)
            {
                ScannerSettings["CameraType"] = totalRows == 3648 ? "BFS" : "Atlas";
                OpticalCenterRow = totalRows / 2.0;
            }
            Points.Clear();
            for (int i = 0; i < 9; i++)
            {
                double fraction = i / 8.0;
                double angle = maxAngle + fraction * (-2.0 * maxAngle);
                double dac = Math.Clamp(Math.Round(angle * scale + offset), minDac, maxDac);
                Points.Add(new Point(dac, Math.Round(fraction * totalRows), Math.Round(angle, 3)));
            }
            ModelType = 3;
        }

        // DevKit Prev/Next never wrap; no selection behaves as index zero.
        public static int SelectAdjacentPoint(int current, int count, int direction) =>
            count == 0 ? -1 : Math.Clamp(Math.Max(0, current) + direction, 0, count - 1);

        // FOV live frames retain the packed format tag after unpacking into uint16 samples.
        public static int FrameBitDepth(int pixelFormat, int bytesPerPixel)
        {
            return pixelFormat == (int)WsoLsoDefs.ColorPixelFormat.BayerRG12Packed ? 12
                : bytesPerPixel == 2 ? 16 : 8;
        }

        public static double? DetectFrameSlitCenter(byte[] data, int width, int height,
            int bytesPerPixel, int pixelFormat, out int top, out int bottom)
        {
            return DetectSlitCenter(data, width, height, bytesPerPixel, out top, out bottom,
                FrameBitDepth(pixelFormat, bytesPerPixel));
        }

        // Literal DevKit center-ROI detector. Sensor samples are reduced to the displayed 8-bit
        // brightness scale; packed RGB/BGR use the same integer luminance as its bitmap.
        public static double? DetectSlitCenter(byte[] data, int width, int height, int bytesPerPixel,
            out int top, out int bottom, int bitDepth = 16)
        {
            top = bottom = 0;
            if (width <= 0 || height <= 0 || data.Length < (long)width * height * bytesPerPixel
                || (bytesPerPixel != 1 && bytesPerPixel != 2 && bytesPerPixel != 3)
                || bitDepth < 8 || bitDepth > 16) return null;
            int start = (int)(width * 0.45);
            int roiWidth = Math.Max(10, (int)(width * 0.10));
            if (start + roiWidth > width) return null;
            int step = Math.Max(1, roiWidth / 32);
            int sampledCols = 0;
            var profile = new double[height];
            for (int x = 0; x < roiWidth; x += step)
            {
                sampledCols++;
                for (int y = 0; y < height; y++)
                {
                    int index = (y * width + start + x) * bytesPerPixel;
                    profile[y] += bytesPerPixel == 3
                        ? (data[index + 2] + (data[index + 1] << 1) + data[index]) >> 2
                        : bytesPerPixel == 2
                            ? (data[index] | (data[index + 1] << 8)) >> (bitDepth - 8)
                            : data[index];
                }
            }
            double max = 0;
            int peak = -1;
            for (int y = 0; y < height; y++)
            {
                profile[y] /= sampledCols;
                if (profile[y] > max) { max = profile[y]; peak = y; }
            }
            if (max < 25.0 || peak < 0) return null;
            double bg = (profile[0] + profile[height - 1]) / 2.0;
            double threshold = bg + 0.35 * (max - bg);
            top = bottom = peak;
            while (top > 0 && profile[top] > threshold) top--;
            while (bottom < height - 1 && profile[bottom] > threshold) bottom++;
            double weighted = 0, weightSum = 0;
            for (int y = top; y <= bottom; y++)
            {
                double weight = Math.Max(0, profile[y] - bg);
                weighted += y * weight;
                weightSum += weight;
            }
            return weightSum > 0 ? weighted / weightSum : null;
        }

        public static double? DetectSlitCenter(ushort[] data, int width, int height, out int top, out int bottom,
            int bitDepth = 16)
        {
            var bytes = new byte[data.Length * 2];
            for (int i = 0; i < data.Length; i++)
            {
                bytes[i * 2] = (byte)data[i];
                bytes[i * 2 + 1] = (byte)(data[i] >> 8);
            }
            return DetectSlitCenter(bytes, width, height, 2, out top, out bottom, bitDepth);
        }

        public List<Point> Points { get; } = new List<Point>();

        // Polynomial order 1..5 (DevKit SelectedModelType). Default Poly3, the DevKit's shipped choice.
        public int ModelType { get; set; } = 3;

        // --- Fit result (valid after a successful TryFit) ---------------------------------

        public bool IsFitted { get; private set; }

        // Order actually fitted: lower than ModelType when there are too few points for it.
        public int FittedOrder { get; private set; }

        public int FittedPointCount { get; private set; }

        public double R2 { get; private set; }

        public double Rmse { get; private set; }

        public double MaxResidual { get; private set; }

        // Coefficients in the normalised variable x = (row - _rowCentre) / _rowScale, which keeps
        // the normal equations well conditioned at 3648+ rows. Lowest order first.
        private double[] _coefficients = Array.Empty<double>();
        private double _rowCentre;
        private double _rowScale = 1.0;

        public string Describe()
        {
            return IsFitted
                ? string.Format(CultureInfo.InvariantCulture, "Poly{0} | N={1} | R\u00b2={2:F4} | RMSE={3:F1} | max {4:F1} DAC",
                    FittedOrder, FittedPointCount, R2, Rmse, MaxResidual)
                : "not fitted";
        }

        // Least squares DAC(row) over the measured points. False, with the reason, when the
        // points cannot describe a curve; the previous fit is dropped either way.
        public bool TryFit(out string error)
        {
            IsFitted = false;
            FittedOrder = 0;
            FittedPointCount = 0;
            R2 = Rmse = MaxResidual = 0.0;
            _coefficients = Array.Empty<double>();
            foreach (Point p in Points)
            {
                p.Residual = double.NaN;
            }
            for (int i = 0; i < Points.Count; i++)
            {
                if (!TryValidatePoint(Points[i], out string pointError))
                {
                    error = $"Points[{i}]: {pointError}";
                    return false;
                }
            }

            if (ModelType == CubicSplineModel)
            {
                error = "the DevKit cubic-spline model is not supported here - choose a polynomial order 1..5.";
                return false;
            }
            if (ModelType < 1 || ModelType > MaxPolynomialOrder)
            {
                error = $"model {ModelType} is not a polynomial order 1..{MaxPolynomialOrder}.";
                return false;
            }

            List<Point> valid = Points.Where(p => p.SensorRow.HasValue).ToList();
            int n = valid.Count;
            if (n < 2)
            {
                error = $"{n} measured point(s) - at least 2 are needed to fit.";
                return false;
            }
            if (valid.All(p => p.GalvoDac == valid[0].GalvoDac))
            {
                error = "every measured point has the same DAC - the calibration has no sweep range.";
                return false;
            }

            // DevKit rule: with too few points for the order, fit the highest order they allow.
            int order = n <= ModelType ? Math.Max(1, n - 1) : ModelType;

            double rowMin = valid.Min(p => p.SensorRow!.Value);
            double rowMax = valid.Max(p => p.SensorRow!.Value);
            _rowCentre = (rowMin / 2.0) + (rowMax / 2.0);
            _rowScale = (rowMax / 2.0) - (rowMin / 2.0);
            if (_rowScale < 5e-7)
            {
                error = "every measured point has the same sensor row - move the galvo further between points.";
                return false;
            }

            // Normal equations (A^T A) c = A^T y, A[i][k] = x_i^k.
            int m = order + 1;
            var ata = new double[m, m];
            var aty = new double[m];
            foreach (Point p in valid)
            {
                double x = (p.SensorRow!.Value - _rowCentre) / _rowScale;
                var powers = new double[2 * m - 1];
                powers[0] = 1.0;
                for (int k = 1; k < powers.Length; k++)
                {
                    powers[k] = powers[k - 1] * x;
                }
                for (int r = 0; r < m; r++)
                {
                    aty[r] += powers[r] * p.GalvoDac;
                    for (int c = 0; c < m; c++)
                    {
                        ata[r, c] += powers[r + c];
                    }
                }
            }

            double[]? solved = SolveLinearSystem(ata, aty);
            if (solved == null || solved.Any(c => !double.IsFinite(c)))
            {
                error = "the points do not determine a curve of that order (singular system) - add points with distinct rows.";
                return false;
            }
            _coefficients = solved;
            if (!TryValidateCurve(rowMin, rowMax, out error))
            {
                return false;
            }

            double mean = valid.Average(p => p.GalvoDac);
            double ssRes = 0.0;
            double ssTot = 0.0;
            double maxRes = 0.0;
            var residuals = new double[n];
            for (int i = 0; i < n; i++)
            {
                Point p = valid[i];
                double res = p.GalvoDac - EvaluateRaw(p.SensorRow!.Value);
                residuals[i] = res;
                ssRes += res * res;
                ssTot += (p.GalvoDac - mean) * (p.GalvoDac - mean);
                maxRes = Math.Max(maxRes, Math.Abs(res));
            }
            if (!double.IsFinite(ssRes) || !double.IsFinite(ssTot) || !double.IsFinite(maxRes))
            {
                error = "the fitted curve has non-finite residuals.";
                return false;
            }
            for (int i = 0; i < n; i++)
            {
                valid[i].Residual = residuals[i];
            }

            FittedOrder = order;
            FittedPointCount = n;
            Rmse = Math.Sqrt(ssRes / n);
            MaxResidual = maxRes;
            R2 = ssTot > 1e-12 ? Math.Max(0.0, 1.0 - (ssRes / ssTot)) : 1.0;
            IsFitted = true;
            error = string.Empty;
            return true;
        }

        // Fitted DAC for a sensor row, clamped to the galvo's int16 range (DevKit EvaluateDac).
        public double EvaluateDac(double sensorRow)
        {
            if (!IsFitted)
            {
                throw new InvalidOperationException("EvaluateDac before a successful TryFit.");
            }
            if (!double.IsFinite(sensorRow))
            {
                throw new ArgumentOutOfRangeException(nameof(sensorRow), "Sensor row must be finite.");
            }
            double dac = EvaluateRaw(sensorRow);
            if (!double.IsFinite(dac))
            {
                throw new InvalidOperationException("The fitted DAC is not finite at that sensor row.");
            }
            return Math.Clamp(dac, short.MinValue, short.MaxValue);
        }

        private double EvaluateRaw(double sensorRow)
        {
            double x = (sensorRow - _rowCentre) / _rowScale;
            return EvaluatePolynomial(_coefficients, x);
        }

        private static double EvaluatePolynomial(double[] coefficients, double x)
        {
            double sum = 0.0;
            for (int k = coefficients.Length - 1; k >= 0; k--)
            {
                sum = (sum * x) + coefficients[k];
            }
            return sum;
        }

        // Validate the continuous polynomial, not just its sampled table: a high-order fit can
        // turn around between measured points or outside them when covering the full sensor.
        private bool TryValidateCurve(double firstRow, double lastRow, out string error)
        {
            double firstX = (firstRow - _rowCentre) / _rowScale;
            double lastX = (lastRow - _rowCentre) / _rowScale;
            double firstDac = EvaluatePolynomial(_coefficients, firstX);
            double lastDac = EvaluatePolynomial(_coefficients, lastX);
            if (!double.IsFinite(firstX) || !double.IsFinite(lastX)
                || !double.IsFinite(firstDac) || !double.IsFinite(lastDac))
            {
                error = "the fitted curve is not finite over the requested row range.";
                return false;
            }
            if (firstDac == lastDac)
            {
                error = "the fitted curve has no DAC range.";
                return false;
            }

            double[] slope = Differentiate(_coefficients);
            var probes = new List<double> { firstX, lastX };
            if (!TryFindPolynomialRoots(Differentiate(slope), firstX, lastX, probes))
            {
                error = "the fitted curve has non-finite slope extrema.";
                return false;
            }
            int direction = lastDac > firstDac ? 1 : -1;
            foreach (double x in probes)
            {
                double value = EvaluatePolynomial(slope, x);
                // Allow only floating-point noise at a stationary tangent (DAC per normalised row).
                if (!double.IsFinite(value) || (direction * value) < -1e-8)
                {
                    error = "the fitted curve is not finite and monotonic over the requested row range.";
                    return false;
                }
            }
            error = string.Empty;
            return true;
        }

        private static double[] Differentiate(double[] coefficients)
        {
            var derivative = new double[Math.Max(0, coefficients.Length - 1)];
            for (int i = 1; i < coefficients.Length; i++)
            {
                derivative[i - 1] = i * coefficients[i];
            }
            return derivative;
        }

        // Roots of the second derivative (degree <= 3) locate every slope extremum. Recursive
        // derivative roots split it into monotonic intervals; bisection resolves each crossing.
        private static bool TryFindPolynomialRoots(double[] coefficients, double first, double last,
            List<double> roots)
        {
            if (coefficients.Length <= 1)
            {
                return coefficients.Length == 0 || double.IsFinite(coefficients[0]);
            }
            var boundaries = new List<double> { first };
            if (!TryFindPolynomialRoots(Differentiate(coefficients), first, last, boundaries))
            {
                return false;
            }
            boundaries.Add(last);
            boundaries.Sort();
            for (int i = 1; i < boundaries.Count; i++)
            {
                double left = boundaries[i - 1];
                double right = boundaries[i];
                double leftValue = EvaluatePolynomial(coefficients, left);
                double rightValue = EvaluatePolynomial(coefficients, right);
                if (!double.IsFinite(leftValue) || !double.IsFinite(rightValue))
                {
                    return false;
                }
                if (leftValue == 0.0)
                {
                    roots.Add(left);
                }
                if (rightValue == 0.0)
                {
                    roots.Add(right);
                }
                if (leftValue == 0.0 || rightValue == 0.0 || (leftValue > 0) == (rightValue > 0))
                {
                    continue;
                }
                for (int iteration = 0; iteration < 128; iteration++)
                {
                    double middle = (left / 2.0) + (right / 2.0);
                    if (middle == left || middle == right)
                    {
                        break;
                    }
                    double value = EvaluatePolynomial(coefficients, middle);
                    if (!double.IsFinite(value))
                    {
                        return false;
                    }
                    if (value == 0.0)
                    {
                        left = right = middle;
                        break;
                    }
                    if ((value > 0) == (leftValue > 0))
                    {
                        left = middle;
                        leftValue = value;
                    }
                    else
                    {
                        right = middle;
                    }
                }
                roots.Add((left / 2.0) + (right / 2.0));
            }
            return true;
        }

        // Galvo Start / End a calibrated sweep over `sensorRows` rows describes: the fit at the
        // first and last row (DevKit LsoPatternForm.UpdateGalvoPosControlsForTrajectoryMode).
        public void SweepEnds(int sensorRows, out short start, out short end)
        {
            if (sensorRows <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(sensorRows), "Sensor Rows must be positive.");
            }
            start = ToDac(EvaluateDac(0));
            end = ToDac(EvaluateDac(sensorRows - 1));
        }

        // The board table of a calibrated FOV sweep, a port of the DevKit's board writer
        // (LsoScanFormUnified.MakeLinearProfileLSO, TrajectoryMode == 1) with one deliberate
        // difference: the row count is the preset's Sensor Rows instead of a per-camera
        // constant. The active part steps the slit evenly from row 0 to row `sensorRows`
        // through the fit, reversed when Start/End run the other way than the fit; the lead-in
        // approaches the first active sample at the active start velocity; the lead-out holds
        // the last value. Null, with the reason, when the geometry has no active part.
        public short[]? GenerateTrajectory(short galvoStartPos, short galvoEndPos, int sampleSize,
            int leadInSamples, int leadOutSamples, int sensorRows, out string error)
        {
            if (!IsFitted)
            {
                error = "the calibration is not fitted.";
                return null;
            }
            if (sampleSize <= 0 || sampleSize >= WsoLsoScan.LSO_SCANNER_SAMPLE_SIZE_MAX)
            {
                error = $"Pattern Size {sampleSize} is outside 1..{WsoLsoScan.LSO_SCANNER_SAMPLE_SIZE_MAX - 1}.";
                return null;
            }
            if (sensorRows <= 0)
            {
                error = "Sensor Rows is not stated - the calibrated sweep has no row range to cover.";
                return null;
            }

            if (leadInSamples <= 0 || leadInSamples >= sampleSize || leadOutSamples < 0
                || leadOutSamples >= sampleSize - leadInSamples)
            {
                error = $"Lead {leadInSamples} / Tail {leadOutSamples} leave no active sweep in Pattern Size {sampleSize} "
                    + "- Recalculate the timing first.";
                return null;
            }
            int active = sampleSize - leadInSamples - leadOutSamples;
            if (!TryValidateCurve(0.0, sensorRows, out error))
            {
                return null;
            }

            double startRow = 0.0;
            double endRow = sensorRows;
            if (ToDac(EvaluateDac(startRow)) == ToDac(EvaluateDac(endRow)))
            {
                error = "the fitted sweep has no DAC range after clamping and rounding.";
                return null;
            }
            bool reverse = (galvoStartPos > galvoEndPos) != (EvaluateDac(startRow) > EvaluateDac(endRow));
            if (reverse)
            {
                (startRow, endRow) = (endRow, startRow);
            }

            var pos = new short[sampleSize];
            var activeDac = new double[active];
            for (int i = 0; i < active; i++)
            {
                double frac = active > 1 ? (double)i / (active - 1) : 0.0;
                double dac = EvaluateRaw(startRow + (frac * (endRow - startRow)));
                if (!double.IsFinite(dac))
                {
                    error = $"the fitted DAC is not finite at active sample {i}.";
                    return null;
                }
                activeDac[i] = Math.Clamp(dac, short.MinValue, short.MaxValue);
                pos[leadInSamples + i] = ToDac(activeDac[i]);
            }

            double vStart = active > 1 ? activeDac[1] - activeDac[0] : 0.0;
            for (int i = 0; i < leadInSamples; i++)
            {
                pos[i] = ToDac(activeDac[0] - (vStart * (leadInSamples - i)));
            }

            short finalDac = ToDac(activeDac[active - 1]);
            for (int i = 0; i < leadOutSamples; i++)
            {
                pos[leadInSamples + active + i] = finalDac;
            }
            pos[sampleSize - 1] = finalDac;

            error = string.Empty;
            return pos;
        }

        // DevKit rounding: (short)Math.Clamp((long)Math.Round(x), -32768, 32767) - to-even.
        private static short ToDac(double value)
        {
            return (short)Math.Clamp(Math.Round(value), short.MinValue, short.MaxValue);
        }

        // Gaussian elimination with partial pivoting; null when singular.
        private static double[]? SolveLinearSystem(double[,] a, double[] b)
        {
            int n = b.Length;
            var m = (double[,])a.Clone();
            var y = (double[])b.Clone();
            for (int col = 0; col < n; col++)
            {
                int pivot = col;
                for (int r = col + 1; r < n; r++)
                {
                    if (Math.Abs(m[r, col]) > Math.Abs(m[pivot, col]))
                    {
                        pivot = r;
                    }
                }
                if (Math.Abs(m[pivot, col]) < 1e-12)
                {
                    return null;
                }
                if (pivot != col)
                {
                    for (int c = 0; c < n; c++)
                    {
                        (m[col, c], m[pivot, c]) = (m[pivot, c], m[col, c]);
                    }
                    (y[col], y[pivot]) = (y[pivot], y[col]);
                }
                for (int r = col + 1; r < n; r++)
                {
                    double f = m[r, col] / m[col, col];
                    for (int c = col; c < n; c++)
                    {
                        m[r, c] -= f * m[col, c];
                    }
                    y[r] -= f * y[col];
                }
            }

            var x = new double[n];
            for (int r = n - 1; r >= 0; r--)
            {
                double sum = y[r];
                for (int c = r + 1; c < n; c++)
                {
                    sum -= m[r, c] * x[c];
                }
                x[r] = sum / m[r, r];
            }
            return x;
        }

        // --- File ---------------------------------------------------------------------------

        // Only absent/null optional values mean "not measured"; malformed stated numbers
        // must not silently shorten the calibration or select a different model.
        public static bool TryLoad(string path, out LsoGalvoSlitCalibration? calibration, out string error)
        {
            calibration = null;
            JsonObject? root;
            try
            {
                root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is JsonException)
            {
                error = $"'{path}': {e.Message}";
                return false;
            }
            if (root == null)
            {
                error = $"'{path}': not a JSON object.";
                return false;
            }

            var result = new LsoGalvoSlitCalibration { _root = root };
            if (root.ContainsKey("SelectedModelType"))
            {
                if (root["SelectedModelType"] is not JsonValue model || !model.TryGetValue(out int modelType)
                    || (modelType != CubicSplineModel && (modelType < 1 || modelType > MaxPolynomialOrder)))
                {
                    error = $"'{path}': SelectedModelType is not a known integer model (1..5 or 10).";
                    return false;
                }
                result.ModelType = modelType;
            }

            if (root["Points"] is not JsonArray points)
            {
                error = $"'{path}': Points is not an array.";
                return false;
            }
            for (int i = 0; i < points.Count; i++)
            {
                if (points[i] is not JsonObject p || !TryReadDouble(p["GalvoDac"], out double dac))
                {
                    error = $"'{path}': Points[{i}] has no finite numeric GalvoDac.";
                    return false;
                }
                double? row = null;
                double? angle = null;
                if (p["SensorRow"] != null)
                {
                    if (!TryReadDouble(p["SensorRow"], out double r))
                    {
                        error = $"'{path}': Points[{i}].SensorRow is not a finite number or null.";
                        return false;
                    }
                    row = r;
                }
                if (p["MechanicalAngleDeg"] != null)
                {
                    if (!TryReadDouble(p["MechanicalAngleDeg"], out double a))
                    {
                        error = $"'{path}': Points[{i}].MechanicalAngleDeg is not a finite number or null.";
                        return false;
                    }
                    angle = a;
                }
                var point = new Point(dac, row, angle);
                if (!TryValidatePoint(point, out string pointError))
                {
                    error = $"'{path}': Points[{i}]: {pointError}";
                    return false;
                }
                result.Points.Add(point);
            }

            calibration = result;
            error = string.Empty;
            return true;
        }

        private static bool TryReadDouble(JsonNode? node, out double value)
        {
            value = 0.0;
            return node is JsonValue v && v.TryGetValue(out value) && double.IsFinite(value);
        }

        private static bool TryValidatePoint(Point point, out string error)
        {
            if (!double.IsFinite(point.GalvoDac) || point.GalvoDac < short.MinValue || point.GalvoDac > short.MaxValue)
            {
                error = "GalvoDac must be finite and in -32768..32767.";
                return false;
            }
            if (point.SensorRow.HasValue && !double.IsFinite(point.SensorRow.Value))
            {
                error = "SensorRow must be finite or null.";
                return false;
            }
            if (point.MechanicalAngleDeg.HasValue && !double.IsFinite(point.MechanicalAngleDeg.Value))
            {
                error = "MechanicalAngleDeg must be finite or null.";
                return false;
            }
            error = string.Empty;
            return true;
        }

        // Writes the points and the model; keys this class does not own stay as they were read.
        // ActiveFit / IsCalibrated are written for a DevKit reader's display only - both sides
        // refit from the points.
        public void Save(string path)
        {
            var points = new JsonArray();
            foreach (Point p in Points)
            {
                points.Add(new JsonObject
                {
                    ["SensorRow"] = p.SensorRow.HasValue ? JsonValue.Create(p.SensorRow.Value) : null,
                    ["GalvoDac"] = JsonValue.Create(p.GalvoDac),
                    ["MechanicalAngleDeg"] = p.MechanicalAngleDeg.HasValue ? JsonValue.Create(p.MechanicalAngleDeg.Value) : null,
                });
            }

            _root["Points"] = points;
            _root["SelectedModelType"] = ModelType;
            _root["ActiveFit"] = new JsonObject
            {
                ["ModelType"] = IsFitted ? FittedOrder : ModelType,
                ["R2"] = IsFitted ? R2 : 0.0,
                ["Rmse"] = IsFitted ? Rmse : 0.0,
                ["MaxResidual"] = IsFitted ? MaxResidual : 0.0,
                ["IsValid"] = IsFitted,
                ["FormulaString"] = Describe(),
            };
            _root["IsCalibrated"] = IsFitted;

            string? directory = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }
            File.WriteAllText(path, _root.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
        }
    }
}
