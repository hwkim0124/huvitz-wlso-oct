using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using WsoToolkit.utils;
using WsoNativeLib;

namespace WsoToolkit
{
    using static WsoNativeLib.WsoDevice;
    using static utils.NumberUtil;

    /// <summary>
    /// Manual control of every hardware LED the board exposes (the Toolkit counterpart of the
    /// DevKit's LED form). Each control names its light in Tag, so one handler set drives all
    /// six lights instead of one copied set per light.
    /// </summary>
    public partial class LightControlPanel : UserControl
    {
        // The board takes one byte per LED (LED_INTENSITY_MAX = 255 in WsoDevice2.h; the USB
        // command casts to uint8_t), so the sliders must span the whole hardware range.
        private const double IntensityMax = 255;

        private sealed class LightRow
        {
            public Slider Slider = null!;
            public TextBox Edit = null!;
            public CheckBox? Mode;   // null: the light has no trigger/continuous toggle
        }

        private readonly Dictionary<LightType, LightRow> _rows = new();

        private bool _isDraggingStarted = false;

        // True while values are being written INTO the controls from the device, so the
        // slider's ValueChanged does not send the value straight back.
        private bool _isLoadingFromDevice = false;

        public LightControlPanel()
        {
            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (_rows.Count == 0)
            {
                _rows[LightType.LsoWhite] = new LightRow { Slider = sliderWhite, Edit = editWhite, Mode = checkWhiteMode };
                _rows[LightType.RetinaIr] = new LightRow { Slider = sliderRetina, Edit = editRetina, Mode = checkRetinaMode };
                _rows[LightType.LsoBlue] = new LightRow { Slider = sliderBlue, Edit = editBlue, Mode = checkBlueMode };
                _rows[LightType.LsoGreen] = new LightRow { Slider = sliderGreen, Edit = editGreen, Mode = checkGreenMode };
                _rows[LightType.CorneaIrLeft] = new LightRow { Slider = sliderCornea1, Edit = editCornea1 };
                _rows[LightType.CorneaIrRight] = new LightRow { Slider = sliderCornea2, Edit = editCornea2 };

                foreach (LightRow row in _rows.Values)
                {
                    row.Slider.Minimum = 0;
                    row.Slider.Maximum = IntensityMax;
                }
            }

            Refresh();
        }

        /// <summary>
        /// Reads every light back from the device. The capture/preview one-call drives the same
        /// LEDs, so a panel left open would otherwise keep showing pre-acquisition values.
        /// </summary>
        public void Refresh()
        {
            foreach (LightType type in _rows.Keys)
            {
                readFromDevice_(type);
            }
        }

        private void readFromDevice_(LightType type)
        {
            LightRow row = _rows[type];

            _isLoadingFromDevice = true;
            try
            {
                ushort value = DeviceLights.GetLightIntensity(type);
                row.Slider.Value = value;
                row.Edit.Text = value.ToString();

                if (row.Mode != null)
                {
                    row.Mode.IsChecked = DeviceLights.GetLightMode(type) == LightMode.Continuous;
                }
            }
            finally
            {
                _isLoadingFromDevice = false;
            }
        }

        // Which light a control belongs to. Tag holds the LightType name (see the XAML).
        private static LightType lightOf_(object sender)
        {
            return (LightType)Enum.Parse(typeof(LightType), (string)((FrameworkElement)sender).Tag);
        }

        private void applyIntensity_(LightType type)
        {
            LightRow row = _rows[type];
            ushort value = (ushort)row.Slider.Value;

            row.Edit.Text = value.ToString();
            DeviceLights.SetLightIntensity(type, value);
        }

        private void Slider_DragStarted(object sender, DragStartedEventArgs e)
        {
            _isDraggingStarted = true;
        }

        private void Slider_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            _isDraggingStarted = false;
            applyIntensity_(lightOf_(sender));
        }

        private void Slider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isLoadingFromDevice || _rows.Count == 0)
            {
                return;
            }

            LightType type = lightOf_(sender);
            if (_isDraggingStarted)
            {
                // Drag in progress: show the value, write it once the thumb is released.
                _rows[type].Edit.Text = ((ushort)((Slider)sender).Value).ToString();
                return;
            }

            applyIntensity_(type);
        }

        private void Edit_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key != Key.Enter)
            {
                return;
            }

            LightRow row = _rows[lightOf_(sender)];
            row.Slider.Value = Math.Clamp(ToInt(row.Edit.Text), row.Slider.Minimum, row.Slider.Maximum);
        }

        private void BtnOn_Click(object sender, RoutedEventArgs e)
        {
            LightType type = lightOf_(sender);

            DeviceLights.TurnOnLight(type);
            readFromDevice_(type);
        }

        // Off switches the light off and leaves its intensity alone: the previous handler set
        // the slider to 0, which wrote intensity 0 to the board and lost the tuned value.
        private void BtnOff_Click(object sender, RoutedEventArgs e)
        {
            DeviceLights.TurnOffLight(lightOf_(sender));
        }

        private void CheckMode_Click(object sender, RoutedEventArgs e)
        {
            LightMode mode = ((CheckBox)sender).IsChecked == true ? LightMode.Continuous : LightMode.Trigger;
            DeviceLights.SetLightMode(lightOf_(sender), mode);
        }

        private void BtnRefresh_Click(object sender, RoutedEventArgs e)
        {
            Refresh();
        }
    }
}
