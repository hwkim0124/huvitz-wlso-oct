using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using WsoNativeLib.Utils;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // How the board table is built when the preset names no CSV. DevKit LsoPatternConfig.TrajectoryMode.
    public enum LsoTrajectoryMode
    {
        Linear = 0,
        CalibratedFov = 1,
    }

    // The galvo scan half of a mode preset, and the DevKit aiming profile it defaults to.
    //
    // These live in their own file rather than beside LsoScanTestWindow because the whole
    // preset stack (LsoModePreset -> LsoScanProfileModel -> LsoModePresetStore) has to be
    // compilable without WPF: that is what lets program/TestCameraPreset check the storage
    // and migration rules against the SAME code the app runs, with no window, no camera and
    // no board. Nothing here was changed in the move.
    public sealed class LsoScanProfileModel
    {
        // The native struct carries these as float, but the board and the INI both work in
        // whole microseconds - keeping them int stops a fractional value from surviving in
        // memory only to be truncated on the next INI round-trip.
        public int ExposureTimeUs { get; set; }

        public ushort TriggerSource { get; set; }

        public ushort AcquisitionMode { get; set; }

        public uint AcqFrameSize { get; set; }

        public ushort SubFrameSize { get; set; }

        public ushort LedOnPosIndex { get; set; }

        public ushort LedOffPosIndex { get; set; }

        public short GalvanoRewindOffset { get; set; }

        public ushort GalvoPatternSize { get; set; }

        public int TimeStepUs { get; set; }

        public short GalvoStartPos { get; set; }

        public short GalvoEndPos { get; set; }

        // Galvo acceleration / deceleration ramps in DAC samples (DevKit "Lead" / "Tail").
        // The native generator prepends LeadInSamples running into GalvoStartPos and appends
        // LeadOutSamples running past GalvoEndPos; the linear part the camera exposes is
        // GalvoPatternSize - LeadIn - LeadOut. The board struct has no such field - these only
        // shape pos[] - so Load Board cannot recover them and keeps the preset's values.
        public ushort LeadInSamples { get; set; }

        public ushort LeadOutSamples { get; set; }

        // Optional CSV with one DAC position per sample (see LoadTrajectoryFile). When set, the
        // native side writes this table to the board instead of the linear
        // Start->End sweep, so Start/End/Lead-in/Lead-out only matter for the calculator and
        // the Slit Align park position. Its row count must equal GalvoPatternSize. Empty = linear.
        public string TrajectoryFile { get; set; } = string.Empty;

        public bool HasTrajectoryFile => !string.IsNullOrWhiteSpace(TrajectoryFile);

        // CalibratedFov: the table is generated from CalibrationFile (LsoGalvoSlitCalibration)
        // at the CURRENT Pattern Size / Lead / Tail / Sensor Rows each time it is installed, so a
        // Row 0 offset nudge never leaves a stale table behind. It takes precedence over
        // TrajectoryFile; Start/End then only set the sweep direction.
        public LsoTrajectoryMode TrajectoryMode { get; set; } = LsoTrajectoryMode.Linear;

        public string CalibrationFile { get; set; } = string.Empty;

        public bool IsCalibratedTrajectory => TrajectoryMode == LsoTrajectoryMode.CalibratedFov;

        // One int16 per sample, separated by newline / comma / semicolon / whitespace; blank
        // lines and lines starting with '#' are skipped. Returns null with a reason on any
        // non-integer token, an out-of-range value or more than the board's 8192 samples.
        public static short[]? LoadTrajectoryFile(string path, out string error)
        {
            error = string.Empty;
            string text;
            try
            {
                text = File.ReadAllText(path);
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                error = ex.Message;
                return null;
            }

            var values = new List<short>();
            string[] lines = text.Split('\n');
            for (int lineNo = 0; lineNo < lines.Length; lineNo++)
            {
                string line = lines[lineNo].Trim();
                if (line.Length == 0 || line[0] == '#')
                {
                    continue;
                }
                foreach (string token in line.Split(new[] { ',', ';', ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries))
                {
                    if (!short.TryParse(token, NumberStyles.Integer, CultureInfo.InvariantCulture, out short value))
                    {
                        error = $"line {lineNo + 1}: '{token}' is not an int16 (-32768..32767).";
                        return null;
                    }
                    values.Add(value);
                }
            }

            if (values.Count == 0)
            {
                error = "the file holds no samples.";
                return null;
            }
            if (values.Count > LSO_SCANNER_SAMPLE_SIZE_MAX)
            {
                error = $"{values.Count} samples, the board holds at most {LSO_SCANNER_SAMPLE_SIZE_MAX}.";
                return null;
            }
            return values.ToArray();
        }

        // Inverse of LoadTrajectoryFile: '#' header lines, then one value per line. The header
        // is informational only - the reader validates by row count, never by header.
        public static bool SaveTrajectoryFile(string path, short[] pos, string headerComment, out string error)
        {
            error = string.Empty;
            var sb = new System.Text.StringBuilder(pos.Length * 7 + 256);
            foreach (string line in headerComment.Split('\n'))
            {
                string trimmed = line.TrimEnd('\r');
                if (trimmed.Length > 0)
                {
                    sb.Append("# ").Append(trimmed).Append('\n');
                }
            }
            sb.Append("# one int16 DAC position per line\n");
            foreach (short v in pos)
            {
                sb.Append(v.ToString(CultureInfo.InvariantCulture)).Append('\n');
            }
            try
            {
                string? directory = Path.GetDirectoryName(path);
                if (!string.IsNullOrEmpty(directory))
                {
                    Directory.CreateDirectory(directory);
                }
                File.WriteAllText(path, sb.ToString());
                return true;
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                error = ex.Message;
                return false;
            }
        }

        // C# mirror of the native LsoScanner::generateGalvanoPositions (both overloads), so the
        // editor can show what the board would get WITHOUT a CSV. Follows the C++, not the
        // DevKit MakeLinearProfile: the C++ lead-out continues the sweep slope past End, the
        // DevKit parks at End. Verified against the native output (docs sample CSV) - keep
        // the two in step when either changes. Returns an empty array where the native side
        // returns false (sampleSize <= 0 or >= 8192).
        public static short[] GenerateLinearTrajectory(short startPos, short endPos, int sampleSize,
            int leadInSamples, int leadOutSamples)
        {
            if (sampleSize <= 0 || sampleSize >= LSO_SCANNER_SAMPLE_SIZE_MAX)
            {
                return Array.Empty<short>();
            }

            var pos = new short[sampleSize];
            int active = sampleSize - leadInSamples - leadOutSamples;
            if (leadInSamples <= 0 || leadOutSamples < 0 || active <= 0)
            {
                // Plain sweep: N-1 steps, both ends included.
                double stepPlain = sampleSize > 1 ? (double)(endPos - startPos) / (sampleSize - 1) : 0.0;
                for (int i = 0; i < sampleSize; i++)
                {
                    pos[i] = ClampToShort(startPos + stepPlain * i);
                }
                return pos;
            }

            double step = (double)(endPos - startPos) / active;
            for (int i = 0; i < leadInSamples; i++)
            {
                pos[i] = ClampToShort(startPos - step * (leadInSamples - i));
            }
            for (int i = 0; i < active; i++)
            {
                pos[leadInSamples + i] = ClampToShort(startPos + step * i);
            }
            for (int i = 0; i < leadOutSamples; i++)
            {
                pos[leadInSamples + active + i] = ClampToShort(endPos + step * i);
            }
            pos[leadInSamples + active - 1] = endPos;
            return pos;
        }

        public short[] GenerateLinearTrajectory()
        {
            return GenerateLinearTrajectory(GalvoStartPos, GalvoEndPos, GalvoPatternSize, LeadInSamples, LeadOutSamples);
        }

        // Stretches or shrinks a table to another sample count by linear interpolation over
        // the normalised index, so the SHAPE (e.g. a sensor-linear sweep) survives when the
        // timing calculator moves Pattern Size by a few dozen samples. Same array back when
        // the count already matches. Time per step is unchanged; the sweep simply takes
        // count * timeStep, exactly as the generated linear sweep would.
        public static short[] ResampleTrajectory(short[] pos, int count)
        {
            if (pos.Length == count || pos.Length == 0 || count <= 0)
            {
                return pos;
            }
            var result = new short[count];
            if (pos.Length == 1 || count == 1)
            {
                Array.Fill(result, pos[0]);
                return result;
            }
            double scale = (double)(pos.Length - 1) / (count - 1);
            for (int i = 0; i < count; i++)
            {
                double x = i * scale;
                int i0 = (int)Math.Floor(x);
                int i1 = Math.Min(i0 + 1, pos.Length - 1);
                double t = x - i0;
                result[i] = ClampToShort(pos[i0] + (pos[i1] - pos[i0]) * t);
            }
            return result;
        }

        // std::round semantics (half away from zero), then the board's int16.
        public static short ClampToShort(double value)
        {
            double rounded = Math.Round(value, MidpointRounding.AwayFromZero);
            return (short)Math.Clamp(rounded, short.MinValue, short.MaxValue);
        }

        // Linear sweep length in samples, i.e. what the exposure window has to fit into.
        public int ActiveSamples => Math.Max(0, GalvoPatternSize - LeadInSamples - LeadOutSamples);

        // --- Full-frame sweep arithmetic --------------------------------------------------
        // Measured on the board (2026-09-17), which is what this arithmetic follows:
        //
        //   * GalvoStartPos -> GalvoEndPos is the movement INSIDE one subframe. Equal values
        //     are a normal setting: the mirror then stands still for that camera frame.
        //   * prescan_pos_rewind_offset is what moves the mirror BETWEEN subframes, and it
        //     ACCUMULATES - subframe k starts at GalvoStartPos + k * rewind.
        //   * It happens subFrameCount - 1 times, not subFrameCount: the first subframe runs
        //     from the stored start. 19000 with rewind -780 and 50 subframes ends at
        //     19000 - 780*49 = -19220, which is what the instrument produced.
        //
        // So the full frame spans (End - Start) + rewind * (N - 1), and the only value the
        // editor has to derive is the rewind. Start/End stay what the operator asked for.
        // Nothing extra is stored: the full-frame range is derived back out of the three.
        public readonly struct GalvoSweepPlan
        {
            public GalvoSweepPlan(short start, short end, short rewindOffset, int totalEnd)
            {
                Start = start;
                End = end;
                RewindOffset = rewindOffset;
                TotalEnd = totalEnd;
            }

            public short Start { get; }

            public short End { get; }

            // What advances the mirror from one subframe to the next, signed: negative for a
            // downward full frame. Zero for a single subframe - nothing has to advance.
            public short RewindOffset { get; }

            // Where the last subframe actually ends. Rounding makes this differ from the
            // requested full-frame end by up to (N-1) DAC counts, so the caller can show what
            // the operator will really get instead of silently dropping it.
            public int TotalEnd { get; }
        }

        // subFrameSweep is the movement inside one subframe (GalvoEndPos - GalvoStartPos), the
        // operator's value - 0 means the mirror stands still and the rewind does all the work.
        //
        // False when the inputs cannot describe a scan at all: no subframes, a rewind that
        // does not fit the board's int16 field, or a configuration where nothing moves at all
        // (a still subframe that never advances).
        public static bool TryPlanGalvoSweep(int totalStart, int totalEnd, int subFrameCount,
            int subFrameSweep, out GalvoSweepPlan plan)
        {
            plan = default;
            if (subFrameCount <= 0)
            {
                return false;
            }

            // The first subframe runs from the stored start, so only the remaining ones are
            // reached by a rewind.
            int hops = subFrameCount - 1;
            int rewind = hops > 0
                ? (int)Math.Round((double)(totalEnd - totalStart - subFrameSweep) / hops)
                : 0;

            if (rewind < short.MinValue || rewind > short.MaxValue)
            {
                return false;
            }

            if (subFrameSweep == 0 && rewind == 0)
            {
                return false;
            }

            short start = ClampToDac(totalStart);
            short end = ClampToDac(start + subFrameSweep);
            plan = new GalvoSweepPlan(start, end, (short)rewind, start + subFrameSweep + (rewind * hops));
            return true;
        }

        // The full-frame range this preset currently describes: one subframe's sweep plus the
        // rewind applied to every subframe after the first. This is the inverse of
        // TryPlanGalvoSweep, and it is how the editor shows a total range without storing one.
        public int FullFrameEndPos =>
            GalvoEndPos + (GalvanoRewindOffset * Math.Max(0, (int)SubFrameSize - 1));

        private static short ClampToDac(int value)
        {
            return (short)Math.Clamp(value, short.MinValue, short.MaxValue);
        }

        // --- Timing calculator inputs -----------------------------------------------------
        // These do NOT cross the ABI and never reach the board: they are what the rolling
        // shutter calculation was run with, kept so the next session recomputes the same
        // Pattern Size / ramps / light indices instead of starting from generic defaults.
        // Cam Delay is deliberately absent - that is the camera preset's TriggerDelayUs, and
        // one physical delay stored in two places is the drift the exposure rule just removed.
        public const double DefaultLineRateHz = 84889.6;
        public const double DefaultGalvoDelayUs = 300.0;
        public const double DefaultGalvoDecelUs = 300.0;

        // Default Row 0 offset (rows) - the DevKit's LsoPatternConfig default (originally the
        // IMX183 optical-black + dummy row count, hence the value).
        public const int DefaultSensorBlankingRows = 39;

        // Camera AcquisitionLineRate. Line time (us) = 1e6 / this, so only the rate is stored.
        public double LineRateHz { get; set; } = DefaultLineRateHz;

        // Sensor rows the sweep has to cover. 0 = not stated; the editor then falls back to
        // the camera preset's RoiMaxHeight.
        public int SensorRows { get; set; }

        public double GalvoDelayUs { get; set; } = DefaultGalvoDelayUs;

        public double GalvoDecelUs { get; set; } = DefaultGalvoDecelUs;

        // Illuminated slit height in SENSOR rows (0 = not measured). Binning is already undone
        // when this is written, so it is comparable with the line time regardless of binning.
        public int SlitHeightRows { get; set; }

        // Row 0 slit-centre offset in SENSOR rows (DevKit "Row 0 Slit Center Offset", fd72e97).
        // Measured on the image: it absorbs vertical blanking, OB/dummy rows, pipeline latency
        // and slit centring in one number. TRIG_FIRST lead-in = G.Delay + this x LineTime.
        // The name and the preset/INI key are kept for compatibility; the board never sees it.
        public int SensorBlankingRows { get; set; } = DefaultSensorBlankingRows;

        // --- Scan sequence (DevKit "Scan Sequence Control") ------------------------------
        // Unlike everything above, these three DO reach the board - but through a command
        // (setLsoScanSequence), not through the profile struct, and the command carries no
        // pattern id: the setting is board-GLOBAL. They are stored per preset anyway because
        // Preview and Capture may want different sequences; whoever starts a mode re-sends
        // them, which is what keeps the board matching the mode that is actually running.
        public LsoScanSequenceMode SequenceMode { get; set; } = LsoScanSequenceMode.TrigFirst;

        // DAC offset of the pre-move position relative to GalvoStartPos (negative: the mirror
        // parks before the sweep start). GALVANO_FIRST only; TRIG_FIRST leaves it at 0.
        public short PreMoveOffset { get; set; }

        // Galvo ramp start -> camera trigger, in us. GALVANO_FIRST only; 0 in TRIG_FIRST,
        // where the board waits on EXPOSURE_ACTIVE instead of timing the trigger itself.
        public ushort PreMoveDelayUs { get; set; }

        // 계산 결과는 DevKit처럼 PreMoveDelayUs에 저장하고 보드로 보낸다.
        // 원래 요청은 별도로 보존하여 반올림된 결과가 다음 계산의 입력이 되지 않게 한다.
        // 이전 프리셋에는 이 값이 없으므로 기존 PreMoveDelayUs를 요청으로 사용한다.
        public ushort? RequestedPreMoveDelayUs { get; set; }

        public ushort TimingPreMoveDelayUs => RequestedPreMoveDelayUs ?? PreMoveDelayUs;

        public double LineTimeUs => LineRateHz > 0 ? 1_000_000.0 / LineRateHz : 0.0;

        // Row 0 offset time: how long after the trigger the slit centre is meant to reach row 0.
        public double BlankingTimeUs => SensorBlankingRows * LineTimeUs;

        // ERS: every sensor row is exposed for one line time, so a slit covering N rows needs
        // N line times of exposure. Derived on purpose - storing it would go stale the moment
        // the line rate changes.
        public double RecommendedExposureUs => SlitHeightRows > 0 ? SlitHeightRows * LineTimeUs : 0.0;

        public static string GetIniSectionName(int patternId)
        {
            return $"LSO_SCANNER_{patternId}";
        }

        public void LoadFromIni(IniFileUtil ini, int patternId)
        {
            string section = GetIniSectionName(patternId);

            ExposureTimeUs = ini.ReadInt(section, "ExposureTime");
            TriggerSource = (ushort)ini.ReadInt(section, "TrgSrc");
            AcquisitionMode = (ushort)ini.ReadInt(section, "AcqMode");
            AcqFrameSize = (uint)ini.ReadInt(section, "AcqFrameSize");
            SubFrameSize = (ushort)ini.ReadInt(section, "SubFrameSize");
            LedOnPosIndex = (ushort)ini.ReadInt(section, "LedOnPosIndex");
            LedOffPosIndex = (ushort)ini.ReadInt(section, "LedOffPosIndex");
            GalvanoRewindOffset = (short)ini.ReadInt(section, "GalvanoRewindOffset");
            GalvoPatternSize = (ushort)ini.ReadInt(section, "GalvoPatternSize");
            TimeStepUs = ini.ReadInt(section, "TimeStep");
            GalvoStartPos = (short)ini.ReadInt(section, "GalvoStartPos");
            GalvoEndPos = (short)ini.ReadInt(section, "GalvoEndPos");
            LeadInSamples = (ushort)ini.ReadInt(section, "LeadInSamples");
            LeadOutSamples = (ushort)ini.ReadInt(section, "LeadOutSamples");
            LineRateHz = ini.ReadDouble(section, "LineRateHz", DefaultLineRateHz);
            SensorRows = ini.ReadInt(section, "SensorRows");
            GalvoDelayUs = ini.ReadDouble(section, "GalvoDelayUs", DefaultGalvoDelayUs);
            GalvoDecelUs = ini.ReadDouble(section, "GalvoDecelUs", DefaultGalvoDecelUs);
            SlitHeightRows = ini.ReadInt(section, "SlitHeightRows");
            SensorBlankingRows = ini.ReadInt(section, "SensorBlankingRows", DefaultSensorBlankingRows);
            SequenceMode = (LsoScanSequenceMode)ini.ReadInt(section, "SequenceMode");
            PreMoveOffset = (short)ini.ReadInt(section, "PreMoveOffset");
            PreMoveDelayUs = (ushort)ini.ReadInt(section, "PreMoveDelayUs");
            int requestedDelay = ini.ReadInt(section, "RequestedPreMoveDelayUs", -1);
            RequestedPreMoveDelayUs = requestedDelay >= 0 ? (ushort)requestedDelay : null;
            TrajectoryFile = ini.ReadString(section, "TrajectoryFile");
            TrajectoryMode = (LsoTrajectoryMode)ini.ReadInt(section, "TrajectoryMode");
            CalibrationFile = ini.ReadString(section, "CalibrationFile");
        }

        public void SaveToIni(IniFileUtil ini, int patternId)
        {
            string section = GetIniSectionName(patternId);

            ini.WriteInt(section, "ExposureTime", ExposureTimeUs);
            ini.WriteInt(section, "TrgSrc", TriggerSource);
            ini.WriteInt(section, "AcqMode", AcquisitionMode);
            ini.WriteInt(section, "AcqFrameSize", (int)AcqFrameSize);
            ini.WriteInt(section, "SubFrameSize", SubFrameSize);
            ini.WriteInt(section, "LedOnPosIndex", LedOnPosIndex);
            ini.WriteInt(section, "LedOffPosIndex", LedOffPosIndex);
            ini.WriteInt(section, "GalvanoRewindOffset", GalvanoRewindOffset);
            ini.WriteInt(section, "GalvoPatternSize", GalvoPatternSize);
            ini.WriteInt(section, "TimeStep", TimeStepUs);
            ini.WriteInt(section, "GalvoStartPos", GalvoStartPos);
            ini.WriteInt(section, "GalvoEndPos", GalvoEndPos);
            ini.WriteInt(section, "LeadInSamples", LeadInSamples);
            ini.WriteInt(section, "LeadOutSamples", LeadOutSamples);
            ini.WriteDouble(section, "LineRateHz", LineRateHz);
            ini.WriteInt(section, "SensorRows", SensorRows);
            ini.WriteDouble(section, "GalvoDelayUs", GalvoDelayUs);
            ini.WriteDouble(section, "GalvoDecelUs", GalvoDecelUs);
            ini.WriteInt(section, "SlitHeightRows", SlitHeightRows);
            ini.WriteInt(section, "SensorBlankingRows", SensorBlankingRows);
            ini.WriteInt(section, "SequenceMode", (int)SequenceMode);
            ini.WriteInt(section, "PreMoveOffset", PreMoveOffset);
            ini.WriteInt(section, "PreMoveDelayUs", PreMoveDelayUs);
            ini.WriteInt(section, "RequestedPreMoveDelayUs", RequestedPreMoveDelayUs.HasValue
                ? RequestedPreMoveDelayUs.Value : -1);
            ini.WriteString(section, "TrajectoryFile", TrajectoryFile);
            ini.WriteInt(section, "TrajectoryMode", (int)TrajectoryMode);
            ini.WriteString(section, "CalibrationFile", CalibrationFile);
        }

        // Note: the galvano position table is not built here. Only start/end/sampleSize
        // cross the ABI; the native LsoScanner::generateGalvanoPositions() interpolates the
        // actual pos[] before it is written to the board.
        public LsoScannerControlParam ToControlParam()
        {
            return new LsoScannerControlParam
            {
                timeStepUs = TimeStepUs,
                exposureTimeUs = ExposureTimeUs,
                triggerSource = TriggerSource,
                acquisitionMode = AcquisitionMode,
                acqFrameSize = (int)AcqFrameSize,
                sampleSize = GalvoPatternSize,
                subFrameSize = SubFrameSize,
                prescanPosRewindOffset = GalvanoRewindOffset,
                ledOnPosIndex = LedOnPosIndex,
                ledOffPosIndex = LedOffPosIndex,
                yGalvoStartPos = GalvoStartPos,
                yGalvoEndPos = GalvoEndPos,
                leadInSamples = LeadInSamples,
                leadOutSamples = LeadOutSamples,
            };
        }

        public void CopyValuesFrom(LsoScanProfileModel source)
        {
            TimeStepUs = source.TimeStepUs;
            ExposureTimeUs = source.ExposureTimeUs;
            TriggerSource = source.TriggerSource;
            AcquisitionMode = source.AcquisitionMode;
            AcqFrameSize = source.AcqFrameSize;
            SubFrameSize = source.SubFrameSize;
            GalvoPatternSize = source.GalvoPatternSize;
            GalvanoRewindOffset = source.GalvanoRewindOffset;
            LedOnPosIndex = source.LedOnPosIndex;
            LedOffPosIndex = source.LedOffPosIndex;
            GalvoStartPos = source.GalvoStartPos;
            GalvoEndPos = source.GalvoEndPos;
            LeadInSamples = source.LeadInSamples;
            LeadOutSamples = source.LeadOutSamples;
            LineRateHz = source.LineRateHz;
            SensorRows = source.SensorRows;
            GalvoDelayUs = source.GalvoDelayUs;
            GalvoDecelUs = source.GalvoDecelUs;
            SlitHeightRows = source.SlitHeightRows;
            SensorBlankingRows = source.SensorBlankingRows;
            SequenceMode = source.SequenceMode;
            PreMoveOffset = source.PreMoveOffset;
            PreMoveDelayUs = source.PreMoveDelayUs;
            RequestedPreMoveDelayUs = source.RequestedPreMoveDelayUs;
            TrajectoryFile = source.TrajectoryFile;
            TrajectoryMode = source.TrajectoryMode;
            CalibrationFile = source.CalibrationFile;
        }

        // Lead/Tail are deliberately NOT taken from param: obtain() reads pos[] back from the
        // board, which carries no ramp lengths, and the native side reports 0/0 there.
        // Overwriting the preset with those zeros would silently flatten the trajectory on
        // the next Apply.
        public void ApplyControlParam(LsoScannerControlParam param)
        {
            TimeStepUs = (int)Math.Round(param.timeStepUs);
            ExposureTimeUs = (int)Math.Round(param.exposureTimeUs);
            TriggerSource = param.triggerSource;
            AcquisitionMode = param.acquisitionMode;
            AcqFrameSize = (uint)param.acqFrameSize;
            GalvoPatternSize = param.sampleSize;
            SubFrameSize = param.subFrameSize;
            GalvanoRewindOffset = param.prescanPosRewindOffset;
            LedOnPosIndex = param.ledOnPosIndex;
            LedOffPosIndex = param.ledOffPosIndex;
            GalvoStartPos = param.yGalvoStartPos;
            GalvoEndPos = param.yGalvoEndPos;
        }
    }

    // The Preview (aiming) scan preset as the DevKit ships it: profile 0 of
    // LSO_DevKit/LsoPatternCfg/LsoPattern_Preview_Bfs.json, which ApplyLsoPatternConfig("Preview")
    // writes into the board there. Scanner fields only - the camera side of the preview
    // (Mono8 + 2x2 binning, gain, HW-trigger wiring from BfsCameraConfigPreview.json) is not
    // owned by the Scan Setting panel; the native preview one-call installs its own camera
    // geometry (installMonoPreviewParams) and the color camera setting window owns the rest.
    public static class LsoScanPreviewPreset
    {
        // Scanner slot the aiming preview runs on. Slot order follows the DevKit, which
        // previews on profile 0 (LsoScanFormUnified: selected_tarj_profile_id = 0) and
        // captures on profile 1 - the board is only validated in that arrangement. The
        // LsoScannerPatternId names describe the camera, not the mode: COLOR is slot 0.
        public const int PatternId = (int)LsoScannerPatternId.COLOR;

        public const int TimeStepUs = 10;
        public const ushort TriggerSource = 0;        // 0 = color camera
        public const ushort AcquisitionMode = 0;      // 0 = continuous (Fixed Frame off)
        public const uint AcqFrameSize = 1;
        public const ushort SubFrameSize = 1;
        public const ushort GalvoPatternSize = 4300;  // JSON SampleSize
        public const short GalvanoRewindOffset = 0;
        public const ushort LedOnPosIndex = 10;
        public const ushort LedOffPosIndex = 4290;
        public const short GalvoStartPos = 20000;
        public const short GalvoEndPos = -20000;
        // DevKit LsoPatternForm defaults (GalvoDelay 300us / Decel 300us at 10us step).
        public const ushort LeadInSamples = 30;
        public const ushort LeadOutSamples = 30;

        // boardExposureUs is the color camera's exposure, not a stored scan value: DevKit
        // ApplyLsoPatternConfig writes GetColorCameraExposureUs into the board profile and
        // never the JSON ExposureTimeUs, and startLsoFundusPreview applies the same rule through
        // capture::resolveBoardExposureUs. A positive reading wins; a non-positive one keeps
        // the profile's previous value so a missing camera cannot zero the field.
        public static void ApplyTo(LsoScanProfileModel profile, int boardExposureUs)
        {
            profile.TimeStepUs = TimeStepUs;
            profile.TriggerSource = TriggerSource;
            profile.AcquisitionMode = AcquisitionMode;
            profile.AcqFrameSize = AcqFrameSize;
            profile.SubFrameSize = SubFrameSize;
            profile.GalvoPatternSize = GalvoPatternSize;
            profile.GalvanoRewindOffset = GalvanoRewindOffset;
            profile.LedOnPosIndex = LedOnPosIndex;
            profile.LedOffPosIndex = LedOffPosIndex;
            profile.GalvoStartPos = GalvoStartPos;
            profile.GalvoEndPos = GalvoEndPos;
            profile.LeadInSamples = LeadInSamples;
            profile.LeadOutSamples = LeadOutSamples;

            if (boardExposureUs > 0)
            {
                profile.ExposureTimeUs = boardExposureUs;
            }
        }
    }

    public static class LsoScanCapturePreset
    {
        // DevKit captures on profile 1 (selected_tarj_profile_id = 1).
        public const int PatternId = (int)LsoScannerPatternId.IR;
        public const int TimeStepUs = 10;
        public const ushort TriggerSource = 0;
        public const ushort AcquisitionMode = 1;
        public const uint AcqFrameSize = 1;
        public const ushort SubFrameSize = 1;
        public const ushort GalvoPatternSize = 4725;
        public const short GalvanoRewindOffset = 0;
        public const ushort LedOnPosIndex = 10;
        public const ushort LedOffPosIndex = 4290;
        public const short GalvoStartPos = 20000;
        public const short GalvoEndPos = -26000;
        public const ushort LeadInSamples = 30;
        public const ushort LeadOutSamples = 30;

        public static void ApplyTo(LsoScanProfileModel profile, int boardExposureUs)
        {
            profile.TimeStepUs = TimeStepUs;
            profile.ExposureTimeUs = boardExposureUs;
            profile.TriggerSource = TriggerSource;
            profile.AcquisitionMode = AcquisitionMode;
            profile.AcqFrameSize = AcqFrameSize;
            profile.SubFrameSize = SubFrameSize;
            profile.GalvoPatternSize = GalvoPatternSize;
            profile.GalvanoRewindOffset = GalvanoRewindOffset;
            profile.LedOnPosIndex = LedOnPosIndex;
            profile.LedOffPosIndex = LedOffPosIndex;
            profile.GalvoStartPos = GalvoStartPos;
            profile.GalvoEndPos = GalvoEndPos;
            profile.LeadInSamples = LeadInSamples;
            profile.LeadOutSamples = LeadOutSamples;
        }
    }

}
