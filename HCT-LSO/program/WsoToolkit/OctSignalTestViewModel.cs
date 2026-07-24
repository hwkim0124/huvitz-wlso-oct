using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using WsoNativeLib;

namespace WsoToolkit
{
    using static WsoDevice;
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;

    public partial class OctSignalTestWindow
    {
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLeftFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaRightFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaRetinaFrameCaptured;
        WsoCallback.JoystickButtonPressed _onJoystickButtonPressed;
        WsoCallback.OptimizeButtonPressed _onOptimizeButtonPressed;

        WsoCallback.OctScanPreviewImageCaptured _onOctPreviewCaptured;
        WsoCallback.OctScanProtocolCompleted _onProtocolCompleted;
        WsoCallback.OctSpectrumDataCaptured _onSpectrumDataCaptured;
        WsoCallback.OctResampleDataCaptured _onResampleDataCaptured;
        WsoCallback.OctIntensityDataCaptured _onIntensityDataCaptured;

        WsoCallback.OctAutoScanOptimized _onAutoScanOptimized;
        WsoCallback.OctAutoFocusOptimized _onAutoFocusOptimized;
        WsoCallback.OctAutoPolarOptimized _onAutoPolarOptimized;
        WsoCallback.OctAutoReferOptimized _onAutoReferOptimized;

        WsoOctScan.OctProtocolInitParam _octInitParam = new();
        WsoOctCalib.OctDispersionParam _disperParam = new();
        WsoOctCalib.OctSpectrometerParam _spectroParam = new();

        public bool IsScanOnRetina { get; set; } = true;
        private bool _isDisposePeakByRefer = false;
        private int _disposePeakDiffStep = 0;
        private int _disposePeakCount = 0;

        // Intensity-graph frame coalescing: keep at most one frame in flight so the
        // dispatcher queue cannot back up. Reused across frames (only one in flight).
        private volatile bool _intensityPending = false;
        private volatile bool _octPreviewPending = false;
        private volatile bool _octSpectrumPending = false;
        private volatile bool _corenaLeftPending = false;
        private volatile bool _corneaRightPending = false;
        private volatile bool _retinaPending = false;
        private float[] _intensityColumn = Array.Empty<float>();
        private byte[] _octPreviewBuffer = Array.Empty<byte>();

