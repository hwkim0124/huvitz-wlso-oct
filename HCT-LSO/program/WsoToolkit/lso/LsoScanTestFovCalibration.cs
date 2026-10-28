using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;
using static WsoNativeLib.WsoLsoScan;

namespace WsoToolkit
{
    // DevKit "FOV Slit Calibration": which table the selected mode preset sweeps (Linear or
    // Calibrated FOV Curve), and the galvo-slit calibration window's access to this screen -
    // the parked galvo, the live alignment frame and the preset the result goes into.
    //
    // Same target as the Sync block: the alignment combo's mode preset. Per mode, like the
    // trajectory CSV - "Both modes" does not copy it.
    public partial class LsoScanTestWindow : Window
    {
        private LsoGalvoCalibrationWindow? _calibrationWindow;

        // One pending "next frame" request from the calibration window. The camera thread
        // completes it; the first frame after the request is skipped, because it may have been
        // exposed while the galvo was still moving to the new position.
        private readonly object _alignFrameWaitLock = new object();
        private TaskCompletionSource<AlignFrame>? _alignFrameWaiter;
        private int _alignFramesToSkip;

        private const int CalibrationFrameTimeoutMs = 3000;

        // Sensor Rows 0 means "not stated": the camera's full height, as the Sync block shows it.
        internal int CalibrationSensorRows => SyncPreset.Scan.SensorRows > 0
            ? SyncPreset.Scan.SensorRows
            : (int)(SyncPreset.Camera.RoiMaxHeight * Math.Max(1u, SyncPreset.Camera.BinningVertical));

        internal int CalibrationGalvoPosition => _galvanoYPosition;

        internal string CalibrationTargetName => $"{AlignTargetMode} preset '{SyncPreset.Name}'";

        internal ColorCameraPresetMode CalibrationTargetMode => AlignTargetMode;
        internal string CalibrationDirectory => _scanTestModel.ModePresets.Directory;
        internal LsoModePreset CalibrationTargetPreset => SyncPreset;
        internal bool CalibrationMeasurementAvailable => _alignActive && _calibrationPositionValid
            && !_galvanoSliderDragging && IsColorCameraLive();

        internal bool IsCalibrationTargetCurrent(ColorCameraPresetMode mode, LsoModePreset preset) =>
            IsLoaded && AlignTargetMode == mode && ReferenceEquals(SyncPreset, preset);

        #region Trajectory mode

        private void reflectTrajectoryMode_()
        {
            if (myCbTrajectoryMode == null)
            {
                return;
            }

            LsoScanProfileModel scan = SyncPreset.Scan;
            _syncUpdating = true;
            try
            {
                myCbTrajectoryMode.SelectedIndex = scan.IsCalibratedTrajectory ? 1 : 0;
            }
            finally
            {
                _syncUpdating = false;
            }
            myLbTrajectoryStatus.Content = describeTrajectory_(scan);
            _calibrationWindow?.RefreshTargetState();
        }

        private static string describeTrajectory_(LsoScanProfileModel scan)
        {
            if (!scan.IsCalibratedTrajectory)
            {
                return scan.HasTrajectoryFile
                    ? "CSV: " + Path.GetFileName(scan.TrajectoryFile)
                    : "linear Start -> End";
            }

            if (string.IsNullOrWhiteSpace(scan.CalibrationFile))
            {
                return "no calibration file - the next Capture is refused";
            }
            if (!LsoGalvoSlitCalibration.TryLoad(scan.CalibrationFile, out LsoGalvoSlitCalibration? calibration, out string error)
                || calibration == null)
            {
                return error;
            }
            return calibration.TryFit(out string fitError)
                ? $"{Path.GetFileName(scan.CalibrationFile)}: {calibration.Describe()}"
                : $"{Path.GetFileName(scan.CalibrationFile)}: {fitError}";
        }

        private void myCbTrajectoryMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_syncUpdating || !IsLoaded)
            {
                return;
            }

            if (myCbTrajectoryMode.SelectedIndex == 0)
            {
                SyncPreset.Scan.TrajectoryMode = LsoTrajectoryMode.Linear;
                reflectTrajectoryMode_();
                return;
            }

