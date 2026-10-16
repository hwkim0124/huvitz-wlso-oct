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
using WsoNativeLib.Presets;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for ColorCameraSettingWindow.xaml
    /// </summary>
    public partial class ColorCameraSettingWindow : UserControl
    {
        private readonly LsoScanTestModel _scanTestModel;

        // Which mode's preset this dialog edits. The dialog itself is not forked per mode -
        // the target preset is the only difference.
        private readonly ColorCameraPresetMode _mode;

        public ColorCameraSettingWindow(LsoScanTestModel scanTestModel, ColorCameraPresetMode mode)
        {
            _scanTestModel = scanTestModel;
            _mode = mode;
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

        // Hosting window API: the panel edits one preset's camera section, the window owns
        // the Apply/Default/Close for the whole preset.
        internal void ReloadFromPreset()
        {
            bInit = true;
            try
            {
                loadParams_();
                updateControlDatas_();
            }
            finally
            {
                bInit = false;
            }
        }

        internal bool CommitToPreset()
        {
            clearTextBoxFocus_();
            bool ok = applyControlsToPreset_();
            ReloadFromPreset();
            return ok;
        }

        internal void ApplyDefaults()
        {
            clearTextBoxFocus_();
            setDefaultCameraParameters_();
            ReloadFromPreset();
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

                applyControlsToPreset_();
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

            applyControlsToPreset_();
            loadParams_();
            updateControlDatas_();
        }

        private void myBtApply_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            applyControlsToPreset_();
            loadParams_();
            updateControlDatas_();
        }

        private void myTb_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                clearTextBoxFocus_();

                applyControlsToPreset_();
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

            applyControlsToPreset_();
            loadParams_();
            updateControlDatas_();
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e)
        {
        }

        private void myBtDefault_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            setDefaultCameraParameters_();

            loadParams_();
            updateControlDatas_();
        }

        // Pull the extended nodes back out of the camera into this preset, so the editor can
        // be filled from what the device is really holding instead of only ever writing to it.
        private void myBtReadFromCamera_Click(object sender, RoutedEventArgs e)
        {
            clearTextBoxFocus_();

            // Commit first: a read-back only fills the nodes the camera reports, and the
            // operator's other edits must not be thrown away by the reload that follows.
            applyControlsToPreset_();
            readExtendedFromCamera_();

            loadParams_();
            updateControlDatas_();
        }

        private void myBtColorMode_Click(object sender, RoutedEventArgs e)
        {
            setDefaultColorMode_();

            applyControlsToPreset_();
            loadParams_();
            updateControlDatas_();
        }

        private void myBtMonoMode_Click(object sender, RoutedEventArgs e)
        {
            setDefaultMonoMode_();

            applyControlsToPreset_();
            loadParams_();
            updateControlDatas_();
        }
    }
}
