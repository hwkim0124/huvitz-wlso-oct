namespace WsoNativeLib.Presets
{
    // The galvo table a preset's scan runs: the CSV its TrajectoryFile names, or the generated
    // linear Start->End sweep when it names none. Shared by the Toolkit and the Agent so both
    // processes drive the board with the same table for the same preset.
    //
    // The native override is per scanner slot and lives for the process, so it is installed
    // right before each one-call that uses the slot: a slot left holding another preset's table
    // would otherwise sweep that table instead.
    public static class LsoModeTrajectory
    {
        // Device-free. Null table = linear sweep. A table whose row count differs from the
        // live Pattern Size (the timing calculator derives it) is resampled to that size, shape
        // kept, and `note` says so; an unreadable file is a failure, never a silent linear run.
        public static bool TryLoad(LsoScanProfileModel scan, out short[]? table, out string issue, out string note)
        {
            table = null;
            issue = string.Empty;
            note = string.Empty;
            if (scan.TrajectoryMode != LsoTrajectoryMode.Linear && scan.TrajectoryMode != LsoTrajectoryMode.CalibratedFov)
            {
                issue = $"Trajectory mode {(int)scan.TrajectoryMode} is not Linear (0) or CalibratedFov (1).";
                return false;
            }
            if (scan.IsCalibratedTrajectory)
            {
                table = TryGenerateCalibrated(scan, out issue);
                if (table != null && scan.HasTrajectoryFile)
                {
                    note = $"calibrated FOV mode - '{System.IO.Path.GetFileName(scan.TrajectoryFile)}' is not used.";
                }
                return table != null;
            }

            if (!scan.HasTrajectoryFile)
            {
                return true;
            }

            short[]? pos = LsoScanProfileModel.LoadTrajectoryFile(scan.TrajectoryFile, out string error);
            if (pos == null)
            {
                issue = $"'{scan.TrajectoryFile}': {error}";
                return false;
            }

            if (pos.Length != scan.GalvoPatternSize)
            {
                note = $"'{System.IO.Path.GetFileName(scan.TrajectoryFile)}' has {pos.Length} samples; "
                    + $"resampled to the live Pattern Size {scan.GalvoPatternSize} (shape kept). "
                    + "Re-export from the Trajectory Editor to make the file match.";
                pos = LsoScanProfileModel.ResampleTrajectory(pos, scan.GalvoPatternSize);
            }

            table = pos;
            return true;
        }

        // The calibrated FOV table for the preset's current geometry. Generated here, not stored,
        // so it always matches the Lead / Tail / Pattern Size the timing calculator last produced.
        public static short[]? TryGenerateCalibrated(LsoScanProfileModel scan, out string issue)
        {
            if (string.IsNullOrWhiteSpace(scan.CalibrationFile))
            {
                issue = "Calibrated FOV trajectory is selected but no calibration file is set.";
                return null;
            }
            if (!LsoGalvoSlitCalibration.TryLoad(scan.CalibrationFile, out LsoGalvoSlitCalibration? calibration, out issue)
                || calibration == null)
            {
                return null;
            }
            if (!calibration.TryFit(out string fitError))
            {
                issue = $"'{System.IO.Path.GetFileName(scan.CalibrationFile)}': {fitError}";
                return null;
            }

            short[]? pos = calibration.GenerateTrajectory(scan.GalvoStartPos, scan.GalvoEndPos, scan.GalvoPatternSize,
                scan.LeadInSamples, scan.LeadOutSamples, scan.SensorRows, out string generateError);
            issue = pos == null ? $"calibrated FOV trajectory: {generateError}" : string.Empty;
            return pos;
        }

        // What the board gets when no CSV overrides it: the calibrated table, or the native
        // linear sweep mirrored in C#. For screens that preview the generated table.
        public static short[]? TryGenerate(LsoScanProfileModel scan, out string issue)
        {
            if (scan.TrajectoryMode != LsoTrajectoryMode.Linear && scan.TrajectoryMode != LsoTrajectoryMode.CalibratedFov)
            {
                issue = $"Trajectory mode {(int)scan.TrajectoryMode} is not Linear (0) or CalibratedFov (1).";
                return null;
            }
            if (scan.IsCalibratedTrajectory)
            {
                return TryGenerateCalibrated(scan, out issue);
            }
            issue = string.Empty;
            return scan.GenerateLinearTrajectory();
        }

        // Device step: a null table clears the slot's override, so the one-call runs linear.
        public static bool Install(int patternId, short[]? table, out string issue)
        {
            if (!LsoScanner.SetLsoScannerTrajectory(patternId, table))
            {
                issue = "the native scanner did not accept the table.";
                return false;
            }

            issue = string.Empty;
            return true;
        }
    }
}
