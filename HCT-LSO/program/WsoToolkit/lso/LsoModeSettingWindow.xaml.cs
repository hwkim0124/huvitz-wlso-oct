using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Numerics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    // One page, one preset: the mode radio selects which mode preset every control edits -
    // camera, galvo and light together. Nothing here touches the hardware until Apply.
    public partial class LsoModeSettingWindow : Window
    {
        private readonly LsoScanTestWindow _owner;
        private readonly LsoScanTestModel _model;
        private readonly ColorCameraSettingWindow _previewCamera;
        private readonly ColorCameraSettingWindow _captureCamera;

        private ColorCameraPresetMode _mode = ColorCameraPresetMode.Capture;
        private bool _isUpdating;

        // The timing recalculation writes the very fields whose commit can trigger it again.
        private bool _recalculating;

        // Same grey the Exposure mirror uses in the XAML, for a field the arithmetic owns.
        private static readonly System.Windows.Media.Brush ReadOnlyFieldBrush =
            new System.Windows.Media.SolidColorBrush(
                System.Windows.Media.Color.FromRgb(0xEF, 0xEF, 0xEF));

        public LsoModeSettingWindow(LsoScanTestWindow owner, LsoScanTestModel model)
        {
            _owner = owner;
            _model = model;
            _previewCamera = new ColorCameraSettingWindow(model, ColorCameraPresetMode.Preview);
            _captureCamera = new ColorCameraSettingWindow(model, ColorCameraPresetMode.Capture);
            InitializeComponent();
        }

        private LsoModePreset Preset => _model.ModePresets.Active(_mode);

        private ColorCameraSettingWindow CameraPanel =>
            _mode == ColorCameraPresetMode.Preview ? _previewCamera : _captureCamera;

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            _isUpdating = true;
            myCbTriggerSrc.Items.Clear();
            myCbTriggerSrc.Items.Add("Color Camera");
            myCbTriggerSrc.Items.Add("IR Camera");
            myCheckBoxDeriveAcquisition.IsChecked = _owner.DeriveAcquisitionFromScan;

            // Set here, not in the XAML: a Checked fired while the markup is still being
            // parsed would reach the Start/End boxes declared below the checkbox.
            myCheckGalvoAutoSweep.IsChecked = true;

            myCbLsoFilterMode.Items.Clear();
            myCbLsoFilterMode.Items.Add("Do not move");                 // index 0 -> -1
            for (int filterMode = 0; filterMode <= 3; ++filterMode)      // index 1..4 -> 0..3
            {
                myCbLsoFilterMode.Items.Add(
                    $"{filterMode}: {LsoModeMotorApply.FilterModeName(filterMode)}");
            }
            _isUpdating = false;

            showMode_(ColorCameraPresetMode.Capture);
        }

        #region Mode / preset selection

        private void Mode_Checked(object sender, RoutedEventArgs e)
        {
            if (_isUpdating || cameraHost == null)
            {
                return;
            }

            // Camera boxes only reach the preset on Enter, and the panel is reloaded from the
            // preset when it is shown again - so the one being left is committed first.
            if (!CameraPanel.CommitToPreset())
            {
                MsgBoxUtil.ShowWarning("These camera fields could not be read and kept their previous values:"
                    + Environment.NewLine + Environment.NewLine
                    + string.Join(Environment.NewLine, CameraPanel.InputErrors));
            }

            showMode_(rbModeCapture.IsChecked == true
                ? ColorCameraPresetMode.Capture
                : ColorCameraPresetMode.Preview);
        }

        private void showMode_(ColorCameraPresetMode mode)
        {
            _mode = mode;

            // Re-parenting the panel tears down and rebuilds its combo containers, and the
            // SelectionChanged that lands afterwards re-enters this method - so the host is
            // only re-pointed when the panel actually changes.
            if (!ReferenceEquals(cameraHost.Content, CameraPanel))
            {
                cameraHost.Content = CameraPanel;
            }

            CameraPanel.ReloadFromPreset();
            refreshPresetList_();
            updateScanControls_();
            updateLightControls_();
            updateMotorControls_();
            updatePostProcessControls_();

            // DevKit recalculates on load too. This OVERWRITES the Pattern Size, ramps and
            // Light indices the preset stored, which is the point of the option - so it is
            // opt-in and off by default.
            if (myCheckBoxAutoSync?.IsChecked == true)
            {
                recalcTiming_(quiet: true);
            }
        }

        #region Motor

        private void updateMotorControls_()
        {
            _isUpdating = true;
            try
            {
                int filterMode = Preset.Motor.LsoFilterMode;
                myCbLsoFilterMode.SelectedIndex = filterMode >= 0 && filterMode <= 3
                    ? filterMode + 1
                    : 0;
                myTbLsoFocusPosition.Text =
                    Preset.Motor.LsoFocusPosition.ToString(CultureInfo.InvariantCulture);
            }
            finally
            {
                _isUpdating = false;
            }
        }

        // Fields -> preset object only, like every other section here. The motors are driven
        // by Apply.
        private void myCbLsoFilterMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            Preset.Motor.LsoFilterMode = myCbLsoFilterMode.SelectedIndex <= 0
                ? LsoModeMotor.NotStated
                : myCbLsoFilterMode.SelectedIndex - 1;
        }

        private void myTbLsoFocusPosition_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                commitFocusPosition_();
            }
        }

        private void myTbLsoFocusPosition_LostFocus(object sender, RoutedEventArgs e)
        {
            commitFocusPosition_();
        }

        private void commitFocusPosition_()
        {
            if (_isUpdating)
            {
                return;
            }

            // Anything below zero is the "leave it where it is" sentinel, so a typo cannot
            // send the focus motor to a negative step.
            int value = ToInt(myTbLsoFocusPosition.Text);
            Preset.Motor.LsoFocusPosition = value < 0 ? LsoModeMotor.NotStated : value;
            updateMotorControls_();
        }

        #endregion Motor

        #region Post-process

        // Capture only: the aiming Preview is never corrected, so the group is greyed out there.
        private void updatePostProcessControls_()
        {
            _isUpdating = true;
            try
            {
                LsoModePostProcess postProcess = Preset.PostProcess;
                myGroupPostProcess.IsEnabled = _mode == ColorCameraPresetMode.Capture;
                myCheckFlatField.IsChecked = postProcess.FlatFieldEnabled;
                myLbFlatField.Content = string.IsNullOrWhiteSpace(postProcess.FlatFieldFile)
                    ? "(none)"
                    : System.IO.Path.GetFileName(postProcess.FlatFieldFile);
                myLbFlatField.ToolTip = string.IsNullOrWhiteSpace(postProcess.FlatFieldFile)
                    ? "Raw Bayer PGM of a uniform target shot with this preset (e.g. lsotune capture's frame.pgm)."
                    : postProcess.FlatFieldFile;

                myCheckReflectedLight.IsChecked = postProcess.ReflectedLightEnabled;
                // A switch already stated in the preset stays clickable outside White capture, so it
                // can still be turned off; captures in those modes run without the removal.
                bool eligible = _mode == ColorCameraPresetMode.Capture
                    && _model.ModePresets.CaptureImageMode == LsoImageMode.Color
                    && Preset.ColorIllumination == ColorIllumination.White;
                myCheckReflectedLight.IsEnabled = eligible || postProcess.ReflectedLightEnabled;
                myCheckReflectedLight.ToolTip = eligible
                    ? "Subtracts this instrument's measured reflected-light map from White captures, per focus position; measured with Reflected Light Cal... in the LSO scan test window."
                    : postProcess.ReflectedLightEnabled
                        ? "White Color capture only - captures in this mode run without it"
                        : "White Color capture only";
                try
                {
                    if (LsoReflectedLightSet.TryLoad(LsoConfigLocation.ReflectedLightDirectory(),
                            out var set, out string error))
                    {
                        myLbReflectedLight.Content = string.Format(CultureInfo.InvariantCulture,
                            "{0} pts, ROI {1},{2} {3}x{4}, {5:yyyy-MM-dd}", set!.Points.Count,
                            set.Roi.X, set.Roi.Y, set.Roi.Width, set.Roi.Height, set.Created.ToLocalTime());
                        myLbReflectedLight.ToolTip = null;
                    }
                    else
                    {
                        myLbReflectedLight.Content = "(not calibrated)";
                        myLbReflectedLight.ToolTip = error;
                    }
                }
                catch (Exception error)
                {
                    myLbReflectedLight.Content = "(not calibrated)";
                    myLbReflectedLight.ToolTip = error.Message;
                }
            }
            finally
            {
                _isUpdating = false;
            }
        }

        // Fields -> preset object only, like the motors; Apply / Save As writes the file. The
        // switch is refused while there is no flat that fits this preset, so a saved preset
        // cannot promise a correction the next capture would then have to refuse.
        private void FlatField_Changed(object sender, RoutedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            bool on = myCheckFlatField.IsChecked == true;
            if (on && !checkFlatFile_(Preset.PostProcess.FlatFieldFile))
            {
                updatePostProcessControls_();
                return;
            }

            Preset.PostProcess.FlatFieldEnabled = on;
        }

        private void ReflectedLight_Changed(object sender, RoutedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            if (myCheckReflectedLight.IsChecked != true)
            {
                Preset.PostProcess.ReflectedLightEnabled = false;
                return;
            }

            CameraPanel.CommitToPreset();
            Preset.PostProcess.ReflectedLightEnabled = true;
            LsoCamera.GetCameraParameters(out var readBack);
            if (!LsoFundusModeCapture.TryPrepare(_model.ModePresets.CaptureImageMode,
                    Preset, _model.ModePresets.Preview, ColorCaptureTriggerMode.HWTrigger,
                    readBack, DeviceLights.GetLightIntensity, true, out var prepared, out string failure))
            {
                Preset.PostProcess.ReflectedLightEnabled = false;
                MsgBoxUtil.ShowWarning(failure);
                updatePostProcessControls_();
            }
            else if (prepared?.ReflectedLightNote is { Length: > 0 } note)
            {
                MsgBoxUtil.ShowWarning(note);
            }
        }

        private void myBtFlatFieldBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Title = "Flat-field reference (raw Bayer PGM of a uniform target)",
                Filter = "Raw Bayer PGM (*.pgm)|*.pgm",
                InitialDirectory = _model.ModePresets.Directory,
            };
            if (dialog.ShowDialog(this) != true || !checkFlatFile_(dialog.FileName))
            {
                return;
            }

            Preset.PostProcess.FlatFieldFile = dialog.FileName;
            updatePostProcessControls_();
        }

        private void myBtFlatFieldClear_Click(object sender, RoutedEventArgs e)
        {
            Preset.PostProcess.FlatFieldEnabled = false;
            Preset.PostProcess.FlatFieldFile = string.Empty;
            updatePostProcessControls_();
        }

        // The same load the capture does, against the geometry in the camera panel.
        private bool checkFlatFile_(string path)
        {
            if (string.IsNullOrWhiteSpace(path))
            {
                MsgBoxUtil.ShowWarning("Pick a flat file first (Browse...).");
                return false;
            }

            CameraPanel.CommitToPreset();
            if (!LsoFlatField.TryLoad(path, (int)Preset.Camera.RoiXWidth, (int)Preset.Camera.RoiYHeight,
                    out _, out string error))
            {
                MsgBoxUtil.ShowWarning(error);
                return false;
            }

            return true;
        }

        #endregion Post-process

        private void refreshPresetList_()
        {
            _isUpdating = true;
            try
            {
                IReadOnlyList<string> names = _model.ModePresets.EnumeratePresetNames();

                // Only rebuild when the list really changed: Clear/Add raises
                // SelectionChanged from the deferred container generation, i.e. after the
                // _isUpdating guard is already back to false, and that handler comes back
                // here - which is an endless loop, not a refresh.
                if (!presetItemsMatch_(names))
                {
                    myCbPreset.Items.Clear();
                    foreach (string name in names)
                    {
                        myCbPreset.Items.Add(name);
                    }
                }

                myCbPreset.SelectedItem = null;
                foreach (string name in names)
                {
                    if (string.Equals(name, Preset.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        myCbPreset.SelectedItem = name;
                        break;
                    }
                }

                myCbPreset.Text = Preset.Name;
                myBtPresetDelete.IsEnabled = !LsoModePresetStore.IsDefaultName(Preset.Name);
            }
            finally
            {
                _isUpdating = false;
            }
        }

        private bool presetItemsMatch_(IReadOnlyList<string> names)
        {
            if (myCbPreset.Items.Count != names.Count)
            {
                return false;
            }

            for (int i = 0; i < names.Count; ++i)
            {
                if (!string.Equals(myCbPreset.Items[i] as string, names[i], StringComparison.Ordinal))
                {
                    return false;
                }
            }

            return true;
        }

        private void myCbPreset_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // The same name is the deferred SelectionChanged of refreshPresetList_, not a
            // request - reloading it from disk would throw the current edits away.
            if (_isUpdating || myCbPreset.SelectedItem is not string name
                || string.Equals(name, Preset.Name, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            // Selecting loads the other preset from its file; whatever was typed here and not
            // saved would silently vanish.
            commitControlsToPreset_();
            if (_model.ModePresets.HasUnsavedChanges(_mode)
                && !MsgBoxUtil.ShowQuestion($"'{Preset.Name}' has changes that are not saved "
                    + $"(Apply or Save As). Discard them and load '{name}'?"))
            {
                refreshPresetList_();
                return;
            }

            _model.ModePresets.Select(_mode, name);
            showMode_(_mode);
            reportIssues_();
        }

        private void myBtPresetSaveAs_Click(object sender, RoutedEventArgs e)
        {
            string name = typedPresetName_();
            if (name.Length == 0 || !confirmOverwrite_(name))
            {
                return;
            }

            // Same rule as Apply: a field that could not be read must not be stored either,
            // or the file would keep a value the operator never typed.
            if (!commitControlsToPreset_())
            {
                MsgBoxUtil.ShowWarning("Nothing was saved - these fields could not be read:"
                    + Environment.NewLine + Environment.NewLine
                    + string.Join(Environment.NewLine, inputErrors_()));
                return;
            }

            _model.ModePresets.SaveAs(_mode, name);
            showMode_(_mode);
            reportIssues_();
        }

        private void myBtPresetNew_Click(object sender, RoutedEventArgs e)
        {
            string name = typedPresetName_();
            if (name.Length == 0 || !confirmOverwrite_(name))
            {
                return;
            }

            _model.ModePresets.CreateDefault(_mode, name);
            showMode_(_mode);
            reportIssues_();
        }

        private void myBtPresetDelete_Click(object sender, RoutedEventArgs e)
        {
            string name = Preset.Name;
            if (LsoModePresetStore.IsDefaultName(name))
            {
                MsgBoxUtil.ShowWarning($"'{name}' is a built-in preset and cannot be deleted.");
                return;
            }

            if (!MsgBoxUtil.ShowQuestion($"Delete preset '{name}'?"))
            {
                return;
            }

            _model.ModePresets.Delete(name);
            showMode_(_mode);
            reportIssues_();
        }

        private string typedPresetName_()
        {
            string name = (myCbPreset.Text ?? string.Empty).Trim();
            if (!LsoModePresetStore.IsValidName(name))
            {
                MsgBoxUtil.ShowWarning("Type a preset name (a valid file name) in the box first.");
                refreshPresetList_();
                return string.Empty;
            }

            return name;
        }

        private bool confirmOverwrite_(string name)
        {
            if (!_model.ModePresets.Exists(name))
            {
                return true;
            }

            if (MsgBoxUtil.ShowQuestion($"Preset '{name}' already exists. Overwrite it?"))
            {
                return true;
            }

            refreshPresetList_();
            return false;
        }

        private void reportIssues_()
        {
            IReadOnlyList<string> issues = _model.ModePresets.DrainIssues();
            if (issues.Count == 0)
            {
                return;
            }

            MsgBoxUtil.ShowWarning("Presets:" + Environment.NewLine + Environment.NewLine
                + string.Join(Environment.NewLine, issues));
        }

        #endregion Mode / preset selection

        #region Scan (galvo)

        private void updateScanControls_()
        {
            LsoScanProfileModel scan = Preset.Scan;

            _isUpdating = true;
            myCbTriggerSrc.SelectedIndex = scan.TriggerSource;
            myCheckBoxFixedFrame.IsChecked = scan.AcquisitionMode != 0;
            myTbCameraTimeStep.Text = scan.TimeStepUs.ToString();
            // Read-only mirror of the camera preset's exposure (the authority). The scan value
            // is only shown when the camera preset has none.
            uint cameraExposure = Preset.Camera.ExposureTime;
            myTbCameraExposureTime.Text = (cameraExposure > 0 ? (int)cameraExposure : scan.ExposureTimeUs).ToString();
            myTbLeadIn.Text = scan.LeadInSamples.ToString();
            myTbLeadOut.Text = scan.LeadOutSamples.ToString();

            // Timing calculator inputs now live in the preset. Sensor Rows 0 means "not
            // stated" and falls back to the camera's full height; Cam Delay is a read-only
            // mirror of the camera preset's TriggerDelayUs, which is the same physical delay.
            myTbLineRate.Text = scan.LineRateHz.ToString("F1", CultureInfo.InvariantCulture);
            myTbLineTime.Text = scan.LineTimeUs.ToString("F2", CultureInfo.InvariantCulture);
            myTbGalvoDelay.Text = scan.GalvoDelayUs.ToString("F1", CultureInfo.InvariantCulture);
            myTbGalvoDecel.Text = scan.GalvoDecelUs.ToString("F1", CultureInfo.InvariantCulture);
            myTbSensorRows.Text = (scan.SensorRows > 0
                ? scan.SensorRows
                : (int)Preset.Camera.RoiMaxHeight).ToString(CultureInfo.InvariantCulture);
            myTbCamDelay.Text = Preset.Camera.TriggerDelayUs >= 0
                ? Preset.Camera.TriggerDelayUs.ToString("F1", CultureInfo.InvariantCulture)
                : "0.0";
            myTbSlitRows.Text = scan.SlitHeightRows.ToString(CultureInfo.InvariantCulture);
            myTbBlankRows.Text = scan.SensorBlankingRows.ToString(CultureInfo.InvariantCulture);
            updateRecExposure_();
            updateBlankTimeLabel_();
            myTbAcqFrame.Text = scan.AcqFrameSize.ToString();
            myTbSubFrame.Text = scan.SubFrameSize.ToString();
            myTbGalvoPatternSize.Text = scan.GalvoPatternSize.ToString();
            myTbGalvoRewindOffset.Text = scan.GalvanoRewindOffset.ToString();
            myTbGalvoStartPos.Text = scan.GalvoStartPos.ToString();
            myTbGalvoEndPos.Text = scan.GalvoEndPos.ToString();
            myTbLightIndexOn.Text = scan.LedOnPosIndex.ToString();
            myTbLightIndexOff.Text = scan.LedOffPosIndex.ToString();
            myLbTrajectory.Content = trajectorySummary_(scan);
            myCbTrajectoryMode.SelectedIndex = scan.IsCalibratedTrajectory ? 1 : 0;

            // Derived, never stored: the full-frame range the stored sweep describes.
            myTbGalvoTotalStart.Text = scan.GalvoStartPos.ToString(CultureInfo.InvariantCulture);
            myTbGalvoTotalEnd.Text = scan.FullFrameEndPos.ToString(CultureInfo.InvariantCulture);
            _isUpdating = false;

            updateGalvoSweepLabel_();
        }

        #region Full-frame sweep

        private bool AutoGalvoSweep => myCheckGalvoAutoSweep.IsChecked == true;

        // The option owns Start Pos (pinned to the full-frame start) and Rewind Offset (the
        // arithmetic) - those go read-only. End Pos stays editable either way: it is the
        // movement INSIDE one subframe, which is the operator's call (equal to Start = the
        // mirror stands still for that frame, which is how the instrument is run today).
        private void applyGalvoSweepEditState_()
        {
            bool auto = AutoGalvoSweep;
            myTbGalvoStartPos.IsReadOnly = auto;
            myTbGalvoRewindOffset.IsReadOnly = auto;
            myTbGalvoStartPos.Background = auto ? ReadOnlyFieldBrush : System.Windows.Media.Brushes.White;
            myTbGalvoRewindOffset.Background = auto ? ReadOnlyFieldBrush : System.Windows.Media.Brushes.White;
        }

        private void updateGalvoSweepLabel_()
        {
            LsoScanProfileModel scan = Preset.Scan;
            int sweep = scan.GalvoEndPos - scan.GalvoStartPos;
            int hops = Math.Max(0, (int)scan.SubFrameSize - 1);
            int requested = ToInt(myTbGalvoTotalEnd.Text);
            int achieved = scan.FullFrameEndPos;
            string drift = achieved == requested
                ? string.Empty
                : $", full frame ends at {achieved} (off by {achieved - requested})";

            myLbGalvoSweep.Content = $"sweep {sweep} + rewind {scan.GalvanoRewindOffset} x {hops} hops{drift}";
            applyGalvoSweepEditState_();
        }

        private void GalvoAutoSweep_Changed(object sender, RoutedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            applyGalvoSweepEditState_();
            if (AutoGalvoSweep)
            {
                recalcGalvoSweep_(quiet: false);
            }
        }

        // Start Pos is pinned to the full-frame start and the rewind offset is what walks the
        // remaining subframes to the full-frame end (N-1 hops, as measured on the board). The
        // movement inside one subframe - End Pos - is kept as the operator set it, including
        // the usual 0 (a still mirror). quiet mirrors recalcTiming_: an incomplete field
        // mid-edit does nothing, an explicit toggle is allowed to complain.
        private void recalcGalvoSweep_(bool quiet)
        {
            int totalStart = ToInt(myTbGalvoTotalStart.Text);
            int totalEnd = ToInt(myTbGalvoTotalEnd.Text);
            int subFrames = ToInt(myTbSubFrame.Text);
            int subFrameSweep = ToInt(myTbGalvoEndPos.Text) - ToInt(myTbGalvoStartPos.Text);

            if (!LsoScanProfileModel.TryPlanGalvoSweep(totalStart, totalEnd, subFrames, subFrameSweep,
                    out LsoScanProfileModel.GalvoSweepPlan plan))
            {
                if (!quiet)
                {
                    MsgBoxUtil.ShowWarning("Sub Frame Count must be positive, the rewind offset it "
                        + "works out to has to fit the board's int16 field (-32768..32767), and "
                        + "something has to move: a still subframe (Start Pos = End Pos) with a "
                        + "zero rewind would never scan.");
                }
                return;
            }

            _recalculating = true;
            try
            {
                myTbGalvoStartPos.Text = plan.Start.ToString(CultureInfo.InvariantCulture);
                myTbGalvoEndPos.Text = plan.End.ToString(CultureInfo.InvariantCulture);
                myTbGalvoRewindOffset.Text = plan.RewindOffset.ToString(CultureInfo.InvariantCulture);

                commitScanControls_();
            }
            finally
            {
                _recalculating = false;
            }

            updateGalvoSweepLabel_();
        }

        #endregion Full-frame sweep

        private bool SyncBothModes => myCheckBoxSyncModes.IsChecked == true;

        // Fields -> preset object only. The board is written by Apply.
        // With "both modes" on, the sweep geometry goes to the Preview AND Capture presets
        // (DevKit Update Sync); Trigger src and Fixed Frame stay per mode, as in the DevKit.
        // Exposure is never taken from the field: each preset's scan mirrors its own camera.
        private void commitScanControls_()
        {
            LsoScanProfileModel scan = Preset.Scan;
            scan.TriggerSource = (ushort)Math.Max(0, myCbTriggerSrc.SelectedIndex);
            scan.AcquisitionMode = (ushort)(myCheckBoxFixedFrame.IsChecked == true ? 1 : 0);

            // Read once, against the shown mode's values: a field that cannot be read keeps
            // what the preset had and is named in _scanInputErrors, so Apply / Save refuse
            // instead of storing a 0 nobody typed (40000 in an int16 box used to become 0).
            _scanInputErrors.Clear();
            int timeStep = readNumber_("Time Step", myTbCameraTimeStep, scan.TimeStepUs);
            uint acqFrame = readNumber_("Acq Frame", myTbAcqFrame, scan.AcqFrameSize);
            ushort subFrame = readNumber_("Sub Frame Count", myTbSubFrame, scan.SubFrameSize);
            ushort patternSize = readNumber_("Galvo. Pattern Size", myTbGalvoPatternSize, scan.GalvoPatternSize);
            short rewind = readNumber_("Galvo. Rewind Offset", myTbGalvoRewindOffset, scan.GalvanoRewindOffset);
            short startPos = readNumber_("Galvo. Start Pos", myTbGalvoStartPos, scan.GalvoStartPos);
            short endPos = readNumber_("Galvo. End Pos", myTbGalvoEndPos, scan.GalvoEndPos);
            ushort ledOn = readNumber_("Light Index On", myTbLightIndexOn, scan.LedOnPosIndex);
            ushort ledOff = readNumber_("Light Index Off", myTbLightIndexOff, scan.LedOffPosIndex);
            ushort leadIn = readNumber_("Lead-in", myTbLeadIn, scan.LeadInSamples);
            ushort leadOut = readNumber_("Lead-out", myTbLeadOut, scan.LeadOutSamples);
            double lineRate = readNumber_("Line Rate", myTbLineRate, scan.LineRateHz);
            int sensorRows = readNumber_("Sensor Rows", myTbSensorRows, scan.SensorRows);
            double galvoDelay = readNumber_("Galvo Delay", myTbGalvoDelay, scan.GalvoDelayUs);
            double galvoDecel = readNumber_("Galvo Decel", myTbGalvoDecel, scan.GalvoDecelUs);
            int slitRows = readNumber_("Slit (rows)", myTbSlitRows, scan.SlitHeightRows);
            int blankRows = readNumber_("Row 0 Offset (rows)", myTbBlankRows, scan.SensorBlankingRows);

            foreach (ColorCameraPresetMode mode in targetModes_())
            {
                LsoModePreset preset = _model.ModePresets.Active(mode);
                LsoScanProfileModel target = preset.Scan;
                target.TimeStepUs = timeStep;
                if (preset.Camera.ExposureTime > 0)
                {
                    target.ExposureTimeUs = (int)preset.Camera.ExposureTime;
                }
                target.AcqFrameSize = acqFrame;
                target.SubFrameSize = subFrame;
                target.GalvoPatternSize = patternSize;
                target.GalvanoRewindOffset = rewind;
                target.GalvoStartPos = startPos;
                target.GalvoEndPos = endPos;
                target.LedOnPosIndex = ledOn;
                target.LedOffPosIndex = ledOff;
                target.LeadInSamples = leadIn;
                target.LeadOutSamples = leadOut;
                // TrajectoryFile is not here on purpose: the editor writes it straight into
                // the mode's own preset, and 'both modes' must not copy a table whose row
                // count only fits one mode's Pattern Size.

                // What the calculator was last run with, so the next session recomputes the
                // same numbers. Cam Delay is not here - it belongs to the camera preset.
                target.LineRateHz = lineRate;
                target.SensorRows = sensorRows;
                target.GalvoDelayUs = galvoDelay;
                target.GalvoDecelUs = galvoDecel;
                target.SlitHeightRows = slitRows;
                target.SensorBlankingRows = blankRows;
            }
        }

        // Fields the last commitScanControls_ could not read, one line each.
        private readonly List<string> _scanInputErrors = new List<string>();

        private IEnumerable<string> inputErrors_() => CameraPanel.InputErrors.Concat(_scanInputErrors);

        // Float style so "8.0" still reads as 8 in an integer box; a fraction, an overflow or
        // a negative number in an unsigned box is refused rather than wrapped to 0.
        private T readNumber_<T>(string label, TextBox box, T current)
            where T : INumber<T>, IMinMaxValue<T>
        {
            string text = (box.Text ?? string.Empty).Trim();
            if (T.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out T? value))
            {
                return value!; // TryParse returned true, so it holds a number
            }

            string range = current is double ? string.Empty : $" (allowed {T.MinValue} .. {T.MaxValue})";
            _scanInputErrors.Add($"{label}: '{text}' is not a valid number{range}.");
            return current;
        }

        private static string trajectorySummary_(LsoScanProfileModel scan)
        {
            if (scan.IsCalibratedTrajectory)
                return "FOV: " + System.IO.Path.GetFileName(scan.CalibrationFile);
            return scan.HasTrajectoryFile
                ? "CSV: " + System.IO.Path.GetFileName(scan.TrajectoryFile)
                : "generated (linear sweep)";
        }

        private void TrajectoryMode_Changed(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdating || !IsLoaded) return;
            Preset.Scan.TrajectoryMode = myCbTrajectoryMode.SelectedIndex == 1
                ? LsoTrajectoryMode.CalibratedFov : LsoTrajectoryMode.Linear;
            myLbTrajectory.Content = trajectorySummary_(Preset.Scan);
        }

        private void CalibrationBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog { Filter = "Calibration JSON|*.json" };
            if (dialog.ShowDialog(this) != true) return;
            if (!LsoGalvoSlitCalibration.TryLoad(dialog.FileName, out var calibration, out string error)
                || calibration == null || !calibration.TryFit(out error))
            {
                MsgBoxUtil.ShowWarning(error);
                return;
            }
            Preset.Scan.CalibrationFile = dialog.FileName;
            Preset.Scan.TrajectoryMode = LsoTrajectoryMode.CalibratedFov;
            if (Preset.Scan.SensorRows <= 0)
                Preset.Scan.SensorRows = (int)Preset.Camera.RoiMaxHeight;
            calibration.SweepEnds(Preset.Scan.SensorRows, out short start, out short end);
            Preset.Scan.GalvoStartPos = start;
            Preset.Scan.GalvoEndPos = end;
            updateScanControls_();
        }

        // The editor edits the mode's preset object directly and returns true when it changed
        // TrajectoryFile. Fields are committed first so the editor sees the geometry as typed.
        private void myBtTrajectoryEdit_Click(object sender, RoutedEventArgs e)
        {
            commitScanControls_();
            LsoModePreset preset = Preset;
            // Camera-side numbers for the slit simulation, same fallbacks the timing
            // calculator panel shows: Sensor Rows 0 = the camera's full height, Cam Delay and
            // Exposure come from the camera preset (the authority).
            int sensorRows = preset.Scan.SensorRows > 0 ? preset.Scan.SensorRows : (int)preset.Camera.RoiMaxHeight;
            double camDelayUs = preset.Camera.TriggerDelayUs > 0 ? preset.Camera.TriggerDelayUs : 0.0;
            double exposureUs = preset.Camera.ExposureTime > 0 ? preset.Camera.ExposureTime : preset.Scan.ExposureTimeUs;
            var editor = new LsoTrajectoryEditorWindow(preset.Scan, _mode.ToString(),
                LsoModePreset.ScanPatternId(_mode), preset.Name, _model.ModePresets.Directory,
                sensorRows, camDelayUs, exposureUs)
            {
                Owner = this,
            };
            if (editor.ShowDialog() == true)
            {
                myLbTrajectory.Content = trajectorySummary_(preset.Scan);
            }
        }

        private IEnumerable<ColorCameraPresetMode> targetModes_()
        {
            yield return _mode;
            if (SyncBothModes)
            {
                yield return _mode == ColorCameraPresetMode.Preview
                    ? ColorCameraPresetMode.Capture
                    : ColorCameraPresetMode.Preview;
            }
        }

        private void ScanControl_Changed(object sender, RoutedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            commitScanControls_();
        }

        private void myTbScanSettings_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter && !_isUpdating)
            {
                commitScanControls_();
                autoGalvoSweepIfEnabled_(sender);
                autoRecalcIfEnabled_(sender);
            }
        }

        private void myTbScanSettings_LostFocus(object sender, RoutedEventArgs e)
        {
            if (!_isUpdating)
            {
                commitScanControls_();
                autoGalvoSweepIfEnabled_(sender);
                autoRecalcIfEnabled_(sender);
            }
        }

        // DevKit "Auto-Calculate Sync": only the three inputs the calculation actually reads
        // from the Galvo grid retrigger it. Editing Pattern Size or a Light index by hand does
        // NOT, or the recalculation would immediately overwrite what was just typed.
        private void autoRecalcIfEnabled_(object sender)
        {
            if (myCheckBoxAutoSync.IsChecked != true || _recalculating)
            {
                return;
            }

            if (ReferenceEquals(sender, myTbCameraTimeStep)
                || ReferenceEquals(sender, myTbGalvoStartPos)
                || ReferenceEquals(sender, myTbGalvoEndPos))
            {
                recalcTiming_(quiet: true);
            }
        }

        // The full-frame inputs: only the two range boxes and the subframe count feed the
        // sweep arithmetic, so editing Start/End by hand (Auto off) never retriggers it.
        private void autoGalvoSweepIfEnabled_(object sender)
        {
            if (!AutoGalvoSweep || _recalculating)
            {
                return;
            }

            if (ReferenceEquals(sender, myTbGalvoTotalStart)
                || ReferenceEquals(sender, myTbGalvoTotalEnd)
                || ReferenceEquals(sender, myTbSubFrame)
                || ReferenceEquals(sender, myTbGalvoEndPos))
            {
                recalcGalvoSweep_(quiet: true);
            }
        }

        private void DeriveAcquisition_Changed(object sender, RoutedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            _owner.DeriveAcquisitionFromScan = myCheckBoxDeriveAcquisition.IsChecked == true;
        }

        #region Timing calculator

        private static double ToDouble_(string text)
        {
            return double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double v) ? v : 0.0;
        }

        private void myTbLineRate_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                lineRateToLineTime_();
                commitTiming_();
            }
        }

        private void myTbLineRate_LostFocus(object sender, RoutedEventArgs e)
        {
            lineRateToLineTime_();
            commitTiming_();
        }

        private void myTbTiming_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                commitTiming_();
            }
        }

        private void myTbTiming_LostFocus(object sender, RoutedEventArgs e)
        {
            commitTiming_();
        }

        // ERS: a slit covering N sensor rows is exposed for N line times. Read-only readout -
        // the number only reaches the camera when Apply to Exposure is pressed.
        private void updateRecExposure_()
        {
            int rows = ToInt(myTbSlitRows.Text);
            double lineTimeUs = ToDouble_(myTbLineTime.Text);
            if (rows <= 0 || lineTimeUs <= 0)
            {
                myLbRecExposure.Content = "Rec Exp: -";
                return;
            }

            double recUs = rows * lineTimeUs;
            myLbRecExposure.Content = string.Format(CultureInfo.InvariantCulture,
                "Rec Exp: {0:F1} us ({1:F2} ms)", recUs, recUs / 1000.0);
        }

        private void myBtApplyRecExposure_Click(object sender, RoutedEventArgs e)
        {
            int rows = ToInt(myTbSlitRows.Text);
            double lineTimeUs = ToDouble_(myTbLineTime.Text);
            if (rows <= 0 || lineTimeUs <= 0)
            {
                MsgBoxUtil.ShowWarning("Measure or enter a slit height in rows, and a line rate, "
                    + "before applying the recommendation.");
                return;
            }

            // The camera panel owns the exposure, so its pending edits are committed first -
            // otherwise reloading it below would discard whatever else the operator typed.
            if (!CameraPanel.CommitToPreset())
            {
                MsgBoxUtil.ShowWarning("The camera panel has invalid input:" + Environment.NewLine
                    + string.Join(Environment.NewLine, CameraPanel.InputErrors));
                return;
            }

            uint recUs = (uint)Math.Max(1.0, Math.Round(rows * lineTimeUs));
            Preset.Camera.ExposureTime = recUs;
            CameraPanel.ReloadFromPreset();
            updateScanControls_();
        }

        // The calculator's inputs are preset values now, so they are committed like any other
        // field. commitScanControls_ writes them (and honours "both modes").
        private void commitTiming_()
        {
            if (!_isUpdating && !_recalculating)
            {
                commitScanControls_();
                updateRecExposure_();
                updateBlankTimeLabel_();
            }
        }

        // AcquisitionLineRate straight off the camera, through the generic node-value export.
        // False means no camera or no such node - the typed value is then left alone rather
        // than being replaced by a zero.
        private void myBtReadLineRate_Click(object sender, RoutedEventArgs e)
        {
            if (!LsoCamera.GetNodeValue("AcquisitionLineRate", out double rate) || rate <= 0)
            {
                MsgBoxUtil.ShowWarning("AcquisitionLineRate could not be read from the camera "
                    + "(is it connected, and does this model implement the node?). "
                    + "The typed Line Rate was kept.");
                return;
            }

            myTbLineRate.Text = rate.ToString("F1", CultureInfo.InvariantCulture);
            lineRateToLineTime_();
            if (myCheckBoxAutoSync.IsChecked == true)
            {
                recalcTiming_(quiet: true);
            }
        }

        private void lineRateToLineTime_()
        {
            double rate = ToDouble_(myTbLineRate.Text);
            if (rate > 0)
            {
                myTbLineTime.Text = (1_000_000.0 / rate).ToString("F2", CultureInfo.InvariantCulture);
            }

            // The ERS recommendation is a function of the line time, so it follows it.
            updateRecExposure_();
        }

        // The rule itself lives in LsoScanTimingCalculator (shared with the scan-test screen
        // and checkable without a device); this only moves values between it and the fields.
        private void myBtRecalcTiming_Click(object sender, RoutedEventArgs e)
        {
            recalcTiming_(quiet: false);
        }

        private void updateBlankTimeLabel_()
        {
            double lineTimeUs = ToDouble_(myTbLineTime.Text);
            int rows = ToInt(myTbBlankRows.Text);
            myLbBlankTime.Content = rows > 0 && lineTimeUs > 0
                ? string.Format(CultureInfo.InvariantCulture, "Offset: {0:F0} us", rows * lineTimeUs)
                : "Offset: -";
        }

        // quiet: the auto-recalculation must not open a modal while the operator is typing -
        // an incomplete field is normal mid-edit, so it just does nothing until the inputs
        // make sense. The button press says so explicitly and is allowed to complain.
        private void recalcTiming_(bool quiet)
        {
            _recalculating = true;
            try
            {
                // The calculator reads the PRESET, so whatever is typed has to land there
                // first. commitScanControls_ already honours "both modes", which is why the
                // calculation below then runs per target mode: each one carries its own
                // sequence, camera delay and exposure.
                commitScanControls_();

                LsoScanTimingCalculator.Result shown = default;
                bool haveShown = false;

                foreach (ColorCameraPresetMode mode in targetModes_())
                {
                    LsoModePreset preset = _model.ModePresets.Active(mode);
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
                    if (mode == _mode)
                    {
                        shown = result;
                        haveShown = true;
                    }
                }

                if (haveShown)
                {
                    double timeStep = Preset.Scan.TimeStepUs;
                    string capped = shown.Capped
                        ? $"  [capped at {LsoScanTimingCalculator.MaxPatternSize} samples]"
                        : string.Empty;
                    myLbPreTrig.Content = $"PreTrig: idx {shown.PreTrigIndex} "
                        + $"({shown.PreTrigIndex * timeStep:F1} us lead){capped}";
                }

                updateScanControls_();
            }
            finally
            {
                _recalculating = false;
            }
        }

        #endregion Timing calculator

        #endregion Scan (galvo)

        #region Light

        // The lamps a fundus mode can image with. Cornea IR and the external fixation LED are
        // left out on purpose: the native request would accept them, but they do not light the
        // retinal path, so offering them here only invites a black capture.
        private static readonly LightType[] SelectableLights =
        {
            LightType.LsoWhite,
            LightType.RetinaIr,
            LightType.LsoBlue,
            LightType.LsoGreen,
        };

        private static string lightDisplayName_(LightType light)
        {
            return light switch
            {
                LightType.LsoWhite => "LSO White",
                LightType.RetinaIr => "Retina IR",
                LightType.LsoBlue => "LSO Blue",
                LightType.LsoGreen => "LSO Green",
                _ => light.ToString(),
            };
        }

        private void updateLightControls_()
        {
            _isUpdating = true;

            if (myCbLightType.Items.Count == 0)
            {
                foreach (LightType light in SelectableLights)
                {
                    myCbLightType.Items.Add(lightDisplayName_(light));
                }
            }

            int index = Array.IndexOf(SelectableLights, Preset.Light.Led);
            myCbLightType.SelectedIndex = index >= 0 ? index : 0;

            mySliderLight.Value = Preset.Light.Intensity;
            myTbLight.Text = Preset.Light.Intensity.ToString();
            myCheckLightContinuous.IsChecked = Preset.Light.Mode == (int)LightMode.Continuous;
            _isUpdating = false;
        }

        private void myCbLightType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdating || myCbLightType.SelectedIndex < 0)
            {
                return;
            }

            Preset.Light.Type = (int)SelectableLights[myCbLightType.SelectedIndex];
        }

        private void mySliderLight_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isUpdating)
            {
                return;
            }

            Preset.Light.Intensity = (ushort)mySliderLight.Value;
            myTbLight.Text = Preset.Light.Intensity.ToString();
        }

        private void myTbLight_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                commitLightText_();
            }
        }

        // Clicking Apply right after typing the intensity must not drop it.
        private void myTbLight_LostFocus(object sender, RoutedEventArgs e)
        {
            if (!_isUpdating)
            {
                commitLightText_();
            }
        }

        private void commitLightText_()
        {
            int value = ToInt(myTbLight.Text);
            if (value >= mySliderLight.Minimum && value <= mySliderLight.Maximum)
            {
                mySliderLight.Value = value;
            }
            else
            {
                updateLightControls_();
            }
        }

        private void LightMode_Changed(object sender, RoutedEventArgs e)
        {
            if (_isUpdating)
            {
                return;
            }

            Preset.Light.Mode = myCheckLightContinuous.IsChecked == true
                ? (int)LightMode.Continuous
                : (int)LightMode.Trigger;
        }

        #endregion Light

        #region Actions

        // False when the camera editor or a scan field could not be read; the caller then
        // stops before any hardware call and shows inputErrors_().
        private bool commitControlsToPreset_()
        {
            bool cameraOk = CameraPanel.CommitToPreset();
            commitScanControls_();
            return cameraOk && _scanInputErrors.Count == 0;
        }

        private void myBtDefault_Click(object sender, RoutedEventArgs e)
        {
            CameraPanel.ApplyDefaults();

            if (_mode == ColorCameraPresetMode.Preview)
            {
                LsoScanPreviewPreset.ApplyTo(Preset.Scan, (int)Preset.Camera.ExposureTime);
            }
            else
            {
                LsoModePreset.ApplyCaptureScanDefaults(Preset.Scan);
            }

            updateScanControls_();
            updateLightControls_();
        }

        // DevKit "Set Default": the current fields become this mode's built-in preset
        // ('Preview' / 'Capture') and that preset is selected. Same read-guard as Save As.
        private void myBtSaveAsDefault_Click(object sender, RoutedEventArgs e)
        {
            string name = _mode == ColorCameraPresetMode.Preview
                ? LsoModePresetStore.DefaultPreviewPresetName
                : LsoModePresetStore.DefaultCapturePresetName;

            if (!MsgBoxUtil.ShowQuestion($"Overwrite the built-in '{name}' preset with the current fields?"))
            {
                return;
            }

            if (!commitControlsToPreset_())
            {
                MsgBoxUtil.ShowWarning("Nothing was saved - these fields could not be read:"
                    + Environment.NewLine + Environment.NewLine
                    + string.Join(Environment.NewLine, inputErrors_()));
                return;
            }

            _model.ModePresets.SaveAs(_mode, name);
            showMode_(_mode);
            reportIssues_();
        }

        // Reads the scanner slot back from the board so the shown values can be compared
        // against the preset instead of only ever being written out.
        private void myBtLoadBoard_Click(object sender, RoutedEventArgs e)
        {
            int patternId = LsoModePreset.ScanPatternId(_mode);
            LsoScannerControlParam param = default;
            if (!LsoScanner.ObtainLsoScannerControlParam(patternId, ref param))
            {
                MessageBox.Show($"Load scanner parameters from board failed! (patternId={patternId})");
                return;
            }

            Preset.Scan.ApplyControlParam(param);
            updateScanControls_();
        }

        // Write camera + galvo + light to the board, and save the preset file only once all
        // three took. This screen exists to tune the sensor, so a running preview/live
        // session is stopped, reprogrammed and restarted rather than refused.
        //
        // Order matters: the file is written AFTER the hardware answered, so a failed Apply
        // leaves the previously saved preset on disk exactly as it was. Saving first (what
        // this did before) meant a rejected value was already the stored one, and the next
        // start would load it back.
        private void myBtApply_Click(object sender, RoutedEventArgs e)
        {
            if (LsoCamera.IsCapturing())
            {
                MsgBoxUtil.ShowWarning("A capture is in progress - wait for it to finish before applying.");
                return;
            }

            // Invalid input never reaches the hardware. The commit names every field it
            // could not read and refuses; nothing has been written at this point.
            if (!commitControlsToPreset_())
            {
                MsgBoxUtil.ShowWarning("Nothing was applied - these fields could not be read:"
                    + Environment.NewLine + Environment.NewLine
                    + string.Join(Environment.NewLine, inputErrors_()));
                return;
            }

            // Writes camera + galvo + light and leaves the imaging session STOPPED; the
            // restart is a separate step below.
            LsoScanTestWindow.PresetApplyReport report = _owner.ApplyPresetToBoard(_mode);
            if (SyncBothModes)
            {
                // The other preset changed in memory only: Apply writes one slot, and the
                // other mode's camera/light/motor would need its own session rules.
                string other = _mode == ColorCameraPresetMode.Preview ? "Capture" : "Preview";
                report.Warnings.Add($"Both-modes edit: the {other} preset's scan values were updated in memory "
                    + $"only. Switch to {other} and press Apply to write and save them.");
            }
            // Saved once the camera and the galvo took the values. A lamp or motor that
            // refused is listed below, but no longer keeps the tuned camera/galvo off disk -
            // before, any one of the four failing left the board new and the file old, and
            // the next start quietly reverted the tuning.
            if (report.CanSave)
            {
                _model.ModePresets.Save(_mode);

                // Which entries and ranges the camera offers depends on the configuration
                // just written, so the vocabulary is re-read once - here, with the camera
                // stopped - instead of on every reload.
                ColorCameraSettingWindow.InvalidateCapabilityCache();
            }

            // Refresh BEFORE the session restarts. ReloadFromPreset re-queries ~31 camera
            // nodes; against a streaming camera that walks the same node map the acquisition
            // thread is reading per frame, and the UI thread stops answering.
            CameraPanel.ReloadFromPreset();
            updateScanControls_();

            _owner.RestartSessionAfterApply(_mode, report);

            if (!report.Success || report.Failures.Count > 0 || report.Warnings.Count > 0)
            {
                // A preset that the instrument cannot currently START is still a valid
                // tuning result: the values went to the camera and the file is saved. Say
                // that plainly rather than calling it a warning about an apply that worked.
                string headline;
                if (!report.CanSave)
                {
                    headline = "Apply to the camera / galvo failed - the preset file was NOT changed.";
                }
                else if (report.StartBlocked)
                {
                    headline = "Applied to the camera and saved. Preview/Capture CANNOT START with this "
                        + "preset until the field below is changed:";
                }
                else if (!report.Success)
                {
                    headline = "Camera and galvo applied and saved. The light / motor part did not take:";
                }
                else
                {
                    headline = "Applied to the board and saved. Notes:";
                }

                MessageBox.Show(headline + Environment.NewLine + Environment.NewLine + report.Describe());
            }

            reportIssues_();
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        #endregion Actions
    }
}
