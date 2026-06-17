using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace WsoToolkit.utils
{
    internal static class MsgBoxUtil
    {
        static string _title = "WSLO Toolkit";

        public static void ShowInfo(string str)
        {
            MessageBox.Show(str, _title, MessageBoxButton.OK, MessageBoxImage.Information);
        }

        public static void ShowError(string str)
        {
            MessageBox.Show(str, _title, MessageBoxButton.OK, MessageBoxImage.Error);
        }

        public static void ShowWarning(string str)
        {
            MessageBox.Show(str, _title, MessageBoxButton.OK, MessageBoxImage.Warning);
        }

        public static bool ShowQuestion(string str)
        {
            return MessageBox.Show(str, _title, MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes;
        }
    }
}
