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

namespace WsoToolkit
{
    public partial class MainWindow : Window
    {
        WsoCallback.WsoLogMsgCallback _logMsgCallback;
        ObservableCollection<LogMsgItem> _logMsgItems;
        DispatcherTimer _timer = new DispatcherTimer();

        private bool StartWsoSystem()
        {
            Bootstrapper.InitializeWsoSystem(_logMsgCallback, true);
            bool result = Bootstrapper.IsWsoSystemInitialized();
            return result;
        }

        private bool ShutdownWsoSystem()
        {
            bool result = Bootstrapper.IsWsoSystemInitialized();
            if (result)
            {
                Bootstrapper.ReleaseWsoSystem();
            }
            return true;
        }

        private void StartInitialTimer()
        {
            _timer.Interval = TimeSpan.FromSeconds(1.0);
            _timer.Tick += InitialTimerTick;
            _timer.Start();
        }

        private void InitialTimerTick(object? sender, EventArgs e)
        {
            // StartupButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
            _timer.Stop();
        }

        private void CallbackWsoLogMessage(string msg, int type)
        {
            Dispatcher.Invoke(() =>
            {
                _logMsgItems.Add(new LogMsgItem() { Message = msg, Type = type });
                LogMsgListBox.ScrollIntoView(_logMsgItems[_logMsgItems.Count - 1]);

            }, DispatcherPriority.Background);
        }

        private struct LogMsgItem
        {
            private string _message;
            public string Message
            {
                get
                {
                    return _message;
                }
                set
                {
                    string time_tag = DateTime.Now.ToString("[dd/MM/yy HH:mm:ss] ");
                    _message = time_tag + value;
                }
            }
            public int Type { get; set; }
        }
    }
}