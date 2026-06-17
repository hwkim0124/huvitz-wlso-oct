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
    using static utils.NumberUtil;

    public partial class XadcMonitorWindow : Window
    {
        public XadcMonitorWindow()
        {
            InitializeComponent();
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            FetchAllDeviceStatus();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateCurrentStatus();
            StartRefreshTimer();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            _timer.Stop();
        }


    }
}
