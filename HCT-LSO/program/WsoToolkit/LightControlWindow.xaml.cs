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
            var value = DeviceLights.GetLightIntensity(LightType.LsoWhite);
            var mode = DeviceLights.GetLightMode(LightType.LsoWhite);
            sliderWhite.Minimum = 0;
            sliderWhite.Maximum = 100;
            sliderWhite.Value = value;

            editWhite.Text = value.ToString();
            checkWhiteMode.IsChecked = (mode == LightMode.Continuous);

            value = DeviceLights.GetLightIntensity(LightType.RetinaIr);
            mode = DeviceLights.GetLightMode(LightType.RetinaIr);
            sliderRetina.Minimum = 0;
            sliderRetina.Maximum = 100;
            sliderRetina.Value = value;

            editRetina.Text = value.ToString();
            checkRetinaMode.IsChecked = (mode == LightMode.Continuous);

            value = DeviceLights.GetLightIntensity(LightType.CorneaIrLeft);
            mode = DeviceLights.GetLightMode(LightType.CorneaIrLeft);
            sliderCornea1.Minimum = 0;
            sliderCornea1.Maximum = 100;
            sliderCornea1.Value = value;

            editCornea1.Text = value.ToString();

            value = DeviceLights.GetLightIntensity(LightType.CorneaIrRight);
            mode = DeviceLights.GetLightMode(LightType.CorneaIrRight);
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
            DeviceLights.SetLightIntensity(LightType.LsoWhite, (ushort)sliderWhite.Value);
            editWhite.Text = sliderWhite.Value.ToString();
        }

        private void SliderWhite_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.LsoWhite, (ushort)sliderWhite.Value);
                editWhite.Text = sliderWhite.Value.ToString();
            }
        }
        
        private void EditWhite_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.LsoWhite, (ushort)sliderWhite.Value);
        }

        private void BtnWhiteOff_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.LsoWhite);
            sliderWhite.Value = 0;
        }

        private void BtnWhiteOn_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.LsoWhite);
            sliderWhite.Value = DeviceLights.GetLightIntensity(LightType.LsoWhite);
        }

        private void CheckWhiteMode_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.SetLightMode(LightType.LsoWhite, checkWhiteMode.IsChecked == true ? LightMode.Continuous : LightMode.Trigger);
        }

        private void SliderRetina_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderRetina_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.RetinaIr, (ushort)sliderRetina.Value);
            editRetina.Text = sliderRetina.Value.ToString();
        }

        private void SliderRetina_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.RetinaIr, (ushort)sliderRetina.Value);
                editRetina.Text = sliderRetina.Value.ToString();
            }
        }

        private void EditRetina_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.RetinaIr, (ushort)sliderRetina.Value);
        }

        private void BtnRetinaOff_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.RetinaIr);
            sliderRetina.Value = 0;
        }

        private void BtnRetinaOn_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.RetinaIr);
            sliderRetina.Value = DeviceLights.GetLightIntensity(LightType.RetinaIr);
        }

        private void CheckRetinaMode_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.SetLightMode(LightType.RetinaIr, checkRetinaMode.IsChecked == true ? LightMode.Continuous : LightMode.Trigger);
        }

        private void SliderCornea1_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderCornea1_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.CorneaIrLeft, (ushort)sliderCornea1.Value);
            editCornea1.Text = sliderCornea1.Value.ToString();
        }

        private void SliderCornea1_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.CorneaIrLeft, (ushort)sliderCornea1.Value);
                editCornea1.Text = sliderCornea1.Value.ToString();
            }
        }

        private void EditCornea1_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.CorneaIrLeft, (ushort)sliderCornea1.Value);
        }

        private void BtnCorneaOff1_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.CorneaIrLeft);
            sliderCornea1.Value = 0;
        }

        private void BtnCorneaOn1_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.CorneaIrLeft);
            sliderCornea1.Value = DeviceLights.GetLightIntensity(LightType.CorneaIrLeft);
        }

        private void SliderCornea2_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }
        private void SliderCornea2_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            DeviceLights.SetLightIntensity(LightType.CorneaIrRight, (ushort)sliderCornea2.Value);
            editCornea2.Text = sliderCornea2.Value.ToString();
        }

        private void SliderCornea2_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!_isDraggingStarted)
            {
                DeviceLights.SetLightIntensity(LightType.CorneaIrRight, (ushort)sliderCornea2.Value);
                editCornea2.Text = sliderCornea2.Value.ToString();
            }
        }

        private void EditCornea2_KeyDown(object sender, KeyEventArgs e)
        {
            DeviceLights.SetLightIntensity(LightType.CorneaIrRight, (ushort)sliderCornea2.Value);
        }

        private void BtnCorneaOff2_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(LightType.CorneaIrRight);
            sliderCornea2.Value = 0;
        }

        private void BtnCorneaOn2_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOnLight(LightType.CorneaIrRight);
            sliderCornea2.Value = DeviceLights.GetLightIntensity(LightType.CorneaIrRight);
        }
    }
}
