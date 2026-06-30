using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Controls;
using System.Windows.Input;

namespace WsoToolkit.utils
{
    internal static class TextBoxUtil
    {
        public static void PreviewFloatInput(object sender, TextCompositionEventArgs e)
        {
            string input = ((TextBox)sender).Text + e.Text;
            Regex regex = new Regex(@"^[+-]?$|^[+-]?(\d+(\.\d*)?|\.\d+)$");
            e.Handled = !regex.IsMatch(input);
        }

        public static void PreviewIntegerInput(object sender, TextCompositionEventArgs e)
        {
            string input = ((TextBox)sender).Text + e.Text;
            Regex regex = new Regex(@"^[+-]?$|^[+-]?\d+$");
            e.Handled = !regex.IsMatch(input);
        }

        public static void PreviewPositionInput(object sender, TextCompositionEventArgs e)
        {
            string input = ((TextBox)sender).Text + e.Text;
            Regex regex = new Regex(@"^[+-]?$|^[+-]?\d+$");
            e.Handled = !regex.IsMatch(input);
        }
    }
}
