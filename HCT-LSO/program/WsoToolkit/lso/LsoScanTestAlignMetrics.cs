using System;
using System.Windows.Media;
using WsoNativeLib.Presets;

namespace WsoToolkit
{
    // DevKit LsoPatternForm "Quantitative Synchronization & Alignment Metrics" status line,
    // shown under the Galvano / Slit / Sync group. Recomputed wherever reflectScanTiming_ runs:
    // after every recalculation and whenever the selected mode preset changes.
    public partial class LsoScanTestWindow
    {
        private void showAlignMetrics_()
        {
            if (myTbAlignMetrics == null)
            {
                return;
            }

            LsoModePreset preset = AlignPreset;
            LsoScanProfileModel scan = preset.Scan;

            // The DevKit reads its two-decimal Sensor Line Time field, and the timing
            // calculator rounds the same way, so the metrics describe the sweep that runs.
            double lineTimeUs = Math.Round(scan.LineTimeUs, 2, MidpointRounding.AwayFromZero);
            int rows = scan.SensorRows > 0 ? scan.SensorRows : (int)preset.Camera.RoiMaxHeight;

            // DevKit precedence: the profile exposure drives the timing view; the camera preset
            // is only a fallback. A slit that was never measured falls back to the DevKit's
            // 100 rows rather than reporting a meaningless 0.
            int exposureUs = scan.ExposureTimeUs > 0
                ? scan.ExposureTimeUs
                : (int)Math.Round((double)preset.Camera.ExposureTime);
            int slitRows = scan.SlitHeightRows > 0 ? scan.SlitHeightRows : 100;

            var inputs = new LsoAlignmentMetrics.Inputs(
                scan.TimeStepUs, lineTimeUs, rows,
                scan.LeadInSamples, scan.LeadOutSamples, scan.GalvoDelayUs,
                exposureUs, slitRows, scan.SensorBlankingRows,
                preset.Camera.TriggerDelayUs > 0 ? preset.Camera.TriggerDelayUs : 0.0,
                scan.PreMoveDelayUs, scan.SequenceMode);

            if (!LsoAlignmentMetrics.TryCompute(in inputs, out LsoAlignmentMetrics.Result result, out string reason))
            {
                myTbAlignMetrics.Text = "Alignment: " + reason;
                myTbAlignMetrics.Foreground = Brushes.DarkRed;
                return;
            }

            myTbAlignMetrics.Text = LsoAlignmentMetrics.Describe(in result);
            myTbAlignMetrics.Foreground = result.IsCentered ? Brushes.DarkGreen
                : result.IsAcceptable ? Brushes.DarkGoldenrod
                : Brushes.DarkRed;
        }
    }
}
