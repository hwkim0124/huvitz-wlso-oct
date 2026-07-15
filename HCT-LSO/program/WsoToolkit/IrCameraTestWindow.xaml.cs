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

namespace WsoToolkit
{
    public partial class IrCameraTestWindow : Window
    {
        // bool _isDraggingStarted = false;

        public IrCameraTestWindow()
        {
            InitializeComponent();

            _onCorneaLeftFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLeftFrameCaptured);
            _onCorneaRightFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraRightFrameCaptured);
            _onCorneaLowerFrameCaptured = new WsoCallback.CorneaCameraFrameCaptured(this.OnCorneaCameraLowerFrameCaptured);
        }

        // Windows Event handlers
        /////////////////////////////////////////////////////////////////////////////////////////////
        private void Window_Initialized(object sender, EventArgs e)
        {

        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            InitializeWindowControls();

            btnStartCamera.Content = "Stop Camera";
            StartCorneaCameraPreview();

            corneaPreview1.IsOverlayAlignGuide = true;
            corneaPreview2.IsOverlayAlignGuide = true;
            corneaPreview3.IsOverlayAlignGuide = true;

            checkAlignGuide1.IsChecked = true;
            checkAlignGuide2.IsChecked = true;
            checkAlignGuide3.IsChecked = true;
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            _lightControlWindow?.Close();
            _focusMotorWindow?.Close();
            _stageMotorWindow?.Close();

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
        private void BtnStartCamera_Click(object sender, RoutedEventArgs e)
        {
            if (IsCorneaCameraLeftPreviewing())
            {
                btnStartCamera.Content = "Start Camera";
                CloseCorneaCameraPreview();
            }
            else
            {
                btnStartCamera.Content = "Stop Camera";
                StartCorneaCameraPreview();
            }
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
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

        private void BtnShowLights_Click(object sender, RoutedEventArgs e)
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
        
        private void BtnShowConfig_Click(object sender, RoutedEventArgs e)
        {
            //var window = new SystemConfigWindow { Owner = this };
            // window.ShowDialog();
        }

        private void EditCorneaAgain1_KeyDown(object sender, KeyEventArgs e)
        {
            SetCorneaCameraConfig();
        }

        private void EditCorneaDgain1_KeyDown(object sender, KeyEventArgs e)
        {
            SetCorneaCameraConfig();
        }

        private void EditCorneaAgain2_KeyDown(object sender, KeyEventArgs e)
        {
            SetCorneaCameraConfig();
        }

        private void EditCorneaDgain2_KeyDown(object sender, KeyEventArgs e)
        {
            SetCorneaCameraConfig();
        }

        private void EditCorneaAgain3_KeyDown(object sender, KeyEventArgs e)
        {
            SetCorneaCameraConfig();
        }

        private void EditCorneaDgain3_KeyDown(object sender, KeyEventArgs e)
        {
            SetCorneaCameraConfig();
        }

        private void checkAlignGuide1_Click(object sender, RoutedEventArgs e)
        {
            corneaPreview1.IsOverlayAlignGuide = checkAlignGuide1.IsChecked == true;
        }

        private void checkAlignGuide2_Click(object sender, RoutedEventArgs e)
        {
            corneaPreview2.IsOverlayAlignGuide = checkAlignGuide2.IsChecked == true;
        }

        private void checkAlignGuide3_Click(object sender, RoutedEventArgs e)
        {
            corneaPreview3.IsOverlayAlignGuide = checkAlignGuide3.IsChecked == true;
        }
    }
}
