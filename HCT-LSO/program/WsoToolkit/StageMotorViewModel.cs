using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;
using WsoNativeLib;


namespace WsoToolkit
{
    using static WsoNativeLib.WsoDevice;
    public partial class StageMotorWindow
    {
        StepMotorStatus _msSwing = new();
        StepMotorStatus _msXstage = new();
        StepMotorStatus _msYstage = new();
        StepMotorStatus _msZstage = new();

        DispatcherTimer _timer = new DispatcherTimer();

        StepMotorPositionChanged _swingChanged;
        StepMotorPositionChanged _xStageChanged;
        StepMotorPositionChanged _yStageChanged;
        StepMotorPositionChanged _zStageChanged;


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

        private void ReleaseMotorCallbacks()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.Swing);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageX);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageY);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageZ);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Callbacks
        /// 
        private void OnSwingPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editSwingPos.Text = pos.ToString();
                if (pos != sliderSwing.Value)
                {
                    sliderSwing.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnXstagePositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editXstagePos.Text = pos.ToString();
                if (pos != sliderXstage.Value)
                {
                    sliderXstage.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnYstagePositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editYstagePos.Text = pos.ToString();
                if (pos != sliderYstage.Value)
                {
                    sliderYstage.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnZstagePositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editZstagePos.Text = pos.ToString();
                if (pos != sliderZstage.Value)
                {
                    sliderZstage.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }
    }
}
