using System;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // Explicit Toolkit tuning Apply only. Preview/Capture starts carry these board-global
    // values in their requests; the native one-call writes them under its session gate.
    // Do not also call this helper before a fundus start.
    public static class LsoScanSequenceApply
    {
        public static bool Apply(LsoScanProfileModel scan, out string failure)
        {
            if (scan == null) { throw new ArgumentNullException(nameof(scan)); }

            if (LsoScanner.SetLsoScanSequence(scan.SequenceMode, scan.PreMoveOffset, scan.PreMoveDelayUs))
            {
                failure = string.Empty;
                return true;
            }

            // Naming both the mode and the numbers matters here: a board that refuses the
            // command is either running firmware without it or was handed a pre-move it
            // cannot honour, and the two look identical from the image alone.
            failure = scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst
                ? $"the board refused GALVANO_FIRST (pre-move {scan.PreMoveOffset} DAC, "
                    + $"{scan.PreMoveDelayUs} us). Firmware without the scan-sequence command "
                    + "runs TRIG_FIRST only."
                : "the board refused TRIG_FIRST. Check that the firmware supports the "
                    + "scan-sequence command.";
            return false;
        }
    }
}
