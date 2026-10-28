using System;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // The rolling-shutter timing rule, as DevKit LsoPatternForm.RecalculateSyncParameters
    // computes it. It lives here rather than in a window because two screens now run it (the
    // Scan Setting editor and the scan-test screen's inline Sync row) and because, like the
    // rest of the preset stack, it must be checkable with no window, no camera and no board.
    //
    // What it decides: how long the galvo has to run BEFORE the sweep the camera images
    // (lead-in), how long after (lead-out), how many DAC samples that makes in total, and
    // which sample indices the illumination is switched on and off at. Everything it needs is
    // the preset plus two values the camera owns - exposure and trigger delay.
    public static class LsoScanTimingCalculator
    {
        // Default ramp-start -> camera-trigger delay for GALVANO_FIRST: the fallback of the
        // DevKit's txtPreMoveDelayUs (LsoPatternForm.cs:1083). The operator value lives in the
        // preset (TimingPreMoveDelayUs) and is passed back in as desiredPreMoveDelayUs.
        public const double DefaultPreMoveDelayUs = 500.0;

        // Open-loop playback has to span the whole sensor frame plus a quiet margin, because
        // nothing re-synchronises it once it starts. DevKit safetyMarginPercent.
        public const double FrameTimeSafetyMarginPercent = 5.0;

        // Illumination is switched on a few samples before the sweep reaches the start, and
        // off a few after it leaves the end. The asymmetry is deliberate and comes from the
        // DevKit: a late switch-off burns a bright band into the last rows, where the mirror
        // is already decelerating.
        public const int LedOnLeadSamples = 5;
        public const int LedOffTrailSamples = 4;

        // One below the native array bound, matching what the editor has always capped to.
        public const int MaxPatternSize = LSO_SCANNER_SAMPLE_SIZE_MAX - 1;

        public readonly struct Result
        {
            public Result(int leadIn, int leadOut, int patternSize, int activeSamples,
                int ledOnIndex, int ledOffIndex, short preMoveOffset, ushort preMoveDelayUs,
                int preTrigIndex, double blankingTimeUs, bool capped)
            {
                LeadInSamples = leadIn;
                LeadOutSamples = leadOut;
                PatternSize = patternSize;
                ActiveSamples = activeSamples;
                LedOnIndex = ledOnIndex;
                LedOffIndex = ledOffIndex;
                PreMoveOffset = preMoveOffset;
                PreMoveDelayUs = preMoveDelayUs;
                PreTrigIndex = preTrigIndex;
                BlankingTimeUs = blankingTimeUs;
                Capped = capped;
            }

            public int LeadInSamples { get; }

            public int LeadOutSamples { get; }

            public int PatternSize { get; }

            public int ActiveSamples { get; }

            public int LedOnIndex { get; }

            public int LedOffIndex { get; }

            public short PreMoveOffset { get; }

            public ushort PreMoveDelayUs { get; }

            // Sample index at which the trigger has to be issued for the exposure to start
            // when the mirror reaches the sweep start. Readout only - the board derives its
            // own timing from PreMoveDelayUs.
            public int PreTrigIndex { get; }

            // Row 0 offset time = SensorBlankingRows x LineTime (name kept for compatibility).
            public double BlankingTimeUs { get; }

            // True when the pattern hit MaxPatternSize and lead-out had to absorb the rest.
            public bool Capped { get; }
        }

        // camTriggerDelayUs / exposureUs are the camera preset's, passed in: the scan profile
        // mirrors them but does not own them.
        //
        // Returns false only when an input cannot describe a sweep at all; `reason` then names
        // the field, so a caller mid-edit can stay quiet and a button press can complain.
        // desiredPreMoveDelayUs is the DevKit's "Pre Move Delay" input. It only shapes
        // GALVANO_FIRST; TRIG_FIRST ignores it, exactly as the DevKit does. 0 (or less) means
        // "not stated" and falls back to DefaultPreMoveDelayUs.
        public static bool TryCalculate(LsoScanProfileModel scan, double camTriggerDelayUs,
            double exposureUs, out Result result, out string reason,
            double desiredPreMoveDelayUs = DefaultPreMoveDelayUs)
        {
            if (scan == null) { throw new ArgumentNullException(nameof(scan)); }

            result = default;

            double timeStepUs = scan.TimeStepUs;

            // Line time to the same two decimals the editors show. The DevKit stores the line
            // TIME (12.05 us) while this stack stores the RATE and derives the time, so an
            // unrounded 12.0540 us would put the sweep 1-2 samples away from the numbers the
            // shipped DevKit profiles were tuned with - over 3648 rows the difference is 15 us,
            // physically nothing, but it stops a preset from round-tripping byte for byte.
            // Rounding to what the operator reads keeps both true.
            double lineTimeUs = Math.Round(scan.LineTimeUs, 2, MidpointRounding.AwayFromZero);
            int rows = scan.SensorRows;

            if (timeStepUs <= 0.0 || lineTimeUs <= 0.0 || rows <= 0)
            {
                reason = "Time Step, Line Rate and Sensor Rows must all be positive.";
                return false;
            }

            double galvoDelayUs = Math.Max(0.0, scan.GalvoDelayUs);
            double galvoDecelUs = scan.GalvoDecelUs > 0.0 ? scan.GalvoDecelUs : galvoDelayUs;
            if (galvoDecelUs <= 0.0)
            {
                galvoDecelUs = LsoScanProfileModel.DefaultGalvoDecelUs;
            }

            int blankingRows = Math.Max(0, scan.SensorBlankingRows);
            double blankingTimeUs = blankingRows * lineTimeUs;
            double camDelayUs = Math.Max(0.0, camTriggerDelayUs);

            int active = (int)Math.Round(rows * lineTimeUs / timeStepUs, MidpointRounding.AwayFromZero);
            int decel = (int)Math.Ceiling(galvoDecelUs / timeStepUs);

            int leadIn;
            int leadOut;
            int total;

            if (scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst)
            {
                // Open loop, ported from DevKit LsoPatternForm.RecalculateSyncParameters
                // (LsoPatternForm.cs:1080-1088):
                //
                //   T_sensor_lead = camDelay + blankingTime + exposure / 2
                //   requiredLead  = max(galvoDelay,
                //                       desiredDelay + T_sensor_lead - galvoDelay)
                //   leadIn        = ceil(requiredLead / timeStep)
                //
                // The "- galvoDelay" is the DevKit's: the mirror is already ramping during the
                // lead-in, so only the part of the delay beyond that ramp has to be waited out.
                double desiredDelayUs = desiredPreMoveDelayUs > 0.0
                    ? desiredPreMoveDelayUs
                    : DefaultPreMoveDelayUs;
                double slitCenteringUs = exposureUs > 0.0 ? exposureUs / 2.0 : 0.0;
                double totalSensorLeadUs = camDelayUs + blankingTimeUs + slitCenteringUs;
                leadIn = (int)Math.Ceiling(
                    Math.Max(galvoDelayUs, desiredDelayUs + totalSensorLeadUs - galvoDelayUs) / timeStepUs);

                double frameUs = ((rows * lineTimeUs) + Math.Max(0.0, exposureUs))
                    * (1.0 + (FrameTimeSafetyMarginPercent / 100.0));
                total = (int)Math.Ceiling(frameUs / timeStepUs);
                if (total % 2 != 0) { total++; }

                leadOut = Math.Max(decel, total - leadIn - active);
                total = leadIn + active + leadOut;
            }
            else
            {
                // Closed loop. Playback begins on EXPOSURE_ACTIVE. The lead-in is the pure
                // mechanical baseline (G.Delay, to get the mirror up to speed) PLUS the Row 0
                // slit-centre offset (SensorBlankingRows x LineTime). That offset is measured on
                // the image and absorbs everything unknown between the trigger and the slit
                // centre reaching row 0: vertical blanking, OB/dummy rows, pipeline latency and
                // slit centring - so slit height and exposure no longer enter here.
                // Same rule as DevKit fd72e97 (LsoPatternForm.RecalculateSyncParameters).
                leadIn = (int)Math.Ceiling((galvoDelayUs + blankingTimeUs) / timeStepUs);
                leadOut = decel > 0 ? decel : (int)Math.Ceiling(LsoScanProfileModel.DefaultGalvoDecelUs / timeStepUs);

                total = leadIn + active + leadOut;
                if (total % 2 != 0) { total++; }
            }

            bool capped = false;
            if (total > MaxPatternSize)
            {
                total = MaxPatternSize;
                leadOut = Math.Max(decel, total - leadIn - active);
                capped = true;
            }

            int ledOn = Math.Max(0, leadIn - LedOnLeadSamples);
            int ledOff = Math.Min(total - 1, leadIn + active + LedOffTrailSamples);

            // Pre-move: where the mirror parks and how long before the trigger it starts.
            // The ramp is only as long as the mechanical response needs; a lead-in longer than
            // that is spent dwelling at the parked position, not accelerating.
            short preMoveOffset = 0;
            if (active > 0)
            {
                double posStep = (double)(scan.GalvoEndPos - scan.GalvoStartPos) / active;
                int ramp = Math.Min(leadIn, (int)Math.Round(galvoDelayUs / timeStepUs, MidpointRounding.AwayFromZero));
                double offset = -Math.Round(posStep * ramp, MidpointRounding.AwayFromZero);
                preMoveOffset = (short)Math.Clamp(offset, short.MinValue, short.MaxValue);
            }

            double leadInUs = leadIn * timeStepUs;
            ushort preMoveDelayUs;
            int preTrig;
            if (scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst)
            {
                // DevKit (LsoPatternForm.cs:1153-1157): recompute the trigger delay from the
                // ROUNDED lead-in for the readout and the persisted board value:
                //   delay   = leadIn * timeStep + galvoDelay - T_sensor_lead
                //   preTrig = round(delay / timeStep)
                double slitCenteringUs = exposureUs > 0.0 ? exposureUs / 2.0 : 0.0;
                double totalSensorLeadUs = camDelayUs + blankingTimeUs + slitCenteringUs;
                int triggerDelayUs = (int)Math.Max(0.0, Math.Round(leadInUs + galvoDelayUs - totalSensorLeadUs));
                preMoveDelayUs = (ushort)Math.Clamp(triggerDelayUs, 0, ushort.MaxValue);
                preTrig = Math.Max(0, (int)Math.Round((double)triggerDelayUs / timeStepUs));
            }
            else
            {
                // TRIG_FIRST is unchanged: the board waits on EXPOSURE_ACTIVE, so the readout
                // is the lead-in minus the camera latency and there is no pre-move delay.
                preMoveDelayUs = 0;
                preTrig = Math.Max(0, (int)Math.Round((leadInUs - camDelayUs) / timeStepUs, MidpointRounding.AwayFromZero));
            }

            result = new Result(leadIn, leadOut, total, active, ledOn, ledOff,
                preMoveOffset, preMoveDelayUs, preTrig, blankingTimeUs, capped);
            reason = string.Empty;
            return true;
        }

        // Writes a result back into the preset. Kept beside the calculation so both callers
        // store exactly the same set of fields - and so nothing else is touched: exposure,
        // start/end and the camera side stay where they are.
        public static void ApplyTo(LsoScanProfileModel scan, in Result result)
        {
            if (scan == null) { throw new ArgumentNullException(nameof(scan)); }

            scan.LeadInSamples = (ushort)result.LeadInSamples;
            scan.LeadOutSamples = (ushort)result.LeadOutSamples;
            scan.GalvoPatternSize = (ushort)result.PatternSize;
            scan.LedOnPosIndex = (ushort)result.LedOnIndex;
            scan.LedOffPosIndex = (ushort)result.LedOffIndex;
            scan.PreMoveOffset = result.PreMoveOffset;
            scan.RequestedPreMoveDelayUs ??= scan.PreMoveDelayUs;
            scan.PreMoveDelayUs = result.PreMoveDelayUs;
        }

        // One line stating what the board will do, for the status label next to the controls.
        public static string DescribeSequence(LsoScanProfileModel scan, in Result result)
        {
            if (scan == null) { throw new ArgumentNullException(nameof(scan)); }

            if (scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst)
            {
                return $"GALVANO_FIRST: pre-move {result.PreMoveOffset} DAC, trigger after "
                    + $"{result.PreMoveDelayUs} us | Row 0 offset {scan.SensorBlankingRows} rows "
                    + $"({result.BlankingTimeUs:F0} us) must be covered by Cam Delay";
            }

            return $"TRIG_FIRST: closed-loop | Row 0 offset {scan.SensorBlankingRows} rows "
                + $"({result.BlankingTimeUs:F0} us) | total {result.PatternSize} smp";
        }
    }
}
