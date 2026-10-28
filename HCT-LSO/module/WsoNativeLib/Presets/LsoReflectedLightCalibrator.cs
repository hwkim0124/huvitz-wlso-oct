using System;
using System.Collections.Generic;
using System.Globalization;
using System.Threading;
using System.Threading.Tasks;

namespace WsoNativeLib.Presets
{
    public sealed record LsoReflectedLightProgress(int Point, int PointCount, int Frame, int FramesPerPoint, string Text);
    public sealed record LsoReflectedLightCalibrationResult(bool Succeeded, bool Cancelled, int PointsWritten, string Error, int RestoredPosition);

    public sealed class LsoReflectedLightCalibrator
    {
        public const int ReachTolerance = 50;
        private readonly Func<int, CancellationToken, Task<int>> _moveFocus;
        private readonly Func<CancellationToken, Task<byte[]>> _captureOne;
        private readonly Func<int> _readFocus;

        public LsoReflectedLightCalibrator(Func<int, CancellationToken, Task<int>> moveFocus,
            Func<CancellationToken, Task<byte[]>> captureOne, Func<int> readFocus)
        {
            _moveFocus = moveFocus;
            _captureOne = captureOne;
            _readFocus = readFocus;
        }

        public static IReadOnlyList<(double Diopter, int Target)> PlanPoints(double startDiopter,
            double endDiopter, double stepDiopter, Func<double, int> diopterToPosition)
        {
            if (!double.IsFinite(startDiopter) || !double.IsFinite(endDiopter)
                || !double.IsFinite(stepDiopter) || stepDiopter <= 0 || startDiopter > endDiopter)
                throw new ArgumentException("Diopters must be finite, start <= end and step > 0.");
            double count = Math.Round((endDiopter - startDiopter) / stepDiopter) + 1;
            if (!double.IsFinite(count) || count > 1000)
                throw new ArgumentException("At most 1000 calibration points are allowed.");
            var points = new List<(double Diopter, int Target)>((int)count);
            for (int i = 0; i < (int)count; i++)
            {
                double diopter = Math.Round(startDiopter + i * stepDiopter, 2);
                points.Add((diopter, diopterToPosition(diopter)));
            }
            return points.AsReadOnly();
        }

        public async Task<LsoReflectedLightCalibrationResult> RunAsync(string directory,
            IReadOnlyList<(double Diopter, int Target)> points, int framesPerPoint, LsoReflectedLightRoi roi,
            LsoReflectedLightConditions conditions, IProgress<LsoReflectedLightProgress>? progress,
            CancellationToken cancel)
        {
            int start;
            try { start = _readFocus(); }
            catch (Exception e)
            {
                return new LsoReflectedLightCalibrationResult(false, e is OperationCanceledException, 0, e.Message, 0);
            }
            int restored = start, written = 0;
            bool succeeded = false, cancelled = false, restoreAttempted = false;
            string error = string.Empty;
            try
            {
                if (framesPerPoint <= 0) throw new ArgumentException("FramesPerPoint must be positive.");
                var collected = new List<(LsoReflectedLightPoint Point, ushort[] Map)>();
                for (int i = 0; i < points.Count; i++)
                {
                    cancel.ThrowIfCancellationRequested();
                    var point = points[i];
                    int reached = await _moveFocus(point.Target, cancel).ConfigureAwait(false);
                    restored = reached;
                    if (Math.Abs((long)reached - point.Target) > ReachTolerance)
                        throw new InvalidOperationException(FormattableString.Invariant(
                            $"Focus at {point.Diopter} D: target {point.Target}, reached {reached}."));
                    var frames = new List<byte[]>(framesPerPoint);
                    for (int frame = 0; frame < framesPerPoint; frame++)
                    {
                        cancel.ThrowIfCancellationRequested();
                        frames.Add(await _captureOne(cancel).ConfigureAwait(false));
                        progress?.Report(new LsoReflectedLightProgress(i + 1, points.Count,
                            frame + 1, framesPerPoint, FormattableString.Invariant($"Capturing {point.Diopter} D")));
                    }
                    cancel.ThrowIfCancellationRequested();
                    bool built = LsoReflectedLightSet.TryBuildPoint(frames, (int)conditions.Width,
                        (int)conditions.Height, roi, out var map, out var floor, out string buildError);
                    // Only the compact ROI map survives into the next point.
                    frames.Clear();
                    if (!built) throw new InvalidOperationException(buildError);
                    collected.Add((new LsoReflectedLightPoint
                    {
                        Diopter = point.Diopter, Position = reached,
                        File = "p_" + reached.ToString(CultureInfo.InvariantCulture) + ".pgm", Floor = floor,
                    }, map));
                }
                cancel.ThrowIfCancellationRequested();
                restoreAttempted = true;
                try { restored = await _moveFocus(start, CancellationToken.None).ConfigureAwait(false); }
                catch (Exception e) { throw new InvalidOperationException("Focus restore failed: " + e.Message, e); }
                if (Math.Abs((long)restored - start) > ReachTolerance)
                    throw new InvalidOperationException(FormattableString.Invariant(
                        $"Focus restore failed: target {start}, reached {restored}."));
                cancel.ThrowIfCancellationRequested();
                if (!LsoReflectedLightSet.TryWrite(directory, conditions, roi, framesPerPoint, collected, out error))
                    throw new InvalidOperationException(error);
                written = collected.Count;
                succeeded = true;
            }
            catch (OperationCanceledException e) { cancelled = true; error = e.Message; }
            catch (Exception e) { error = e.Message; }
            finally
            {
                if (!restoreAttempted)
                {
                    try
                    {
                        restoreAttempted = true;
                        restored = await _moveFocus(start, CancellationToken.None).ConfigureAwait(false);
                        if (Math.Abs((long)restored - start) > ReachTolerance)
                            throw new InvalidOperationException(FormattableString.Invariant(
                                $"target {start}, reached {restored}."));
                    }
                    catch (Exception e)
                    {
                        succeeded = false;
                        error = (error.Length == 0 ? string.Empty : error + " ") + "Focus restore failed: " + e.Message;
                    }
                }
            }
            return new LsoReflectedLightCalibrationResult(succeeded, cancelled, written, error, restored);
        }
    }
}
