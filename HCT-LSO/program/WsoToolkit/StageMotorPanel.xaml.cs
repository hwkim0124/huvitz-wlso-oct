using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class StageMotorPanel : UserControl
    {
        private readonly StepMotorPositionChanged _swingChanged;
        private readonly StepMotorPositionChanged _xStageChanged;
        private readonly StepMotorPositionChanged _yStageChanged;
        private readonly StepMotorPositionChanged _zStageChanged;

        private StepMotorStatus _msSwing = new();
        private StepMotorStatus _msXstage = new();
        private StepMotorStatus _msYstage = new();
        private StepMotorStatus _msZstage = new();

        private bool _isActive;
        private bool _isDraggingStarted;
        private bool _isUpdatingFromDevice;

        public StageMotorPanel()
        {
            _swingChanged = new StepMotorPositionChanged(OnSwingPositionChanged);
            _xStageChanged = new StepMotorPositionChanged(OnXstagePositionChanged);
            _yStageChanged = new StepMotorPositionChanged(OnYstagePositionChanged);
            _zStageChanged = new StepMotorPositionChanged(OnZstagePositionChanged);

            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (_isActive)
            {
                return;
            }

            FetchAllMotorStatus();

            configureSlider_(sliderSwing, editSwingPos, _msSwing);
            configureSlider_(sliderXstage, editXstagePos, _msXstage);
            configureSlider_(sliderYstage, editYstagePos, _msYstage);
            configureSlider_(sliderZstage, editZstagePos, _msZstage);

            ConnectMotorCallbacks();
            _isActive = true;
        }

        private void UserControl_Unloaded(object sender, RoutedEventArgs e)
        {
            if (!_isActive)
            {
                return;
            }

            _isActive = false;

            // Stop first while callbacks are still connected, then release them.
            StopAllMotors();
            ReleaseMotorCallbacks();
        }

        private static void configureSlider_(Slider slider, TextBox edit, StepMotorStatus status)
        {
            slider.Minimum = status.rangeMin;
            slider.Maximum = status.rangeMax;
            slider.TickFrequency = 10;
            slider.Value = status.currPos;
            edit.Text = status.currPos.ToString();
        }

        private void FetchAllMotorStatus()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.Swing, out _msSwing);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageX, out _msXstage);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageY, out _msYstage);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageZ, out _msZstage);
        }

        private void ConnectMotorCallbacks()
        {
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.Swing, _swingChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageX, _xStageChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageY, _yStageChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageZ, _zStageChanged);
        }

        private static void ReleaseMotorCallbacks()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.Swing);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageX);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageY);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageZ);
        }

        private static void StopAllMotors()
        {
            DeviceMotors.StopStepMotor(MotorType.Swing);
            DeviceMotors.StopStepMotor(MotorType.StageX);
            DeviceMotors.StopStepMotor(MotorType.StageY);
            DeviceMotors.StopStepMotor(MotorType.StageZ);
            AlignStage.StopChinrest();
        }

        private void SliderSwing_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.Swing, sliderSwing);
        }

        private void SliderXstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.StageX, sliderXstage);
        }

        private void SliderYstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.StageY, sliderYstage);
        }

        private void SliderZstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            moveSlider_(MotorType.StageZ, sliderZstage);
        }

        private void moveSlider_(MotorType type, Slider slider)
        {
            if (_isActive && !_isDraggingStarted && !_isUpdatingFromDevice)
            {
                DeviceMotors.MoveStepMotorPositionAsync(type, (int)slider.Value);
            }
        }

        private void SliderSwing_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderXstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderYstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderZstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderSwing_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.Swing, sliderSwing);
        }

        private void SliderXstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.StageX, sliderXstage);
        }

        private void SliderYstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.StageY, sliderYstage);
        }

        private void SliderZstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            completeDrag_(MotorType.StageZ, sliderZstage);
        }

        private void completeDrag_(MotorType type, Slider slider)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(type, (int)slider.Value);
        }

        private void BtnSwingOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.Swing);
        }

        private void BtnXstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageX);
        }

        private void BtnYstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageY);
        }

        private void BtnZstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageZ);
        }

        private void BtnSwingSetAs_Click(object sender, RoutedEventArgs e)
        {
        }

        private void BtnXstageSetAs_Click(object sender, RoutedEventArgs e)
        {
        }

        private void BtnYstageSetAs_Click(object sender, RoutedEventArgs e)
        {
        }

        private void BtnZstageSetAs_Click(object sender, RoutedEventArgs e)
        {
        }

        private void BtnSwingBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.Swing, editSwingValue, -1);
        }

        private void BtnSwingFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.Swing, editSwingValue, 1);
        }

        private void BtnXstageBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageX, editXstageValue, -1);
        }

        private void BtnXstageFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageX, editXstageValue, 1);
        }

        private void BtnYstageBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageY, editYstageValue, -1);
        }

        private void BtnYstageFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageY, editYstageValue, 1);
        }

        private void BtnZstageBwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageZ, editZstageValue, -1);
        }

        private void BtnZstageFwd_Click(object sender, RoutedEventArgs e)
        {
            moveBy_(MotorType.StageZ, editZstageValue, 1);
        }

        private static void moveBy_(MotorType type, TextBox edit, int direction)
        {
            int step = Math.Clamp(ToInt(edit.Text), 10, 1000) * direction;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(type, step);
        }

        private void EditSwingPos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, editSwingPos, sliderSwing);
        }

        private void EditXstagePos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, editXstagePos, sliderXstage);
        }

        private void EditYstagePos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, editYstagePos, sliderYstage);
        }

        private void EditZstagePos_KeyDown(object sender, KeyEventArgs e)
        {
            setPositionOnEnter_(e, editZstagePos, sliderZstage);
        }

        private static void setPositionOnEnter_(KeyEventArgs e, TextBox edit, Slider slider)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            int value = ToInt(edit.Text);
            if (value >= slider.Minimum && value <= slider.Maximum)
            {
                slider.Value = value;
            }
        }

        private void BtnChinrestStop_Click(object sender, RoutedEventArgs e)
        {
            AlignStage.StopChinrest();
        }

        private void BtnChinrestDown_Click(object sender, RoutedEventArgs e)
        {
            AlignStage.MoveChinrestDown();
        }

        private void BtnChinrestUp_Click(object sender, RoutedEventArgs e)
        {
            AlignStage.MoveChinrestUp();
        }

        private void OnSwingPositionChanged(int pos, float value)
        {
            updatePosition_(sliderSwing, editSwingPos, pos);
        }

        private void OnXstagePositionChanged(int pos, float value)
        {
            updatePosition_(sliderXstage, editXstagePos, pos);
        }

        private void OnYstagePositionChanged(int pos, float value)
        {
            updatePosition_(sliderYstage, editYstagePos, pos);
        }

        private void OnZstagePositionChanged(int pos, float value)
        {
            updatePosition_(sliderZstage, editZstagePos, pos);
        }

        private void updatePosition_(Slider slider, TextBox edit, int pos)
        {
            if (!_isActive)
            {
                return;
            }

            Dispatcher.BeginInvoke(() =>
            {
                if (!_isActive)
                {
                    return;
                }

                _isUpdatingFromDevice = true;
                try
                {
                    edit.Text = pos.ToString();
                    if (pos != slider.Value)
                    {
                        slider.Value = pos;
                    }
                }
                finally
                {
                    _isUpdatingFromDevice = false;
                }
            }, DispatcherPriority.Normal);
        }
    }
}
