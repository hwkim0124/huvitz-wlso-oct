using System;
using System.Windows;
using System.Windows.Controls;
using OpenCvSharp;
using WsoNativeLib;
using WsoNativeLib.Presets;

namespace WsoToolkit.controls
{
    // DevKit-parity FAF review: the capture can be viewed as color or as the extracted
    // emission (blue FAF defaults to the green plane, green FAF is the red plane), and the
    // blue FAF composite 0.6 G + 0.4 R is selectable. Extraction uses the high-depth sensor
    // frame when the camera delivered one, then scales once to 8-bit gray.
    public partial class LsoScanImagePreview
    {
        // FAF gray is the default view for a FAF capture; the toggle asks for the color frame.
        private bool _showFafColor;
        private bool _isUpdatingFafControls;

        // The Capture role's image mode and the Blue-FAF option it is set to. The scan-test
        // view model keeps these in step with the selected preset.
        public LsoImageMode? CaptureImageMode { get; set; }

        public FafProcessingOption FafOption { get; set; } = FafProcessingOption.GreenOnly;

        // Raised when the operator changes the Blue-FAF option in the review area, so the
        // host can store it in the Capture preset.
        public Action<FafProcessingOption>? FafOptionChanged;

        public void SetFafContext(LsoImageMode mode, FafProcessingOption option)
        {
            CaptureImageMode = mode;
            FafOption = option;
            refreshFafControls_();
        }

        private bool IsFafCapture_()
        {
            return CaptureImageMode == LsoImageMode.BlueFaf || CaptureImageMode == LsoImageMode.GreenFaf;
        }

        private void refreshFafControls_()
        {
            if (myFafPanel == null)
            {
                return;
            }

            if (myCbFafOption.Items.Count == 0)
            {
                myCbFafOption.Items.Add("G only");
                myCbFafOption.Items.Add("G+R (0.6/0.4)");
            }

            bool fafMode = IsFafCapture_();
            bool hasCapture = (_captureImageList.Count > 0) || HasProcessed;
            bool visible = fafMode && IsReviewMode && hasCapture;

            myFafPanel.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
            myBtFafView.Content = _showFafColor ? "Color View" : "FAF View";
            // Processed is the averaged FAF image itself; there is no color frame to switch to.
            myBtFafView.IsEnabled = visible && _reviewView != ReviewViewMode.Processed;

            bool blueFaf = CaptureImageMode == LsoImageMode.BlueFaf;
            myCbFafOption.Visibility = visible && blueFaf ? Visibility.Visible : Visibility.Collapsed;

            _isUpdatingFafControls = true;
            try
            {
                myCbFafOption.SelectedIndex = FafOption == FafProcessingOption.GreenPlusRed ? 1 : 0;
            }
            finally
            {
                _isUpdatingFafControls = false;
            }
        }

        private void myBtFafView_Click(object sender, RoutedEventArgs e)
        {
            _showFafColor = !_showFafColor;
            refreshFafControls_();
            showReviewIndex_((int)mySliderReviewImageIndex.Value);
        }

        private void myCbFafOption_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_isUpdatingFafControls)
            {
                return;
            }

            FafOption = myCbFafOption.SelectedIndex == 1
                ? FafProcessingOption.GreenPlusRed
                : FafProcessingOption.GreenOnly;
            FafOptionChanged?.Invoke(FafOption);
            showReviewIndex_((int)mySliderReviewImageIndex.Value);
        }

        // True when the review should draw the extracted FAF gray instead of the color image.
        private bool TryRenderFafView_(int index, Mat source, string label)
        {
            if (!IsFafCapture_() || _showFafColor || _reviewView == ReviewViewMode.Processed)
            {
                return false;
            }

            if (!TryExtractFaf8_(index, source, out Mat? faf) || faf == null)
            {
                return false;
            }

            using (faf)
            {
                imageViewport.Source = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(faf);
                lblImageStatusReview.Content = $"{label}   FAF {FafOptionLabel_()}   {faf.Width} x {faf.Height}";
            }
            return true;
        }

        // frameIndex is the capture frame for the Full frames view; other views pass the
        // frame that happens to be on screen and fall back to the 8-bit review image.
        private bool TryExtractFaf8_(int frameIndex, Mat source, out Mat? faf)
        {
            faf = null;
            if (!IsFafCapture_() || source == null || source.Empty())
            {
                return false;
            }

            LsoModeProcessing rule = LsoImageModes.Processing(CaptureImageMode!.Value, FafOption);
            CameraHighDepthData? high = null;
            if (_reviewView == ReviewViewMode.FullFrames
                && frameIndex >= 0 && frameIndex < _highDepthFrames.Count)
            {
                high = _highDepthFrames[frameIndex];
            }

            faf = LsoFafImage.ExtractDisplay8(high, source, rule);
            return true;
        }

        private string FafOptionLabel_()
        {
            if (CaptureImageMode == LsoImageMode.BlueFaf)
            {
                return FafOption == FafProcessingOption.GreenPlusRed ? "G+R 0.6/0.4" : "G only";
            }
            return "R only";
        }
    }
}
