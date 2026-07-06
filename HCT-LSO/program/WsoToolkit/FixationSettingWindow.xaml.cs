using System;
using System.Collections.Generic;
using System.Linq;
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
using WsoNativeLib;

namespace WsoToolkit
{
    /// <summary>
    /// Interaction logic for FixationSettingWindow.xaml
    /// </summary>
    public partial class FixationSettingWindow : Window
    {
        public FixationSettingWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            ReflectInternalFixationSettings();
        }

        private void myBtFixationApply_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            UploadConfigSettings();

            if (!Configuration.SaveSystemConfiguration())
            {
                MessageBox.Show("Failed to save system configuration.");
                return;
            }

            if (!Configuration.ApplySystemConfiguration())
            {
                MessageBox.Show("Failed to apply system configuration to devices!");
            }

            Mouse.OverrideCursor = null;
        }

        private void myBtFixationClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
