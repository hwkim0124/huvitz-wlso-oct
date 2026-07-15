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
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class StageMotorWindow : Window
    {
        bool _isDraggingStarted = false;
        bool _isPositionChanged = false;

        public StageMotorWindow()
        {
            InitializeComponent();

            _swingChanged = new StepMotorPositionChanged(this.OnSwingPositionChanged);
            _xStageChanged = new StepMotorPositionChanged(this.OnXstagePositionChanged);
            _yStageChanged = new StepMotorPositionChanged(this.OnYstagePositionChanged);
            _zStageChanged = new StepMotorPositionChanged(this.OnZstagePositionChanged);
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            FetchAllMotorStatus();

            sliderSwing.Minimum = _msSwing.rangeMin;
            sliderSwing.Maximum = _msSwing.rangeMax;
            sliderSwing.TickFrequency = 10;

            sliderXstage.Minimum = _msXstage.rangeMin;
            sliderXstage.Maximum = _msXstage.rangeMax;
            sliderXstage.TickFrequency = 10;

            sliderYstage.Minimum = _msYstage.rangeMin;
            sliderYstage.Maximum = _msYstage.rangeMax;
            sliderYstage.TickFrequency = 10;

            sliderZstage.Minimum = _msZstage.rangeMin;
            sliderZstage.Maximum = _msZstage.rangeMax;
            sliderZstage.TickFrequency = 10;

            ConnectMotorCallbacks();

            DeviceMotors.MoveStepMotorPosition(MotorType.Swing, _msSwing.currPos);
            DeviceMotors.MoveStepMotorPosition(MotorType.StageX, _msXstage.currPos);
            DeviceMotors.MoveStepMotorPosition(MotorType.StageY, _msYstage.currPos);
            DeviceMotors.MoveStepMotorPosition(MotorType.StageZ, _msZstage.currPos);
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            ReleaseMotorCallbacks();
            DeviceMotors.StopChinrestMove();

            _timer.Stop();

            Thread.Sleep(500);
            Mouse.OverrideCursor = null;
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            /*
            if (ShowQuestion("Are you sure to close this window?") == false)
            {
                e.Cancel = true;
            }
            */
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }


        private void SliderSwing_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.Swing, (int)sliderSwing.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderSwing_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderSwing_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.Swing, (int)sliderSwing.Value);
        }
        private void BtnSwingOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.Swing);
        }

        private void BtnSwingSetAs_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnSwingBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editSwingValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.Swing, step);
        }

        private void BtnSwingFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editSwingValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.Swing, step);
        }

        private void SliderXstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageX, (int)sliderXstage.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderXstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderXstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageX, (int)sliderXstage.Value);
        }
        private void BtnXstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageX);
        }

        private void BtnXstageSetAs_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnXstageBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editXstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageX, step);
        }

        private void BtnXstageFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editXstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageX, step);
        }

        private void SliderYstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageY, (int)sliderYstage.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderYstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderYstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageY, (int)sliderYstage.Value);
        }
        private void BtnYstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageY);
        }

        private void BtnYstageSetAs_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnYstageBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editYstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageY, step);
        }

        private void BtnYstageFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editYstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageY, step);
        }

        private void SliderZstage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted && !_isPositionChanged)
            {
                DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageZ, (int)sliderZstage.Value);
            }
            _isPositionChanged = false;
        }

        private void SliderZstage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void SliderZstage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceMotors.MoveStepMotorPositionAsync(MotorType.StageZ, (int)sliderZstage.Value);
        }
        private void BtnZstageOrigin_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveStepMotorToOriginAsync(MotorType.StageZ);
        }

        private void BtnZstageSetAs_Click(object sender, RoutedEventArgs e)
        {

        }

        private void BtnZstageBwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editZstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * -1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageZ, step);
        }

        private void BtnZstageFwd_Click(object sender, RoutedEventArgs e)
        {
            int step = ToInt(editZstageValue.Text);
            step = Math.Max(Math.Min(step, 1000), 10) * +1;
            DeviceMotors.MoveStepMotorByPositionOffsetAsync(MotorType.StageZ, step);
        }

        private void BtnChinrestStop_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.StopChinrestMove();
        }

        private void BtnChinrestDown_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveChinrestDown();
        }

        private void BtnChinrestUp_Click(object sender, RoutedEventArgs e)
        {
            DeviceMotors.MoveChinrestUp();
        }
    }
}
