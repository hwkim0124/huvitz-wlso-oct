using System.Windows.Controls;
using WsoNativeLib.Presets;

namespace WsoToolkit
{
    // Color illumination of the Capture preset: White (fundus_mode 0, the default) or RGB
    // (fundus_mode 3). The choice only means something for Color mode, so the combo is enabled
    // only while Color is selected; FA and ICG stay unavailable through LsoImageModes.Available.
    public partial class LsoScanTestWindow
    {
        private bool _isUpdatingColorIllumination;

        private void initColorIlluminationControl_()
        {
            _isUpdatingColorIllumination = true;
            myCbColorIllumination.ItemsSource = new[] { "White", "RGB" };
            _isUpdatingColorIllumination = false;
        }

        private void myCbColorIllumination_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            commitColorIllumination_();
        }

        // Same persistence path as every other inline preset field: write the Capture preset
        // object, then save it (which also rebinds the controls from what was stored).
        private void commitColorIllumination_()
        {
            if (_isUpdatingColorIllumination || _isUpdatingCameraPresets)
            {
                return;
            }

            ColorIllumination illumination = myCbColorIllumination.SelectedIndex == 1
                ? ColorIllumination.Rgb
                : ColorIllumination.White;
            if (_scanTestModel.ModePresets.Capture.ColorIllumination == illumination)
            {
                return;
            }

            _scanTestModel.ModePresets.Capture.ColorIllumination = illumination;
            saveCameraPreset_(ColorCameraPresetMode.Capture);
        }

        // Called from refreshCameraPresetControls_ after the Mode combo and summary are bound:
        // mirrors the Capture preset's value, enables the combo only for Color, and states the
        // illumination in the mode summary ("Color (RGB)").
        private void refreshColorIlluminationControl_()
        {
            bool isColor = _scanTestModel.ModePresets.CaptureImageMode == LsoImageMode.Color;
            ColorIllumination illumination = _scanTestModel.ModePresets.Capture.ColorIllumination;

            _isUpdatingColorIllumination = true;
            try
            {
                myCbColorIllumination.IsEnabled = isColor;
                myCbColorIllumination.SelectedIndex = illumination == ColorIllumination.Rgb ? 1 : 0;
                myLbModeSummary.Content = isColor
                    ? $"Color ({(illumination == ColorIllumination.Rgb ? "RGB" : "White")}) | {modeSummary_()}"
                    : modeSummary_();
            }
            finally
            {
                _isUpdatingColorIllumination = false;
            }
        }
    }
}
