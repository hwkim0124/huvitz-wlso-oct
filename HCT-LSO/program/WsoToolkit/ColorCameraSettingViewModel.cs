using OpenCvSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using WsoNativeLib;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.utils.NumberUtil;
using static WsoNativeLib.LsoCamera;

namespace WsoToolkit
{
    public partial class ColorCameraSettingWindow
    {
        bool bInit = false;

        private List<TextBox> _imageFormatInputBoxsList = new List<TextBox>();
        private LsoColorCameraSettingParam _colorCameraParam = new LsoColorCameraSettingParam();

        private void syncModelAndIni_()
        {
            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam param);
            _scanTestModel.ColorCamera.ApplySettingParam(param);
            _scanTestModel.SaveConfigToIniFile();
        }

        private List<string> _AcqModeList = new List<string>() { "Continuous", "Single Frame", "Multi Frame"};
        private SortedDictionary<string, uint> _PixelFormatList = new SortedDictionary<string, uint>() 
        {
            { "Mono8", (uint)ColorPixelFormat.Mono8 },
            { "Mono16", (uint)ColorPixelFormat.Mono16 },
            //{ "RGB8Packed", (uint)ColorPixelFormat.RGB8Packed },
            { "BayerRG8", (uint)ColorPixelFormat.BayerRG8 },
            { "BayerRG16", (uint)ColorPixelFormat.BayerRG16 },
            //{ "BGR8", (uint)ColorPixelFormat.BGR8 },
        };
        private SortedDictionary<string, uint> _AdcBitDepthList = new SortedDictionary<string, uint>()
        {
            { "10 Bit", (uint)ColorAdcBitDepth.Bit10 },
            { "12 Bit", (uint)ColorAdcBitDepth.Bit12 },
        };

        private SortedDictionary<string, uint> _BinningValueList = new SortedDictionary<string, uint>()
        {
            { "1", 1 },
            { "2", 2 },
        };

        private void initControls_()
        {
            _imageFormatInputBoxsList.Add(myTbROIXWidth);
            _imageFormatInputBoxsList.Add(myTbROIYHeight);
            _imageFormatInputBoxsList.Add(myTbROIXOffset);
            _imageFormatInputBoxsList.Add(myTbROIYOffset);
            _imageFormatInputBoxsList.Add(myTbExposureTime);
            _imageFormatInputBoxsList.Add(myTbAcquisitionFrmaeCount);
            _imageFormatInputBoxsList.Add(myTbGain);

            myComboAcquisitionMode.ItemsSource = _AcqModeList;

            foreach (var pixelformat in _PixelFormatList)
            {
                var item = new ComboBoxItem { Content = pixelformat.Key, Tag = pixelformat.Value };
                myComboPixelFormat.Items.Add(item);
            }

            foreach (var depth in _AdcBitDepthList)
            {
                var item = new ComboBoxItem { Content = depth.Key, Tag = depth.Value };
                myComboADCBitDepth.Items.Add(item);
            }

            foreach (var value in _BinningValueList)
            {
                var item = new ComboBoxItem { Content = value.Key, Tag = value.Value };
                myComboBinningHorizontal.Items.Add(item);
            }

            foreach (var value in _BinningValueList)
            {
                var item = new ComboBoxItem { Content = value.Key, Tag = value.Value };
                myComboBinningVertical.Items.Add(item);
            }
        }

        private void loadParams_()
        {
            LsoCamera.GetCameraParameters(out _colorCameraParam);
        }

        private void updateControlDatas_()
        {
            myTbROIXWidth.Text = _colorCameraParam.roiXWidth.ToString();
            myTbROIYHeight.Text = _colorCameraParam.roiYHeight.ToString();
            myTbROIXOffset.Text = _colorCameraParam.roiXOffset.ToString();
            myTbROIYOffset.Text = _colorCameraParam.roiYOffset.ToString();
            myTbExposureTime.Text = _colorCameraParam.exposureTime.ToString();
            myTbGain.Text = _colorCameraParam.gain.ToString();

            myComboAcquisitionMode.SelectedIndex = (int)_colorCameraParam.acquisitionMode;

            myTbAcquisitionFrmaeCount.Text = _colorCameraParam.acquisitionFrameCount.ToString();

            uint nPixelFormat = _colorCameraParam.pixelFormat;
            foreach (ComboBoxItem item in myComboPixelFormat.Items)
            {
                if (item.Tag is uint value && value == nPixelFormat)
                {
                    myComboPixelFormat.SelectedItem = item;
                }
            }

            uint nBinningHorizontal = _colorCameraParam.binningHorizontal;
            foreach (ComboBoxItem item in myComboBinningHorizontal.Items)
            {
                if (item.Tag is uint value && value == nBinningHorizontal)
                {
                    myComboBinningHorizontal.SelectedItem = item;
                }
            }

            uint nBinningVertical = _colorCameraParam.binningVertical;
            foreach (ComboBoxItem item in myComboBinningVertical.Items)
            {
                if (item.Tag is uint value && value == nBinningVertical)
                {
                    myComboBinningVertical.SelectedItem = item;
                }
            }

            uint nAdcBitDepth = _colorCameraParam.adcDepthIndex;
            foreach (ComboBoxItem item in myComboADCBitDepth.Items)
            {
                if (item.Tag is uint value && value == nAdcBitDepth)
                {
                    myComboADCBitDepth.SelectedItem = item;
                }
            }
        }

