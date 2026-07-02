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
    }
}
