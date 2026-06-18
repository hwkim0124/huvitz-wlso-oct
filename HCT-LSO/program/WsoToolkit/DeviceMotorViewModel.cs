using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;
using WsoNativeLib;


namespace WsoToolkit
{
    using static WsoNativeLib.WsoDevice;
    internal class DeviceMotorViewModel
    {
        StepMotorStatus _octFocus = new();
        StepMotorStatus _octRefer = new();
        StepMotorStatus _octPolar = new();
        StepMotorStatus _lsoFocus = new();
        StepMotorStatus _retMirror = new();
        StepMotorStatus _octReferNd = new();
        StepMotorStatus _octAntLens = new();
        StepMotorStatus _swing = new();
        StepMotorStatus _stageX = new();
        StepMotorStatus _stageY = new();
        StepMotorStatus _stageZ = new();

        DispatcherTimer _timer = new DispatcherTimer();

        StepMotorPositionChanged _octFocusChanged;
        StepMotorPositionChanged _octReferChanged;
        StepMotorPositionChanged _octPolarChanged;
        StepMotorPositionChanged _lsoFocusChanged;
        StepMotorPositionChanged _retMirrorChanged;
        StepMotorPositionChanged _octReferNdChanged;
        StepMotorPositionChanged _octAntLensChanged;
        StepMotorPositionChanged _swingChanged;
        StepMotorPositionChanged _xStageChanged;
        StepMotorPositionChanged _yStageChanged;
        StepMotorPositionChanged _zStageChanged;

        private void FetchAllMotorStatus()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.OctFocus, out _octFocus);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctRefer, out _octRefer);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctPolar, out _octPolar);
            DeviceMotors.FetchStepMotorStatus(MotorType.LsoFocus, out _lsoFocus);
            DeviceMotors.FetchStepMotorStatus(MotorType.RetMirror, out _retMirror);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctReferND, out _octReferNd);
            DeviceMotors.FetchStepMotorStatus(MotorType.OctAntLens, out _octAntLens);
            DeviceMotors.FetchStepMotorStatus(MotorType.Swing, out _swing);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageX, out _stageX);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageY, out _stageY);
            DeviceMotors.FetchStepMotorStatus(MotorType.StageZ, out _stageZ);
        }

    }
}
