using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using WsoNativeLib;
using WsoToolkit.utils;
using WsoToolkit.entity;


namespace WsoToolkit
{
    using static MsgBoxUtil;

    public partial class OctScanModeWindow : Window
    {
        bool _isDraggingStarted = false;
        string _exportRootPath = ".\\results";
        string _openedExamPath = "";

        public OctScanModeWindow()
        {
            InitializeComponent();

            _onCorneaLeftFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLeftFrameCaptured);
            _onCorneaRightFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraRightFrameCaptured);
            _onCorneaLowerFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLowerFrameCaptured);

            _onJoystickButtonPressed = new WsoCallback.JoystickButtonPressed(this.OnJoystickButtonPressed);
            _onOptimizeButtonPressed = new WsoCallback.OptimizeButtonPressed(this.OnOptimizeButtonPressed);

            _onOctPreviewCaptured = new WsoCallback.OctScanPreviewImageCaptured(this.OnOctScanPreviewImageCaptured);
            _onOctEnfaceCaptured = new WsoCallback.OctScanEnfaceImageCaptured(this.OnOctScanEnfaceImageCaptured);
            _onProtocolCompleted = new WsoCallback.OctScanProtocolCompleted(this.OnOctScanProtocolCompleted);

            _onAutoFocusOptimized = new WsoCallback.OctAutoFocusOptimized(this.OnOctAutoFocusOptimized);
            _onAutoPolarOptimized = new WsoCallback.OctAutoPolarOptimized(this.OnOctAutoPolarOptimized);
            _onAutoReferOptimized = new WsoCallback.OctAutoReferOptimized(this.OnOctAutoReferOptimized);
            _onAutoScanOptimized = new WsoCallback.OctAutoScanOptimized(this.OnOctAutoScanOptimized);

            octScanEnfacePreivew.ChartCenterRelocated = OnChartCenterRelocated;
        }

        // Windows Event handlers
        /////////////////////////////////////////////////////////////////////////////////////////////
        private void Window_Initialized(object sender, EventArgs e)
        {
            InitializeWindowControls();

            FetchOctScanPatterns();

            Title += " " + DatabaseRepository.GetCurrentPatientTag();

            var path = DatabaseRepository.GetCurrentPatientPath();
            if (path.Length > 0)
            {
                _exportRootPath = path;
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            ReflectScanPatternSelected(true);

            StartCorneaCameraPreview();

            BoardDevice.ConnectJoystickButtonPressed(_onJoystickButtonPressed);
            BoardDevice.ConnectOptimizeButtonPressed(_onOptimizeButtonPressed);
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            _lightControlWindow?.Close();
            _focusMotorWindow?.Close();
            _stageMotorWindow?.Close();

            BoardDevice.ReleaseJoystickButtonPressed();
            BoardDevice.ReleaseOptimizeButtonPressed();

            OctScanning.CancelScanning();
            OctAnalysis.ClearAllDataReports();
            OctAnalysis.ClearAllDataSources();

            CloseCorneaCameraPreview();
            Thread.Sleep(500);
            Mouse.OverrideCursor = null;
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (MsgBoxUtil.ShowQuestion("Are you sure to close this window?") == false)
            {
                e.Cancel = true;
            }
        }

        // Operational button event handlers
        /////////////////////////////////////////////////////////////////////////////////////////////
        private void BtnStartScan_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (IsOctScanning())
            {
                CancelOctScanPreview();
            }
            else
            {
                StartOctScanPreview();
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnCapture_Click(object sender, RoutedEventArgs e)
        {
            StartOctScanMeasure();
            return;
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void BtnOptimize_Click(object sender, RoutedEventArgs e)
        {
            if (!IsOctOptimizing())
            {
                StartOctOptimizing();
            }
            else
            {
                CancelOctOptimizing();
            }
        }

        private void BtnAutoRefer_Click(object sender, RoutedEventArgs e)
        {
            if (!IsOctOptimizing())
            {
                StartAutoReference();
            }
            else
            {
                CancelOctOptimizing();
            }
        }

        private void BtnAutoFocus_Click(object sender, RoutedEventArgs e)
        {
            if (!IsOctOptimizing())
            {
                StartAutoDiopterFocus();
            }
            else
            {
                CancelOctOptimizing();
            }
        }

        private void BtnAutoPolar_Click(object sender, RoutedEventArgs e)
        {
            if (!IsOctOptimizing())
            {
                StartAutoPolarization();
            }
            else
            {
                CancelOctOptimizing();
            }
        }

        private void BtnShowFocusM_Click(object sender, RoutedEventArgs e)
        {
            if (_focusMotorWindow == null || _focusMotorWindow.IsLoaded)
            {
                _focusMotorWindow = new();
                _focusMotorWindow.Show();
            }
            else
            {
                _focusMotorWindow?.Focus();
            }
        }

        private void BtnShowStageM_Click(object sender, RoutedEventArgs e)
        {
            if (_stageMotorWindow == null || !_stageMotorWindow.IsLoaded)
            {
                _stageMotorWindow = new();
                _stageMotorWindow.Show();
            }
            else
            {
                _stageMotorWindow?.Focus();
            }
        }

        private void BtnShowLight_Click(object sender, RoutedEventArgs e)
        {
            if (_lightControlWindow == null || !_lightControlWindow.IsLoaded)
            {
                _lightControlWindow = new();
                _lightControlWindow.Show();
            }
            else
            {
                _lightControlWindow.Focus();
            }
        }

        private void BtnAnalyzeResult_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (OctAnalysis.IsProtocolResultReady())
            {
                if (!OctAnalysis.ProcessProtocolResult(false, true, ref _sourceId))
                {
                    ShowError("Failed to process protocol result!");
                    Mouse.OverrideCursor = null;
                    return;
                }
            }

            if (OctAnalysis.AnalyzeProtocolData(_sourceId, true, ref _reportId))
            {
                _isResultAnalyzed = true;
                FetchProtocolInitParam();
                FetchReportProtocolDescript();
                FetchReportSummaryDescript();

                ReflectStratumOptionsByProtocol();
                UpdateLayerStratumParam();

                ReflectPatternEnfacePreview();
                ReflectPatternImageSelected(true);

                ReflectReportThicknessMap();
                ReflectReportSectionChart(false);
                ReflectProtocolSummary();
                ReflectReportSummary();

                RefelectChartCenterPosition();
                EnableButtonAtAnalyzed();
                ShowInfo("Protocol data analyzed successfully!");
            }
            else
            {
                ShowError("Failed to analyze protocol data!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnImportResult_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                var result = dialog.ShowDialog();
                if (result == System.Windows.Forms.DialogResult.OK)
                {
                    var path = dialog.SelectedPath;
                    if (OctAnalysis.ImportProtocolResult(path, false, ref _sourceId))
                    {
                        _openedExamPath = path;
                        octScanImagePreview.SetAsAnalysisMode();
                        octScanEnfacePreivew.SetAsAnalysisMode();
                        btnAnalyzeResult.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                    }
                    else
                    {
                        ShowError("Failed to import protocol result!");
                    }
                }
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnExportResult_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (_isResultAnalyzed)
            {
                string out_path;
                if (OctAnalysis.ExportProtocolResult(_sourceId, _exportRootPath, out out_path))
                {
                    ShowInfo("Protocol result exported!\nFile path: " + out_path);
                }
                else
                {
                    ShowError("Failed to export protocol result!");
                }
            }
            else
            {
                ShowError("Protocol result is not analyzed!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnUpdateLayers_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (_isResultAnalyzed)
            {
                UpdateLayerStratumParam();
                ReflectPatternEnfacePreview();
                ReflectPatternImagePreviewSelected();

                ReflectReportSectionChart(false);
            }
            else
            {
                ShowError("Protocol result is not analyzed!");
            }
            Mouse.OverrideCursor = null;
        }

        // Supplementary GUI event handlers
        /////////////////////////////////////////////////////////////////////////////////////////
        private void CmbScanPattern_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ReflectScanPointsSelected();
            ReflectScanRangeSelected();
        }

        private void CmbScanRangeX_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            int index = cmbScanRangeX.SelectedIndex;
            if (cmbScanRangeY.Items.Count > 0)
            {
                cmbScanRangeY.SelectedIndex = Math.Min(index, cmbScanRangeY.Items.Count - 1);
            }
        }

        private void CmbScanPoints_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            RefelectScanLinesSelected();
        }

        private void CmbScanLines_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ReflectScanOverlapsSelected();
        }

        private void EditPatternImageNumber_KeyDown(object sender, KeyEventArgs e)
        {
            int numb = NumberUtil.ToInt(editPatternImageNumber.Text);
            numb = Math.Min(Math.Max(1, numb), (int)sliderPatternImage.Maximum);
            sliderPatternImage.Value = numb;
        }

        private void SliderPatternImage_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderPatternImage_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            ReflectPatternImagePreviewSelected();
        }

        private void SliderPatternImage_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                ReflectPatternImagePreviewSelected();
            }
        }

        private void Window_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (!octScanImagePreview.IsPreviewMode)
            {
                if (sliderPatternImage.Maximum > 0)
                {
                    if (e.Delta > 0)
                    {
                        sliderPatternImage.Value -= 1;
                    }
                    else
                    {
                        sliderPatternImage.Value += 1;
                    }
                }
            }
        }
        private void RadioCameraSpeed1_Click(object sender, RoutedEventArgs e)
        {

        }

        private void RadioCameraSpeed2_Click(object sender, RoutedEventArgs e)
        {

        }

        private void RadioCameraSpeed3_Click(object sender, RoutedEventArgs e)
        {

        }

        private void CheckOverlayLayers_Click(object sender, RoutedEventArgs e)
        {
            octScanImagePreview.IsOverlayLayers = checkOverlayLayers.IsChecked == true;
            if (octScanImagePreview.IsAnalysisMode())
            {
                octScanImagePreview.UpdateFrameImage();
            }
        }

        private void CheckOverlayCenter_Click(object sender, RoutedEventArgs e)
        {
            octScanEnfacePreivew.IsOverlayCenter = checkOverlayCenter.IsChecked == true;
            if (octScanEnfacePreivew.IsAnalysisMode())
            {
                octScanEnfacePreivew.UpdateFrameImage();
            }
        }

        private void CheckFitToHeight_Click(object sender, RoutedEventArgs e)
        {
            octScanImagePreview.IsStretchToHeight = checkFitToHeight.IsChecked == true;
            if (octScanImagePreview.IsAnalysisMode())
            {
                octScanImagePreview.UpdateFrameImage();
            }
        }

        private void CheckOverlayLength_Click(object sender, RoutedEventArgs e)
        {
            octScanImagePreview.IsOverlayLength = checkOverlayLength.IsChecked == true;
            if (octScanImagePreview.IsAnalysisMode())
            {
                octScanImagePreview.UpdateFrameImage();
            }

            octScanEnfacePreivew.IsOverlayLength = checkOverlayLength.IsChecked == true;
            if (octScanEnfacePreivew.IsAnalysisMode())
            {
                octScanEnfacePreivew.UpdateFrameImage();
            }
        }
    }
}