        private void clearTextBoxFocus_()
        {
            // 1) TextBox 포커스 잃기
            Keyboard.ClearFocus();

            foreach (TextBox Tb in _imageFormatInputBoxsList)
            {
                var be = Tb.GetBindingExpression(TextBox.TextProperty); be?.UpdateSource();
            }
        }

        private bool setCameraParameters()
        {
            bool bRet = false;

            try
            {
                LsoColorCameraSettingParam colorCameraParam = new LsoColorCameraSettingParam();

                colorCameraParam.roiXWidth = ToUInt(myTbROIXWidth.Text);
                colorCameraParam.roiYHeight = ToUInt(myTbROIYHeight.Text);
                colorCameraParam.roiXOffset = ToUInt(myTbROIXOffset.Text);
                colorCameraParam.roiYOffset = ToUInt(myTbROIYOffset.Text);
                colorCameraParam.exposureTime = ToUInt(myTbExposureTime.Text);
                colorCameraParam.gain = ToFloat(myTbGain.Text);

                colorCameraParam.acquisitionMode = (uint)myComboAcquisitionMode.SelectedIndex;

                colorCameraParam.acquisitionFrameCount = ToUInt(myTbAcquisitionFrmaeCount.Text);

                ComboBoxItem item = (ComboBoxItem)myComboPixelFormat.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.pixelFormat = (uint)item.Tag;
                }

                item = (ComboBoxItem)myComboBinningHorizontal.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.binningHorizontal = (uint)item.Tag;
                }

                item = (ComboBoxItem)myComboBinningVertical.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.binningVertical = (uint)item.Tag;
                }

                item = (ComboBoxItem)myComboADCBitDepth.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.adcDepthIndex = (uint)item.Tag;
                }

                LsoCamera.SetCameraParameters(ref colorCameraParam);
                syncModelAndIni_();
                bRet = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                bRet = false;
            }

            return bRet;
        }

        private bool setDefaultCameraParameters_()
        {
            bool bRet = false;

            try
            {
                LsoColorCameraSettingParam colorCameraParam = new LsoColorCameraSettingParam();

                colorCameraParam.roiXWidth = 4000;
                colorCameraParam.roiYHeight = 3000;
                colorCameraParam.roiXOffset = 736;
                colorCameraParam.roiYOffset = 324;
                colorCameraParam.exposureTime = 43997;
                colorCameraParam.gain = 1;
                colorCameraParam.acquisitionMode = 0;
                colorCameraParam.acquisitionFrameCount = 2;
                colorCameraParam.pixelFormat = 4;
                colorCameraParam.adcDepthIndex = 2;

                LsoCamera.SetCameraParameters(ref colorCameraParam);
                syncModelAndIni_();
                bRet = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                bRet = false;
            }

            return bRet;
        }

        private void setDefaultColorMode_()
        {
            uint nMode = (uint)ColorPixelFormat.BayerRG8;
            foreach (ComboBoxItem item in myComboPixelFormat.Items)
            {
                if (item.Tag is uint value && value == nMode)
                {
                    myComboPixelFormat.SelectedItem = item;
                    break;
                }
            }

            uint nBinningHorizontal = 1;
            foreach (ComboBoxItem item in myComboBinningHorizontal.Items)
            {
                if (item.Tag is uint value && value == nBinningHorizontal)
                {
                    myComboBinningHorizontal.SelectedItem = item;
                    break;
                }
            }

            uint nBinningVertical = 1;
            foreach (ComboBoxItem item in myComboBinningVertical.Items)
            {
                if (item.Tag is uint value && value == nBinningVertical)
                {
                    myComboBinningVertical.SelectedItem = item;
                    break;
                }
            }

            uint nMaxWidth = _colorCameraParam.roiMaxWidth;
            uint nMaxHeight = _colorCameraParam.roiMaxHeight;

            myTbROIXWidth.Text = nMaxWidth.ToString();
            myTbROIYHeight.Text = nMaxHeight.ToString();
            myTbROIXOffset.Text = "0";
            myTbROIYOffset.Text = "0";
        }

        private void setDefaultMonoMode_()
        {
            uint nMode = (uint)ColorPixelFormat.Mono8;
            foreach (ComboBoxItem item in myComboPixelFormat.Items)
            {
                if (item.Tag is uint value && value == nMode)
                {
                    myComboPixelFormat.SelectedItem = item;
                    break;
                }
            }

            uint nBinningHorizontal = 2;
            foreach (ComboBoxItem item in myComboBinningHorizontal.Items)
            {
                if (item.Tag is uint value && value == nBinningHorizontal)
                {
                    myComboBinningHorizontal.SelectedItem = item;
                    break;
                }
            }

            uint nBinningVertical = 2;
            foreach (ComboBoxItem item in myComboBinningVertical.Items)
            {
                if (item.Tag is uint value && value == nBinningVertical)
                {
                    myComboBinningVertical.SelectedItem = item;
                    break;
                }
            }

            uint nMaxWidth = _colorCameraParam.roiMaxWidth;
            uint nMaxHeight = _colorCameraParam.roiMaxHeight;

            myTbROIXWidth.Text = nMaxWidth.ToString();
            myTbROIYHeight.Text = nMaxHeight.ToString();
            myTbROIXOffset.Text = "0";
            myTbROIYOffset.Text = "0";
        }
    }
}
