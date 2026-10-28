using System;
using System.Globalization;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // The four DevKit alignment numbers, lifted out of LsoPatternForm.UpdateTimingVisualization
    // ("--- 5. Quantitative Synchronization & Alignment Metrics ---") so the scan-test status
    // line, the trajectory editor's shutter model and program/TestCameraPreset all use the same
    // arithmetic. Device-free and window-free on purpose: the check runs it with no camera,
    // board or native DLL.
    //
    // Time base matches the DevKit: t = 0 is the first DAC sample. In TRIG_FIRST the camera was
    // triggered before t = 0, so row 0 opens after the blanking rows; in GALVANO_FIRST the
    // trigger fires after the pre-move delay, so camera delay and blanking follow it.
    //
    // lineTimeUs is the value the operator reads (two decimals), not 1e6 / line rate at full
    // precision - the DevKit reads txtSensorLineTime. LsoScanTimingCalculator rounds the same
    // way, so the timing it computes and the metrics shown beside it describe one scan.
    public static class LsoAlignmentMetrics
    {
        public readonly struct Inputs
        {
            public Inputs(int timeStepUs, double lineTimeUs, int sensorRows,
                int leadInSamples, int leadOutSamples, double galvoDelayUs,
                int exposureTimeUs, int slitHeightRows, int sensorBlankingRows,
                double cameraTriggerDelayUs, double preMoveDelayUs,
                LsoScanSequenceMode sequenceMode)
            {
                TimeStepUs = timeStepUs;
                LineTimeUs = lineTimeUs;
                SensorRows = sensorRows;
                LeadInSamples = leadInSamples;
                LeadOutSamples = leadOutSamples;
                GalvoDelayUs = galvoDelayUs;
                ExposureTimeUs = exposureTimeUs;
                SlitHeightRows = slitHeightRows;
                SensorBlankingRows = sensorBlankingRows;
                CameraTriggerDelayUs = cameraTriggerDelayUs;
                PreMoveDelayUs = preMoveDelayUs;
                SequenceMode = sequenceMode;
            }

            public int TimeStepUs { get; }

            public double LineTimeUs { get; }

            public int SensorRows { get; }

            public int LeadInSamples { get; }

            public int LeadOutSamples { get; }

            public double GalvoDelayUs { get; }

            public int ExposureTimeUs { get; }

            public int SlitHeightRows { get; }

            public int SensorBlankingRows { get; }

            public double CameraTriggerDelayUs { get; }

            public double PreMoveDelayUs { get; }

            public LsoScanSequenceMode SequenceMode { get; }
        }

        public readonly struct Result
        {
            public Result(double row0OpenTimeUs, double actualRow0MidUs, double alignErrorUs,
                double alignErrorRows, double slitOverlapPercent, double activeSweepMs,
                double decelMarginUs, double row0OffsetUs)
            {
                Row0OpenTimeUs = row0OpenTimeUs;
                ActualRow0MidUs = actualRow0MidUs;
                AlignErrorUs = alignErrorUs;
                AlignErrorRows = alignErrorRows;
                SlitOverlapPercent = slitOverlapPercent;
                ActiveSweepMs = activeSweepMs;
                DecelMarginUs = decelMarginUs;
                Row0OffsetUs = row0OffsetUs;
            }

            // Slit-centre arrival time at row 0 the camera timing asks for (exposure midpoint).
            public double Row0OpenTimeUs { get; }

            // Row 0 midpoint the galvo actually delivers (lead-in + Galvo Delay).
            public double ActualRow0MidUs { get; }

            // Actual - optimal. Negative: the slit arrives early.
            public double AlignErrorUs { get; }

            public double AlignErrorRows { get; }

            public double SlitOverlapPercent { get; }

            public double ActiveSweepMs { get; }

            public double DecelMarginUs { get; }

            // Row 0 offset time = blanking rows x line time (DevKit "Row 0 Offset").
            public double Row0OffsetUs { get; }

            public bool IsCentered => Math.Abs(AlignErrorRows) <= 1.5;

            public bool IsAcceptable => Math.Abs(AlignErrorRows) <= 4.0;
        }

        // DevKit tRow0OpenUs. blankingTimeUs is SensorBlankingRows x LineTime; it is passed in
        // rather than derived so the trajectory editor keeps its own line-time precision.
        public static double Row0OpenTimeUs(LsoScanSequenceMode sequenceMode,
            double preMoveDelayUs, double cameraTriggerDelayUs, double blankingTimeUs)
        {
            return sequenceMode == LsoScanSequenceMode.GalvanoFirst
                ? preMoveDelayUs + cameraTriggerDelayUs + blankingTimeUs
                : blankingTimeUs;
        }

        // Slit-centre crossing time of sensor row `row` minus the row's exposure midpoint, in
        // rows. Row 0 with actualRow0MidUs is the DevKit "Row 0 Centering Error".
        public static double CenteringErrorRows(double actualMidTimeUs, double row0OpenTimeUs,
            int row, double lineTimeUs, double exposureUs)
        {
            return (actualMidTimeUs - (row0OpenTimeUs + (row * lineTimeUs) + (exposureUs / 2.0)))
                / lineTimeUs;
        }

        public static bool TryCompute(in Inputs inputs, out Result result, out string reason)
        {
            result = default;

            if (inputs.TimeStepUs <= 0 || inputs.LineTimeUs <= 0.0 || inputs.SensorRows <= 0)
            {
                reason = "Time Step, Line Time and Sensor Rows must all be positive.";
                return false;
            }
            if (inputs.LeadInSamples < 0 || inputs.LeadOutSamples < 0)
            {
                reason = "Lead and Tail must not be negative.";
                return false;
            }
            if (inputs.ExposureTimeUs < 0 || inputs.SlitHeightRows < 0
                || inputs.SensorBlankingRows < 0 || inputs.CameraTriggerDelayUs < 0.0
                || inputs.PreMoveDelayUs < 0.0 || inputs.GalvoDelayUs < 0.0)
            {
                reason = "Exposure, slit, blanking, camera delay and galvo delay must not be negative.";
                return false;
            }

            double lineTimeUs = inputs.LineTimeUs;
            double blankingTimeUs = inputs.SensorBlankingRows * lineTimeUs;
            double row0OpenUs = Row0OpenTimeUs(inputs.SequenceMode,
                inputs.PreMoveDelayUs, inputs.CameraTriggerDelayUs, blankingTimeUs);

            // The mirror reaches Start (= row 0) after the lead-in and the galvo transport delay.
            double actualRow0MidUs = (inputs.LeadInSamples * inputs.TimeStepUs) + inputs.GalvoDelayUs;
            double alignErrorUs = actualRow0MidUs - (row0OpenUs + (inputs.ExposureTimeUs / 2.0));
            double alignErrorRows = alignErrorUs / lineTimeUs;

            double slitDurationUs = inputs.SlitHeightRows * lineTimeUs;
            double slitHalfDurUs = slitDurationUs / 2.0;
            double slitLeadUs = actualRow0MidUs - slitHalfDurUs;
            double slitTrailUs = actualRow0MidUs + slitHalfDurUs;
            double exposureOpenUs = row0OpenUs;
            double exposureCloseUs = row0OpenUs + inputs.ExposureTimeUs;
            double overlapUs = Math.Max(0.0,
                Math.Min(slitTrailUs, exposureCloseUs) - Math.Max(slitLeadUs, exposureOpenUs));
            double overlapPercent = slitDurationUs > 0.0
                ? Math.Min(100.0, (overlapUs / slitDurationUs) * 100.0)
                : 100.0;

            double activeSweepMs = (inputs.SensorRows * lineTimeUs) / 1000.0;
            double decelMarginUs = inputs.LeadOutSamples * inputs.TimeStepUs;

            result = new Result(row0OpenUs, actualRow0MidUs, alignErrorUs, alignErrorRows,
                overlapPercent, activeSweepMs, decelMarginUs, blankingTimeUs);
            reason = string.Empty;
            return true;
        }

        // DevKit lblAlignmentMetrics wording, value for value.
        public static string Describe(in Result result)
        {
            string sign = result.AlignErrorUs >= 0.0 ? "+" : string.Empty;
            return "Row 0 Centering Error: "
                + sign + result.AlignErrorUs.ToString("F1", CultureInfo.InvariantCulture) + " us ("
                + sign + result.AlignErrorRows.ToString("F1", CultureInfo.InvariantCulture) + " rows) | "
                + "Slit Overlap: " + result.SlitOverlapPercent.ToString("F0", CultureInfo.InvariantCulture) + "% | "
                + "Active Sweep: " + result.ActiveSweepMs.ToString("F2", CultureInfo.InvariantCulture) + " ms | "
                + "Decel Margin: " + result.DecelMarginUs.ToString("F0", CultureInfo.InvariantCulture) + " us";
        }
    }
}
