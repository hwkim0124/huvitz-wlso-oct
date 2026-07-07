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
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.MsgBoxUtil;
    using static utils.TextBoxUtil;

    public partial class SystemConfigurationWindow : Window
    {
        public SystemConfigurationWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            ReflectConfigSettings();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {

        }

        private void Window_Closed(object sender, EventArgs e)
        {

        }

        private void LoadButton_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (!Configuration.LoadSystemConfiguration())
            {
                ShowWarning("Failed to load system configuration.");
            }
            else
            {
                ReflectConfigSettings();
                ShowInfo("System configuration loaded from system.");
            }
            Mouse.OverrideCursor = null;
        }

        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            UploadConfigSettings();
            if (!Configuration.SaveSystemConfiguration())
            {
                ShowWarning("Failed to save system configuration.");
            }
            else
            {
                ShowInfo("System configuration saved to system.");
            }
            Mouse.OverrideCursor = null;
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void BtnApply_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            UploadConfigSettings();
            if (!Configuration.ApplySystemConfiguration())
            {
                ShowWarning("Failed to apply system configuration to devices!");
            }
            else
            {
                ShowInfo("System configuration applied to devices.");
            }
            Mouse.OverrideCursor = null;
        }

        private void IntegerTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            PreviewIntegerInput(sender, e);
        }

        private void FloatTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            PreviewFloatInput(sender, e);
        }
    }
}
