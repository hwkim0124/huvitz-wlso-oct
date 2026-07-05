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
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoDomain;
using static WsoNativeLib.WsoLsoDefs;
using static WsoToolkit.controls.LsoScanImagePreview;
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
            InitializeComponent();

            initCallbacks_();
            initSetting_();
            initLsoFocusMotor_();
            initFixation_();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            releaseLsoFocusMotor_();
        }

        #region Scan Mode

        private void myBtStartScan_Click(object sender, RoutedEventArgs e)
        {

        }

        private void myBtCapture_Click(object sender, RoutedEventArgs e)
        {
            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            readyToCapture_(PreviewDisplayMode.REVIEW);

            StartColorCameraOriginal();
        }

        private void myBtCaptureSlide_Click(object sender, RoutedEventArgs e)
        {
            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            readyToCapture_(PreviewDisplayMode.REVIEW_SLICE);

            StartColorCameraOriginal();
        }

        private void myBtStartColorCameraLive_Click(object sender, RoutedEventArgs e)
        {
            myColorPreview.ClearReviewImages();

            if (IsColorCameraLive())
            {
                myBtStartColorCameraLive.Content = "Start Color Live";
                PauseColorCameraLive();

                applyScannerControlParam_((int)WsoNativeLib.WsoLsoDefs.LsoScannerPatternId.COLOR); // Color Camera Galvano Setting 다시 복원
            }
            else
            {
                updateScannerControls_();

                myBtStartColorCameraLive.Content = "Pause Color Live";
                StartColorCameraLive();
            }
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

        #region Settings 

        private void myBtSettingColorCamera_Click(object sender, RoutedEventArgs e)
        {
            var window = new ColorCameraSettingWindow(_scanTestModel) { Owner = this };
            window.ShowDialog();
        }

        private void myBtSettingFixation_Click(object sender, RoutedEventArgs e)
        {
            var window = new FixationSettingWindow { Owner = this };
            window.ShowDialog();
        }

        #endregion Setting 

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

        #region LSO Scanner Setting

        private void myCbPatternID_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdatingScannerControls)
            {
                return;
            }

            updateScannerControls_();
        }

        private void myCbTriggerSrc_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdatingScannerControls)
            {
                return;
            }

            int nPatternId = myCbPatternID.SelectedIndex;
            _scanTestModel.GetScanProfile(nPatternId).TriggerSource = (ushort)myCbTriggerSrc.SelectedIndex;
            applyScannerControlParam_(nPatternId);
        }

        private void myCheckBoxFixedFrame_CheckedNUnchecked(object sender, RoutedEventArgs e)
        {
            if (_isUpdatingScannerControls)
            {
                return;
            }

            int nPatternId = myCbPatternID.SelectedIndex;
            _scanTestModel.GetScanProfile(nPatternId).AcquisitionMode = (ushort)(myCheckBoxFixedFrame.IsChecked == true ? 1 : 0);
            applyScannerControlParam_(nPatternId);
        }

        private void myTbScanSettings_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            commitScanSettingTextBox_(sender as TextBox);
        }

        private void myTbScanSettings_LostFocus(object sender, RoutedEventArgs e)
        {
            commitScanSettingTextBox_(sender as TextBox);
        }

        private void myBtApply_Click(object sender, RoutedEventArgs e)
        {
            int nPatternId = myCbPatternID.SelectedIndex;
            if (nPatternId < 0)
            {
                return;
            }

            applyScannerControlParam_(nPatternId);
            _scanTestModel.SaveConfigToIniFile();
        }

        #endregion  LSO Scanner Setting

        #region Galvano Move

        private void myTbGalvanoYPos_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            moveGalvanoPositionY_();
        }

        private void myBtGalvanoYPosMove_Click(object sender, RoutedEventArgs e)
        {
            moveGalvanoPositionY_();
        }

        #endregion Galvano Move

        #region LSO Focus Motor

        private StepMotorStatus _lsoFocusMotorStatus = new();

        private StepMotorPositionChanged _onLsoFocusPositionChanged;

        // 콜백에서 슬라이더 값을 갱신할 때 발생하는 ValueChanged가 모터를 다시 이동시키는 것을 막는 플래그.
        private bool _isLsoFocusSliderDragging;
        private bool _isLsoFocusPositionChanged;

        private void initLsoFocusMotor_()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.LsoFocus, out _lsoFocusMotorStatus);

            mySliderLsoFocus.Minimum = _lsoFocusMotorStatus.rangeMin;
            mySliderLsoFocus.Maximum = _lsoFocusMotorStatus.rangeMax;
            mySliderLsoFocus.TickFrequency = 100;

            myLbLsoFocusMin.Content = _lsoFocusMotorStatus.rangeMin.ToString();
            myLbLsoFocusMax.Content = _lsoFocusMotorStatus.rangeMax.ToString();

            _onLsoFocusPositionChanged = new StepMotorPositionChanged(this.OnLsoFocusPositionChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.LsoFocus, _onLsoFocusPositionChanged);

            DeviceMotors.MoveStepMotorPosition(MotorType.LsoFocus, _lsoFocusMotorStatus.currPos);
        }

        private void releaseLsoFocusMotor_()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.LsoFocus);
        }

        private void OnLsoFocusPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                myTbLsoFocusPos.Text = pos.ToString();
                myTbLsoFocusValue.Text = value.ToString("N1");

                if (pos != mySliderLsoFocus.Value)
                {
                    mySliderLsoFocus.Value = pos;
                    _isLsoFocusPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

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

        #region Internal Fixation

        private void mySliderFixationRow_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            setCurrentInternalFixation_();
        }

        private void mySliderFixationCol_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
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
            setPresetInternalFixation_(FixationTarget.FUNDUS);
        }

        private void myBtFixationMarcular_Click(object sender, RoutedEventArgs e)
        {
            setPresetInternalFixation_(FixationTarget.CENTER);
        }

        private void myBtFixationDisc_Click(object sender, RoutedEventArgs e)
        {
            setPresetInternalFixation_(FixationTarget.OPTIC_DISC);
        }

        #endregion Internal Fixation

        #region Align Guide

        private void myCheckAlignGuide_CheckedNUnchecked(object sender, RoutedEventArgs e)
        {
            myColorPreview.IsOverlayAlignGuide = myCheckAlignGuide.IsChecked == true;
        }

        #endregion Align Guide

        private void myBtCaptureHWTrig_Click(object sender, RoutedEventArgs e)
        {
            readyToCapture_(PreviewDisplayMode.REVIEW);

            if (IsColorCameraLive())
            {
                StopColorCameraHWTriggerLive();
            }

            StartColorCameraHWTriggerLive();

            bool bBegin = false;
            for (int i = 0; i < 20; ++i)
            {
                bBegin = LsoCamera.IsColorCameraStreaming(); // Color Camera가 Acqusition 중인지 확인
                if (bBegin)
                {
                    break;
                }
                Thread.Sleep(10);
            }

            LsoScanner.StartLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
        }

        private void myBtCaptureROI_Click(object sender, RoutedEventArgs e)
        {
            if (IsColorCameraLive())
            {
                PauseColorCameraLive();
            }

            readyToCapture_(PreviewDisplayMode.REVIEW_ROI);

            StartColorCameraOriginal();
        }

        private void myBtCaptureROISetting_Click(object sender, RoutedEventArgs e)
        {
            var window = new CaptureROISettingWindow(_scanTestModel)
            {
                Owner = this
            };

            System.Windows.Point pt = myBtCaptureROISetting.PointToScreen(new System.Windows.Point(0, 0));
            window.WindowStartupLocation = WindowStartupLocation.Manual;
            window.Left = pt.X + (myBtCaptureROISetting.ActualWidth / 2) - (window.Width / 2);
            window.Top = pt.Y + myBtCaptureROISetting.ActualHeight;

            int nFrameCount = ToInt(myTbSubFrame.Text);
            window.FrameCount = nFrameCount;

            window.ShowDialog();
        }
    }
}
