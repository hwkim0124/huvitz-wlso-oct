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
using WsoToolkit.utils;
using WsoNativeLib;

namespace WsoToolkit
{ 
    public partial class LightControlWindow : Window
    {
        public LightControlWindow()
        {
            InitializeComponent();
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            DeviceLights.ObtainOctSldStatusParam(ref _sldStatus, true);
            DeviceLights.ObtainOctSldCalibParam(ref _sldStatus.calibParam, true);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            InitializeControls();
            StartRefreshTimer();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            _timer.Stop();
        }

        private void BtnSldOn_Click(object sender, RoutedEventArgs e)
        {
            if (DeviceLights.TurnOnOctSld())
            {
                btnSldOn.IsEnabled = false;
                btnSldOff.IsEnabled = true;
            }
        }

        private void BtnSldOff_Click(object sender, RoutedEventArgs e)
        {
            if (DeviceLights.TurnOffOctSld())
            {
                btnSldOn.IsEnabled = true;
                btnSldOff.IsEnabled = false;
            }
        }

        private void BtnSldApply_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            SubmitCalibrationParams();
            MsgBoxUtil.ShowInfo("Oct SLD calibration applied!");
            Mouse.OverrideCursor = null;
        }

        private void BtnSldSave_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            SubmitCalibrationParams();
            if (!DeviceLights.SaveOctSldCalibration())
            {
                MsgBoxUtil.ShowWarning("Oct SLD calibration not saved in system!");
            }
            else
            {
                MsgBoxUtil.ShowInfo("Oct SLD calibration saved in system!");
            }
            Mouse.OverrideCursor = null;
        }

        private void BtnSldGet_Click(object sender, RoutedEventArgs e)
        {
            Mouse.OverrideCursor = Cursors.Wait;
            if (DeviceLights.LoadOctSldCalibration())
            {
                DeviceLights.ObtainOctSldCalibParam(ref _sldStatus.calibParam, true);
                PopulateCalibParamControls();
                MsgBoxUtil.ShowInfo("Oct SLD calibration loaded!");
            }
            else
            {
                MsgBoxUtil.ShowWarning("Oct SLD calibration not loaded!");
            }
            Mouse.OverrideCursor = null;
        }

        private void EditSldHighCode_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SubmitHighCode();
            }
        }

        private void EditSldLowCode1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SubmitLowCode1();
            }
        }
        private void EditSldLowCode2_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SubmitLowCode2();
            }
        }
        private void EditSldRsiCode_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SubmitRsiCode();
            }
        }
    }
}
