using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.controls;
using WsoToolkit.utils;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    // Slit alignment (DevKit LsoPatternForm group E), the fourth part of this window's partial
    // class so the two files that already carry it do not grow further.
    //
    // The scan sweeps the galvo, so during a scan there is no stationary slit to look at. This
    // mode parks the galvo at one position, switches the illumination to continuous and runs the
    // camera free-running, which is what makes the slit visible - and what the ERS slit-height
    // measurement will need.
    public partial class LsoScanTestWindow : Window
    {
        private volatile bool _alignActive;
        private bool _alignToggling;
        private bool _slitMeasureHooked;

        // Latest coalesced live frame, kept only while aligning. Outside that mode the
        // galvo is sweeping, so a frame is not a stationary slit the Auto button can measure.
        private volatile AlignFrame? _alignFrame;
        private sealed record AlignFrame(byte[] Data, int Width, int Height, int BytesPerPixel, int PixelFormat);
        private int _galvanoYPosition;
        private int _calibrationPositionVersion;
        private bool _calibrationPositionValid;
        private bool _galvanoSliderDragging;
        private bool _galvanoSliderSyncing;

        // Suppresses the mode checkbox's own handler while a shared-state notification is
        // mirrored into it; a mirrored value must never drive the board a second time.
        private bool _isSyncingLightState;

        // The light this mode turned on, remembered so the exit path can turn off exactly that
        // one even if the target-mode combo changed meanwhile.
        private LightType _alignLight = LightType.LsoWhite;

        private ColorCameraPresetMode AlignTargetMode =>
            myCbAlignTargetMode != null && myCbAlignTargetMode.SelectedIndex == 1
                ? ColorCameraPresetMode.Capture
                : ColorCameraPresetMode.Preview;

        private LsoModePreset AlignPreset => _scanTestModel.ModePresets.Active(AlignTargetMode);

        #region Mode toggle

        private void myCheckAlignMode_CheckedNUnchecked(object sender, RoutedEventArgs e)
        {
            if (_alignToggling)
            {
                return;
            }

            _alignToggling = true;
            try
            {
                if (myCheckAlignMode.IsChecked == true)
                {
                    if (!enterAlignMode_())
                    {
                        myCheckAlignMode.IsChecked = false;
                    }
                }
                else
                {
                    exitAlignMode_();
                }
            }
            finally
            {
                _alignToggling = false;
                updateAlignControls_();
            }
        }

        private bool enterAlignMode_()
        {
            stopLiveCapture_();
            if (LsoCamera.IsCapturing())
            {
                MsgBoxUtil.ShowWarning("A capture is in progress. Wait for it to finish before aligning.");
                return false;
            }

            // 1. Everything that drives the camera or the galvo has to stop first.
            if (IsRetinaPreviewing_())
            {
                StopRetinaPreview_();
            }
            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            LsoModePreset preset = AlignPreset;

            // 2. Park the galvo where the sweep starts - that is where the slit will be.
            int pos = preset.Scan.GalvoStartPos;
            setGalvanoYPosition_(pos);
            if (!LsoScanner.MoveLsoScannerYposition(pos))
            {
                _calibrationPositionValid = false;
                MsgBoxUtil.ShowWarning("The galvo did not move to the alignment position.");
                return false;
            }
            _calibrationPositionVersion++;
            _calibrationPositionValid = true;

            // 3. Continuous illumination. Intensity stays window-local: aligning brightness is
            //    not what the preset should remember.
            _alignLight = preset.Light.Led;
            ushort intensity = alignIntensity_(preset);
            myTbAlignIntensity.Text = intensity.ToString();

            DeviceLights.SetLightIntensity(_alignLight, intensity);
            LightControlState.PublishIntensity(_alignLight, intensity);
            bool continuousAccepted = DeviceLights.SetLightMode(_alignLight, LightMode.Continuous);
            if (!continuousAccepted)
            {
                MsgBoxUtil.ShowWarning($"{_alignLight} refused continuous mode; the slit may stay dark.");
            }

            // "Continuous Slit" decides whether the lamp is actually lit; the mode above is
            // set either way, so ticking the box mid-alignment only has to switch it on.
            // Publish after that decision so mirroring the mode into the box cannot change it.
            if (ContinuousSlit)
            {
                DeviceLights.TurnOnLight(_alignLight);
            }
            if (continuousAccepted)
            {
                LightControlState.PublishContinuous(_alignLight, true);
            }
            refreshLightControls_();

            // 4. Live WITHOUT scanner grabbing: startLiveMode() puts the camera in free-run
            //    (ColorCamera::setupCameraTriggerOnOffOnly(false)), so frames do not depend on
            //    the scanner - and not grabbing is exactly what keeps the galvo parked.
            _alignActive = true;
            StartColorCameraLive(sweepGalvo: false);

            refreshAlignCamGain_();
            hookSlitMeasure_();
            showSlitRows_(preset.Scan.SlitHeightRows);
            return true;
        }

        // The array is already the callback's private copy; this only remembers it, and
        // only while aligning - a frame from a sweeping galvo is not a stationary slit.
        private void rememberAlignFrame_(byte[] data, int width, int height, int bytesPerPixel, int pixelFormat)
        {
            AlignFrame? frame = _alignActive ? new AlignFrame(data, width, height, bytesPerPixel, pixelFormat) : null;
            _alignFrame = frame;
            if (frame != null) signalAlignFrame_(frame);
        }

        // Never throws and never gives up halfway: a half-exited alignment leaves the
        // illumination on, which is the one outcome that must not happen.
        private void exitAlignMode_()
        {
            if (!_alignActive)
            {
                return;
            }

            _alignActive = false;
            _alignFrame = null;
            _calibrationPositionValid = false;
            cancelCalibrationFrameWait_();
            _calibrationWindow?.CancelMeasurement();

            try
            {
                if (IsColorCameraLive())
                {
                    PauseColorCameraLive();
                }
            }
            catch (Exception ex)
            {
                System.Console.WriteLine($"[SlitAlign] live stop failed: {ex.Message}");
            }

            try
            {
                DeviceLights.TurnOffLight(_alignLight);
                LightMode restored = (LightMode)AlignPreset.Light.Mode;
                if (DeviceLights.SetLightMode(_alignLight, restored))
                {
                    LightControlState.PublishContinuous(_alignLight, restored == LightMode.Continuous);
                }
            }
            catch (Exception ex)
            {
                System.Console.WriteLine($"[SlitAlign] light restore failed: {ex.Message}");
            }
            refreshLightControls_();

            try
            {
                myColorPreview.Image.SlitMeasure.ClearMarkers();
            }
            catch (Exception ex)
            {
                System.Console.WriteLine($"[SlitAlign] overlay clear failed: {ex.Message}");
            }

            try
            {
                moveAlignGalvoTo_(0);

                // The scan profile the board holds was not touched, but the live toggle restores
                // it on its own exit too - do the same so the next scan starts from the preset.
                applyScannerControlParam_(LsoScanCapturePreset.PatternId);
            }
            catch (Exception ex)
            {
                System.Console.WriteLine($"[SlitAlign] galvo restore failed: {ex.Message}");
            }
        }

        // Called from Window_Closed: the window may go away with the light still on.
        private void releaseSlitAlign_()
        {
            cancelCalibrationFrameWait_();
            _calibrationWindow?.CancelMeasurement();
            if (_alignActive)
            {
                exitAlignMode_();
            }
        }

        private ushort alignIntensity_(LsoModePreset preset)
        {
            ushort typed = (ushort)Math.Clamp(ToInt(myTbAlignIntensity.Text), 0, ushort.MaxValue);
            if (typed > 0)
            {
                return typed;
            }

            return preset.Light.Intensity > 0 ? preset.Light.Intensity : (ushort)50;
        }

        private void updateAlignControls_()
        {
            myPanelAlignPos.IsEnabled = _alignActive;
            myPanelAlignJog.IsEnabled = _alignActive;
            myPanelAlignGoto.IsEnabled = _alignActive;
            myPanelAlignLight.IsEnabled = _alignActive;

            // Scanning and aligning both own the galvo and the camera; only one at a time.
            myBtStartScan.IsEnabled = !_alignActive;
            myBtCapture.IsEnabled = !_alignActive;
            myCbAlignTargetMode.IsEnabled = !_alignActive;

            setGalvanoYPosition_(_galvanoYPosition);

            // The slit height belongs to the selected mode preset, so the readout follows the
            // target-mode combo - unless the operator is typing in it right now.
            if (!myTbSlitRows.IsKeyboardFocusWithin)
            {
                showSlitRows_(AlignPreset.Scan.SlitHeightRows);
            }

            myLbAlignLight.Content = _alignActive
                ? $"Light : {_alignLight}"
                : $"Light : {AlignPreset.Light.Led}";

            // The target-mode combo can select a different light; show that light's shared
            // intensity / continuous state instead of the previous channel's numbers.
            reflectLightState_();

            showAlignRange_();

            // The Sync row below follows the same target-mode combo, and the alignment state
            // decides whether its Apply may touch the board.
            reflectScanTiming_();
        }

        // Go Start / Go End move to these, and Set as ... writes them. Keep the editable
        // fields and the compact sweep readout together: Set as Start/End becomes observable
        // immediately, while a field the operator is actively typing is never overwritten.
        private void showAlignRange_()
        {
            LsoScanProfileModel scan = AlignPreset.Scan;
            if (!myTbGalvoStartPos.IsKeyboardFocusWithin)
            {
                myTbGalvoStartPos.Text = scan.GalvoStartPos.ToString();
            }
            if (!myTbGalvoEndPos.IsKeyboardFocusWithin)
            {
                myTbGalvoEndPos.Text = scan.GalvoEndPos.ToString();
            }

            myLbAlignRange.Content =
                $"Sweep {scan.GalvoStartPos} -> {scan.GalvoEndPos}  (d {scan.GalvoEndPos - scan.GalvoStartPos})";
        }

        private void myCbAlignTargetMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!IsLoaded || _alignActive)
            {
                return;
            }

            updateAlignControls_();
        }

        private bool ContinuousSlit => myCheckContinuousSlit?.IsChecked == true;

        // Switches the alignment lamp without leaving alignment mode. Outside alignment it
        // does nothing: the lamp then belongs to whatever the scan or the Light panel is doing.
        private void myCheckContinuousSlit_CheckedNUnchecked(object sender, RoutedEventArgs e)
        {
            if (!IsLoaded || !_alignActive || _isSyncingLightState)
            {
                return;
            }

            if (ContinuousSlit)
            {
                if (DeviceLights.SetLightMode(_alignLight, LightMode.Continuous))
                {
                    LightControlState.PublishContinuous(_alignLight, true);
                }
                DeviceLights.TurnOnLight(_alignLight);
            }
            else
            {
                DeviceLights.TurnOffLight(_alignLight);

                // Restore strobe mode as the DevKit does; leaving the hardware continuous
                // while the shared state says otherwise would make the two controls diverge.
                if (DeviceLights.SetLightMode(_alignLight, LightMode.Trigger))
                {
                    LightControlState.PublishContinuous(_alignLight, false);
                }
            }

            refreshLightControls_();
        }

        #endregion Mode toggle

        #region Galvo positioning

        // The galvo takes a short command position; the DevKit lets the whole range be typed or
        // dragged and nothing in the native surface reports narrower hardware limits.
        private void setGalvanoYPosition_(int pos)
        {
            int clamped = Math.Clamp(pos, short.MinValue, short.MaxValue);
            if (_galvanoYPosition != clamped) _calibrationPositionVersion++;
            _galvanoYPosition = clamped;

            _galvanoSliderSyncing = true;
            try
            {
                myTbGalvanoYPos.Text = _galvanoYPosition.ToString();
                mySliderGalvanoYPos.Value = _galvanoYPosition;
            }
            finally
            {
                _galvanoSliderSyncing = false;
            }
        }

        private void moveAlignGalvoTo_(int pos)
        {
            int clamped = Math.Clamp(pos, short.MinValue, short.MaxValue);
            setGalvanoYPosition_(clamped);

            _calibrationPositionVersion++;
            _calibrationPositionValid = LsoScanner.MoveLsoScannerYposition(clamped);
            _calibrationWindow?.RefreshTargetState();
            if (!_calibrationPositionValid)
            {
                MsgBoxUtil.ShowWarning("Move galvano position Y failed!");
            }
        }

        private void myTbGalvanoYPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                moveAlignGalvoTo_(ToInt(myTbGalvanoYPos.Text));
            }
        }

        private void mySliderGalvanoYPos_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _galvanoSliderDragging = true;
            _calibrationWindow?.RefreshTargetState();
        }

        private void mySliderGalvanoYPos_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _galvanoSliderDragging = false;
            moveAlignGalvoTo_((int)mySliderGalvanoYPos.Value);
        }

        private void mySliderGalvanoYPos_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_galvanoSliderSyncing)
            {
                return;
            }

            int pos = (int)e.NewValue;
            if (_galvanoSliderDragging)
            {
                // Dragging only previews the number; the move happens on release.
                if (_galvanoYPosition != pos) _calibrationPositionVersion++;
                _galvanoYPosition = pos;
                myTbGalvanoYPos.Text = pos.ToString();
                return;
            }

            moveAlignGalvoTo_(pos);
        }

        private void myBtAlignJog_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && int.TryParse(button.Tag?.ToString(), out int delta))
            {
                moveAlignGalvoTo_(_galvanoYPosition + delta);
            }
        }

        private void myBtAlignGoStart_Click(object sender, RoutedEventArgs e)
        {
            moveAlignGalvoTo_(AlignPreset.Scan.GalvoStartPos);
        }

        private void myBtAlignGoCenter_Click(object sender, RoutedEventArgs e)
        {
            moveAlignGalvoTo_(0);
        }

        private void myBtAlignGoEnd_Click(object sender, RoutedEventArgs e)
        {
            moveAlignGalvoTo_(AlignPreset.Scan.GalvoEndPos);
        }

        // Set as Start/End edits the preset in memory only - it is saved by the mode setting
        // window's Save, like every other preset field.
        private void myBtAlignSetAsStart_Click(object sender, RoutedEventArgs e)
        {
            AlignPreset.Scan.GalvoStartPos = (short)Math.Clamp(
                _galvanoYPosition,
                short.MinValue,
                short.MaxValue);
            showAlignRange_();
            autoRecalcScanTiming_();
        }

        private void myBtAlignSetAsEnd_Click(object sender, RoutedEventArgs e)
        {
            AlignPreset.Scan.GalvoEndPos = (short)Math.Clamp(
                _galvanoYPosition,
                short.MinValue,
                short.MaxValue);
            showAlignRange_();
            autoRecalcScanTiming_();
        }

        #endregion Galvo positioning

        #region Alignment illumination

        private void applyAlignIntensity_()
        {
            if (!_alignActive)
            {
                return;
            }

            ushort intensity = (ushort)Math.Clamp(ToInt(myTbAlignIntensity.Text), 0, 255);
            myTbAlignIntensity.Text = intensity.ToString();

            if (!DeviceLights.SetLightIntensity(_alignLight, intensity))
            {
                MsgBoxUtil.ShowWarning($"{_alignLight} did not accept intensity {intensity}.");
            }

            // Publish even when the board refuses: both editors keep showing the value the
            // operator committed, and the warning above is what reports the failure.
            LightControlState.PublishIntensity(_alignLight, intensity);
        }

        // Mirrors the shared LED state into the alignment block. Only the editors are touched:
        // writing the board from a notification would echo back to the publishing control.
        private void onLightStateChanged_(object? sender, LightControlState.ChangedEventArgs e)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.BeginInvoke(new Action(() => onLightStateChanged_(sender, e)));
                return;
            }

            LightType type = _alignActive ? _alignLight : AlignPreset.Light.Led;
            if (e.Type != type)
            {
                return;
            }

            _isSyncingLightState = true;
            try
            {
                if (e.IntensityChanged && !myTbAlignIntensity.IsKeyboardFocusWithin)
                {
                    myTbAlignIntensity.Text = e.Intensity.ToString();
                }

                if (e.ContinuousChanged && myCheckContinuousSlit.IsChecked != e.Continuous)
                {
                    myCheckContinuousSlit.IsChecked = e.Continuous;
                }
            }
            finally
            {
                _isSyncingLightState = false;
            }
        }

        // Seeds the alignment block when the selected light changes (or when the panel has
        // just read the device). Unknown channels are left alone.
        private void reflectLightState_()
        {
            LightType type = _alignActive ? _alignLight : AlignPreset.Light.Led;
            if (!LightControlState.TryGet(type, out int intensity, out bool continuous))
            {
                return;
            }

            _isSyncingLightState = true;
            try
            {
                if (!myTbAlignIntensity.IsKeyboardFocusWithin)
                {
                    myTbAlignIntensity.Text = intensity.ToString();
                }

                if (myCheckContinuousSlit.IsChecked != continuous)
                {
                    myCheckContinuousSlit.IsChecked = continuous;
                }
            }
            finally
            {
                _isSyncingLightState = false;
            }
        }

        private void myBtAlignIntensityApply_Click(object sender, RoutedEventArgs e)
        {
            applyAlignIntensity_();
        }

        private void myTbAlignIntensity_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                applyAlignIntensity_();
            }
        }

        // Camera gain is a viewing aid here, like the intensity: the mode preset keeps its own
        // gain, and the next Preview/Capture one-call restores it.
        private void refreshAlignCamGain_()
        {
            if (LsoCamera.GetNodeValue("Gain", out double actual) && actual >= 0.0)
            {
                myTbAlignCamGain.Text = actual.ToString("F1");
            }
        }

        private void applyAlignCamGain_()
        {
            if (!_alignActive)
            {
                return;
            }

            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam param);
            param.gain = (float)ToDouble(myTbAlignCamGain.Text);

            if (!LsoCamera.SetCameraParameters(ref param))
            {
                MsgBoxUtil.ShowWarning($"The camera did not accept gain {param.gain:F1} dB.");
            }

            // The camera quantises gain, so show what it actually holds.
            refreshAlignCamGain_();
        }

        private void myBtAlignCamGainApply_Click(object sender, RoutedEventArgs e)
        {
            applyAlignCamGain_();
        }

        private void myTbAlignCamGain_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                applyAlignCamGain_();
            }
        }

        #endregion Alignment illumination

        #region Slit height measurement

        private void hookSlitMeasure_()
        {
            SlitMeasureOverlay overlay = myColorPreview.Image.SlitMeasure;

            if (!_slitMeasureHooked)
            {
                overlay.Measured += onSlitMeasured_;
                overlay.CenterMeasured += onSlitCenterMeasured_;
                overlay.MeasuringChanged += onSlitMeasuringChanged_;
                _slitMeasureHooked = true;
            }

            // ERS counts sensor rows, and a binned image row covers several of them.
            overlay.VerticalBinning = (int)Math.Max(1u, AlignPreset.Camera.BinningVertical);
        }

        private void onSlitMeasuringChanged_(bool measuring)
        {
            myBtMeasureSlit.Content = measuring ? "Cancel" : "Measure";
        }

        private void onSlitMeasured_(int sensorRows)
        {
            if (sensorRows <= 0)
            {
                return;
            }

            // The height belongs to the preset the alignment is aimed at; the mode setting
            // window shows the same number and turns it into the recommended exposure.
            showSlitRows_(sensorRows);
        }

        private void onSlitCenterMeasured_(double centre, int rows)
        {
            myLbSlitBand.Content = $"Center : {centre:F1}   Height : {rows}  (2-click, sensor rows)";
        }

        private void showSlitRows_(int sensorRows)
        {
            myTbSlitRows.Text = sensorRows > 0 ? sensorRows.ToString() : string.Empty;
            updateRecExposure_();
        }

        // ERS: a slit covering N sensor rows is exposed for N line times.
        private void updateRecExposure_()
        {
            int rows = ToInt(myTbSlitRows.Text);
            double lineTimeUs = AlignPreset.Scan.LineTimeUs;

            myLbErsFormula.Content = lineTimeUs > 0
                ? $"Rec Exp (us) = Slit Rows * Line Time ({lineTimeUs:F2} us)"
                : "Rec Exp (us) = Slit Rows * Line Time";

            if (rows <= 0 || lineTimeUs <= 0)
            {
                myLbSlitResult.Content = "Rec Exp : -";
                return;
            }

            double recUs = rows * lineTimeUs;
            myLbSlitResult.Content = $"Rec Exp : {recUs:F1} us ({recUs / 1000.0:F2} ms)";
        }

        private void myTbSlitRows_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!IsInitialized)
            {
                return;
            }

            int rows = ToInt(myTbSlitRows.Text);
            if (rows > 0)
            {
                AlignPreset.Scan.SlitHeightRows = rows;
            }
            updateRecExposure_();
        }

        // The recommendation only reaches the camera here; the preset's exposure is the one
        // the Preview/Capture one-call applies, so that is what gets written.
        private void myBtApplyRecExposure_Click(object sender, RoutedEventArgs e)
        {
            int rows = ToInt(myTbSlitRows.Text);
            double lineTimeUs = AlignPreset.Scan.LineTimeUs;
            if (rows <= 0 || lineTimeUs <= 0)
            {
                MsgBoxUtil.ShowWarning("Measure or enter a slit height in rows, and set a line rate "
                    + "in Settings, before applying the recommendation.");
                return;
            }

            uint recUs = (uint)Math.Max(1.0, Math.Round(rows * lineTimeUs));
            AlignPreset.Scan.SlitHeightRows = rows;
            AlignPreset.Camera.ExposureTime = recUs;

            MsgBoxUtil.ShowInfo($"Recommended exposure {recUs} us applied to the {AlignTargetMode} "
                + "preset. Save the preset in Settings to keep it.");
        }

        private void myBtMeasureSlit_Click(object sender, RoutedEventArgs e)
        {
            SlitMeasureOverlay overlay = myColorPreview.Image.SlitMeasure;

            if (overlay.IsMeasuring)
            {
                overlay.CancelMeasure();
                return;
            }

            if (!offerAlignMode_())
            {
                return;
            }

            if (!IsColorCameraLive())
            {
                MsgBoxUtil.ShowWarning("The live image is not running, so there is nothing to measure on.");
                return;
            }

            hookSlitMeasure_();
            overlay.StartMeasure();
        }

        // Without alignment mode the galvo is sweeping and there is no stationary slit to
        // measure, so offer to turn it on instead of just refusing (as the DevKit does).
        private bool offerAlignMode_()
        {
            if (_alignActive)
            {
                return true;
            }

            if (!MsgBoxUtil.ShowQuestion("Enable Alignment Mode (free-running camera + continuous "
                    + "illumination) to see the stationary slit?"))
            {
                return false;
            }

            myCheckAlignMode.IsChecked = true;
            return _alignActive; // entering the mode failed and already explained why
        }

        private void myBtAutoSlit_Click(object sender, RoutedEventArgs e)
        {
            if (!offerAlignMode_())
            {
                return;
            }

            AlignFrame? frame = _alignFrame;
            if (!IsColorCameraLive() || frame == null)
            {
                MsgBoxUtil.ShowWarning("There is no live frame yet, so the slit cannot be measured.");
                return;
            }

            double[] profile = LsoSlitBand.RowProfile(frame.Data, frame.Width, frame.Height, frame.BytesPerPixel);
            LsoSlitBandResult band = LsoSlitBand.Measure(profile);
            if (!band.Found)
            {
                MsgBoxUtil.ShowWarning(band.Reason);
                return;
            }

            int binning = (int)Math.Max(1u, AlignPreset.Camera.BinningVertical);
            double centre = band.CenterRow * binning;
            int fwhm = band.FwhmRows * binning;
            double eq = band.EquivalentRows * binning;

            SlitMeasureOverlay overlay = myColorPreview.Image.SlitMeasure;
            hookSlitMeasure_();
            overlay.ShowBand(band.TopRow, band.BottomRow, band.CenterRow,
                $"AUTO SLIT: centre row {centre:F1} | FWHM {fwhm} | Eq {eq:F0} sensor rows");

            myLbSlitBand.Content = $"Center : {centre:F1}   FWHM : {fwhm}   Eq : {eq:F0}  (sensor rows)";

            // Eq is the light-bearing width. The 2-click edges give the full width, about 2x
            // wider (measured 2026-09-20), so Rec Exp follows Eq rather than that full width.
            showSlitRows_((int)Math.Round(eq));
        }

        private void myBtClearSlitMarkers_Click(object sender, RoutedEventArgs e)
        {
            myColorPreview.Image.SlitMeasure.ClearMarkers();
        }

        // Esc cancels a running measurement, as in the DevKit.
        private void Window_PreviewKeyDown_SlitMeasure_(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape && myColorPreview.Image.SlitMeasure.IsMeasuring)
            {
                myColorPreview.Image.SlitMeasure.CancelMeasure();
                e.Handled = true;
            }
        }

        #endregion Slit height measurement
    }
}
