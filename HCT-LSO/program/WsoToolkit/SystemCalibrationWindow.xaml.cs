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
using WsoToolkit;

namespace WsoToolkit
{
    using static utils.TextBoxUtil;
    using static utils.MsgBoxUtil;

    public partial class SystemCalibrationWindow : Window
    {
        public SystemCalibrationWindow()
        {
            InitializeComponent();
        }

        private void IntegerTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            PreviewIntegerInput(sender, e);
        }

        private void FloatTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            PreviewFloatInput(sender, e);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateSystemCalibration();
            UpdateDataToControls();
        }

        private void BtnFetch_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (!FetchSystemCalibration())
            {
                ShowError("Failed to fetch calibration from system memory!");
            }
            else
            {
                UpdateDataToControls();
                ShowInfo("Calibration fetched from system memory!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnWrite_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            CaptureDataFromControls();
            if (!WriteSystemCalibration())
            {
                ShowError("Failed to write calibration to system memory!");
            }
            else
            {
                ShowInfo("Calibration written to system memory!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnUpdate_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (!UpdateSystemCalibration())
            {
                ShowError("Failed to update items from system calibration!");
            }
            else
            {
                UpdateDataToControls();
                ShowInfo("Calibration items updated from system calibration!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnApply_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            CaptureDataFromControls();
            if (!ApplySystemCalibration())
            {
                ShowError("Failed to apply calibration to system devices!");
            }
            else
            {
                ShowInfo("Calibration applied to system devices!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
