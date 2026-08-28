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

            CorneaCamera.StartPreview(_onCorneaLeftFrameCaptured, _onCorneaRightFrameCaptured, _onCorneaLowerFrameCaptured);
            cmbViewFormat.IsEnabled = false;
            cmbHorzFlip.IsEnabled = false;
            cmbVertFlip.IsEnabled = false;
            cmbOutputMode.IsEnabled = false;
            cmbTestPattern.IsEnabled = false;

            checkRollingBar.IsEnabled = false;
            checkTransparent.IsEnabled = false;
        }

        public void CloseCorneaCameraPreview()
        {
            CorneaCamera.ClosePreview();
            cmbViewFormat.IsEnabled = true;
            cmbHorzFlip.IsEnabled = true;
            cmbVertFlip.IsEnabled = true;
            cmbOutputMode.IsEnabled = true;
            cmbTestPattern.IsEnabled = true;

            checkRollingBar.IsEnabled = true;
            checkTransparent.IsEnabled = true;
        }

        public bool IsCorneaCameraPreviewing()
        {
            return CorneaCamera.IsPreviewing();
        }

        private void ReflectCorneaChannelControls()
        {
            var cameraType = GetCameraTypeSelected();

            var again = CorneaCamera.GetAnalogGain(cameraType);
            var dgain = CorneaCamera.GetDigitalGain(cameraType);
            var expos = CorneaCamera.GetExposureTime(cameraType);
            var mode = IrCameraOutputMode.Normal;
            var pattern = IrCameraTestPattern.Standard;
            var horzFlip = IrCameraMirrorMode.MirrorOff;
            var vertFlip = IrCameraMirrorMode.MirrorOff;
            int option = 0;

            CorneaCamera.GetOutputMode(cameraType, out mode, out pattern, out option);
            CorneaCamera.GetMirrorMode(cameraType, out horzFlip, out vertFlip);

            editCorneaAgain.Text = again.ToString("F0");
            editCorneaDgain.Text = dgain.ToString("F0");
            editCorneaExposure.Text = expos.ToString();

            cmbOutputMode.SelectedIndex = (int)mode;
            cmbTestPattern.SelectedIndex = GetIndexOfTestPattern(pattern);
            cmbHorzFlip.SelectedIndex = (int)horzFlip;
            cmbVertFlip.SelectedIndex = (int)vertFlip;

            var rolled = ((option & (int)IrCameraTestOption.RollingBar) == (int)IrCameraTestOption.RollingBar);
            checkRollingBar.IsChecked = rolled;
            var trans = ((option & (int)IrCameraTestOption.Transparent) == (int)IrCameraTestOption.Transparent);
            checkTransparent.IsChecked = trans;
            return;
        }

        private int GetIndexOfTestPattern(IrCameraTestPattern pattern)
        {
            switch (pattern)
            {
                case IrCameraTestPattern.Standard: return 0;
                case IrCameraTestPattern.TopBottomBar: return 1;
                case IrCameraTestPattern.LeftRightBar: return 2;
                case IrCameraTestPattern.Random: return 3;
                case IrCameraTestPattern.Square: return 4;
                case IrCameraTestPattern.Black: return 5;
            }
            return 0;
        }

        private void UpdateCorneaOutputMode()
        {
            var type = GetCameraTypeSelected();

            int mode = cmbOutputMode.SelectedIndex;
            var pattern = GetTestPatternSelected();
            var option = GetPatternOptionSelected();
            if (mode < 0 || pattern < 0)
            {
                return;
            }
            CorneaCamera.SetOutputMode(type, (IrCameraOutputMode)mode, pattern, option);
        }

        private void UpdateCorneaMirrorMode()
        {
            var type = GetCameraTypeSelected();

            int hflip = cmbHorzFlip.SelectedIndex;
            int vflip = cmbVertFlip.SelectedIndex;
            if (hflip < 0 || vflip < 0)
            {
                return;
            }
            CorneaCamera.SetMirrorMode(type, (IrCameraMirrorMode)hflip, (IrCameraMirrorMode)vflip);
        }

        private CameraType GetCameraTypeSelected()
        {
            int index = cmbCorneaChannel.SelectedIndex;
            if (index < 0)
            {
                throw new InvalidOperationException("No cornea camera channel selected.");
            }
            return index switch
            {
                0 => CameraType.IrCorneaLower,
                1 => CameraType.IrCorneaRight,
                2 => CameraType.IrCorneaLeft,
                _ => throw new ArgumentOutOfRangeException()
            };
        }

        private IrCameraTestPattern GetTestPatternSelected()
        {
            int index = cmbTestPattern.SelectedIndex;
            return index switch
            {
                0 => IrCameraTestPattern.Standard,
                1 => IrCameraTestPattern.TopBottomBar,
                2 => IrCameraTestPattern.LeftRightBar,
                3 => IrCameraTestPattern.Random,
                4 => IrCameraTestPattern.Square,
                5 => IrCameraTestPattern.Black,
                _ => IrCameraTestPattern.Standard
            };
        }

        private int GetPatternOptionSelected()
        {
            int option = 0;
            if (checkRollingBar.IsChecked == true)
            {
                option |= (int)IrCameraTestOption.RollingBar;
            }
            if (checkTransparent.IsChecked == true)
            {
                option |= (int)IrCameraTestOption.Transparent;
            }
            return option;
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
