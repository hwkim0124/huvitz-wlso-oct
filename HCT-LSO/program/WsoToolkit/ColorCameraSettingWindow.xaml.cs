using WsoNativeLib;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.RightsManagement;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using static WsoNativeLib.WsoDevice;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for ColorCameraSettingWindow.xaml
    /// </summary>
    public partial class ColorCameraSettingWindow : Window
    {
        private readonly LsoScanTestModel _scanTestModel;

        public ColorCameraSettingWindow(LsoScanTestModel scanTestModel)
        {
            _scanTestModel = scanTestModel;
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            bInit = true;

            initControls_();
            loadParams_();
            updateControlDatas_();

            bInit = false;
        }

        private void myBtCenterROI_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            try {

                uint nMaxWidth = _colorCameraParam.roiMaxWidth;
                uint nMaxHeight = _colorCameraParam.roiMaxHeight;

                uint nXWidth = ToUInt(myTbROIXWidth.Text);
                uint nYHeight = ToUInt(myTbROIYHeight.Text);

                if (nXWidth > nMaxWidth)
                {
                    throw new Exception($"Width is more than Max Width. (Width : {nXWidth}, Max Width : {nMaxWidth})");
                }
                else if (nYHeight > nMaxHeight)
                {
                    throw new Exception($"Height is more than Max Height. (Height : {nYHeight}, Max Height : {nMaxHeight})");
                }

                uint nXOffset = (nMaxWidth - nXWidth)/2;
                uint nYOffset = (nMaxHeight - nYHeight)/2;

                if (nXOffset < 0 ||  nYOffset < 0)
                {
                    throw new Exception("Calculated offset value is not valid.");
                }

                myTbROIXOffset.Text = nXOffset.ToString();
                myTbROIYOffset.Text = nYOffset.ToString();

                setCameraParameters();
                loadParams_();
                updateControlDatas_();
            }
            catch (Exception ex) {
                MessageBox.Show(ex.Message);
            }
            return;
        }

        private void myBtMaxSize_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            uint nMaxWidth = _colorCameraParam.roiMaxWidth;
            uint nMaxHeight = _colorCameraParam.roiMaxHeight;

            myTbROIXWidth.Text = nMaxWidth.ToString();
            myTbROIYHeight.Text = nMaxHeight.ToString();
            myTbROIXOffset.Text = "0";
            myTbROIYOffset.Text = "0";

            setCameraParameters();
            loadParams_();
            updateControlDatas_();
        }

        private void myBtApply_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            setCameraParameters();
            loadParams_();
            updateControlDatas_();
        }

        private void myTb_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                clearTextBoxFocus_();

                setCameraParameters();
                loadParams_();
                updateControlDatas_();
            }
        }

        private void myCombo_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (bInit)
            {
                return;
            }

            clearTextBoxFocus_();

            setCameraParameters();
            loadParams_();
            updateControlDatas_();
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void myBtDefault_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            setDefaultCameraParameters_();

            loadParams_();
            updateControlDatas_();
        }

        private void myBtColorMode_Click(object sender, RoutedEventArgs e)
        {
            setDefaultColorMode_();

            setCameraParameters();
            loadParams_();
            updateControlDatas_();
        }

        private void myBtMonoMode_Click(object sender, RoutedEventArgs e)
        {
            setDefaultMonoMode_();

            setCameraParameters();
            loadParams_();
            updateControlDatas_();
        }
    }
}
