using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using static WsoToolkit.controls.LsoScanImagePreview;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for LsoScanTestWindow.xaml
    /// </summary>
    public partial class LsoScanTestWindow : Window
    {
        public LsoScanTestWindow()
        {
            InitializeComponent();

            initCallbacks_();
            initSetting_();
        }

        private void myBtStartScan_Click(object sender, RoutedEventArgs e)
        {

        }

        private void myBtCapture_Click(object sender, RoutedEventArgs e)
        {
            //if (IsRetinaCameraPreviewing())
            //{
            //    CloseRetinaCameraPreview();
            //    closeCorneaCameraPreview_();
            //    myBtStartScan.Content = "Start Scan";
            //}

            readyToCapture_(PreviewDisplayMode.REVIEW);

            StartColorCameraOriginal();
        }

        private void myBtSettingColorCamera_Click(object sender, RoutedEventArgs e)
        {
            var window = new ColorCameraSettingWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        private void myCbPatternID_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdatingScannerControls)
            {
                return;
            }

            updateScannerControls_();
        }

        private void myCbTriggerSrc_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdatingScannerControls)
            {
                return;
            }

            int nPatternId = myCbPatternID.SelectedIndex;
            _scanTestModel.GetScanProfile(nPatternId).TriggerSource = (ushort)myCbTriggerSrc.SelectedIndex;
            applyScannerControlParam_(nPatternId);
        }

        private void myCheckBoxFixedFrame_CheckedNUnchecked(object sender, RoutedEventArgs e)
        {
            if (_isUpdatingScannerControls)
            {
                return;
            }

            int nPatternId = myCbPatternID.SelectedIndex;
            _scanTestModel.GetScanProfile(nPatternId).AcquisitionMode = (ushort)(myCheckBoxFixedFrame.IsChecked == true ? 1 : 0);
            applyScannerControlParam_(nPatternId);
        }

        private void myTbScanSettings_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            commitScanSettingTextBox_(sender as TextBox);
        }

        private void myTbScanSettings_LostFocus(object sender, RoutedEventArgs e)
        {
            commitScanSettingTextBox_(sender as TextBox);
        }

        private void myBtApply_Click(object sender, RoutedEventArgs e)
        {
            int nPatternId = myCbPatternID.SelectedIndex;
            if (nPatternId < 0)
            {
                return;
            }

            applyScannerControlParam_(nPatternId);
            _scanTestModel.SaveConfigToIniFile();
        }
    }
}
