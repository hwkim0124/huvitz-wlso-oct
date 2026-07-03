using System;
using System.Collections.Generic;
using System.Text;
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
            releaseLsoFocusMotor_();
        }

        #region Scan Mode

        private void myBtStartScan_Click(object sender, RoutedEventArgs e)
        {

        }

        private void myBtCapture_Click(object sender, RoutedEventArgs e)
        {
            //if (IsRetinaCameraPreviewing())
            //{
            //    CloseRetinaCameraPreview();
            //    closeCorneaCameraPreview_();
            //    myBtStartScan.Content = "Start Scan";
            //}

            readyToCapture_(PreviewDisplayMode.REVIEW);

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

        }

        #endregion Scan Mode

        #region Setting 

        private void myBtSettingColorCamera_Click(object sender, RoutedEventArgs e)
        {
            var window = new ColorCameraSettingWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        #endregion Setting 

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

        private void initFixation_()
        {
            mySliderFixationRow.Minimum = FIXATION_ROW_MIN;
            mySliderFixationRow.Maximum = FIXATION_ROW_MAX;
            mySliderFixationCol.Minimum = FIXATION_COL_MIN;
            mySliderFixationCol.Maximum = FIXATION_COL_MAX;

            setPresetInternalFixation_(FixationTarget.FUNDUS);
        }

        private void getCurrentInternalFixation_()
        {
            if (Fixation.GetCurrentInternalFixation(out int row, out int col))
            {
                mySliderFixationRow.Value = row;
                mySliderFixationCol.Value = col;
                myTbFixationRow.Text = row.ToString();
                myTbFixationCol.Text = col.ToString();
            }
        }

        private void setCurrentInternalFixation_()
        {
            int row = (int)mySliderFixationRow.Value;
            int col = (int)mySliderFixationCol.Value;
            Fixation.TurnOnInternalFixation(row, col);
            getCurrentInternalFixation_();
        }

        private void setPresetInternalFixation_(FixationTarget target)
        {
            EyeSide side = myRbSideOD.IsChecked == true ? EyeSide.OD : EyeSide.OS;
            Fixation.TurnOnInternalFixationWithTarget(side, target);
            getCurrentInternalFixation_();
        }

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
    }
}
