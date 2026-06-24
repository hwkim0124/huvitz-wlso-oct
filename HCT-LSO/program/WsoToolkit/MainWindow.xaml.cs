using System;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static WsoNativeLib.WsoCallback;

    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            this.Title = "Huvitz Wide LSO-OCT Toolkit (ver 0.01.0 - 2026/06/17)";

            _logMsgCallback = new WsoLogMsgCallback(this.CallbackWsoLogMessage);
            _logMsgItems = new ObservableCollection<LogMsgItem>();
        }

        private void WindowMain_Loaded(object sender, RoutedEventArgs e)
        {
            // Data binding for log message list box. 
            LogMsgListBox.ItemsSource = _logMsgItems;
            StartInitialTimer();
        }

        private void StartupButton_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (StartWsoSystem())
            {
                StartupButton.IsEnabled = false;
                ReleaseButton.IsEnabled = true;
            }
            Mouse.OverrideCursor = null;
        }

        private void ReleaseButton_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (ShutdownWsoSystem())
            {
                StartupButton.IsEnabled = true;
                ReleaseButton.IsEnabled = false;
            }
            Mouse.OverrideCursor = null;
        }

        private void PatientListButton_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new PatientListWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void ConfigreButton_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new SystemConfigWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void CalibrationButton_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new SystemCalibWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void BtnSloScanTest_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new SloScanTestWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void BtnSloScanMode_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new SloScanModeWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void btnOctScanMode_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new OctScanModeWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void btnOctAngioMode_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new OctAngioModeWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void btnOctSignalTest_Click(object sender, RoutedEventArgs e)
        {
            var window = new OctSignalTestWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        private void BtnOctSpectroCalib_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new OctSpectrometerWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void BtnLongRunTest_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new LongRunTestWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void BtnIrCameraTest_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new IrCameraTestWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void BtnDeviceMotorTest_Click(object sender, RoutedEventArgs e)
        {
            var window = new DeviceMotorWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        private void BtnOctSldControl_Click(object sender, RoutedEventArgs e)
        {
            var window = new LightControlWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        private void BtnSloQldControl_Click(object sender, RoutedEventArgs e)
        {
            /*
            var window = new LaserControlWindow
            {
                Owner = this
            };
            window.ShowDialog();
            */
        }

        private void BtnXadcMonitor_Click(object sender, RoutedEventArgs e)
        {
            var window = new XadcMonitorWindow
            {
                Owner = this
            };
            window.ShowDialog();
        }

        private void WindowMain_Closed(object sender, EventArgs e)
        {
            _timer.Stop();
        }

        private void WindowMain_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (MsgBoxUtil.ShowQuestion("Are you sure to close this window?") == false)
            {
                e.Cancel = true;
            }
            else
            {
                Mouse.OverrideCursor = Cursors.Wait;
                ShutdownWsoSystem();
                Mouse.OverrideCursor = null;
            }
        }
    }
}