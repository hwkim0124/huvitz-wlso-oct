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
            TextBox textBox = (TextBox)sender;

            // Combine text considering selection replacement
            string currentText = textBox.Text;
            if (textBox.SelectionLength > 0)
            {
                currentText = currentText.Remove(textBox.SelectionStart, textBox.SelectionLength);
            }
            string input = currentText.Insert(textBox.SelectionStart, e.Text);

            // 1. Base Structure Regex (allows intermediate typing states)
            Regex baseRegex = new Regex(@"^[+-]?$|" +
                                        @"^[+-]?\d*\.?\d*([eE][+-]?\d*)?$|" +
                                        @"^[+-]?\.\d*([eE][+-]?\d*)?$");

            if (!baseRegex.IsMatch(input) || input == "." || input == "+." || input == "-." ||
                input.Equals("e", StringComparison.OrdinalIgnoreCase))
            {
                e.Handled = true;
                return;
            }

            // 2. Extract components for precision/exponent validation
            // Match groups: 1 = Sign/Digits/Decimal, 2 = Exponent Sign/Digits
            Match match = Regex.Match(input, @"^([+-]?\d*\.?\d*)(?:[eE]([+-]?\d*))?$", RegexOptions.IgnoreCase);
            if (match.Success)
            {
                string significand = match.Groups[1].Value;
                string exponentStr = match.Groups[2].Value;

                // Count significant digits (digits only, ignore signs and decimals)
                int digitCount = Regex.Replace(significand, @"[+-.]", "").Length;
                if (digitCount > 7) // Float max ~7 significant digits
                {
                    e.Handled = true;
                    return;
                }

                // Validate exponent range
                if (!string.IsNullOrEmpty(exponentStr) && exponentStr != "+" && exponentStr != "-")
                {
                    if (int.TryParse(exponentStr, out int exponentValue))
                    {
                        if (Math.Abs(exponentValue) > 38) // Float max exponent is 38
                        {
                            e.Handled = true;
                            return;
                        }
                    }
                }
            }

            e.Handled = false;
        }

        public static void PreviewDoubleInput(object sender, TextCompositionEventArgs e)
        {
            TextBox textBox = (TextBox)sender;

            // Combine text considering selection replacement
            string currentText = textBox.Text;
            if (textBox.SelectionLength > 0)
            {
                currentText = currentText.Remove(textBox.SelectionStart, textBox.SelectionLength);
            }
            string input = currentText.Insert(textBox.SelectionStart, e.Text);

            // 1. Base Structure Regex
            Regex baseRegex = new Regex(@"^[+-]?$|" +
                                        @"^[+-]?\d*\.?\d*([eE][+-]?\d*)?$|" +
                                        @"^[+-]?\.\d*([eE][+-]?\d*)?$");

            if (!baseRegex.IsMatch(input) || input == "." || input == "+." || input == "-." ||
                input.Equals("e", StringComparison.OrdinalIgnoreCase))
            {
                e.Handled = true;
                return;
            }

            // 2. Extract components for precision/exponent validation
            Match match = Regex.Match(input, @"^([+-]?\d*\.?\d*)(?:[eE]([+-]?\d*))?$", RegexOptions.IgnoreCase);
            if (match.Success)
            {
                string significand = match.Groups[1].Value;
                string exponentStr = match.Groups[2].Value;

                // Count significant digits
                int digitCount = Regex.Replace(significand, @"[+-.]", "").Length;
                if (digitCount > 15) // Double max ~15-17 significant digits (15 is safe)
                {
                    e.Handled = true;
                    return;
                }

                // Validate exponent range
                if (!string.IsNullOrEmpty(exponentStr) && exponentStr != "+" && exponentStr != "-")
                {
                    if (int.TryParse(exponentStr, out int exponentValue))
                    {
                        if (Math.Abs(exponentValue) > 308) // Double max exponent is 308
                        {
                            e.Handled = true;
                            return;
                        }
                    }
                }
            }

            e.Handled = false;
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
