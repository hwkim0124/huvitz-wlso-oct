using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.controls;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class IrCameraTestWindow
    {
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLeftFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaRightFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLowerFrameCaptured;

        LightControlWindow? _lightControlWindow = null;
        FocusMotorWindow? _focusMotorWindow = null;
        StageMotorWindow? _stageMotorWindow = null;

        private void InitializeWindowControls()
        {
            GetCorneaCameraConfig();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Cornea Camera
        /// 
        public void StartCorneaCameraPreview()
        {
            CorneaCamera.StartPreview(CameraType.IrCorneaLeft, _onCorneaLeftFrameCaptured);
            CorneaCamera.StartPreview(CameraType.IrCorneaRight, _onCorneaRightFrameCaptured);
            CorneaCamera.StartPreview(CameraType.IrCorneaLower, _onCorneaLowerFrameCaptured);
        }

        public void CloseCorneaCameraPreview()
        {
            CorneaCamera.ClosePreview(CameraType.IrCorneaLeft);
            CorneaCamera.ClosePreview(CameraType.IrCorneaRight);
            CorneaCamera.ClosePreview(CameraType.IrCorneaLower);
        }

        public bool IsCorneaCameraLeftPreviewing()
        {
            return CorneaCamera.IsPreviewing(CameraType.IrCorneaLeft);
        }

        public bool IsCorneaCameraRightPreviewing()
        {
            return CorneaCamera.IsPreviewing(CameraType.IrCorneaRight);
        }

        public bool IsCorneaCameraLowerPreviewing()
        {
            return CorneaCamera.IsPreviewing(CameraType.IrCorneaLower);
        }

        private void GetCorneaCameraConfig()
        {
            var dgain = CorneaCamera.GetDigitalGain(CameraType.IrCorneaLeft);
            var again = CorneaCamera.GetAnalogGain(CameraType.IrCorneaLeft);
            editCorneaAgain1.Text = again.ToString("F1");
            editCorneaDgain1.Text = dgain.ToString("F1");

            dgain = CorneaCamera.GetDigitalGain(CameraType.IrCorneaRight);
            again = CorneaCamera.GetAnalogGain(CameraType.IrCorneaRight);
            editCorneaAgain2.Text = again.ToString("F1");
            editCorneaDgain2.Text = dgain.ToString("F1");

            dgain = CorneaCamera.GetDigitalGain(CameraType.IrCorneaLower);
            again = CorneaCamera.GetAnalogGain(CameraType.IrCorneaLower);
            editCorneaAgain3.Text = again.ToString("F1");
            editCorneaDgain3.Text = dgain.ToString("F1");
        }

        private void SetCorneaCameraConfig()
        {
            var again = ToFloat(editCorneaAgain1.Text);
            var dgain = ToFloat(editCorneaDgain1.Text);
            CorneaCamera.SetDigitalGain(CameraType.IrCorneaLeft, dgain);
            CorneaCamera.SetAnalogGain(CameraType.IrCorneaLeft, again);

            again = ToFloat(editCorneaAgain2.Text);
            dgain = ToFloat(editCorneaDgain2.Text);
            CorneaCamera.SetDigitalGain(CameraType.IrCorneaRight, dgain);
            CorneaCamera.SetAnalogGain(CameraType.IrCorneaRight, again);

            again = ToFloat(editCorneaAgain3.Text);
            dgain = ToFloat(editCorneaDgain3.Text);
            CorneaCamera.SetDigitalGain(CameraType.IrCorneaLower, dgain);
            CorneaCamera.SetAnalogGain(CameraType.IrCorneaLower, again);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Callbacks
        /// 
        private void OnOctScanProtocolCompleted(bool result)
        {
            Dispatcher.BeginInvoke(() =>
            {
            }, DispatcherPriority.Normal);
        }

        private void OnCorneaCameraLeftFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview1.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraRightFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview2.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraLowerFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview3.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }
    }
}
