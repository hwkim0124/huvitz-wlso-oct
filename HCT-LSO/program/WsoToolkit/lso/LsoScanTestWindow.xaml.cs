using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;
using WsoNativeLib;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoDomain;
using static WsoNativeLib.WsoLsoDefs;
using static WsoToolkit.controls.LsoScanImagePreview;
using WsoNativeLib.Presets;
using WsoToolkit.utils;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for LsoScanTestWindow.xaml
    /// </summary>
    public partial class LsoScanTestWindow : Window
    {
        public LsoScanTestWindow()
        {
            _onColorCaptureImageCaptured = new WsoCallback.ColorCameraImageCaptured(this.OnColorCameraCaptureFrameCaptured);
            _onColorCaptureCompleted = new WsoCallback.ColorCaptureCompleted(this.OnColorCaptureCompleted);
            _onColorLiveFrameCaptured = new WsoCallback.ColorCameraFrameCaptured(this.OnColorCameraFrameCaptured);

            _onCorneaLeftFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLeftFrameCaptured);
            _onCorneaRightFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraRightFrameCaptured);
            _onCorneaLowerFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLowerFrameCaptured);

            _onLsoFocusPositionChanged = new StepMotorPositionChanged(this.OnLsoFocusPositionChanged);
            _onRetMirrorPositionChanged = new StepMotorPositionChanged(this.OnRetMirrorPositionChanged);
            _onLsoFilterPositionChanged = new StepMotorPositionChanged(this.OnLsoFilterPositionChanged);

            InitializeComponent();

            initSetting_();
            LightControlState.Changed += onLightStateChanged_;
            initLsoFocusMotor_();
            initRetMirrorMotor_();
            initLsoFilterMotor_();
            initFixation_();
            myColorPreview.Image.FixationDragRequested += onFixationDragRequested_;
            updateAlignControls_();
            reflectScanTiming_();

            // The anterior (cornea IR) preview is always running while this window is open.
            StartCorneaCameraPreview();
            initAnteriorCamera_();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            LightControlState.Changed -= onLightStateChanged_;
            stopLiveCapture_();
            // Stop a running capture one-call first (cancels + joins the native worker).
            if (LsoCamera.IsCapturing())
            {
                LsoCamera.StopColorCapture();
            }

            myColorPreview.CloseAdjustmentWindow();

            // Alignment holds the illumination on continuously - release it before anything else
            // so no close path can leave the light burning.
            releaseSlitAlign_();

            // If the aiming (mono live) is on, stop it first (IR LED off + param restore + live pause).
            // Do this before pausing color live so the IR LED is reliably turned off.
            if (IsRetinaPreviewing_())
            {
                StopRetinaPreview_();
            }

            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            if (isCorneaCameraPreviewing_())
            {
                CloseCorneaCameraPreview();
            }

            releaseLsoFocusMotor_();
            releaseRetMirrorMotor_();
            releaseLsoFilterMotor_();
        }

        #region Scan Mode

        private void myBtStartScan_Click(object sender, RoutedEventArgs e)
        {
            stopLiveCapture_();
            System.Console.WriteLine("myBtStartScan_Click");
            if (LsoCamera.IsCapturing())
            {
                return; // capture one-call in progress - the camera session is busy
            }
            if (IsRetinaPreviewing_())
            {
                StopRetinaPreview_();
            }
            else
            {
                // If color live (Bayer) is running, stop it first, then start the mono aiming live.
                // (They share the one color camera, so the two lives are mutually exclusive.)
                if (IsColorCameraLive())
                {
                    PauseColorCameraLive();
                }
                StartRetinaPreview_();
            }

            ShowRetinaPreview_();
        }

        private void myBtCapture_Click(object sender, RoutedEventArgs e)
        {
            stopLiveCapture_();
            System.Console.WriteLine("myBtCapture_Click");
            if (LsoCamera.IsCapturing())
            {
                return; // capture one-call in progress
            }
            if (IsRetinaPreviewing_())
            {
                StopRetinaPreview_();
            }

            ShowColorPreview_();

            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            readyToCapture_();

            StartColorCameraOriginal();
        }

        private void myBtCorneaStart_Click(object sender, RoutedEventArgs e)
        {
            if (isCorneaCameraPreviewing_())
            {
                CloseCorneaCameraPreview();
            }
            else
            {
                StartCorneaCameraPreview();
            }
        }

        #endregion Scan Mode

        private void myBtImageAnalysis_Click(object sender, RoutedEventArgs e)
        {
            new LsoImageAnalysisWindow { Owner = this }.ShowDialog();
        }

        #region Settings 

        private void myBtSettings_Click(object sender, RoutedEventArgs e)
        {
            openModeSettings_();
        }

        #region Camera Preset

        private void myCbPreviewPreset_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            commitCameraPresetSelection_(ColorCameraPresetMode.Preview);
        }

        private void myCbCapturePreset_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            commitCameraPresetSelection_(ColorCameraPresetMode.Capture);
        }

        private void myCbCaptureImageMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            commitCaptureImageMode_();
        }

        private void myBtPreviewPresetSaveAs_Click(object sender, RoutedEventArgs e)
        {
            saveCameraPresetAs_(ColorCameraPresetMode.Preview);
        }

        private void myBtCapturePresetSaveAs_Click(object sender, RoutedEventArgs e)
        {
            saveCameraPresetAs_(ColorCameraPresetMode.Capture);
        }

        #endregion Camera Preset

        #region Anterior Camera

        private void initAnteriorCamera_()
        {
            myCbAnteriorCamera.ItemsSource = new string[] { "Lower", "Left", "Right" };
            myCbAnteriorCamera.SelectedIndex = 0;
            reflectAnteriorCamera_();
        }

        private CameraType anteriorCameraSelected_()
        {
            return myCbAnteriorCamera.SelectedIndex switch
            {
                1 => CameraType.IrCorneaLeft,
                2 => CameraType.IrCorneaRight,
                _ => CameraType.IrCorneaLower,
            };
        }

        private void reflectAnteriorCamera_()
        {
            var type = anteriorCameraSelected_();
            myTbAnteriorGain.Text = CorneaCamera.GetAnalogGain(type).ToString("F0");
            myTbAnteriorExposure.Text = CorneaCamera.GetExposureTime(type).ToString();
        }

        private void applyAnteriorCamera_()
        {
            if (!LsoScanTestInput.TryParseAnterior(myTbAnteriorGain.Text, myTbAnteriorExposure.Text,
                    out int gain, out float exposureMs, out string error))
            {
                MsgBoxUtil.ShowWarning(error);
                return;
            }

            var type = anteriorCameraSelected_();
            myLbAnteriorResult.Text = anteriorCameraName_(type) + " " + applyAnteriorCameraTo_(type, gain, exposureMs);
            reflectAnteriorCamera_();
        }

        private static string anteriorCameraName_(CameraType type)
        {
            return type switch
            {
                CameraType.IrCorneaLeft => "Left",
                CameraType.IrCorneaRight => "Right",
                _ => "Lower",
            };
        }

        private static string applyAnteriorCameraTo_(CameraType type, int gain, float exposureMs)
        {
            bool gainOk = CorneaCamera.SetAnalogGain(type, gain);
            bool exposureOk = CorneaCamera.SetExposureTime(type, exposureMs);
            return gainOk && exposureOk ? "OK" : "FAIL";
        }

        private void myCbAnteriorCamera_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!IsInitialized)
            {
                return;
            }
            reflectAnteriorCamera_();
        }

        private void myTbAnteriorValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                applyAnteriorCamera_();
            }
        }

        private void myBtAnteriorApply_Click(object sender, RoutedEventArgs e)
        {
            applyAnteriorCamera_();
        }

        private void myBtAnteriorApplyAll_Click(object sender, RoutedEventArgs e)
        {
            if (!LsoScanTestInput.TryParseAnterior(myTbAnteriorGain.Text, myTbAnteriorExposure.Text,
                    out int gain, out float exposureMs, out string error))
            {
                MsgBoxUtil.ShowWarning(error);
                return;
            }

            string lower = applyAnteriorCameraTo_(CameraType.IrCorneaLower, gain, exposureMs);
            string left = applyAnteriorCameraTo_(CameraType.IrCorneaLeft, gain, exposureMs);
            string right = applyAnteriorCameraTo_(CameraType.IrCorneaRight, gain, exposureMs);
            myLbAnteriorResult.Text = $"Lower {lower}, Left {left}, Right {right}";
            reflectAnteriorCamera_();
        }

        #endregion Anterior Camera

        #endregion Setting 

        #region LSO Focus Motor

        private void mySliderLsoFocus_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isLsoFocusSliderDragging = true;
        }

        private void mySliderLsoFocus_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isLsoFocusSliderDragging = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, (int)mySliderLsoFocus.Value);
        }

        private void mySliderLsoFocus_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isLsoFocusSliderDragging && !_isLsoFocusPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, (int)mySliderLsoFocus.Value);
            }
            _isLsoFocusPositionChanged = false;
        }

        private void myTbLsoFocusPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            int value = ToInt(myTbLsoFocusPos.Text);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, value);
        }

        private void myTbLsoFocusValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            double diopter = ToFloat(myTbLsoFocusValue.Text);
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, mapLsoFocusValueToPosition_(diopter));
        }

        private void myBtLsoFocusBwd_Click(object sender, RoutedEventArgs e)
        {
            double diopter = ToFloat(myTbLsoFocusValue.Text) - 1.0;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, mapLsoFocusValueToPosition_(diopter));
        }

        private void myBtLsoFocusFwd_Click(object sender, RoutedEventArgs e)
        {
            double diopter = ToFloat(myTbLsoFocusValue.Text) + 1.0;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, mapLsoFocusValueToPosition_(diopter));
        }

        private void myBtLsoFocusZero_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFocus, mapLsoFocusValueToPosition_(0.0));
        }

        // 디옵터 값을 모터 위치로 선형 변환한다. (rangeMinValue~rangeMaxValue -> rangeMin~rangeMax)
        private int mapLsoFocusValueToPosition_(double diopter)
        {
            return (int)mappingRangeToValue_(
                diopter,
                _lsoFocusMotorStatus.rangeMinValue, _lsoFocusMotorStatus.rangeMaxValue,
                _lsoFocusMotorStatus.rangeMin, _lsoFocusMotorStatus.rangeMax);
        }

        private static double mappingRangeToValue_(double x, double fromMin, double fromMax, double toMin, double toMax)
        {
            if (Math.Abs(fromMax - fromMin) < double.Epsilon)
            {
                return toMin;
            }

            double normalized = (x - fromMin) / (fromMax - fromMin);
            double mapped = normalized * (toMax - toMin) + toMin;
            return Math.Round(mapped, 2);
        }

        #endregion LSO Focus Motor

        #region Return Mirror

        private void mySliderRetMirror_DragStarted(object sender, DragStartedEventArgs e)
        {
            _isRetMirrorSliderDragging = true;
        }

        private void mySliderRetMirror_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            _isRetMirrorSliderDragging = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, (int)mySliderRetMirror.Value);
        }

        private void mySliderRetMirror_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isRetMirrorSliderDragging && !_isRetMirrorPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, (int)mySliderRetMirror.Value);
            }
            _isRetMirrorPositionChanged = false;
        }

        private void myTbRetMirrorPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            DeviceMotors.MoveStepMotorPositionAsync(MotorType.RetMirror, ToInt(myTbRetMirrorPos.Text));
        }

        private void myBtRetMirrorIn_Click(object sender, RoutedEventArgs e)
        {
            myCbRetMirror.SelectedIndex = 0;
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.RetMirror, (int)MirrorMotorMode.MirrorIn);
        }

        private void myBtRetMirrorOut_Click(object sender, RoutedEventArgs e)
        {
            myCbRetMirror.SelectedIndex = 1;
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.RetMirror, (int)MirrorMotorMode.MirrorOut);
        }

        private void myBtRetMirrorSetAs_Click(object sender, RoutedEventArgs e)
        {
            var mode = myCbRetMirror.SelectedIndex == 0 ? MirrorMotorMode.MirrorIn : MirrorMotorMode.MirrorOut;
            DeviceMotors.SetStepMotorCurrentPositionAsOrigin(MotorType.RetMirror, (int)mode);
        }

        #endregion Return Mirror

        #region LSO Filter Motor

        // Tag = LsoFilterMode(0:Color, 1:BlueFaf, 2:GreenFaf, 3:Icga).
        // 각 모드는 네이티브가 보유한 필터 위치(HBS 프로파일 캘리브 값)로 이동한다.
        private void myRbLsoFilterMode_Click(object sender, RoutedEventArgs e)
        {
            int mode = ToInt((string)((RadioButton)sender).Tag);
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.LsoFilter, mode);
        }

        private void myBtLsoFilterMove_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFilter, ToInt(myTbLsoFilterPos.Text));
        }

        private void myTbLsoFilterPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            DeviceMotors.MoveStepMotorPositionAsync(MotorType.LsoFilter, ToInt(myTbLsoFilterPos.Text));
        }

        #endregion LSO Filter Motor

        #region Internal Fixation

        private void mySliderFixationRow_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isFixationSyncing) return;
            setCurrentInternalFixation_();
        }

        private void mySliderFixationCol_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isFixationSyncing) return;
            setCurrentInternalFixation_();
        }

        private void myTbFixationRow_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            int value = ToInt(myTbFixationRow.Text);
            if (value >= mySliderFixationRow.Minimum && value <= mySliderFixationRow.Maximum)
            {
                mySliderFixationRow.Value = value;
            }
        }

        private void myTbFixationCol_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            int value = ToInt(myTbFixationCol.Text);
            if (value >= mySliderFixationCol.Minimum && value <= mySliderFixationCol.Maximum)
            {
                mySliderFixationCol.Value = value;
            }
        }

        private void myBtFixationFundus_Click(object sender, RoutedEventArgs e)
        {
            setCenterInternalFixation_();
        }

        private void myBtFixationMarcular_Click(object sender, RoutedEventArgs e)
        {
            setCenterInternalFixation_();
        }

        private void myBtFixationDisc_Click(object sender, RoutedEventArgs e)
        {
            setPresetInternalFixation_(FixationTarget.OPTIC_DISC);
        }

        #endregion Internal Fixation

        #region LSO Trigger Mode (0x4007)

        // Board-global LSO_TRIG_MODE command (0x4007), the same setting the DevKit LED window
        // exposes as "Flash Trig Mode". The mode byte is sent as-is:
        //   0 = Traj.Controller: the trajectory controller starts the camera
        //   1 = Camera: the camera triggers the scan
        // The board serializes the USB command inside the native facade, which is the same
        // call path PauseColorCameraLive uses (it sends 0 when it pauses the live view).
        private void myCbLsoTriggerMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // XAML selects index 0 while InitializeComponent runs; do not touch the board
            // before the window has loaded. The pause path still sends its 0 as before.
            if (!IsLoaded)
            {
                return;
            }

            int mode = myCbLsoTriggerMode.SelectedIndex == 1 ? 1 : 0;
            if (!LsoScanner.SetLsoScannerTriggerMode(mode))
            {
                MsgBoxUtil.ShowWarning("The board refused the LSO trigger mode command (0x4007). "
                    + "Check the board connection and firmware.");
            }
        }

        #endregion LSO Trigger Mode (0x4007)

        #region Align Guide

        private void myCheckAlignGuide_CheckedNUnchecked(object sender, RoutedEventArgs e)
        {
            myColorPreview.IsOverlayAlignGuide = myCheckAlignGuide.IsChecked == true;

            corneaPreview1.IsStageControlEnabled = myCheckAlignGuide.IsChecked == true;
            corneaPreview2.IsStageControlEnabled = myCheckAlignGuide.IsChecked == true;
            corneaPreview3.IsStageControlEnabled = myCheckAlignGuide.IsChecked == true;

            corneaPreview1.IsOverlayFocusGuide = myCheckAlignGuide.IsChecked == true;
            corneaPreview2.IsOverlayFocusGuide = myCheckAlignGuide.IsChecked == true;
            corneaPreview3.IsOverlayFocusGuide = myCheckAlignGuide.IsChecked == true;
        }


        private void myBtImageAdjust_Click(object sender, RoutedEventArgs e)
        {
            myColorPreview.OpenImageAdjustment();
        }

        // Owned window: closes with this one; closing it un-toggles the button.
        private LsoZoomAreaWindow? _zoomAreaWindow;

        private void myToggleZoomView_Click(object sender, RoutedEventArgs e)
        {
            if (myToggleZoomView.IsChecked != true)
            {
                _zoomAreaWindow?.Close();
                return;
            }

            if (_zoomAreaWindow != null)
            {
                _zoomAreaWindow.Activate();
                return;
            }

            _zoomAreaWindow = new LsoZoomAreaWindow(myColorPreview.Image) { Owner = this };
            _zoomAreaWindow.Closed += (_, _) =>
            {
                _zoomAreaWindow = null;
                myToggleZoomView.IsChecked = false;
            };
            _zoomAreaWindow.Show();
        }
        #endregion Align Guide

    }
}
