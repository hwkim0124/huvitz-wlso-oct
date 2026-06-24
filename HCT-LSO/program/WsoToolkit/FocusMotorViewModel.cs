using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Threading;
using WsoNativeLib;

namespace WsoToolkit
{
    using static WsoNativeLib.WsoDevice;
    public partial class FocusMotorWindow
    {
        StepMotorStatus _msOctFocus = new();
        StepMotorStatus _msOctRefer = new();
        StepMotorStatus _msOctPolar = new();
        StepMotorStatus _msOctRefNd = new();
        StepMotorStatus _msLsoFocus = new();
        StepMotorStatus _msRetMirror = new();
        StepMotorStatus _msOctAntLens = new();
        StepMotorStatus _msLsoFilter = new();

        DispatcherTimer _timer = new DispatcherTimer();

        StepMotorPositionChanged _octFocusChanged;
        StepMotorPositionChanged _octReferChanged;
        StepMotorPositionChanged _octPolarChanged;
        StepMotorPositionChanged _octRefNdChanged;
        StepMotorPositionChanged _lsoFocusChanged;
        StepMotorPositionChanged _retMirrorChanged;
        StepMotorPositionChanged _octAntLensChanged;
        StepMotorPositionChanged _lsoFilterChanged;

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
    }
}