            // Calibrated needs a usable calibration; without one, open the table to make one
            // instead of storing a mode the next Capture would refuse.
            if (!UseCalibrationInPreset(SyncPreset.Scan.CalibrationFile, quiet: true))
            {
                reflectTrajectoryMode_();
                openCalibrationWindow_();
            }
        }

        // Points the selected mode preset at a calibration and switches it to the calibrated
        // sweep. Start / End become the fit's ends (DevKit behaviour) so the alignment Go Start /
        // Go End and the sweep direction agree with the table, and the timing is recalculated.
        // In memory, like Set as Start; Apply to board / Settings save it.
        internal bool UseCalibrationInPreset(string path, bool quiet)
            => UseCalibrationInPreset(AlignTargetMode, SyncPreset, path, quiet);

        internal bool UseCalibrationInPreset(ColorCameraPresetMode expectedMode, LsoModePreset expectedPreset,
            string path, bool quiet)
        {
            if (!IsCalibrationTargetCurrent(expectedMode, expectedPreset))
            {
                if (!quiet) MsgBoxUtil.ShowWarning("The selected mode or preset changed. Close and reopen calibration for the new target.");
                return false;
            }
            string failure;
            LsoGalvoSlitCalibration? calibration = null;
            if (string.IsNullOrWhiteSpace(path))
            {
                failure = "no calibration file is set.";
            }
            else if (!LsoGalvoSlitCalibration.TryLoad(path, out calibration, out failure) || calibration == null)
            {
                calibration = null;
            }
            else if (!calibration.TryFit(out failure))
            {
                calibration = null;
            }

            if (calibration == null)
            {
                if (!quiet)
                {
                    MsgBoxUtil.ShowWarning($"The calibration was not used: {failure}");
                }
                return false;
            }

            if (CalibrationSensorRows <= 0)
            {
                if (!quiet) MsgBoxUtil.ShowWarning("Set Sensor Rows before using a calibrated FOV curve.");
                return false;
            }
            LsoScanProfileModel scan = expectedPreset.Scan;
            if (scan.SensorRows <= 0)
            {
                scan.SensorRows = CalibrationSensorRows;
            }
            calibration.SweepEnds(scan.SensorRows, out short start, out short end);
            scan.CalibrationFile = path;
            scan.TrajectoryMode = LsoTrajectoryMode.CalibratedFov;
            scan.GalvoStartPos = start;
            scan.GalvoEndPos = end;

            reflectScanTiming_();
            autoRecalcScanTiming_();
            showAlignRange_();
            return true;
        }

        private void myBtFovCalibration_Click(object sender, RoutedEventArgs e)
        {
            openCalibrationWindow_();
        }

        // Modeless: the galvo is jogged and the alignment toggled on this window while the
        // table is open, exactly as with the DevKit's FOV Calibration Table.
        private void openCalibrationWindow_()
        {
            if (_calibrationWindow != null)
            {
                _calibrationWindow.Activate();
                return;
            }

            string file = SyncPreset.Scan.CalibrationFile;
            if (string.IsNullOrWhiteSpace(file))
            {
                file = Path.Combine(_scanTestModel.ModePresets.Directory,
                    SyncPreset.Name + "_" + LsoGalvoSlitCalibration.DefaultFileName);
            }

            _calibrationWindow = new LsoGalvoCalibrationWindow(this, file, SyncPreset.Scan.GalvoStartPos,
                SyncPreset.Scan.GalvoEndPos)
            {
                Owner = this,
            };
            _calibrationWindow.Closed += (_, _) => _calibrationWindow = null;
            _calibrationWindow.Show();
        }

        #endregion Trajectory mode

        #region Calibration measurement

        internal async Task<(double? Row, string Error)> MeasureSlitEdgesForCalibrationAsync(
            ColorCameraPresetMode mode, LsoModePreset preset, CancellationToken token)
        {
            if (!IsCalibrationTargetCurrent(mode, preset) || !CalibrationMeasurementAvailable)
                return (null, "Turn Alignment on at a stationary galvo position first.");
            int version = _calibrationPositionVersion;
            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam camera);
            hookSlitMeasure_();
            var overlay = myColorPreview.Image.SlitMeasure;
            overlay.VerticalBinning = (int)Math.Max(1u, camera.binningVertical);
            var completion = new TaskCompletionSource<double>(TaskCreationOptions.RunContinuationsAsynchronously);
            void measured(double center, int height) => completion.TrySetResult(center);
            void measuringChanged(bool active)
            {
                // CenterMeasured follows MeasuringChanged(false) in the same UI event.
                if (!active) Dispatcher.BeginInvoke(new Action(() => completion.TrySetCanceled()));
            }
            overlay.CenterMeasured += measured;
            overlay.MeasuringChanged += measuringChanged;
            try
            {
                overlay.StartMeasure();
                double row = await completion.Task.WaitAsync(token);
                LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam current);
                if (!IsCalibrationTargetCurrent(mode, preset) || !CalibrationMeasurementAvailable
                    || version != _calibrationPositionVersion || current.binningVertical != camera.binningVertical
                    || current.roiYOffset != camera.roiYOffset || current.roiXWidth != camera.roiXWidth
                    || current.roiYHeight != camera.roiYHeight)
                    return (null, "Target, position or camera geometry changed; measure again.");
                return (row + camera.roiYOffset, string.Empty);
            }
            finally
            {
                overlay.CenterMeasured -= measured;
                overlay.MeasuringChanged -= measuringChanged;
                overlay.CancelMeasure();
            }
        }

        // Moves the parked galvo. Only in alignment mode: outside it the galvo belongs to the
        // running scan, and there is no stationary slit to measure anyway.
        internal bool MoveGalvoForCalibration(int dac, out string failure)
        {
            if (!_alignActive)
            {
                failure = "turn Alignment on in the scan test window first.";
                return false;
            }

            if (dac < short.MinValue || dac > short.MaxValue)
            {
                failure = "Galvo DAC must be from -32768 to 32767.";
                return false;
            }
            if (!LsoScanner.MoveLsoScannerYposition(dac))
            {
                _calibrationPositionValid = false;
                failure = $"the galvo did not move to {dac}.";
                return false;
            }
            _calibrationPositionVersion++;
            _calibrationPositionValid = true;
            setGalvanoYPosition_(dac);

            failure = string.Empty;
            return true;
        }

        // Slit centre in absolute sensor rows, measured on a frame exposed after this call (the
        // same band measurement as the Auto button). Null with the reason when there is none.
        internal async Task<(double? Row, string Error)> MeasureSlitRowForCalibrationAsync(
            ColorCameraPresetMode expectedMode, LsoModePreset expectedPreset, CancellationToken cancellationToken)
        {
            if (!IsCalibrationTargetCurrent(expectedMode, expectedPreset))
            {
                return (null, "the selected mode or preset changed; reopen calibration for the new target.");
            }
            if (!CalibrationMeasurementAvailable)
            {
                return (null, "turn Alignment on in the scan test window first - the slit is only stationary and visible there.");
            }

            cancellationToken.ThrowIfCancellationRequested();
            int positionVersion = _calibrationPositionVersion;
            int dac = _galvanoYPosition;
            // Alignment live retains the actual camera geometry; it need not match the
            // selected preset until Apply. Do not convert a live row with un-applied fields.
            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam camera);
            var waiter = new TaskCompletionSource<AlignFrame>(TaskCreationOptions.RunContinuationsAsynchronously);
            lock (_alignFrameWaitLock)
            {
                if (_alignFrameWaiter != null)
                {
                    return (null, "a slit measurement is already waiting for a live frame.");
                }
                _alignFrameWaiter = waiter;
                _alignFramesToSkip = 1;
            }

            AlignFrame frame;
            try
            {
                frame = await waiter.Task.WaitAsync(TimeSpan.FromMilliseconds(CalibrationFrameTimeoutMs), cancellationToken);
            }
            catch (TimeoutException)
            {
                return (null, $"no live frame arrived within {CalibrationFrameTimeoutMs / 1000} s.");
            }
            finally
            {
                lock (_alignFrameWaitLock)
                {
                    if (ReferenceEquals(_alignFrameWaiter, waiter)) _alignFrameWaiter = null;
                }
            }
            cancellationToken.ThrowIfCancellationRequested();
            if (!IsCalibrationTargetCurrent(expectedMode, expectedPreset) || !CalibrationMeasurementAvailable
                || positionVersion != _calibrationPositionVersion)
            {
                return (null, "the preset, alignment state or galvo position changed during measurement; record again at a stationary position.");
            }

            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam currentCamera);
            if (camera.binningVertical != currentCamera.binningVertical || camera.roiYOffset != currentCamera.roiYOffset
                || camera.roiXWidth != currentCamera.roiXWidth || camera.roiYHeight != currentCamera.roiYHeight)
            {
                return (null, "camera geometry changed during measurement; record again.");
            }
            double? center = LsoGalvoSlitCalibration.DetectFrameSlitCenter(frame.Data, frame.Width, frame.Height,
                frame.BytesPerPixel, frame.PixelFormat, out int top, out int bottom);
            if (!center.HasValue)
            {
                return (null, "No bright slit found in the central 10% ROI; check exposure or use 2-click.");
            }

            int binning = (int)Math.Max(1u, camera.binningVertical);
            double row = (center.Value * binning) + camera.roiYOffset;

            hookSlitMeasure_();
            myColorPreview.Image.SlitMeasure.ShowBand(top, bottom, center.Value,
                $"CAL: galvo {dac} -> centre row {row:F1}");
            return (row, string.Empty);
        }

        private void cancelCalibrationFrameWait_()
        {
            TaskCompletionSource<AlignFrame>? waiter;
            lock (_alignFrameWaitLock)
            {
                waiter = _alignFrameWaiter;
                _alignFrameWaiter = null;
            }
            waiter?.TrySetCanceled();
        }

        // Camera thread, from rememberAlignFrame_.
        private void signalAlignFrame_(AlignFrame frame)
        {
            TaskCompletionSource<AlignFrame>? waiter;
            lock (_alignFrameWaitLock)
            {
                waiter = _alignFrameWaiter;
                if (waiter == null)
                {
                    return;
                }
                if (_alignFramesToSkip > 0)
                {
                    _alignFramesToSkip--;
                    return;
                }
                _alignFrameWaiter = null;
            }
            waiter.TrySetResult(frame);
        }

        #endregion Calibration measurement
    }
}
