using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;
using WsoNativeLib;


namespace WsoToolkit
{
    using static WsoNativeLib.WsoDevice;
    public partial class DeviceMotorWindow
    {
        StepMotorStatus _msOctFocus = new();
        StepMotorStatus _msOctRefer = new();
        StepMotorStatus _msOctPolar = new();
        StepMotorStatus _msOctRefNd = new();
        StepMotorStatus _msLsoFocus = new();
        StepMotorStatus _msRetMirror = new();
        StepMotorStatus _msOctAntLens = new();
        StepMotorStatus _msLsoFilter = new();
        StepMotorStatus _msSwing = new();
        StepMotorStatus _msXstage = new();
        StepMotorStatus _msYstage = new();
        StepMotorStatus _msZstage = new();

        DispatcherTimer _timer = new DispatcherTimer();

        StepMotorPositionChanged _octFocusChanged;
        StepMotorPositionChanged _octReferChanged;
        StepMotorPositionChanged _octPolarChanged;
        StepMotorPositionChanged _octRefNdChanged;
        StepMotorPositionChanged _lsoFocusChanged;
        StepMotorPositionChanged _retMirrorChanged;
        StepMotorPositionChanged _octAntLensChanged;
        StepMotorPositionChanged _lsoFilterChanged;
        StepMotorPositionChanged _swingChanged;
        StepMotorPositionChanged _xStageChanged;
        StepMotorPositionChanged _yStageChanged;
        StepMotorPositionChanged _zStageChanged;

        private void FetchAllMotorStatus()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.OctFocus, out _msOctFocus);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctRefer, out _msOctRefer);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctPolar, out _msOctPolar);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctRefNd, out _msOctRefNd);
            DeviceMotors.FetchStepMotorStatus(MotorType.LsoFocus, out _msLsoFocus);
            DeviceMotors.FetchStepMotorStatus(MotorType.RetMirror, out _msRetMirror);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctAntLens, out _msOctAntLens);
            DeviceMotors.FetchStepMotorStatus(MotorType.LsoFilter, out _msLsoFilter);
            DeviceMotors.FetchStepMotorStatus(MotorType.Swing, out _msSwing);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageX, out _msXstage);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageY, out _msYstage);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageZ, out _msZstage);
        }

        private void ConnectMotorCallbacks()
        {
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.OctFocus, _octFocusChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.OctRefer, _octReferChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.OctPolar, _octPolarChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.OctRefNd, _octRefNdChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.LsoFocus, _lsoFocusChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.RetMirror, _retMirrorChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.OctAntLens, _octAntLensChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.LsoFilter, _lsoFilterChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.Swing, _swingChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageX, _xStageChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageY, _yStageChanged);
            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.StageZ, _zStageChanged);
        }

        private void ReleaseMotorCallbacks()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.OctFocus);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.OctRefer);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.OctPolar);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.OctRefNd);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.LsoFocus);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.RetMirror);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.OctAntLens);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.LsoFilter);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.Swing);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageX);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageY);
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.StageZ);
        }

        private void StopAllMotors()
        {
            DeviceMotors.StopStepMotor(MotorType.OctFocus);
            DeviceMotors.StopStepMotor(MotorType.OctRefer);
            DeviceMotors.StopStepMotor(MotorType.OctPolar);
            DeviceMotors.StopStepMotor(MotorType.OctRefNd);
            DeviceMotors.StopStepMotor(MotorType.LsoFocus);
            DeviceMotors.StopStepMotor(MotorType.RetMirror);
            DeviceMotors.StopStepMotor(MotorType.OctAntLens);
            DeviceMotors.StopStepMotor(MotorType.LsoFilter);
            DeviceMotors.StopStepMotor(MotorType.Swing);
            DeviceMotors.StopStepMotor(MotorType.StageX);
            DeviceMotors.StopStepMotor(MotorType.StageY);
            DeviceMotors.StopStepMotor(MotorType.StageZ);

            AlignStage.StopChinrest();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Callbacks
        /// 
        private void OnOctFocusPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editOctFocusPos.Text = pos.ToString();
                editOctFocusValue.Text = value.ToString("N1");
                if (pos != sliderOctFocus.Value)
                {
                    sliderOctFocus.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnOctReferPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editOctReferPos.Text = pos.ToString();
                if (pos != sliderOctRefer.Value)
                {
                    sliderOctRefer.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnOctPolarPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editOctPolarPos.Text = pos.ToString();
                editOctPolarValue.Text = value.ToString("N0");
                if (pos != sliderOctPolar.Value)
                {
                    sliderOctPolar.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnOctRefNdPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editOctRefNdPos.Text = pos.ToString();
                if (pos != sliderOctRefNd.Value)
                {
                    sliderOctRefNd.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnLsoFocusPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editLsoFocusPos.Text = pos.ToString();
                editLsoFocusValue.Text = value.ToString("N1");
                if (pos != sliderLsoFocus.Value)
                {
                    sliderLsoFocus.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnRetMirrorPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editRetMirrorPos.Text = pos.ToString();
                if (pos != sliderRetMirror.Value)
                {
                    sliderRetMirror.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnOctAntLensPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editOctAntLensPos.Text = pos.ToString();
                if (pos != sliderOctAntLens.Value)
                {
                    sliderOctAntLens.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        private void OnLsoFilterPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                editLsoFilterPos.Text = pos.ToString();
                if (pos != sliderLsoFilter.Value)
                {
                    sliderLsoFilter.Value = pos;
                    _isPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

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

        private void StartRefreshTimer()
        {
            _timer.Interval = TimeSpan.FromMilliseconds(100.0);
            _timer.Tick += TimerRefreshTick;
            _timer.Start();
        }

        private void TimerRefreshTick(object? sender, EventArgs e)
        {
            Func<bool, string> boolToIntStr = value => value ? "1" : "0";

            // editOdOs.Text = boolToIntStr(DeviceMotors.IsOdOsStatusFlagSet());
            editCrestPi.Text = boolToIntStr(AlignStage.IsChinrestAtUpperEnd()) + " | " + boolToIntStr(AlignStage.IsChinrestAtLowerEnd());
            editXstagePi.Text = boolToIntStr(DeviceMotors.IsXstageMotorAtPiPosition());
            editYstagePi.Text = boolToIntStr(DeviceMotors.IsYstageMotorAtUpperLimit()) + " | " + boolToIntStr(DeviceMotors.IsYstageMotorAtLowerLimit());
            editZstagePi.Text = boolToIntStr(DeviceMotors.IsZstageMotorAtPiPosition());
            editSwingPi.Text = boolToIntStr(DeviceMotors.IsSwingMotorAtPiPosition());

            editLsoFocusPi.Text = boolToIntStr(DeviceMotors.IsLsoFocusMotorAtPiPosition());
            editLsoFilterPi.Text = boolToIntStr(DeviceMotors.IsLsoFilterMotorAtPiPosition());
            editOctPolarPi.Text = boolToIntStr(DeviceMotors.IsOctPolarMotorAtPiPosition());
            editOctReferPi.Text = boolToIntStr(DeviceMotors.IsOctReferMotorAtPiPosition());
            editOctFocusPi.Text = boolToIntStr(DeviceMotors.IsOctFocusMotorAtPiPosition());
            editMirrorPi.Text = boolToIntStr(DeviceMotors.IsRetMirrorMotorAtPiPosition());
        }
    }
}
