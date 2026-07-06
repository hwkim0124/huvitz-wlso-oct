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
    using static WsoNativeLib.WsoDevice;
    public partial class LightControlWindow : Window
    {
        bool _isDraggingStarted = false;

        public LightControlWindow()
        {
            InitializeComponent();
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            var value = DeviceLights.GetLightIntensity(LightType.LsoWhiteLed);
            var mode = DeviceLights.GetLightMode(LightType.LsoWhiteLed);
            sliderWhite.Minimum = 0;
            sliderWhite.Maximum = 100;
            sliderWhite.Value = value;

            editWhite.Text = value.ToString();
            checkWhiteMode.IsChecked = (mode == LightMode.Continuous);

            value = DeviceLights.GetLightIntensity(LightType.RetinaIrLed);
            mode = DeviceLights.GetLightMode(LightType.RetinaIrLed);
            sliderRetina.Minimum = 0;
            sliderRetina.Maximum = 100;
            sliderRetina.Value = value;

            editRetina.Text = value.ToString();
            checkRetinaMode.IsChecked = (mode == LightMode.Continuous);

            value = DeviceLights.GetLightIntensity(LightType.CorneaIrLedLeft);
            mode = DeviceLights.GetLightMode(LightType.CorneaIrLedLeft);
            sliderCornea1.Minimum = 0;
            sliderCornea1.Maximum = 100;
            sliderCornea1.Value = value;

            editCornea1.Text = value.ToString();

            value = DeviceLights.GetLightIntensity(LightType.CorneaIrLedRight);
            mode = DeviceLights.GetLightMode(LightType.CorneaIrLedRight);
            sliderCornea2.Minimum = 0;
            sliderCornea2.Maximum = 100;
            sliderCornea2.Value = value;

            editCornea2.Text = value.ToString();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
        }

        private void SliderWhite_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderWhite_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.LsoWhiteLed, (ushort)sliderWhite.Value);
        }

        private void SliderWhite_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.LsoWhiteLed, (ushort)sliderWhite.Value);
            }
        }
        
        private void EditWhite_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.LsoWhiteLed, (ushort)sliderWhite.Value);
        }

        private void BtnWhiteOff_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.LsoWhiteLed);
        }

        private void BtnWhiteOn_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.LsoWhiteLed);
        }

        private void CheckWhiteMode_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.SetLightMode(LightType.LsoWhiteLed, checkWhiteMode.IsChecked == true ? LightMode.Continuous : LightMode.Trigger);
        }

        private void SliderRetina_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderRetina_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.RetinaIrLed, (ushort)sliderRetina.Value);
        }

        private void SliderRetina_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.RetinaIrLed, (ushort)sliderRetina.Value);
            }
        }

        private void EditRetina_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.RetinaIrLed, (ushort)sliderRetina.Value);
        }

        private void BtnRetinaOff_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.RetinaIrLed);
        }

        private void BtnRetinaOn_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.RetinaIrLed);
        }

        private void CheckRetinaMode_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.SetLightMode(LightType.RetinaIrLed, checkRetinaMode.IsChecked == true ? LightMode.Continuous : LightMode.Trigger);
        }

        private void SliderCornea1_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderCornea1_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.CorneaIrLedLeft, (ushort)sliderCornea1.Value);
        }

        private void SliderCornea1_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.CorneaIrLedLeft, (ushort)sliderCornea1.Value);
            }
        }

        private void EditCornea1_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.CorneaIrLedLeft, (ushort)sliderCornea1.Value);
        }

        private void BtnCorneaOff1_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.CorneaIrLedLeft);
        }

        private void BtnCorneaOn1_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.CorneaIrLedLeft);
        }

        private void SliderCornea2_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderCornea2_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.CorneaIrLedRight, (ushort)sliderCornea2.Value);
        }

        private void SliderCornea2_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.CorneaIrLedRight, (ushort)sliderCornea2.Value);
            }
        }

        private void EditCornea2_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.CorneaIrLedRight, (ushort)sliderCornea2.Value);
        }

        private void BtnCorneaOff2_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.CorneaIrLedRight);
        }

        private void BtnCorneaOn2_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.CorneaIrLedRight);
        }
    }
}
