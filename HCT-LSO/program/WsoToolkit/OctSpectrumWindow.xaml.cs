using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Timers;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using WsoNativeLib;
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;

    public partial class OctSpectrumWindow : Window
    {
        LightControlWindow? _lightControlWindow = null;
        LaserControlWindow? _laserControlWindow = null;
        FocusMotorWindow? _focusMotorWindow = null;
        StageMotorWindow? _stageMotorWindow = null;

        public OctSpectrumWindow()
        {
            InitializeComponent();

            _onCorneaLeftFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaLeftFrameCaptured);
            _onCorneaRightFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaRightFrameCaptured);
            _onCorneaLowerFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaLowerFrameCaptured);

            _onJoystickButtonPressed = new WsoCallback.JoystickButtonPressed(this.OnJoystickButtonPressed);

            _onOctPreviewCaptured = new WsoCallback.OctScanPreviewImageCaptured(this.OnOctScanPreviewImageCaptured);
            _onProtocolCompleted = new WsoCallback.OctScanProtocolCompleted(this.OnOctScanProtocolCompleted);
            _onResampleDataCaptured = new WsoCallback.OctResampleDataCaptured(this.OnOctResampleDataCaptured);
            _onSpectrumDataCaptured = new WsoCallback.OctSpectrumDataCaptured(this.OnOctSpectrumDataCaptured);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Window event handlers
        /// 
        private void Window_Initialized(object sender, EventArgs e)
        {
            InitializeWindowControls();
        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            StartCorneaCameraPreview();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (MsgBoxUtil.ShowQuestion("Are you sure to close this window?") == false)
            {
                e.Cancel = true;
            }
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            _lightControlWindow?.Close();
            _focusMotorWindow?.Close();
            _stageMotorWindow?.Close();

            BoardDevice.ReleaseJoystickButtonPressed();
            BoardDevice.ReleaseOptimizeButtonPressed();

            CancelOctScanning(false);
            CloseCorneaCameraPreview();

            Thread.Sleep(500);
            Mouse.OverrideCursor = null;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Update GUI controls with current system status.
        /// 
        private void InitializeWindowControls()
        {
            PopulateFbgClassList();
            PopulateFbgClassValues();
            btnGetFbgClass.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));

            cmbProfileNumber.ItemsSource = new string[] { "0", "1", "2", "4", "8", "16", "32", "64", "128", "256", "512" };
            cmbProfileNumber.SelectedIndex = 9;
            btnGetProfileNumber.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));

            PopulateGraphDisplayControls();

            radioCameraSpeed3.IsChecked = true;

            sliderPeakThresh.Minimum = PEAK_THRESHOLD_MIN;
            sliderPeakThresh.Maximum = PEAK_THRESHOLD_MAX;
            sliderPeakThresh.Value = PEAK_THRESHOLD_INIT;
            sliderSubsThresh.Minimum = PEAK_THRESHOLD_MIN;
            sliderSubsThresh.Maximum = PEAK_THRESHOLD_MAX;
            sliderSubsThresh.Value = PEAK_THRESHOLD_INIT;
            radioPolyOrder4.IsChecked = true;

            PopulateSpectrometerControls();
            SetupResampleGraphDisplay();
        }

        private void EnableButtonsAtInitScreen()
        {
            btnStartScan.Content = "Start Scan";
        }

        private void EnableButtonsAtScanning()
        {
            btnStartScan.Content = "Stop Scan";
        }

        private void EnableButtonsAtCaptured()
        {
            EnableButtonsAtInitScreen();
        }

        private void EnableButtonsAtCanceled()
        {
            EnableButtonsAtInitScreen();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Button event handlers
        /// 
        private void BtnStartScan_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (IsOctScanning())
            {
                CancelOctScanning(false);
                EnableButtonsAtCanceled();
            }
            else
            {
                StartOctScanPreview();
                EnableButtonsAtScanning();
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnShowLights_Click(object sender, RoutedEventArgs e)
        {
            if (_lightControlWindow == null || !_lightControlWindow.IsLoaded)
            {
                _lightControlWindow = new();
                _lightControlWindow.Show();
            }
            else
            {
                _lightControlWindow.Focus();
            }
        }

        private void BtnShowLasers_Click(object sender, RoutedEventArgs e)
        {
            if (_laserControlWindow == null || !_laserControlWindow.IsLoaded)
            {
                _laserControlWindow = new();
                _laserControlWindow.Show();
            }
            else
            {
                _laserControlWindow?.Focus();
            }
        }

        private void BtnShowFocusM_Click(object sender, RoutedEventArgs e)
        {
            if (_focusMotorWindow == null || _focusMotorWindow.IsLoaded)
            {
                _focusMotorWindow = new();
                _focusMotorWindow.Show();
            }
            else
            {
                _focusMotorWindow?.Focus();
            }
        }

        private void BtnShowStageM_Click(object sender, RoutedEventArgs e)
        {
            if (_stageMotorWindow == null || !_stageMotorWindow.IsLoaded)
            {
                _stageMotorWindow = new();
                _stageMotorWindow.Show();
            }
            else
            {
                _stageMotorWindow?.Focus();
            }
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void BtnReferLoop_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnPolarLoop_Click(object sender, RoutedEventArgs e)
        {

        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// FBG class, wavelenghts and profile number
        /// 
        private void PopulateFbgClassList()
        {
            var size = OctCalibration.GetFbgClassListSize();
            var data = new string[size];

            for (int i = 0; i < size; i++)
            {
                if (OctCalibration.GetFbgClassName(i, out var name))
                {
                    data[i] = name;
                }
            }
            cmbFbgClass.ItemsSource = data;

            if (size > 5)
            {
                cmbFbgClass.SelectedIndex = 5;
            }
        }

        private void PopulateFbgClassValues()
        {
            var index = cmbFbgClass.SelectedIndex;

            lstFbgClass.Items.Clear();
            if (OctCalibration.GetFbgClassValues(index, out var values) > 0)
            {
                for (int i = 0; i < values.Length; i++)
                {
                    string s = String.Format("  [{0:D3}] {1}", i, values[i].ToString("F3"));
                    lstFbgClass.Items.Add(s);
                }
            }
        }

        private void BtnGetFbgClass_Click(object sender, RoutedEventArgs e)
        {
            lstWavelenghts.Items.Clear();
            for (int i = 0; i < lstFbgClass.Items.Count; i++)
            {
                lstWavelenghts.Items.Add(lstFbgClass.Items[i]);
            }
        }

        private void CmbFbgClass_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbFbgClass.SelectedIndex >= 0)
            {
                PopulateFbgClassValues();
            }
        }

        private void ReorderWavelenghtList()
        {
            var data = new List<double>();
            for (int i = 0; i < lstWavelenghts.Items.Count; i++)
            {
                var s = lstWavelenghts.Items[i].ToString() ?? "";
                if (s.Contains(']'))
                {
                    var v = ToDouble(s[(s.IndexOf(']') + 1)..]);
                    data.Add(v);
                }
            }
            data.Sort();

            lstWavelenghts.Items.Clear();
            for (int i = 0; i < data.Count; i++)
            {
                string s = String.Format("  [{0:D3}] {1}", i, data[i].ToString("F3"));
                lstWavelenghts.Items.Add(s);
            }
        }

        private void BtnAddWavelength_Click(object sender, RoutedEventArgs e)
        {
            var value = ToDouble(editWavelenght.Text);
            if (value > 0.0f)
            {
                editWavelenght.Text = "";
                string s = String.Format("  [{0:D3}] {1}", 99, value);
                lstWavelenghts.Items.Add(s);

                ReorderWavelenghtList();
            }
        }

        private void BtnDelWavelength_Click(object sender, RoutedEventArgs e)
        {
            int index = lstWavelenghts.SelectedIndex;
            if (index >= 0)
            {
                lstWavelenghts.Items.RemoveAt(index);
                ReorderWavelenghtList();
            }
        }

        private void ReorderPeakPositionList()
        {
            var data = new List<int>();
            for (int i = 0; i < lstPeakPositions.Items.Count; i++)
            {
                var s = lstPeakPositions.Items[i].ToString() ?? "";
                if (s.Contains('@'))
                {
                    var v = ToInt(s[(s.IndexOf('@') + 1)..]);
                    data.Add(v);
                }
            }
            data.Sort();

            lstPeakPositions.Items.Clear();
            for (int i = 0; i < data.Count; i++)
            {
                var wlen = OctScanner.GetOctWavelengthAtPixelPosition(data[i]);
                string s = String.Format("  [{0:D3}] {1:F3} @ {2:F0}", i, wlen, data[i]);
                lstPeakPositions.Items.Add(s);
            }
        }

        private void BtnAddPeakPosition_Click(object sender, RoutedEventArgs e)
        {
            var value = ToInt(editPeakPosition.Text);
            if (value > 0.0f)
            {
                editPeakPosition.Text = "";
                var wlen = OctScanner.GetOctWavelengthAtPixelPosition(value);
                string s = String.Format("  [{0:D3}] {1:F3} @ {2:F0}", 99, wlen, value);
                lstPeakPositions.Items.Add(s);

                ReorderPeakPositionList();
            }
        }

        private void BtnDelPeakPosition_Click(object sender, RoutedEventArgs e)
        {
            int index = lstPeakPositions.SelectedIndex;
            if (index >= 0)
            {
                lstPeakPositions.Items.RemoveAt(index);
                ReorderPeakPositionList();
            }
        }

        private void ReorderProfileNumberList()
        {
            var data = new List<int>();
            for (int i = 0; i < lstProfileNumbers.Items.Count; i++)
            {
                var s = lstProfileNumbers.Items[i].ToString();
                var v = ToInt(s);
                data.Add(v);
            }
            data.Sort();

            lstProfileNumbers.Items.Clear();
            for (int i = 0; i < data.Count; i++)
            {
                lstProfileNumbers.Items.Add(data[i].ToString());
            }
        }

        private void BtnGetProfileNumber_Click(object sender, RoutedEventArgs e)
        {
            var value = ToInt(cmbProfileNumber.SelectedItem.ToString());
            editPeakPosition.Text = "";

            foreach (var item in lstProfileNumbers.Items)
            {
                if (ToInt(item.ToString()) == value)
                {
                    return;
                }
            }
            lstProfileNumbers.Items.Add(value);
            ReorderProfileNumberList();
        }

        private void BtnDelProfileNumber_Click(object sender, RoutedEventArgs e)
        {
            int index = lstPeakPositions.SelectedIndex;
            if (index >= 0)
            {
                lstPeakPositions.Items.RemoveAt(index);
            }
        }

        private void LstProfileNumbers_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (lstProfileNumbers.SelectedIndex >= 0)
            {
                octResampleGraph.SelectedProfileIndex = lstProfileNumbers.SelectedIndex;
            }
        }


        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Graph Display Controls
        /// 
        private void EditRangeXmin_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void EditRangeXmax_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void EditRangeYmin_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void EditRangeYmax_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void EditCenterX_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void EditKernelSize_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void EditAverageSize_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupResampleGraphDisplay();
            }
        }

        private void PopulateGraphDisplayControls()
        {
            editRangeXmin.Text = "0";
            editRangeXmax.Text = "2048";
            editRangeYmin.Text = "0";
            editRangeYmax.Text = "4096";
            editKernelSize.Text = "1"; // "5";
            editAverageSize.Text = "1"; // "5";
        }

        private void BtnResetRange_Click(object sender, RoutedEventArgs e)
        {
            PopulateGraphDisplayControls();
            SetupResampleGraphDisplay();
        }
        private void BtnClearRecords_Click(object sender, RoutedEventArgs e)
        {
            octResampleGraph.ClearRecordingValues();
        }
        private void BtnAroundCenter_Click(object sender, RoutedEventArgs e)
        {
            editRangeXmin.Text = "924";
            editRangeXmax.Text = "1124";
            editRangeYmin.Text = "0";
            editRangeYmax.Text = "4096";
            SetupResampleGraphDisplay();
        }
        private void BtnAroundStart_Click(object sender, RoutedEventArgs e)
        {
            editRangeXmin.Text = "0";
            editRangeXmax.Text = "16";
            editRangeYmin.Text = "0";
            editRangeYmax.Text = "128";
            SetupResampleGraphDisplay();
        }

        private void BtnAroundEnd_Click(object sender, RoutedEventArgs e)
        {
            editRangeXmin.Text = "2032";
            editRangeXmax.Text = "2048";
            editRangeYmin.Text = "0";
            editRangeYmax.Text = "128";
            SetupResampleGraphDisplay();
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Spectrum Calibration
        /// 
        private void SliderPeakThresh_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            editPeakThresh.Text = sliderPeakThresh.Value.ToString("N0");
            octResampleGraph.SetPeakThresholds(ToInt(editPeakThresh.Text), ToInt(editSubsThresh.Text));
        }

        private void SliderSubsThresh_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            editSubsThresh.Text = sliderSubsThresh.Value.ToString("N0");
            octResampleGraph.SetPeakThresholds(ToInt(editPeakThresh.Text), ToInt(editSubsThresh.Text));
        }

        private void EditSubsThresh_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var value = ToInt(editSubsThresh.Text);
                if (value >= PEAK_THRESHOLD_MIN && value <= PEAK_THRESHOLD_MAX)
                {
                    sliderSubsThresh.Value = value;
                }
            }
        }

        private void EditPeakThresh_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var value = ToInt(editPeakThresh.Text);
                if (value >= PEAK_THRESHOLD_MIN && value <= PEAK_THRESHOLD_MAX)
                {
                    sliderPeakThresh.Value = value;
                }
            }
        }

        private void BtnFindPeaks_Click(object sender, RoutedEventArgs e)
        {
            FindSpectrumDataPeaks();
        }

        private void BtnCalibrate_Click(object sender, RoutedEventArgs e)
        {
            ComputeSpectroCoefficients();
        }

        private void BtnResetPeaks_Click(object sender, RoutedEventArgs e)
        {
            sliderPeakThresh.Value = 300;
            sliderSubsThresh.Value = 150;
        }

        private void PopulateSpectrometerControls(bool setZeros = false)
        {
            double[] values = { 0.0, 0.0, 0.0, 0.0 };

            if (!setZeros)
            {
                var param = GetSpectrometerParam();
                values = param.wfCoeffs;
            }

            editSpectParam1.Text = values[0].ToString("E6");
            editSpectParam2.Text = values[1].ToString("E6");
            editSpectParam3.Text = values[2].ToString("E6");
            editSpectParam4.Text = values[3].ToString("E6");
        }

        private void ReflectSpectrometerControls()
        {
            var param = GetSpectrometerParam();
            param.wfCoeffs[0] = ToDouble(editSpectParam1.Text);
            param.wfCoeffs[1] = ToDouble(editSpectParam2.Text);
            param.wfCoeffs[2] = ToDouble(editSpectParam3.Text);
            param.wfCoeffs[3] = ToDouble(editSpectParam4.Text);
            SetSpectrometerParam(param);
        }

        private void BtnLoadSpectParams_Click(object sender, RoutedEventArgs e)
        {
            PopulateSpectrometerControls(false);
        }

        private void BtnSetSpectZeros_Click(object sender, RoutedEventArgs e)
        {
            PopulateSpectrometerControls(true);
        }

        private void BtnDrawCalib_Click(object sender, RoutedEventArgs e)
        {
            var param = GetSpectrometerParam();
            param.wfCoeffs[0] = ToDouble(editSpectParam1.Text);
            param.wfCoeffs[1] = ToDouble(editSpectParam2.Text);
            param.wfCoeffs[2] = ToDouble(editSpectParam3.Text);
            param.wfCoeffs[3] = ToDouble(editSpectParam4.Text);

            var values = new double[WsoOctDefs.LINE_CAMERA_CCD_PIXELS];
            for (int i = 0; i < values.Length; i++)
            {
                double v = param.wfCoeffs[0];
                v += param.wfCoeffs[1] * i;
                v += param.wfCoeffs[2] * i * i;
                v += param.wfCoeffs[3] * i * i * i;
                values[i] = v;
            }

            var points = new List<int>();

            var waves = new List<double>();
            for (int i = 0; i < lstWavelenghts.Items.Count; i++)
            {
                var s = lstWavelenghts.Items[i].ToString() ?? "";
                if (s.Contains(']'))
                {
                    var v = ToDouble(s[(s.IndexOf(']') + 1)..]);
                    waves.Add(v);
                }
            }

            for (int i = 0, j = 0; i < waves.Count; i++)
            {
                for (; j < values.Length; j++)
                {
                    if (waves[i] <= values[j])
                    {
                        points.Add(j);
                        break;
                    }
                }
            }

            octResampleGraph.SetCalibratedIndexList(points);
        }

        private void BtnClearCalib_Click(object sender, RoutedEventArgs e)
        {
            octResampleGraph.SetCalibratedIndexList(new List<int>());
        }

        private void BtnApplyCalib_Click(object sender, RoutedEventArgs e)
        {
            ReflectSpectrometerControls();
            ShowInfo("Spectrometer calibration applied to system!");
        }
    }
}