        private void InitializeViewModel()
        {

        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Scanning
        /// 
        public void PrepareOctScanPreview()
        {
            RetreiveOctScanInitParam();

            OctScanning.ConnectSpectrumDataCaptured(_onSpectrumDataCaptured);
            OctScanning.ConnectResampleDataCaptured(_onResampleDataCaptured);
            OctScanning.ConnectIntensityDataCaptured(_onIntensityDataCaptured);

            OctScanner.GetOctDetectorWavelengths(out double[] wlens);
            octSpectrumGraph.SetDetectorWavelengths(wlens);
        }

        public bool StartOctScanPreview()
        {
            PrepareOctScanPreview();
            SetupIntensityGraphDisplay();

            if (!OctScanning.SetupScanProtocol(_octInitParam))
            {
                ShowError("Failed to setup scan protocol!");
                return false;
            }
            if (!OctScanning.StartPreview(_onOctPreviewCaptured, null))
            {
                ShowError("Failed to start scan preview!");
                return false;
            }

            StartCorneaCameraPreview();
            octScanImagePreview.IsPreviewMode = true;
            return true;
        }

        public void CaptureOctScanning()
        {
            if (OctScanning.IsPreviewing())
            {
                CancelOctScanning(true);
            }
        }

        public void CancelOctScanning(bool capture)
        {
            OctScanning.ClosePreview(capture, _onProtocolCompleted);

            OctScanner.IsFFTZeroPaddingEnabled(true, false);
            OctScanner.IsRefreshPhaseShiftParamEnabled(true, false);

            OctScanning.ReleaseResampleDataCaptured();
            OctScanning.ReleaseSpectrumDataCaptured();
            OctScanning.ReleaseIntensityDataCaptured();
        }

        public bool IsOctScanning()
        {
            return OctScanning.IsWorking();
        }

        public void OptimizeOctPreview()
        {
            if (OctScanning.IsPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (OctScanning.IsAutoOptimizing())
                {
                    if (OctScanning.StartAutoOptimize(_onAutoScanOptimized))
                    {
                        octScanImagePreview.IsOptimizing = true;
                        return;
                    }
                    else
                    {
                        ShowError("Failed to start auto optimize!");
                    }
                }
                else
                {
                    octScanImagePreview.IsOptimizing = false;
                    OctScanning.CancelAutoOptimizing();
                }
                Mouse.OverrideCursor = null;
            }
        }

        public void OptimizeAutoReference()
        {
            if (OctScanning.IsPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (!OctScanning.IsAutoOptimizing())
                {
                    if (OctScanning.StartAutoReference(_onAutoReferOptimized))
                    {
                        octScanImagePreview.IsOptimizing = true;
                        return;
                    }
                    else
                    {
                        ShowError("Failed to start auto reference optimize!");
                    }
                }
                else
                {
                    octScanImagePreview.IsOptimizing = false;
                    OctScanning.CancelAutoOptimizing();
                }
                Mouse.OverrideCursor = null;
            }
        }

        public void OptimizeAuoPolarization()
        {
            if (OctScanning.IsPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (!OctScanning.IsAutoOptimizing())
                {
                    if (OctScanning.StartAutoPolarization(_onAutoPolarOptimized))
                    {
                        octScanImagePreview.IsOptimizing = true;
                        return;
                    }
                    else
                    {
                        ShowError("Failed to start auto polar optimize!");
                    }
                }
                else
                {
                    octScanImagePreview.IsOptimizing = false;
                    OctScanning.CancelAutoOptimizing();
                }
                Mouse.OverrideCursor = null;
            }
        }

        public void OptimizeAutoFocus()
        {
            if (OctScanning.IsPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (!OctScanning.IsAutoOptimizing())
                {
                    if (OctScanning.StartAutoDiopterFocus(_onAutoFocusOptimized))
                    {
                        octScanImagePreview.IsOptimizing = true;
                        return;
                    }
                    else
                    {
                        ShowError("Failed to start auto focus optimize!");
                    }
                }
                else
                {
                    octScanImagePreview.IsOptimizing = false;
                    OctScanning.CancelAutoOptimizing();
                }
                Mouse.OverrideCursor = null;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Cornea Camera
        /// 
        public void StartCorneaCameraPreview()
        {
            corneaPreview1.CameraType = CameraType.IrCorneaLeft;
            corneaPreview2.CameraType = CameraType.IrCorneaRight;
            retinaPreview.CameraType = CameraType.IrRetina;

            corneaPreview1.Callback = _onCorneaLeftFrameCaptured;
            corneaPreview2.Callback = _onCorneaRightFrameCaptured;
            retinaPreview.Callback = _onCorneaRetinaFrameCaptured;

            corneaPreview1.Play();
            corneaPreview2.Play();
            retinaPreview.Play();
        }

        public void CloseCorneaCameraPreview()
        {
            corneaPreview1.Stop();
            corneaPreview2.Stop();
            retinaPreview.Stop();
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Retrieve user selections from GUI controls.
        /// 
        public void RetreiveOctScanInitParam()
        {
            _octInitParam = new();

            var index = cmbScanPattern.SelectedIndex;
            _octInitParam.patternCode = index switch
            {
                0 => (IsScanOnRetina == true ? WsoOctDefs.OctPatternCode.MACULAR_POINT : WsoOctDefs.OctPatternCode.CORNEA_POINT),
                _ => (IsScanOnRetina == true ? WsoOctDefs.OctPatternCode.MACULAR_LINE : WsoOctDefs.OctPatternCode.CORNEA_LINE)
            };
            _octInitParam.previewType = index == 0 ? WsoOctDefs.OctPatternType.POINT : WsoOctDefs.OctPatternType.LINE;

            _octInitParam.scanPoints = ToInt(cmbScanPoints.SelectedItem.ToString());
            _octInitParam.scanLines = 1;
            _octInitParam.scanDirection = index != 2 ? WsoOctDefs.OctScanDirection.X_TO_Y : WsoOctDefs.OctScanDirection.Y_TO_X;
            _octInitParam.scanRangeX = index != 2 ? ToFloat(cmbScanRange.SelectedItem.ToString()) : 0.0f;
            _octInitParam.scanRangeY = index != 2 ? 0.0f : ToFloat(cmbScanRange.SelectedItem.ToString());
            _octInitParam.scanSpeed = radioCameraSpeed1.IsChecked == true ? WsoOctDefs.OctScanSpeed.SLOW : radioCameraSpeed2.IsChecked == true ? WsoOctDefs.OctScanSpeed.NORMAL : WsoOctDefs.OctScanSpeed.FAST;

            _octInitParam.scanOffsetX = ToFloat(cmbScanOffsetX.SelectedItem.ToString());
            _octInitParam.scanOffsetY = ToFloat(cmbScanOffsetY.SelectedItem.ToString());
            _octInitParam.scanScaleX = ToFloat(editScanScaleX.Text);
            _octInitParam.scanScaleY = ToFloat(editScanScaleY.Text);

            _octInitParam.isFirstScanOnSide = false;
            _octInitParam.isNotImageGrabbing = checkNotGrabbing.IsChecked == true;

            var multi = ToInt(cmbZeroPadding.SelectedItem.ToString());
            var isZeroPadding = multi > 1;
            OctScanner.IsFFTZeroPaddingEnabled(true, isZeroPadding, multi);
            OctScanner.IsRefreshPhaseShiftParamEnabled(true, true);
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Graph Dispaly
        /// 
        public void SetupIntensityGraphDisplay()
        {
            octIntensityGraph.AxisY_Min = ToInt(cmbPeakRangeMin.SelectedItem.ToString());
            octIntensityGraph.AxisY_Max = ToInt(cmbPeakRangeMax.SelectedItem.ToString());
            octIntensityGraph.AxisX_Min = ToInt(editDataRangeMin.Text);
            octIntensityGraph.AxisX_Max = ToInt(editDataRangeMax.Text);
            octIntensityGraph.FindPeakOffset1 = ToInt(editPeakOffset1.Text);
            octIntensityGraph.FindPeakOffset2 = ToInt(editPeakOffset2.Text);

            var noise = ToFloat(editNoiseLevel.Text);
            octIntensityGraph.FixedNoiseLevel = noise;

            var zeros = ToInt(cmbZeroPadding.SelectedItem.ToString());
            octIntensityGraph.MultipleOfZeros = zeros;

            if (!IsOctScanning())
            {
                var size = Math.Min(Math.Max(ToInt(editAverageSize.Text), 1), 100);
                octIntensityGraph.SetAveragingSize(size);
            }

            octIntensityGraph.UpdatePlotAxisRange();
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Capture Control
        /// 
        public void DisposePeakPositionByReference()
        {
            int topIndex = octIntensityGraph.GetPeakTopIndex();

            switch (_disposePeakDiffStep)
            {
                case 0:
                    {
                        int diff = Math.Abs(topIndex - 26);
                        if (diff <= 2)
                        {
                            _isDisposePeakByRefer = false;
                            ShowInfo("Peak position at 26 is found!");
                            Mouse.OverrideCursor = null;
                        }
                        else
                        {
                            int offset = (diff <= 20 ? -1 : (diff <= 100 ? -5 : -30));
                            DeviceMotors.MoveStepMotorByPositionOffset(WsoDevice.MotorType.OctRefer, offset);
                        }
                    }
                    break;
                case 1:
                    {
                        int diff = Math.Abs(topIndex - 51);
                        if (diff <= 2)
                        {
                            _isDisposePeakByRefer = false;
                            ShowInfo("Peak position at 51 is found!");
                            Mouse.OverrideCursor = null;
                        }
                        else
                        {
                            int offset = (diff <= 20 ? -1 : (diff <= 100 ? -5 : -30));
                            DeviceMotors.MoveStepMotorByPositionOffset(WsoDevice.MotorType.OctRefer, offset);
                        }
                    }
                    break;
                case 2:
                    {
                        int diff = Math.Abs(topIndex - 381);
                        if (diff <= 2)
                        {
                            _isDisposePeakByRefer = false;
                            ShowInfo("Peak position at 381 is found!");
                            Mouse.OverrideCursor = null;
                        }
                        else
                        {
                            int offset = (diff <= 20 ? 1 : (diff <= 100 ? 5 : 30));
                            DeviceMotors.MoveStepMotorByPositionOffset(WsoDevice.MotorType.OctRefer, offset);
                        }
                    }
                    break;
                case 3:
                    {
                        int diff = Math.Abs(topIndex - 751);
                        if (diff <= 2)
                        {
                            _isDisposePeakByRefer = false;
                            ShowInfo("Peak position at 751 is found!");
                            Mouse.OverrideCursor = null;
                        }
                        else
                        {
                            int offset = (diff <= 20 ? 1 : (diff <= 100 ? 5 : 30));
                            DeviceMotors.MoveStepMotorByPositionOffset(WsoDevice.MotorType.OctRefer, offset);
                        }
                    }
                    break;
            }

            _disposePeakCount += 1;
            if (_disposePeakCount > 100)
            {
                ShowError("Failed to find peak position!");
                _isDisposePeakByRefer = false;
                _disposePeakCount = 0;
                Mouse.OverrideCursor = null;
            }
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Scanning Calibration
        /// 
        public WsoOctCalib.OctDispersionParam GetDispersionParam()
        {
            OctScanner.ObtainOctDispersionParam(ref _disperParam);
            return _disperParam;
        }

        public bool SetDispersionParam(WsoOctCalib.OctDispersionParam param)
        {
            if (OctScanner.SubmitOctDispersionParam(param))
            {
                _disperParam = param;
                return true;
            }
            return false;
        }

        public WsoOctCalib.OctSpectrometerParam GetSpectrometerParam()
        {
            OctScanner.ObtainOctSpectrometerParam(ref _spectroParam);
            return _spectroParam;
        }

        public bool SetSpectrometerParam(WsoOctCalib.OctSpectrometerParam param)
        {
            if (OctScanner.SubmitOctSpectrometerParam(param))
            {
                _spectroParam = param;
                return true;
            }
            return false;
        }


        /////////////////////////////////////////////////////////////////////////////////////////
        /// Callbacks
        /// 
        private void OnOctScanProtocolCompleted(bool result)
        {
            Dispatcher.BeginInvoke(() =>
            {
                btnStartScan.Content = "Start Scan";
                EnableButtonsAtCaptured();
                octScanImagePreview.IsPreviewMode = false;
                Mouse.OverrideCursor = null;
            }, DispatcherPriority.Normal);
        }

        private void OnOctScanPreviewImageCaptured(nint data, int width, int height, float quality, float snr_ratio, int ref_point, int index_image)
        {
            if (data == 0) return;

            // Count the true capture rate before the coalescing gate, so the preview's
            // frame rate reflects the capture rate rather than the throttled update rate.
            octScanImagePreview.MarkFrameCaptured();

            // Coalesce: keep at most one frame in flight so the dispatcher queue cannot back up.
            if (_octPreviewPending) return;

            // Copy the native preview image here, on the callback thread, while the source
            // pointer is still valid. Native hands out a pointer into a 10-deep ring of
            // reused image buffers; if we deferred the read into the dispatcher lambda and
            // the UI thread fell >=10 frames behind, the ring would wrap and overwrite the
            // slot, tearing the displayed frame. Copying up front lets the UI touch only
            // managed memory. The preview is 8-bit grayscale (CV_8UC1), so 1 byte/pixel.
            int length = width * height;
            if (_octPreviewBuffer.Length != length)
            {
                _octPreviewBuffer = new byte[length];
            }
            byte[] buffer = _octPreviewBuffer;

            unsafe
            {
                byte* pRawData = (byte*)data.ToPointer();
                ReadOnlySpan<byte> nativeSpan = new ReadOnlySpan<byte>(pRawData, length);
                nativeSpan.CopyTo(buffer);
            }

            // Update GUI preview control asynchronously.
            _octPreviewPending = true;
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.CallbackOctScanPreviewImageCaptured(buffer, width, height, quality, snr_ratio, ref_point, index_image);
                _octPreviewPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnOctSpectrumDataCaptured(nint data, int width, int height)
        {
            if (data == 0) return;
            if (_octSpectrumPending) return;

            _octSpectrumPending = true;
            int length = width * height;
            ushort[] buff = new ushort[length];

            unsafe
            {
                // Interpret the raw memory address of a short array directly as a ushort pointer
                ushort* pRawData = (ushort*)data.ToPointer();
                ReadOnlySpan<ushort> nativeSpan = new ReadOnlySpan<ushort>(pRawData, length);
                nativeSpan.CopyTo(buff);
            }

            Dispatcher.BeginInvoke(() =>
            {
                octSpectrumGraph.CallbackOctSpectrumDataCaptured(buff, width, height);
                _octSpectrumPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnOctResampleDataCaptured(nint data, int width, int height)
        {
            if (data == 0) return;
            if (_octSpectrumPending) return;

            _octSpectrumPending = true;
            int length = width * height;
            float[] buff = new float[length];

            unsafe
            {
                float* pRawData = (float*)data.ToPointer();
                ReadOnlySpan<float> nativeSpan = new ReadOnlySpan<float>(pRawData, length);
                nativeSpan.CopyTo(buff);
            }

            Dispatcher.BeginInvoke(() =>
            {
                octSpectrumGraph.CallbackOctResampleDataCatpured(buff, width, height);
                _octSpectrumPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnOctIntensityDataCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Coalesce: if the UI thread hasn't consumed the previous frame yet, drop
            // this one. The graph only ever needs the newest frame, and this keeps the
            // dispatcher queue from growing without bound under a fast frame stream.
            if (_intensityPending) return;

            // The graph reads only a single transverse column, so extract just that
            // column (~height floats) instead of copying the whole ~4 MB B-scan.
            if (_intensityColumn.Length != height)
            {
                _intensityColumn = new float[height];
            }
            float[] column = _intensityColumn;

            int col = octIntensityGraph.CurrentLinePosition;
            if (col < 0 || col >= width)
            {
                col = width / 2;
            }

            unsafe
            {
                float* pRawData = (float*)data.ToPointer();
                for (int i = 0; i < height; i++)
                {
                    column[i] = pRawData[i * width + col];
                }
            }

            _intensityPending = true;
            Dispatcher.BeginInvoke(() =>
            {
                octIntensityGraph.CallbackOctIntensityDataCaptured(column, height);
                _intensityPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnOctAutoFocusOptimized(bool result, float quality, float diopter)
        {
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.IsOptimizing = false;
                Mouse.OverrideCursor = null;
            }, DispatcherPriority.Normal);
        }

        private void OnOctAutoPolarOptimized(bool result, float quality, float degree)
        {
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.IsOptimizing = false;
                Mouse.OverrideCursor = null;
            }, DispatcherPriority.Normal);
        }

        private void OnOctAutoReferOptimized(bool result, float quality, int ref_point, int position)
        {
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.IsOptimizing = false;
                Mouse.OverrideCursor = null;
            }, DispatcherPriority.Normal);
        }

        private void OnOctAutoScanOptimized(bool result)
        {
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.IsOptimizing = false;
                Mouse.OverrideCursor = null;
            }, DispatcherPriority.Normal);
        }

        private void OnCorneaLeftFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Count the true capture rate before the coalescing gate, so the preview's
            // FPS reflects the camera rate rather than the throttled update rate.
            corneaPreview1.MarkFrameCaptured();
            if (_corenaLeftPending) return;

            _corenaLeftPending = true;
            Dispatcher.BeginInvoke(() =>
            {
                corneaPreview1.CallbackCorneaCameraFrame(data, width, height);
                _corenaLeftPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnCorneaRightFrameCaptured(IntPtr data, int width, int height)
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

        private void OnCorneaRetinaFrameCaptured(IntPtr data, int width, int height)
        {
            if (data == 0) return;

            retinaPreview.MarkFrameCaptured();
            if (_retinaPending) return;

            _retinaPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                retinaPreview.CallbackCorneaCameraFrame(data, width, height);
                _retinaPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnJoystickButtonPressed()
        {
            Dispatcher.BeginInvoke(() =>
            {
                if (OctScanning.IsPreviewing() && !OctScanning.IsAutoOptimizing())
                {
                    btnStartScan.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                }
            }, DispatcherPriority.Normal);
        }

        private void OnOptimizeButtonPressed()
        {
            Dispatcher.BeginInvoke(() =>
            {
                if (OctScanning.IsPreviewing() && !OctScanning.IsAutoOptimizing())
                {
                    btnOptimize.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                }
            }, DispatcherPriority.Normal);
        }
    }
}
