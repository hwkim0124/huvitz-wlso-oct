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
    using static utils.MsgBoxUtil;
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class OctAngioModeWindow
    {
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLeftFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaRightFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLowerFrameCaptured;

        WsoCallback.JoystickButtonPressed _onJoystickButtonPressed;
        WsoCallback.OptimizeButtonPressed _onOptimizeButtonPressed;

        WsoCallback.OctScanPreviewImageCaptured _onOctPreviewCaptured;
        WsoCallback.OctScanEnfaceImageCaptured _onOctEnfaceCaptured;
        WsoCallback.OctScanProtocolCompleted _onProtocolCompleted;

        WsoCallback.OctAutoScanOptimized _onAutoScanOptimized;
        WsoCallback.OctAutoFocusOptimized _onAutoFocusOptimized;
        WsoCallback.OctAutoPolarOptimized _onAutoPolarOptimized;
        WsoCallback.OctAutoReferOptimized _onAutoReferOptimized;


        LightControlWindow? _lightControlWindow = null;
        FocusMotorWindow? _focusMotorWindow = null;
        StageMotorWindow? _stageMotorWindow = null;

        WsoOctScan.OctScanPattern[] _scanPatterns = Array.Empty<WsoOctScan.OctScanPattern>();
        WsoOctScan.OctProtocolInitParam _octInitParam = new();
        WsoOctScan.OctStratumParam _stratumParam = new();
        WsoOctScan.OctProtocolDescript _octProtocol = new();
        WsoOctScan.OctMacularSummaryDescriptor _macularSummary = new();
        WsoOctScan.OctOpticDiscSummaryDescriptor _optdiscSummary = new();

        bool _isResultAnalyzed = false;
        int _sourceId = 0;
        int _reportId = 0;

        private void InitializeWindowControls()
        {
            cmbScanDirection.ItemsSource = new string[] { "X-Y", "Y-X" };
            cmbScanDirection.SelectedIndex = 0;
            cmbUpperLayer.ItemsSource = new string[] { "ILM", "NFL", "IPL", "OPL", "IOS", "RPE", "BRM" };
            cmbUpperLayer.SelectedIndex = 1;
            cmbLowerLayer.ItemsSource = new string[] { "ILM", "NFL", "IPL", "OPL", "IOS", "RPE", "BRM" };
            cmbLowerLayer.SelectedIndex = 2;

            radioCameraSpeed3.IsChecked = true;
            radioEyeOd.IsChecked = true;

            checkOverlayLayers.IsChecked = true;
            checkOverlayCenter.IsChecked = true;
            checkOverlayLength.IsChecked = false;
            checkFullDepthEnface.IsChecked = false;
            checkFitToHeight.IsChecked = true;

            editScanOffsetX.Text = "0";
            editScanOffsetY.Text = "0";
            editScanScaleX.Text = "1.0";
            editScanScaleY.Text = "1.0";

            EnableButtonsAtInitScreen();
        }


        //  Enable GUI buttons and controls by processing state.
        /////////////////////////////////////////////////////////////////////////////////////////////////////
        private void EnableButtonsAtInitScreen()
        {
            btnCapture.IsEnabled = false;
            btnOptimize.IsEnabled = false;
            btnAutoFocus.IsEnabled = false;
            btnAutoRefer.IsEnabled = false;
            btnAutoPolar.IsEnabled = false;

            btnAnalyzeResult.IsEnabled = false;
            btnImportResult.IsEnabled = true;
            btnExportResult.IsEnabled = false;
            btnUpdateLayers.IsEnabled = false;
            btnSaveAngioImage.IsEnabled = false;
        }

        private void EnableButtonsAtScanning(bool on_scan)
        {
            EnableButtonsAtInitScreen();

            btnCapture.IsEnabled = on_scan;
            btnOptimize.IsEnabled = on_scan;
            btnAutoFocus.IsEnabled = on_scan;
            btnAutoRefer.IsEnabled = on_scan;
            btnAutoPolar.IsEnabled = on_scan;

            if (on_scan)
            {
                btnAnalyzeResult.IsEnabled = false;
                btnImportResult.IsEnabled = false;
                btnExportResult.IsEnabled = false;
                btnUpdateLayers.IsEnabled = false;
                btnSaveAngioImage.IsEnabled = false;
            }
        }

        private void EnableButtonsAtCaptured()
        {
            EnableButtonsAtScanning(false);

            btnAnalyzeResult.IsEnabled = true;
            btnImportResult.IsEnabled = true;
            btnExportResult.IsEnabled = true;
        }

        private void EnableButtonAtAnalyzed()
        {
            btnUpdateLayers.IsEnabled = true;
            btnSaveAngioImage.IsEnabled = true;
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

        /////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Oct scanning.
        /// 
        private void StartOctScanPreview()
        {
            octScanImagePreview.IsOptimizing = false;

            RetreiveProtocolInitParam();
            if (!OctScanning.SetupScanProtocol(_octInitParam))
            {
                Mouse.OverrideCursor = null;
                ShowError("Failed to setup scan protocol!");
                return;
            }
            if (!OctScanning.StartPreview(_onOctPreviewCaptured, _onOctEnfaceCaptured))
            {
                Mouse.OverrideCursor = null;
                ShowError("Failed to start scan preview!");
                return;
            }

            // Intialize protocol descriptor from the current previewing protocol.
            _octProtocol.copyFromInitParam(_octInitParam);

            btnStartScan.Content = "Stop Scan";
            RefelectLineCameraOptions();
            ReflectGalvanometerOptions();
            EnableButtonsAtScanning(true);

            octScanImagePreview.SetAsPreviewMode();
            octScanEnfacePreivew.SetAsPreviewMode();

            // Resume playing cornea camera paused by capture. 
            StartCorneaCameraPreview();
            _isResultAnalyzed = false;
        }

        private void CancelOctScanPreview()
        {
            if (OctScanning.IsWorking())
            {
                OctScanning.ClosePreview(false, _onProtocolCompleted);
                btnStartScan.Content = "Start Scan";
                EnableButtonsAtScanning(false);
                return;
            }
        }

        private void StartOctScanMeasure()
        {
            if (IsOctScanPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                OctScanning.ClosePreview(true, _onProtocolCompleted);

                // Processing operations should be run in the completed callback.
            }
        }
        private void StartOctOptimizing()
        {
            if (IsOctScanPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (OctScanning.StartAutoOptimize(_onAutoScanOptimized))
                {
                    octScanImagePreview.IsOptimizing = true;
                }
                else
                {
                    ShowError("Failed to start auto optimize!");
                    Mouse.OverrideCursor = null;
                }
            }
        }

        private void StartAutoReference()
        {
            if (IsOctScanPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (OctScanning.StartAutoReference(_onAutoReferOptimized))
                {
                    octScanImagePreview.IsOptimizing = true;
                }
                else
                {
                    ShowError("Failed to start auto reference optimize!");
                    Mouse.OverrideCursor = null;
                }
            }
        }

        private void StartAutoDiopterFocus()
        {
            if (IsOctScanPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (OctScanning.StartAutoDiopterFocus(_onAutoFocusOptimized))
                {
                    octScanImagePreview.IsOptimizing = true;
                }
                else
                {
                    ShowError("Failed to start auto focus optimize!");
                    Mouse.OverrideCursor = null;
                }
            }
        }

        private void StartAutoPolarization()
        {
            if (IsOctScanPreviewing())
            {
                Mouse.OverrideCursor = Cursors.Wait;
                if (OctScanning.StartAutoPolarization(_onAutoPolarOptimized))
                {
                    octScanImagePreview.IsOptimizing = true;
                }
                else
                {
                    ShowError("Failed to start auto polar optimize!");
                    Mouse.OverrideCursor = null;
                }
            }
        }

        private void CancelOctOptimizing()
        {
            octScanImagePreview.IsOptimizing = false;
            OctScanning.CancelAutoOptimizing();
            Mouse.OverrideCursor = null;
        }

        private bool IsOctScanPreviewing()
        {
            return OctScanning.IsPreviewing();
        }

        private bool IsOctScanning()
        {
            return OctScanning.IsWorking();
        }

        private bool IsOctOptimizing()
        {
            return OctScanning.IsAutoOptimizing();
        }


        // Fetch protocol parameters from WSO library.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        private void FetchProtocolInitParam()
        {
            var param = new WsoOctScan.OctProtocolInitParam();
            if (OctAnalysis.GetProtocolInitParam(_reportId, ref param))
            {
                _octInitParam = param;
            }
        }
        private void FetchOctScanPatterns()
        {
            int count = OctScanner.GetOctScanPatternCount(not_point: true);
            if (count > 0)
            {
                _scanPatterns = new WsoOctScan.OctScanPattern[count];
                OctScanner.GetOctScanPatternList(_scanPatterns, count, not_point: true);
            }
        }

        private void FetchReportProtocolDescript()
        {
            var desc = new WsoOctScan.OctProtocolDescript();
            if (OctAnalysis.GetProtocolDescript(_reportId, ref desc))
            {
                _octProtocol = desc;
            }
        }

        private void FetchReportSummaryDescript()
        {
            if (WsoOctUtil.IsMacularScan(_octProtocol.patternCode))
            {
                var desc = new WsoOctScan.OctMacularSummaryDescriptor();
                if (OctAnalysis.GetMacularSummary(_reportId, ref desc))
                {
                    _macularSummary = desc;
                    _stratumParam.chartCenterX = desc.foveaCenterX;
                    _stratumParam.chartCenterY = desc.foveaCenterY;
                }
            }
            else if (WsoOctUtil.IsOpticDiscScan(_octProtocol.patternCode))
            {
                var desc = new WsoOctScan.OctOpticDiscSummaryDescriptor();
                if (OctAnalysis.GetOpticDiscSummary(_reportId, ref desc))
                {
                    _optdiscSummary = desc;
                    _stratumParam.chartCenterX = desc.nerveHeadCenterX;
                    _stratumParam.chartCenterY = desc.nerveHeadCenterY;
                }
            }
        }

        private bool IsMacularScanResult()
        {
            return WsoOctUtil.IsMacularScan(_octProtocol.patternCode);
        }

        private bool IsOpticDiscScanResult()
        {
            return WsoOctUtil.IsOpticDiscScan(_octProtocol.patternCode);
        }

        private bool IsCorneaScanResult()
        {
            return WsoOctUtil.IsCorneaScan(_octProtocol.patternCode);
        }

        // Retrieve user selections from GUI controls. 
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        private void RetreiveProtocolInitParam()
        {
            _octInitParam.eyeSide = radioEyeOd.IsChecked == true ? WsoDomain.EyeSide.OD : WsoDomain.EyeSide.OS;
            _octInitParam.diopter = ToFloat(editDiopter.Text);

            var name = cmbScanPattern.SelectedItem.ToString();
            var code = Array.Find(_scanPatterns, item => item.patternName == name).patternCode;
            _octInitParam.patternCode = code;
            _octInitParam.previewType = WsoOctDefs.OctPatternType.LINE;

            _octInitParam.scanPoints = ToInt(cmbScanPoints.SelectedItem.ToString());
            _octInitParam.scanLines = ToInt(cmbScanLines.SelectedItem.ToString());
            _octInitParam.scanOverlaps = ToInt(cmbScanOverlaps.SelectedItem.ToString());

            _octInitParam.scanRangeX = ToFloat(cmbScanRangeX.SelectedItem.ToString());
            _octInitParam.scanRangeY = ToFloat(cmbScanRangeY.SelectedItem?.ToString() ?? "0.0");
            _octInitParam.lineSpace = ToFloat(cmbLineSpaces.SelectedItem?.ToString() ?? "0.0");

            _octInitParam.scanDirection = cmbScanDirection.SelectedIndex == 0 ? WsoOctDefs.OctScanDirection.X_TO_Y : WsoOctDefs.OctScanDirection.Y_TO_X;
            _octInitParam.scanSpeed = radioCameraSpeed1.IsChecked == true ? WsoOctDefs.OctScanSpeed.SLOW : radioCameraSpeed2.IsChecked == true ? WsoOctDefs.OctScanSpeed.NORMAL : WsoOctDefs.OctScanSpeed.FAST;

            _octInitParam.scanOffsetX = ToFloat(editScanOffsetX.Text);
            _octInitParam.scanOffsetY = ToFloat(editScanOffsetY.Text);
            _octInitParam.scanScaleX = ToFloat(editScanScaleX.Text);
            _octInitParam.scanScaleY = ToFloat(editScanScaleY.Text);

            _octInitParam.isFirstScanOnSide = false; // true;
        }

        private bool GetScanPatternSelected(out WsoOctScan.OctScanPattern pattern)
        {
            int index = cmbScanPattern.SelectedIndex;
            if (index >= 0)
            {
                pattern = _scanPatterns[index];
                return true;
            }
            pattern = default;
            return false;
        }

        private bool GetScanPointsSelected(out int points)
        {
            int index = cmbScanPoints.SelectedIndex;
            if (index >= 0)
            {
                points = (int)cmbScanPoints.Items[index];
                return true;
            }
            points = 0;
            return false;
        }

        private bool GetScanLinesSelected(out int lines)
        {
            int index = cmbScanLines.SelectedIndex;
            if (index >= 0)
            {
                lines = (int)cmbScanLines.Items[index];
                return true;
            }
            lines = 0;
            return false;
        }

        private void UpdateLayerStratumParam()
        {
            _stratumParam.upperType = WsoOctUtil.LayerStringToType(cmbUpperLayer.SelectedItem.ToString() ?? "");
            _stratumParam.lowerType = WsoOctUtil.LayerStringToType(cmbLowerLayer.SelectedItem.ToString() ?? "");
            _stratumParam.upperOffset = ToFloat(editUpperOffset.Text);
            _stratumParam.lowerOffset = ToFloat(editLowerOffset.Text);
        }


        // Update GUI controls from protocol result and analyzed report.
        /////////////////////////////////////////////////////////////////////////////////////////////////
        private void RefelectLineCameraOptions()
        {
            var exp_time = OctScanner.GetLineCameraExposureTime();
            editExposure.Text = string.Format("{0:F2}", exp_time);
        }

        private void ReflectGalvanometerOptions()
        {
            var time_step = OctScanner.GetGalvanoTriggerTimeStep();
            var time_delay = OctScanner.GetGalvanoTriggerTimeDelay();
            var fore_padds = OctScanner.GetGalvanoTriggerForePaddings(_octInitParam.scanSpeed);
            var post_padds = OctScanner.GetGalvanoTriggerPostPaddings(_octInitParam.scanSpeed);

            editTriggTimeDelay.Text = time_delay.ToString();
            editTriggTimeStep.Text = string.Format("{0:F2}", time_step);
            editTriggForePadds.Text = fore_padds.ToString();
            editTriggPostPadds.Text = post_padds.ToString();
        }

        private void ReflectPatternImageSelected(bool enable)
        {
            sliderPatternImage.IsEnabled = false;
            editPatternImageNumber.IsEnabled = false;

            int size;
            if (_isResultAnalyzed)
            {
                size = OctAnalysis.GetPatternContentCount(_sourceId);
            }
            else
            {
                size = OctArchive.GetPatternSectionCount();
            }

            if (enable && size >= 1)
            {
                sliderPatternImage.IsEnabled = true;
                editPatternImageNumber.IsEnabled = true;

                sliderPatternImage.Minimum = 1;
                sliderPatternImage.Maximum = size;
                int numb = Math.Max(size / 2, 1);
                sliderPatternImage.Value = numb;
                editPatternImageNumber.Text = numb.ToString();
            }
            else
            {
                editPatternImageNumber.Text = "";
            }
        }

        private void ReflectPatternImagePreviewSelected()
        {
            int numb = (int)sliderPatternImage.Value;
            editPatternImageNumber.Text = numb.ToString();

            var image = new WsoOctScan.OctScanImageDescript();
            int index = numb - 1;
            bool flag;

            if (_isResultAnalyzed)
            {
                flag = OctAnalysis.GetPatternImage(_sourceId, index, ref image);
                if (checkOverlayLayers.IsChecked == true)
                {
                    octScanImagePreview.ClearOverlayStatus();
                    if (OctAnalysis.GetBsegmLayerPoints(_sourceId, index, _stratumParam.upperType, out WsoOctSegm.OctBsegmLayerPoints uppers))
                    {
                        if (OctAnalysis.GetBsegmLayerPoints(_sourceId, index, _stratumParam.lowerType, out WsoOctSegm.OctBsegmLayerPoints lowers))
                        {
                            octScanImagePreview.SetLayerPoints(_stratumParam, uppers, lowers);
                        }
                    }
                }
            }
            else
            {
                flag = OctArchive.GetPatternImage(index, ref image);
            }

            if (flag)
            {
                octScanImagePreview.IsPreviewMode = false;
                octScanImagePreview.ProtocolDescriptor = _octProtocol;
                OnOctScanPreviewImageCaptured(image.data, image.width, image.height, image.quality, image.snrRatio, image.refPoint, index);

                octScanEnfacePreivew.SelectedLineIndex = index;
                octScanEnfacePreivew.UpdateFrameImage();
            }
        }

        private void RefelectChartCenterPosition()
        {
            if (_isResultAnalyzed)
            {
                if (octScanEnfacePreivew.IsChartCenterLocated())
                {
                    var position = octScanEnfacePreivew.GetChartCenterPosition();
                    var line_index = position.Y;
                    var lateral_pos = position.X;
                    octScanImagePreview.SetChartCenterPosition(line_index, lateral_pos);

                    var image_numb = line_index + 1;
                    if (sliderPatternImage.Value != image_numb)
                    {
                        sliderPatternImage.Value = image_numb;
                    }
                    else
                    {
                        octScanEnfacePreivew.UpdateFrameImage();
                    }
                }
            }
        }


        private void ReflectPatternEnfacePreview()
        {
            var image = new WsoOctScan.OctEnfaceImageDescript();

            if (WsoOctUtil.IsStratumParamValid(_stratumParam))
            {
                var param = _stratumParam;
                if (checkFullDepthEnface.IsChecked == true)
                {
                    param.upperOffset = -9999.0f;
                    param.lowerOffset = +9999.0f;
                    octScanEnfacePreivew.ClipLimit = 8.0;
                }
                else
                {
                    octScanEnfacePreivew.ClipLimit = 2.0;
                }

                octScanEnfacePreivew.ProtocolDescriptor = _octProtocol;
                octScanEnfacePreivew.LayerStratumParam = _stratumParam;

                if (OctAnalysis.GetEnfaceImage(_reportId, param, ref image))
                {
                    OnOctScanEnfaceImageCaptured(image.data, image.width, image.height);
                }
            }
        }

        private void ReflectAngioImagePreview()
        {
            var image = new WsoOctAngio.OctAngioImageDescript();

            if (WsoOctUtil.IsStratumParamValid(_stratumParam))
            {
                scanAngioPreview.ProtocolDescriptor = _octProtocol;

                if (OctAnalysis.GetAngioImage(_reportId, _stratumParam, ref image))
                {
                    OnOctScanAngioImageCaptured(image.data, image.width, image.height);
                }
            }
        }

        private void ReflectStratumOptionsByProtocol()
        {
            if (IsMacularScanResult() || IsOpticDiscScanResult())
            {
                cmbUpperLayer.ItemsSource = new string[] { "ILM", "NFL", "IPL", "OPL", "IOS", "RPE", "BRM" };
                cmbLowerLayer.ItemsSource = new string[] { "ILM", "NFL", "IPL", "OPL", "IOS", "RPE", "BRM" };
            }
            else
            {
                cmbUpperLayer.ItemsSource = new string[] { "EPI", "BOW", "END" };
                cmbLowerLayer.ItemsSource = new string[] { "EPI", "BOW", "END" };
            }

            if (IsMacularScanResult())
            {
                cmbUpperLayer.SelectedIndex = 1;
                cmbLowerLayer.SelectedIndex = 2;
            }
            else if (IsOpticDiscScanResult())
            {
                cmbUpperLayer.SelectedIndex = 0;
                cmbLowerLayer.SelectedIndex = 1;
            }
            else if (IsCorneaScanResult())
            {
                cmbUpperLayer.SelectedIndex = 0;
                cmbLowerLayer.SelectedIndex = 2;
            }
            editUpperOffset.Clear();
            editLowerOffset.Clear();
        }

        private void ReflectProtocolSummary()
        {
            if (!_isResultAnalyzed)
            {
                textProtocolSummary.Text = "Protocol Summary";
                return;
            }

            string s = string.Format("{0}, {1}\r\n", _octProtocol.protocolName, WsoOctUtil.GetEyeSideString(_octProtocol.eyeSide));
            s += string.Format("{0} x {1}, {2} overlaps\r\n", _octProtocol.scanPoints, _octProtocol.scanLines, _octProtocol.scanOverlaps);
            s += string.Format("{0} mm x {1} mm\r\n", _octProtocol.scanRangeX, _octProtocol.scanRangeY);
            s += string.Format("{0}, {1}\r\n", WsoOctUtil.GetScanDirectionString(_octProtocol.scanDirection), WsoOctUtil.GetScanSpeedString(_octProtocol.scanSpeed));
            textProtocolSummary.Text = s;
        }

        private void ReflectReportSummary()
        {
            if (!_isResultAnalyzed)
            {
                textReportSummary.Text = "Analysis Report";
                return;
            }

            if (IsMacularScanResult())
            {
                string s = string.Format("Fovea center: ({0:F2}, {1:F2})\r\n", _macularSummary.foveaCenterX, _macularSummary.foveaCenterY);
                s += string.Format("Center line: {0}, lateral pos.: {1}\r\n", _macularSummary.foveaCenterLine, _macularSummary.foveaCenterXpos);
                s += string.Format("Center thickness: {0:F0} um\r\n", _macularSummary.foveaCenterThick);
                s += string.Format("Fovea valid: {0}\r\n", _macularSummary.isFoveaValid);
                textReportSummary.Text = s;
            }
            else if (IsOpticDiscScanResult())
            {
                string s = string.Format("Disc area: {0:F2}, volume: {1:F2} \r\n", _optdiscSummary.discArea, _optdiscSummary.discVolume);
                s += string.Format("Cup area: {0:F2}, volume: {1:F2} \r\n", _optdiscSummary.cupArea, _optdiscSummary.cupVolume);
                s += string.Format("Rim area: {0:F2}, volume: {1:F2} \r\n", _optdiscSummary.rimArea, _optdiscSummary.rimVolume);
                s += string.Format("C/D size ratio: {0:F2} vert., {1:F2} horz.\r\n", _optdiscSummary.cupDiscVertRatio, _optdiscSummary.cupDiscHorzRatio);
                s += string.Format("C/D area ratio: {0:F2}\r\n", _optdiscSummary.cupDiscAreaRatio);
                s += string.Format("Disc valid: {0}, Cup valid: {1}\r\n", _optdiscSummary.isOpticDiscValid, _optdiscSummary.isOpticCupValid);
                textReportSummary.Text = s;
            }
            else if (IsCorneaScanResult())
            {
            }
        }

        private void ReflectReportAngioChart(bool clear)
        {
            reportAngioChart.ClearChart();
            if (!clear)
            {
                if (WsoOctUtil.IsStratumParamValid(_stratumParam))
                {
                    if (IsMacularScanResult() || IsOpticDiscScanResult())
                    {
                        var desc = new WsoOctAngio.OctAngioChartDescript();
                        if (OctAnalysis.GetAngioChart(_reportId, _stratumParam, ref desc))
                        {
                            reportAngioChart.ChartDescript = desc;
                        }
                    }
                }
            }
            reportAngioChart.Refresh();
        }


        // Update GUI controls for OCT scanning.
        /////////////////////////////////////////////////////////////////////////////////////////////////////
        private void ReflectScanPatternSelected(bool init)
        {
            cmbScanPattern.Items.Clear();
            if (_scanPatterns.Length > 0)
            {
                foreach (var item in _scanPatterns)
                {
                    cmbScanPattern.Items.Add(item.patternName);
                }
                if (init)
                {
                    int count = cmbScanPattern.Items.Count;
                    if (count > 0)
                    {
                        cmbScanPattern.SelectedIndex = (count > 5 ? 5 : 0);
                    }
                }
            }
        }

        private void ReflectScanPointsSelected()
        {
            cmbScanPoints.Items.Clear();

            WsoOctScan.OctScanPattern pattern;
            if (GetScanPatternSelected(out pattern))
            {
                int buffSize = 32;
                int[] points = new int[buffSize];

                int count = OctScanner.GetOctScanPatternAscanList(points, buffSize, pattern.patternCode);
                for (int i = 0; i < count; i++)
                {
                    cmbScanPoints.Items.Add(points[i]);
                }

                if (count > 0)
                {
                    cmbScanPoints.SelectedIndex = (count > 1 ? 0 : 0);
                }
                return;
            }
        }

        private void RefelectScanLinesSelected()
        {
            cmbScanLines.Items.Clear();

            WsoOctScan.OctScanPattern pattern;
            int points;
            if (GetScanPatternSelected(out pattern) && GetScanPointsSelected(out points))
            {
                int buffSize = 32;
                int[] lines = new int[buffSize];

                int count = OctScanner.GetOctScanPatternBscanList(lines, buffSize, pattern.patternCode, points);
                for (int i = 0; i < count; i++)
                {
                    cmbScanLines.Items.Add(lines[i]);
                }

                if (count > 0)
                {
                    cmbScanLines.SelectedIndex = (count > 1 ? 1 : 0);
                }
                return;
            }
        }

        private void ReflectScanOverlapsSelected()
        {
            cmbScanOverlaps.Items.Clear();

            WsoOctScan.OctScanPattern pattern;
            int points, lines;
            if (GetScanPatternSelected(out pattern) && GetScanPointsSelected(out points) && GetScanLinesSelected(out lines))
            {
                int buffSize = 32;
                int[] overlaps = new int[buffSize];

                int count = OctScanner.GetOctScanPatternOverlapList(overlaps, buffSize, pattern.patternCode, points, lines);
                for (int i = 0; i < count; i++)
                {
                    cmbScanOverlaps.Items.Add(overlaps[i]);
                }

                if (count > 0)
                {
                    cmbScanOverlaps.SelectedIndex = (count > 1 ? 1 : 0);
                }
                return;
            }
        }

        private void ReflectScanRangeSelected()
        {
            cmbScanRangeX.Items.Clear();
            cmbScanRangeY.Items.Clear();

            WsoOctScan.OctScanPattern pattern;
            if (GetScanPatternSelected(out pattern))
            {
                int xcount = OctScanner.GetOctScanPatternRangeXList(out float[] xranges, pattern.patternCode);
                int ycount = OctScanner.GetOctScanPatternRangeYList(out float[] yranges, pattern.patternCode);

                for (int i = 0; i < xcount; i++)
                {
                    cmbScanRangeX.Items.Add(xranges[i].ToString("F2"));
                }
                for (int i = 0; i < ycount; i++)
                {
                    cmbScanRangeY.Items.Add(yranges[i].ToString("F2"));
                }

                if (xcount > 0)
                {
                    cmbScanRangeX.SelectedIndex = (xcount > 1 ? 0 : 0);
                }
                if (ycount > 0)
                {
                    cmbScanRangeY.SelectedIndex = (ycount > 1 ? 0 : 0);
                }
                return;
            }
        }


        // Callback functions from WSO library.
        /////////////////////////////////////////////////////////////////////////////////////////////////////
         private void OnOctScanPreviewImageCaptured(nint data, int width, int height, float quality, float snr_ratio, int ref_point, int index_image)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() =>
            {
                octScanImagePreview.CallbackOctScanPreviewImageCaptured(data, width, height, quality, snr_ratio, ref_point, index_image);
            }, DispatcherPriority.Background);
        }

        private void OnOctScanEnfaceImageCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() =>
            {
                octScanEnfacePreivew.CallbackOctScanEnfaceImageCaptured(data, width, height);
            }, DispatcherPriority.Normal);
        }

        private void OnOctScanAngioImageCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() =>
            {
                scanAngioPreview.CallbackOctScanAngioImageCaptured(data, width, height);
            }, DispatcherPriority.Normal);
        }

        private void OnOctScanProtocolCompleted(bool result)
        {
            Dispatcher.BeginInvoke(() =>
            {
                CloseCorneaCameraPreview();

                if (result)
                {
                    btnStartScan.Content = "Start Scan";
                    EnableButtonsAtScanning(false);

                    octScanImagePreview.SetAsAnalysisMode();
                    octScanEnfacePreivew.SetAsAnalysisMode();

                    if (!OctArchive.IsProtocolResultValid())
                    {
                        ShowError("Protocol result is not available!");
                    }
                    else
                    {
                        ReflectPatternImageSelected(true);
                        EnableButtonsAtCaptured();
                        ShowInfo("Scan measure protocol completed!");
                    }
                }
                else
                {
                    ShowError("Scan measure protocol failed!");
                }
                Mouse.OverrideCursor = null;
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

        private void OnJoystickButtonPressed()
        {
            Dispatcher.BeginInvoke(() =>
            {
                if (OctScanning.IsPreviewing() && !OctScanning.IsAutoOptimizing())
                {
                    btnCapture.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
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

        private void OnChartCenterRelocated(float x, float y, int image_idx, int later_pos)
        {
            Dispatcher.BeginInvoke(() =>
            {
                Mouse.OverrideCursor = Cursors.Wait;
                _stratumParam.chartCenterX = x;
                _stratumParam.chartCenterY = y;
                ReflectReportAngioChart(false);
                octScanEnfacePreivew.SetChartCenterLocation(x, y);
                octScanEnfacePreivew.UpdateFrameImage();

                octScanImagePreview.SetChartCenterPosition(image_idx, later_pos);
                var image_numb = image_idx + 1;
                if (sliderPatternImage.Value != image_numb)
                {
                    sliderPatternImage.Value = image_numb;
                }
                else
                {
                    octScanImagePreview.UpdateFrameImage();
                }
                Mouse.OverrideCursor = null;
            }, DispatcherPriority.Normal);
        }
    }
}
