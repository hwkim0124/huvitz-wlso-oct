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

        // Frame coalescing: keep at most one frame in flight so the dispatcher queue
        // cannot back up under a fast frame stream.
        private volatile bool _corneaLeftPending = false;
        private volatile bool _corneaRightPending = false;
        private volatile bool _corneaLowerPending = false;

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
            corneaPreview1.CameraType = CameraType.IrCorneaLeft;
            corneaPreview2.CameraType = CameraType.IrCorneaRight;
            corneaPreview3.CameraType = CameraType.IrCorneaLower;

            corneaPreview1.Callback = _onCorneaLeftFrameCaptured;
            corneaPreview2.Callback = _onCorneaRightFrameCaptured;
            corneaPreview3.Callback = _onCorneaLowerFrameCaptured;

            corneaPreview1.Play();
            corneaPreview2.Play();
            corneaPreview3.Play();
        }

        public void CloseCorneaCameraPreview()
        {
            corneaPreview1.Stop();
            corneaPreview2.Stop();
            corneaPreview3.Stop();
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
        private void OnCorneaCameraLeftFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Count the true capture rate before the coalescing gate, so the preview's
            // FPS reflects the camera rate rather than the throttled update rate.
            corneaPreview1.MarkFrameCaptured();
            if (_corneaLeftPending) return;

            _corneaLeftPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview1.CallbackCorneaCameraFrame(data, width, height);
                _corneaLeftPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraRightFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            corneaPreview2.MarkFrameCaptured();
            if (_corneaRightPending) return;

            _corneaRightPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview2.CallbackCorneaCameraFrame(data, width, height);
                _corneaRightPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraLowerFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            corneaPreview3.MarkFrameCaptured();
            if (_corneaLowerPending) return;

            _corneaLowerPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview3.CallbackCorneaCameraFrame(data, width, height);
                _corneaLowerPending = false;
            }, DispatcherPriority.Background);
        }
    }
}
