using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    // Synchronisation / timing and scan sequence (DevKit LsoPatternForm groups
    // "Synchronization & Timing Parameters" and "Scan Sequence Control"), inline on this
    // screen instead of only inside the Settings dialog: they are tuned while looking at the
    // live image, which is here.
    //
    // The fields belong to the mode preset the alignment combo selects, so one selector drives
    // the whole group; the arithmetic itself is LsoScanTimingCalculator, shared with the
    // Settings window so the two screens cannot drift.
    public partial class LsoScanTestWindow : Window
    {
        // True while the fields are being written FROM the preset, so the commit handlers do
        // not write them straight back.
        private bool _syncUpdating;

        private bool SyncAutoRecalc => myCheckSyncAuto?.IsChecked == true;

        private bool SyncBothModes => myCheckSyncBoth?.IsChecked == true;

        // Same target as the alignment row above: one combo, one selected preset.
        private LsoModePreset SyncPreset => AlignPreset;

        private IEnumerable<ColorCameraPresetMode> syncTargetModes_()
        {
            yield return AlignTargetMode;
            if (SyncBothModes)
            {
                yield return AlignTargetMode == ColorCameraPresetMode.Preview
                    ? ColorCameraPresetMode.Capture
                    : ColorCameraPresetMode.Preview;
            }
        }

        #region Reflect / commit

        // Preset -> fields. Called whenever the preset behind them may have changed: window
        // load, target-mode switch, preset selection, and after the Settings window closes.
        private void reflectScanTiming_()
        {
            if (myTbSyncLineRate == null)
            {
                return;
            }

            LsoModePreset preset = SyncPreset;
            LsoScanProfileModel scan = preset.Scan;

            _syncUpdating = true;
            try
            {
                myTbSyncLineRate.Text = scan.LineRateHz.ToString("F1", CultureInfo.InvariantCulture);
                myTbSyncLineTime.Text = scan.LineTimeUs.ToString("F2", CultureInfo.InvariantCulture);
                myTbSyncRows.Text = (scan.SensorRows > 0
                    ? scan.SensorRows
                    : (int)preset.Camera.RoiMaxHeight).ToString(CultureInfo.InvariantCulture);
                myTbSyncBlank.Text = scan.SensorBlankingRows.ToString(CultureInfo.InvariantCulture);
                myTbSyncGalvoDelay.Text = scan.GalvoDelayUs.ToString("F0", CultureInfo.InvariantCulture);
                myTbSyncGalvoDecel.Text = scan.GalvoDecelUs.ToString("F0", CultureInfo.InvariantCulture);
                myTbSyncCamDelay.Text = (preset.Camera.TriggerDelayUs > 0 ? preset.Camera.TriggerDelayUs : 0.0)
                    .ToString("F0", CultureInfo.InvariantCulture);

                myTbGalvoStartPos.Text = scan.GalvoStartPos.ToString(CultureInfo.InvariantCulture);
                myTbGalvoEndPos.Text = scan.GalvoEndPos.ToString(CultureInfo.InvariantCulture);

                myTbSyncLeadIn.Text = scan.LeadInSamples.ToString(CultureInfo.InvariantCulture);
                myTbSyncLeadOut.Text = scan.LeadOutSamples.ToString(CultureInfo.InvariantCulture);
                myTbSyncPattern.Text = scan.GalvoPatternSize.ToString(CultureInfo.InvariantCulture);
                myTbSyncLedOn.Text = scan.LedOnPosIndex.ToString(CultureInfo.InvariantCulture);
                myTbSyncLedOff.Text = scan.LedOffPosIndex.ToString(CultureInfo.InvariantCulture);

                myCbScanSequence.SelectedIndex = scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst ? 1 : 0;
                myTbSyncPreMoveOffset.Text = scan.PreMoveOffset.ToString(CultureInfo.InvariantCulture);
                myTbSyncPreMoveDelay.Text = scan.TimingPreMoveDelayUs.ToString(CultureInfo.InvariantCulture);
            }
            finally
            {
                _syncUpdating = false;
            }

            updateSyncBlankUs_();
            updateSyncControls_();
            reflectTrajectoryMode_();
            showAlignMetrics_();
        }

        // "(N us)" beside Row0 Ofs: the offset rows converted with the line time on screen,
        // so a Line Rate / Line Time edit moves it too.
        private void updateSyncBlankUs_()
        {
            if (myLbSyncBlankUs == null)
            {
                return;
            }

            int rows = ToInt(myTbSyncBlank.Text);
            double lineTimeUs = toDouble_(myTbSyncLineTime.Text);
            myLbSyncBlankUs.Content = rows > 0 && lineTimeUs > 0
                ? string.Format(CultureInfo.InvariantCulture, "({0:F0} us)", rows * lineTimeUs)
                : "(- us)";
        }

        // Fields -> preset(s). Lead / Tail / Pattern / LED are NOT read back - they are results
        // of the calculation - and Cam Delay belongs to the camera preset.
        //
        // editedLineTime: the DevKit lets the operator state the sensor line time directly, but
        // this stack stores the line RATE and derives the time from it. Both fields therefore
        // edit the same value, and which one was typed in decides the direction of the
        // conversion - otherwise whichever field is read second would undo the other.
        private void commitScanTiming_(bool editedLineTime = false)
        {
            if (_syncUpdating || myTbSyncLineRate == null)
            {
                return;
            }

            double lineRateHz = toDouble_(myTbSyncLineRate.Text);
            if (editedLineTime)
            {
                double lineTimeUs = toDouble_(myTbSyncLineTime.Text);
                lineRateHz = lineTimeUs > 0 ? 1_000_000.0 / lineTimeUs : 0.0;
            }

            LsoScanSequenceMode sequence = myCbScanSequence.SelectedIndex == 1
                ? LsoScanSequenceMode.GalvanoFirst
                : LsoScanSequenceMode.TrigFirst;

            foreach (ColorCameraPresetMode mode in syncTargetModes_())
            {
                LsoScanProfileModel scan = _scanTestModel.ModePresets.Active(mode).Scan;

                if (lineRateHz > 0)
                {
                    scan.LineRateHz = lineRateHz;
                }

                scan.SensorRows = Math.Max(0, ToInt(myTbSyncRows.Text));
                scan.SensorBlankingRows = Math.Max(0, ToInt(myTbSyncBlank.Text));
                scan.GalvoDelayUs = Math.Max(0.0, toDouble_(myTbSyncGalvoDelay.Text));
                scan.GalvoDecelUs = Math.Max(0.0, toDouble_(myTbSyncGalvoDecel.Text));

                // The sweep itself. Both ends are per mode even with "Both modes" off for the
                // rest, because a Preview and a Capture sweep are allowed to differ - so they
                // are written to the selected mode only.
                if (mode == AlignTargetMode)
                {
                    scan.GalvoStartPos = (short)Math.Clamp(ToInt(myTbGalvoStartPos.Text), short.MinValue, short.MaxValue);
                    scan.GalvoEndPos = (short)Math.Clamp(ToInt(myTbGalvoEndPos.Text), short.MinValue, short.MaxValue);
                }

                scan.SequenceMode = sequence;
                scan.PreMoveOffset = (short)Math.Clamp(ToInt(myTbSyncPreMoveOffset.Text), short.MinValue, short.MaxValue);
                scan.PreMoveDelayUs = (ushort)Math.Clamp(ToInt(myTbSyncPreMoveDelay.Text), 0, ushort.MaxValue);
                scan.RequestedPreMoveDelayUs = scan.PreMoveDelayUs;
            }

            // Both halves of the line-rate pair follow immediately, so what the next calculation
            // will use is on screen before it runs.
            _syncUpdating = true;
            try
            {
                LsoScanProfileModel shown = SyncPreset.Scan;
                myTbSyncLineRate.Text = shown.LineRateHz.ToString("F1", CultureInfo.InvariantCulture);
                myTbSyncLineTime.Text = shown.LineTimeUs.ToString("F2", CultureInfo.InvariantCulture);
            }
            finally
            {
                _syncUpdating = false;
            }

            updateSyncBlankUs_();
            updateRecExposure_();
            showAlignRange_();
        }

        private static double toDouble_(string text)
        {
            return double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double value)
                ? value
                : 0.0;
        }

        // Pre-move is a GALVANO_FIRST concept; in TRIG_FIRST the board waits on
        // EXPOSURE_ACTIVE and both values are 0, so they are shown read-only rather than
        // inviting an edit that the calculation will discard.
        private void updateSyncControls_()
        {
            if (myTbSyncPreMoveOffset == null)
            {
                return;
            }

            bool galvanoFirst = myCbScanSequence.SelectedIndex == 1;
            myTbSyncPreMoveOffset.IsReadOnly = !galvanoFirst;
            myTbSyncPreMoveDelay.IsReadOnly = !galvanoFirst;

            // The board is busy whenever a scan, a capture or an alignment owns it.
            myBtSyncApply.IsEnabled = !_alignActive && !IsRetinaPreviewing_() && !LsoCamera.IsCapturing();
        }

        #endregion Reflect / commit

        #region Handlers

        private void myTbSync_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            commitScanTiming_(editedLineTime: ReferenceEquals(sender, myTbSyncLineTime));
            autoRecalcScanTiming_();
            if (ReferenceEquals(sender, myTbSyncBlank))
                queueLiveOffsetUpdate_();
        }

        private void myTbSync_LostFocus(object sender, RoutedEventArgs e)
        {
            commitScanTiming_(editedLineTime: ReferenceEquals(sender, myTbSyncLineTime));
            autoRecalcScanTiming_();
            if (ReferenceEquals(sender, myTbSyncBlank))
                queueLiveOffsetUpdate_();
        }

        private void myCbScanSequence_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!IsLoaded)
            {
                return;
            }

            commitScanTiming_();
            updateSyncControls_();

            // The sequence changes the whole shape of the calculation, so it is recalculated
            // even when Auto is off - leaving TRIG_FIRST numbers under a GALVANO_FIRST label
            // would describe a scan that is not the one about to run.
            recalcScanTiming_(quiet: true);
        }

        private void autoRecalcScanTiming_()
        {
            if (SyncAutoRecalc)
            {
                recalcScanTiming_(quiet: true);
            }
        }

        private void myBtSyncRecalc_Click(object sender, RoutedEventArgs e)
        {
            recalcScanTiming_(quiet: false);
        }

        private void myBtSyncNudge_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && int.TryParse(button.Tag?.ToString(), out int delta))
            {
                myTbSyncBlank.Text = Math.Max(0L, (long)ToInt(myTbSyncBlank.Text) + delta)
                    .ToString(CultureInfo.InvariantCulture);
                commitScanTiming_();
                autoRecalcScanTiming_();
                queueLiveOffsetUpdate_();
            }
        }

        // Straight off the camera, like the Settings window's button. A failure keeps the
        // typed rate instead of replacing it with a zero.
        private void myBtSyncReadLineRate_Click(object sender, RoutedEventArgs e)
        {
            if (!LsoCamera.GetNodeValue("AcquisitionLineRate", out double rate) || rate <= 0)
            {
                MsgBoxUtil.ShowWarning("AcquisitionLineRate could not be read from the camera. "
                    + "The typed Line Rate was kept.");
                return;
            }

            myTbSyncLineRate.Text = rate.ToString("F1", CultureInfo.InvariantCulture);
            commitScanTiming_();
            autoRecalcScanTiming_();
        }

        private void recalcScanTiming_(bool quiet)
        {
            commitScanTiming_();

            foreach (ColorCameraPresetMode mode in syncTargetModes_())
            {
                LsoModePreset preset = _scanTestModel.ModePresets.Active(mode);
                double camDelayUs = preset.Camera.TriggerDelayUs > 0 ? preset.Camera.TriggerDelayUs : 0.0;
                double exposureUs = preset.Camera.ExposureTime > 0
                    ? preset.Camera.ExposureTime
                    : preset.Scan.ExposureTimeUs;

                if (!LsoScanTimingCalculator.TryCalculate(preset.Scan, camDelayUs, exposureUs,
                    out LsoScanTimingCalculator.Result result, out string reason,
                    desiredPreMoveDelayUs: preset.Scan.TimingPreMoveDelayUs))
                {
                    if (!quiet)
                    {
                        MsgBoxUtil.ShowWarning(reason);
                    }
                    return;
                }

                LsoScanTimingCalculator.ApplyTo(preset.Scan, result);

                if (mode == AlignTargetMode)
                {
                    myLbSyncStatus.Content = LsoScanTimingCalculator.DescribeSequence(preset.Scan, result)
                        + (result.Capped ? "  [capped]" : string.Empty);
                }
            }

            reflectScanTiming_();
        }

        // 정지 중 명시적 Apply. Live Row0 변경은 직렬 Dispatcher 경로에서 즉시 RAM을 갱신한다.
        private void myBtSyncApply_Click(object sender, RoutedEventArgs e)
        {
            if (_alignActive || IsRetinaPreviewing_() || LsoCamera.IsCapturing())
            {
                MsgBoxUtil.ShowWarning("Stop the scan, capture or alignment first - "
                    + "the board is running one of them.");
                return;
            }

            commitScanTiming_();

            foreach (ColorCameraPresetMode mode in syncTargetModes_())
            {
                int patternId = LsoModePreset.ScanPatternId(mode);
                if (!ApplyScannerControlParam(patternId))
                {
                    MsgBoxUtil.ShowWarning($"The {mode} scan profile was not written. "
                        + "Pattern Size must be non-zero - press Recalculate first.");
                    return;
                }

                // The sequence is board-global, so the mode applied LAST owns it - the same
                // rule an acquisition start follows.
                if (!LsoScanSequenceApply.Apply(_scanTestModel.ModePresets.Active(mode).Scan,
                    out string sequenceFailure))
                {
                    MsgBoxUtil.ShowWarning($"The {mode} scan profile was written, but "
                        + sequenceFailure);
                    return;
                }

                _scanTestModel.ModePresets.Save(mode);
            }

            myLbSyncStatus.Content = $"Applied to board ({string.Join(" + ", syncTargetModes_())}).";
        }

        #endregion Handlers
    }
}
