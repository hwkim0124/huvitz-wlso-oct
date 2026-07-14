using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.controls;
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;

    public partial class OctSignalTestWindow : Window
    {
        LightControlWindow? _lightControlWindow = null;
        LaserControlWindow? _laserControlWindow = null;
        FocusMotorWindow? _focusMotorWindow = null;
        StageMotorWindow? _stageMotorWindow = null;
        private bool _isWindowLoaded = false;

        DispatcherTimer _timer = new DispatcherTimer();


        public OctSignalTestWindow()
        {
            InitializeComponent();

            _onCorneaLeftFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaLeftFrameCaptured);
            _onCorneaRightFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaRightFrameCaptured);
            _onCorneaLowerFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaLowerFrameCaptured);

            _onJoystickButtonPressed = new WsoCallback.JoystickButtonPressed(this.OnJoystickButtonPressed);
            _onOptimizeButtonPressed = new WsoCallback.OptimizeButtonPressed(this.OnOptimizeButtonPressed);

            _onAutoFocusOptimized = new WsoCallback.OctAutoFocusOptimized(this.OnOctAutoFocusOptimized);
            _onAutoPolarOptimized = new WsoCallback.OctAutoPolarOptimized(this.OnOctAutoPolarOptimized);
            _onAutoReferOptimized = new WsoCallback.OctAutoReferOptimized(this.OnOctAutoReferOptimized);
            _onAutoScanOptimized = new WsoCallback.OctAutoScanOptimized(this.OnOctAutoScanOptimized);

            _onOctPreviewCaptured = new WsoCallback.OctScanPreviewImageCaptured(this.OnOctScanPreviewImageCaptured);
            _onProtocolCompleted = new WsoCallback.OctScanProtocolCompleted(this.OnOctScanProtocolCompleted);
            _onResampleDataCaptured = new WsoCallback.OctResampleDataCaptured(this.OnOctResampleDataCaptured);
            _onSpectrumDataCaptured = new WsoCallback.OctSpectrumDataCaptured(this.OnOctSpectrumDataCaptured);
            _onIntensityDataCaptured = new WsoCallback.OctIntensityDataCaptured(this.OnOctIntensityDataCaptured);
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
            OctScanner.IsFixedNoiseReductionEnabled(true, false);
            OctScanner.IsAdaptiveGrayScalingEnabled(true, false);

            PopulateSignalOptionControls();
            PopulateDispersionControls(false);
            PopulateSpectrometerControls(false);

            BoardDevice.ConnectJoystickButtonPressed(_onJoystickButtonPressed);
            BoardDevice.ConnectOptimizeButtonPressed(_onOptimizeButtonPressed);
            StartCorneaCameraPreview();
            _isWindowLoaded = true;

            _timer.Interval = TimeSpan.FromMilliseconds(100);
            _timer.Tick += new EventHandler(OnTimerTick);
            _timer.Start();
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
            _timer.Stop();
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

        private void OnTimerTick(object? sender, EventArgs e)
        {
            if (_isDisposePeakByRefer)
            {
                DisposePeakPositionByReference();
            }
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Update GUI controls with current system status.
        /// 
        private void InitializeWindowControls()
        {
            cmbPeakRangeMin.ItemsSource = new string[] { "0", "128", "256", "512" };
            cmbPeakRangeMax.ItemsSource = new string[] { "5120", "10240", "20480", "30720", "40960", "81920", "163840", "327680", "700000" };
            cmbPeakRangeMin.SelectedIndex = 0;
            cmbPeakRangeMax.SelectedIndex = 7;

            editPeakOffset1.Text = "10";
            editPeakOffset2.Text = "50";
            editDataRangeMin.Text = "0";
            editDataRangeMax.Text = "1024";

            checkIntensity.IsChecked = true;
            checkFwhm.IsChecked = false;
            checkSnrMax.IsChecked = false;
            checkAlignGuide.IsChecked = false;

            editAverageSize.Text = "5";
            editNoiseLevel.Text = "0";

            cmbControlMode.ItemsSource = new string[] { "Reference", "Focus", "Polarization" };
            cmbCaptureCount.ItemsSource = new string[] { "5", "10", "25", "50", "100", "250", "500" };
            cmbCaptureStep.ItemsSource = new string[] { "10", "15", "20", "25", "30", "50" };
            cmbControlMode.SelectedIndex = 0;
            cmbCaptureStep.SelectedIndex = 4;
            cmbCaptureCount.SelectedIndex = 1;

            cmbPeakAt.ItemsSource = new string[] { "26", "51", "381", "751" };
            cmbPeakAt.SelectedIndex = 0;

            cmbZeroPadding.ItemsSource = new string[] { "1", "2", "4" };
            cmbZeroPadding.SelectedIndex = 0;

            cmbScanPattern.ItemsSource = new string[] { "Point", "Horizontal Line", "Vertical Line" };
            cmbScanRange.ItemsSource = new string[] { "0.0", "1.0", "2.0", "3.0", "4.0", "5.0", "6.0", "7.0", "8.0", "9.0", "10.0", "11.0", "12.0", "13.0", "14.0", "15.0", "16.0" };
            cmbScanPoints.ItemsSource = new string[] { "512", "1024", "2048" };
            cmbScanOffsetX.ItemsSource = new string[] { "-4.5", "-3.0", "0.0", "+0.1", "+3.0", "+4.5" };
            cmbScanOffsetY.ItemsSource = new string[] { "-4.5", "-3.0", "0.0", "+0.1", "+3.0", "+4.5" };
            cmbScanPattern.SelectedIndex = 1;
            cmbScanPoints.SelectedIndex = 1;
            cmbScanRange.SelectedIndex = 6;
            cmbScanOffsetY.SelectedIndex = 2;
            cmbScanOffsetX.SelectedIndex = 2;

            editScanScaleX.Text = "1.0";
            editScanScaleY.Text = "1.0";

            checkNotGrabbing.IsChecked = false;
            checkIsAnterior.IsChecked = false;

            radioCameraSpeed3.IsChecked = true;

            corneaPreview1.CameraType = WsoDevice.CameraType.IrCorneaLeft;
            corneaPreview2.CameraType = WsoDevice.CameraType.IrCorneaRight;
            corneaPreview3.CameraType = WsoDevice.CameraType.IrCorneaLower;
        }


        /////////////////////////////////////////////////////////////////////////////////////////////
        /// Button click event handlers
        /// 
        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
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
                _lightControlWindow?.Focus();
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
                ReflectDispersionControls();
                ReflectSpectrometerControls();

                StartOctScanPreview();
                EnableButtonsAtScanning();
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnOptimize_Click(object sender, RoutedEventArgs e)
        {
            OptimizeOctPreview();
        }

        private void BtnAutoPolar_Click(object sender, RoutedEventArgs e)
        {
            OptimizeAuoPolarization();
        }

        private void BtnAutoFocus_Click(object sender, RoutedEventArgs e)
        {
            OptimizeAutoFocus();
        }

        private void BtnAutoRefer_Click(object sender, RoutedEventArgs e)
        {
            OptimizeAutoReference();
        }

        private void BtnFindPeak_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnPolarLoop_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnReferLoop_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnAutoTest_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnMakeBackground_Click(object sender, RoutedEventArgs e)
        {

        }

        private void EnableButtonsAtInitScreen()
        {
            btnStartScan.Content = "Start Scan";
            btnOptimize.IsEnabled = false;
            btnAutoFocus.IsEnabled = false;
            btnAutoRefer.IsEnabled = false;
            btnAutoPolar.IsEnabled = false;
        }

        private void EnableButtonsAtScanning()
        {
            btnStartScan.Content = "Stop Scan";
            btnOptimize.IsEnabled = true;
            btnAutoFocus.IsEnabled = true;
            btnAutoRefer.IsEnabled = true;
            btnAutoPolar.IsEnabled = true;
        }

        private void EnableButtonsAtCaptured()
        {
            EnableButtonsAtInitScreen();
        }

        private void EnableButtonsAtCanceled()
        {
            EnableButtonsAtInitScreen();
        }

        private void PopulateScanPatternControls()
        {

        }

        private void CheckIsAnterior_Click(object sender, RoutedEventArgs e)
        {
            IsScanOnRetina = checkIsAnterior.IsChecked != true;
        }

        private void PopulateCaptureControls()
        {
            var value = cmbControlMode.SelectedIndex switch
            {
                0 => DeviceMotors.GetStepMotorPosition(WsoDevice.MotorType.OctRefer),
                1 => DeviceMotors.GetStepMotorPosition(WsoDevice.MotorType.OctFocus),
                2 => DeviceMotors.GetStepMotorPosition(WsoDevice.MotorType.OctPolar),
                _ => 0
            };
            editCapturePosition.Text = value.ToString();
        }

        private void ReflectCaptureControls()
        {
            var value = ToInt(editCapturePosition.Text);
            switch (cmbControlMode.SelectedIndex)
            {
                case 0:
                    DeviceMotors.MoveStepMotorPosition(WsoDevice.MotorType.OctRefer, value);
                    break;
                case 1:
                    DeviceMotors.MoveStepMotorPosition(WsoDevice.MotorType.OctFocus, value);
                    break;
                case 2:
                    DeviceMotors.MoveStepMotorPosition(WsoDevice.MotorType.OctPolar, value);
                    break;
            }
        }

        private void PopulateSignalOptionControls()
        {
            bool flag = OctScanner.IsBackgroundSubtractionEnabled();
            checkBackgroundSubtract.IsChecked = flag;
            flag = OctScanner.IsDispersionCompensationEnabled();
            checkDispersionCompensation.IsChecked = flag;
            flag = OctScanner.IsFixedNoiseReductionEnabled();
            checkFixedNoiseReduction.IsChecked = flag;
            flag = OctScanner.IsAdaptiveGrayScalingEnabled();
            checkAdaptiveGrayscale.IsChecked = flag;
            flag = OctScanner.IsFFTWindowingEnabled();
            checkFFTWindowing.IsChecked = flag;
            flag = OctScanner.IsKLinearResamplingEnabled();
            checkKLinearResample.IsChecked = flag;
        }
        private void CheckBackgroundSubtract_Click(object sender, RoutedEventArgs e)
        {
            bool flag = checkBackgroundSubtract.IsChecked == true;
            OctScanner.IsBackgroundSubtractionEnabled(true, flag);
        }

        private void CheckDispersionCompensation_Click(object sender, RoutedEventArgs e)
        {
            bool flag = checkDispersionCompensation.IsChecked == true;
            OctScanner.IsDispersionCompensationEnabled(true, flag);
        }

        private void CheckFixedNoiseReduction_Click(object sender, RoutedEventArgs e)
        {
            bool flag = checkFixedNoiseReduction.IsChecked == true;
            OctScanner.IsFixedNoiseReductionEnabled(true, flag);
        }

        private void CheckAdaptiveGrayscale_Click(object sender, RoutedEventArgs e)
        {
            bool flag = checkAdaptiveGrayscale.IsChecked == true;
            OctScanner.IsAdaptiveGrayScalingEnabled(true, flag);
        }

        private void CheckFFTWindowing_Click(object sender, RoutedEventArgs e)
        {
            bool flag = checkFFTWindowing.IsChecked == true;
            OctScanner.IsFFTWindowingEnabled(true, flag);
        }

        private void CheckKLinearResample_Click(object sender, RoutedEventArgs e)
        {
            bool flag = checkKLinearResample.IsChecked == true;
            OctScanner.IsKLinearResamplingEnabled(true, flag);
        }

        private void PopulateDispersionControls(bool setZeros = false)
        {
            double[] values = { 0.0, 0.0, 0.0 };
            if (!setZeros)
            {
                var param = GetDispersionParam();
                if (IsScanOnRetina)
                {
                    values = param.retinaCoeffs;
                }
                else
                {
                    values = param.corneaCoeffs;
                }
            }

            editDisperParam1.Text = values[0].ToString("F4");
            editDisperParam2.Text = values[1].ToString("F4");
            editDisperParam3.Text = values[2].ToString("F4");
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

        private void ReflectDispersionControls()
        {
            var param = GetDispersionParam();
            if (IsScanOnRetina)
            {
                param.retinaCoeffs[0] = ToDouble(editDisperParam1.Text);
                param.retinaCoeffs[1] = ToDouble(editDisperParam2.Text);
                param.retinaCoeffs[2] = ToDouble(editDisperParam3.Text);
            }
            else
            {
                param.corneaCoeffs[0] = ToDouble(editDisperParam1.Text);
                param.corneaCoeffs[1] = ToDouble(editDisperParam2.Text);
                param.corneaCoeffs[2] = ToDouble(editDisperParam3.Text);
            }
            SetDispersionParam(param);
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

        private void BtnLoadDisperParams_Click(object sender, RoutedEventArgs e)
        {
            PopulateDispersionControls(false);
        }

        private void BtnSetDisperZeros_Click(object sender, RoutedEventArgs e)
        {
            PopulateDispersionControls(true);
        }

        private void CmbPeakRangeMin_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isWindowLoaded)
            {
                SetupIntensityGraphDisplay();
            }
        }

        private void CmbPeakRangeMax_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isWindowLoaded)
            {
                SetupIntensityGraphDisplay();
            }
        }

        private void EditDataRangeMin_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupIntensityGraphDisplay();
            }
        }

        private void EditDataRangeMax_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupIntensityGraphDisplay();
            }
        }

        private void EditNoiseLevel_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SetupIntensityGraphDisplay();
            }
        }

        private void EditCapturePosition_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                ReflectCaptureControls();
            }
        }

        private void EditDisperParam1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                ReflectDispersionControls();
            }
        }

        private void EditDisperParam2_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                ReflectDispersionControls();
            }
        }

        private void EditDisperParam3_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                ReflectDispersionControls();
            }
        }

        private void CmbControlMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            PopulateCaptureControls();
        }

        private void BtnCapturePeak_Click(object sender, RoutedEventArgs e)
        {
            if (_isDisposePeakByRefer)
            {
                _isDisposePeakByRefer = false;
                Mouse.OverrideCursor = null;
            }
            else
            {
                _disposePeakDiffStep = cmbPeakAt.SelectedIndex;
                _disposePeakCount = 0;
                _isDisposePeakByRefer = true;
                Mouse.OverrideCursor = Cursors.Wait;
            }
        }

        private void CheckAlignGuide_Click(object sender, RoutedEventArgs e)
        {
            var flag = checkAlignGuide.IsChecked == true;
            var range = ToFloat(cmbScanRange.SelectedItem.ToString());

            octScanImagePreview.BscanRange = range;
            octScanImagePreview.IsOverlayAlignGuide = flag;
        }

        private void BtnApplyDisperParams_Click(object sender, RoutedEventArgs e)
        {
            ReflectDispersionControls();
            ShowInfo("Dispersion calibration applied to system!");
        }

        private void BtnApplySpectParams_Click(object sender, RoutedEventArgs e)
        {
            ReflectSpectrometerControls();
            ShowInfo("Spectrometer calibration applied to system!");
        }
    }
}
