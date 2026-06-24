using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using System.Windows.Input;
using WsoNativeLib;


namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class OctSpectrumWindow
    {
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLeftFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaRightFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLowerFrameCaptured;

        WsoCallback.JoystickButtonPressed _onJoystickButtonPressed;
        WsoCallback.OctScanPreviewImageCaptured _onOctPreviewCaptured;
        WsoCallback.OctScanProtocolCompleted _onProtocolCompleted;
        WsoCallback.OctSpectrumDataCaptured _onSpectrumDataCaptured;
        WsoCallback.OctResampleDataCaptured _onResampleDataCaptured;

        WsoOctScan.OctProtocolInitParam _octInitParam = new();
        WsoOctCalib.OctSpectrometerParam _spectroParam = new();

        const int PEAK_THRESHOLD_MAX = 4096;
        const int PEAK_THRESHOLD_MIN = 0;
        const int PEAK_THRESHOLD_INIT = 3685;

        private void InitializeViewModel()
        {

        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct Scanning
        /// 
        public void PrepareOctScanPreview()
        {
            _octInitParam = new();
            _octInitParam.patternCode = WsoOctDefs.OctPatternCode.CALIBRATION_POINT;
            _octInitParam.previewType = WsoOctDefs.OctPatternType.POINT;
            _octInitParam.scanPoints = 512;
            _octInitParam.scanDirection = WsoOctDefs.OctScanDirection.X_TO_Y;
            _octInitParam.scanRangeX = 3.0f;
            _octInitParam.scanRangeY = 0.0f;
            _octInitParam.scanSpeed = radioCameraSpeed1.IsChecked == true ? WsoOctDefs.OctScanSpeed.SLOWER : radioCameraSpeed2.IsChecked == true ? WsoOctDefs.OctScanSpeed.NORMAL : WsoOctDefs.OctScanSpeed.FASTER;

            OctScanning.ConnectSpectrumDataCaptured(_onSpectrumDataCaptured);
            OctScanning.ConnectResampleDataCaptured(_onResampleDataCaptured);
        }
        public bool StartOctScanPreview()
        {
            PrepareOctScanPreview();
            SetupResampleGraphDisplay();

            octResampleGraph.ClearDataPeakList();
            octResampleGraph.ClearRecordingValues();

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

            OctScanning.ReleaseResampleDataCaptured();
            OctScanning.ReleaseSpectrumDataCaptured();
        }

        public bool IsOctScanning()
        {
            return OctScanning.IsWorking();
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Cornea Camera
        /// 
        public void StartCorneaCameraPreview()
        {
            CorneaCamera.StartPreview(CameraType.IrCorneaLeft, _onCorneaLeftFrameCaptured);
            CorneaCamera.StartPreview(CameraType.IrCorneaRight, _onCorneaRightFrameCaptured);
            // CorneaCamera.StartPreview(CameraType.IrCorneaLower, _onCorneaLowerFrameCaptured);
        }

        public void CloseCorneaCameraPreview()
        {
            CorneaCamera.ClosePreview(CameraType.IrCorneaLeft);
            CorneaCamera.ClosePreview(CameraType.IrCorneaRight);
            // CorneaCamera.ClosePreview(CameraType.IrCorneaLower);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Resampling Graph Display
        /// 
        private void SetupResampleGraphDisplay()
        {
            octResampleGraph.AxisX_Max = ToInt(editRangeXmax.Text);
            octResampleGraph.AxisX_Min = ToInt(editRangeXmin.Text);
            octResampleGraph.AxisY_Max = ToInt(editRangeYmax.Text);
            octResampleGraph.AxisY_Min = ToInt(editRangeYmin.Text);

            if (!IsOctScanning())
            {
                var data = new List<int>();
                for (int i = 0; i < lstProfileNumbers.Items.Count; i++)
                {
                    var s = lstProfileNumbers.Items[i].ToString() ?? "";
                    if (s != "")
                    {
                        var v = ToInt(s);
                        data.Add(v);
                    }
                }
                octResampleGraph.SetupProfileIndexList(data);
            }

            if (lstProfileNumbers.SelectedIndex >= 0 && lstProfileNumbers.SelectedIndex < lstProfileNumbers.Items.Count)
            {
                octResampleGraph.SelectedProfileIndex = lstProfileNumbers.SelectedIndex;
            }
            else
            {
                octResampleGraph.SelectedProfileIndex = 0;
            }

            var ksize = ToInt(editKernelSize.Text);
            ksize = ksize % 2 == 0 ? ksize + 1 : ksize;
            ksize = ksize < 3 ? 3 : ksize;
            octResampleGraph.SpectrumKernelSize = ksize;

            octResampleGraph.PeakThreshold = ToInt(editPeakThresh.Text);
            octResampleGraph.SubsThreshold = ToInt(editSubsThresh.Text);

            octResampleGraph.SetAveragingSize(ToInt(editAverageSize.Text));
            octResampleGraph.UpdatePlotAxisRange();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Spectrometer Calibration
        /// 
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

        private void FindSpectrumDataPeaks()
        {
            if (IsOctScanning())
            {
                btnStartScan.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
            }

            var data = octResampleGraph.GetSpectrumData();
            if (data.Length == 0)
            {
                ShowError("No spectrum data to find peaks!");
                return;
            }

            var threshold1 = octResampleGraph.PeakThreshold;
            var threshold2 = octResampleGraph.SubsThreshold;

            var count = OctCalibration.FindSpectrumDataPeaks(data, threshold1, threshold2, out var values, out var indice);
            if (count > 0)
            {
                lstPeakPositions.Items.Clear();
                for (int i = 0; i < indice.Count; i++)
                {
                    var wlen = OctScanner.GetOctWavelengthAtPixelPosition(indice[i]);
                    string s = String.Format("  [{0:D2}] {1:3} @ {2:F0}", i, wlen, indice[i]);
                    lstPeakPositions.Items.Add(s);
                }

                octResampleGraph.ClearDataPeakList();
                octResampleGraph.SetDataPeakList(values, indice);
                ShowInfo("Found " + count + " peaks in the spectrum data!");
            }
            else
            {
                ShowError("No peaks found in the spectrum data!");
            }
        }

        private void ComputeSpectroCoefficients()
        {
            var values = new List<double>();
            for (int i = 0; i < lstWavelenghts.Items.Count; i++)
            {
                var s = lstWavelenghts.Items[i].ToString() ?? "";
                if (s.Contains("]"))
                {
                    var v = ToDouble(s[(s.IndexOf(']') + 1)..]);
                    values.Add(v);
                }
            }

            var indice = new List<int>();
            for (int i = 0; i < lstPeakPositions.Items.Count; i++)
            {
                var s = lstPeakPositions.Items[i].ToString() ?? "";
                if (s.Contains('@'))
                {
                    var v = ToInt(s[(s.IndexOf('@') + 1)..]);
                    indice.Add(v);
                }
            }

            if (values.Count != indice.Count || values.Count == 0 || indice.Count == 0)
            {
                ShowError("Wavelenghts and peak positions are not matched!");
                return;
            }

            var order = radioPolyOrder3.IsChecked == true ? 3 : 4;

            var count = OctCalibration.ComputeSpectrometerCoefficients(indice.ToArray(), values.ToArray(), order, out var coeffs);
            if (count == WsoOctDefs.OCT_SPECTROMETER_COEFFS_SIZE && coeffs != null)
            {
                OctCalibration.ApplySpectrometerCoefficients(coeffs.ToArray());
                editSpectParam1.Text = coeffs[0].ToString("E6");
                editSpectParam2.Text = coeffs[1].ToString("E6");
                editSpectParam3.Text = coeffs[2].ToString("E6");
                editSpectParam4.Text = coeffs[3].ToString("E6");
                ShowInfo("Spectrometer calibration completed!");
            }
            else
            {
                editSpectParam1.Text = "";
                editSpectParam2.Text = "";
                editSpectParam3.Text = "";
                editSpectParam4.Text = "";
                ShowInfo("Failed to compute spectrometer coefficients!");
            }
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////
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

        private void OnOctScanPreviewImageCaptured(IntPtr data, int width, int height, float quality, float snr_ratio, int ref_point, int index_image)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.CallbackOctScanPreviewImageCaptured(data, width, height, quality, snr_ratio, ref_point, index_image);
            }, DispatcherPriority.Background);
        }
        private void OnOctSpectrumDataCaptured(IntPtr data, int width, int height)
        {
            if (data == 0) return;
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
                octResampleGraph.CallbackOctSpectrumDataCaptured(buff, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnOctResampleDataCaptured(IntPtr data, int width, int height)
        {
            if (data == 0) return;
            int length = width * height;
            float[] buff = new float[length];

            Dispatcher.BeginInvoke(() =>
            {
                octResampleGraph.CallbackOctResampleDataCatpured(buff, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaLeftFrameCaptured(IntPtr data, int width, int height)
        {
            if (data == 0) return;

            Dispatcher.BeginInvoke(() => {
                corneaPreview1.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaRightFrameCaptured(IntPtr data, int width, int height)
        {
            if (data == 0) return;

            Dispatcher.BeginInvoke(() => {
                corneaPreview2.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaLowerFrameCaptured(IntPtr data, int width, int height)
        {
            if (data == 0) return;

            Dispatcher.BeginInvoke(() => {
                corneaPreview3.CallbackCorneaCameraFrame(data, width, height);
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
    }
}
