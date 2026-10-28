using System;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using Microsoft.Win32;
using WsoNativeLib.Imaging;
using WsoNativeLib.Presets;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit
{
    // Draft postprocessing review. The processor is device-free and runs on a worker; this window
    // only owns the input, the one run in flight and the stage views.
    public partial class LsoPostProcessingWindow : System.Windows.Window
    {
        private PostProcessInput? _input;
        private PostProcessResult? _result;
        private CancellationTokenSource? _running;
        private int _generation;

        public LsoPostProcessingWindow()
        {
            InitializeComponent();
        }

        public bool ProcessAfterCapture => myCheckAfterCapture.IsChecked == true;

        // The uncorrected frame of the last single-frame Color capture plus the map it was taken
        // with. Kept so the reflected-light switch has something to act on: applying the map to a
        // frame the capture already corrected would subtract it twice.
        private static (byte[] Data, int Width, int Height, int BitDepth, LsoReflectedLightCorrection Correction)? _rawCapture;

        public static void RememberRawCapture(byte[] data, int width, int height, int bitDepth,
            LsoReflectedLightCorrection correction)
            => _rawCapture = (data, width, height, bitDepth, correction);

        public static void ClearRawCapture() => _rawCapture = null;

        // The frame the review should process: the raw capture when a map was applied at capture,
        // otherwise the corrected frame the analysis window kept. Null when there is neither.
        public static PostProcessInput? BuildLatestInput()
        {
            if (_rawCapture is { } raw)
            {
                return PostProcessInput.FromBayer(raw.Data, raw.Width, raw.Height, raw.BitDepth,
                    $"Latest capture {raw.Width}x{raw.Height} ({raw.BitDepth}-bit, before reflected-light removal)",
                    raw.Correction);
            }
            if (LsoImageAnalysisWindow.Latest is not { } latest) return null;
            bool bayer = latest.Bytes == 2
                && (latest.Format == (int)ColorPixelFormat.BayerRG12Packed || latest.Format == (int)ColorPixelFormat.BayerRG16);
            if (!bayer) return null;
            int bitDepth = latest.Format == (int)ColorPixelFormat.BayerRG12Packed ? 12 : 16;
            return PostProcessInput.FromBayer(latest.Data, latest.Width, latest.Height, bitDepth,
                $"Latest capture {latest.Width}x{latest.Height} ({bitDepth}-bit, after capture corrections)",
                reflectedLightNote: "already corrected at capture, or the capture found no map for it; the frame kept here has no map to switch");
        }

        // The settings on screen now, without the dialog: the capture path reads them silently and
        // falls back to the defaults when a field does not parse.
        public PostProcessOptions CurrentOptions => TryReadOptions(out PostProcessOptions options)
            ? options
            : new PostProcessOptions();

        // The frame the capture callback kept (unpacked 12/16-bit Bayer, after the capture path's
        // own corrections). Called by the Scan Test window after a successful single-frame capture.
        public void ProcessLatestCapture()
        {
            if (TryLoadLatest()) StartRun();
        }

        private bool TryLoadLatest()
        {
            PostProcessInput? input = BuildLatestInput();
            if (input == null)
            {
                myLbStatus.Text = LsoImageAnalysisWindow.Latest == null
                    ? "No captured frame yet."
                    : $"Latest capture (format {LsoImageAnalysisWindow.Latest.Value.Format}, {LsoImageAnalysisWindow.Latest.Value.Bytes} bytes/px) is not a 16-bit Bayer frame; not processed.";
                return false;
            }
            SetInput(input);
            return true;
        }

        private void SetInput(PostProcessInput input)
        {
            _input = input;
            myBtRun.IsEnabled = true;
            bool removable = input.ReflectedLight != null;
            myCheckReflectedLight.IsEnabled = removable;
            // A switch that cannot act must not read as "removal is on for this run".
            if (!removable) myCheckReflectedLight.IsChecked = false;
            string reflected = removable
                ? " Reflected-light map kept; the switch applies it."
                : $" Reflected light cannot be toggled for this input: {input.ReflectedLightNote}.";
            myCheckReflectedLight.ToolTip = removable
                ? "Subtracts the reflected-light map from the uncorrected capture frame before the circle cut."
                : "No map for this input: " + input.ReflectedLightNote + ".";
            myLbStatus.Text = $"{input.Source}: {input.Width}x{input.Height}, {(input.IsBayer ? "Bayer RGGB" : "RGB48")}, 0..{input.MaxValue}.{reflected} Run to process.";
        }

        private async void Open_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog { Filter = "TIFF|*.tif;*.tiff|All files|*.*" };
            if (dialog.ShowDialog(this) != true) return;
            string path = dialog.FileName;
            bool gray16IsBayer = myCheckGray16Bayer.IsChecked == true;
            myBtOpen.IsEnabled = false;
            myLbStatus.Text = $"Loading {Path.GetFileName(path)} ...";
            try
            {
                // A 13 M pixel TIFF decodes off the UI thread; the window stays responsive.
                PostProcessInput input = await Task.Run(() => LsoFundusPostProcessor.LoadTiff(path, gray16IsBayer));
                SetInput(input);
                StartRun();
            }
            catch (Exception ex)
            {
                myLbStatus.Text = $"Open failed: {ex.Message}";
            }
            finally
            {
                myBtOpen.IsEnabled = true;
            }
        }

        private void Latest_Click(object sender, RoutedEventArgs e)
        {
            if (TryLoadLatest()) StartRun();
        }

        private void Run_Click(object sender, RoutedEventArgs e) => StartRun();

        private void Cancel_Click(object sender, RoutedEventArgs e) => _running?.Cancel();

        private PostProcessOptions? ReadOptions()
        {
            return TryReadOptions(out PostProcessOptions options) ? options : null;
        }

        private bool TryReadOptions(out PostProcessOptions options)
        {
            options = new PostProcessOptions();
            if (!TryGain(myTbWbR, out double r) || !TryGain(myTbWbG, out double g) || !TryGain(myTbWbB, out double b)
                || !TryGain(myTbGain, out double gain) || !TryGain(myTbRoiRatio, out double roiRatio)
                || !TryGain(myTbAutoThreshold, out double threshold)
                || !TryGain(myTbFlattenSigma, out double flattenSigma) || !TryGain(myTbFlattenStrength, out double flattenStrength)
                || !TryGain(myTbMatchTarget, out double matchTarget)
                || !TryGain(myTbDiscHeadroom, out double discHeadroom)
                || !TryGain(myTbGamma, out double gamma)
                || !TryGain(myTbDiscBlend, out double discBlend))
            {
                return false;
            }
            options = new PostProcessOptions
            {
                WhiteBalance = myCheckWb.IsChecked == true,
                Red = r,
                Green = g,
                Blue = b,
                Denoise = myCheckDenoise.IsChecked == true,
                CircleMask = myCheckCircle.IsChecked == true,
                ReflectedLight = myCheckReflectedLight.IsChecked == true,
                Flatten = myCheckFlatten.IsChecked == true,
                FlattenSigmaRatio = flattenSigma,
                FlattenStrength = flattenStrength,
                Brightness = gain,
                AutoLimit = myCheckAuto.IsChecked == true,
                RoiRatio = roiRatio,
                Threshold = threshold,
                SoftKnee = myCheckKnee.IsChecked == true,
                MatchLuminance = myCheckMatch.IsChecked == true,
                LuminanceTarget = matchTarget,
                MatchDisc = myCheckDisc.IsChecked == true,
                DiscHeadroom = discHeadroom,
                Gamma = myCheckGamma.IsChecked == true ? gamma : 1.0,
                GammaOnValue = myCheckGammaOnValue.IsChecked == true,
                DiscLocal = myCheckDiscLocal.IsChecked == true,
                DiscBlendRatio = discBlend,
            };
            return true;
        }

        private static bool TryGain(TextBox box, out double gain)
        {
            return double.TryParse(box.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out gain) && gain > 0.0;
        }

        // One run at a time on the screen: a newer run cancels the older one, and the older one's
        // result is discarded by the generation check instead of being shown late.
        private async void StartRun()
        {
            if (_input == null) return;
            PostProcessOptions? options = ReadOptions();
            if (options == null)
            {
                MessageBox.Show(this, "White balance gains, brightness, the ROI ratio, the threshold, the flatten sigma / strength, the disc headroom and the gamma must be positive numbers.", "Post Processing");
                return;
            }
            _running?.Cancel();
            var cts = new CancellationTokenSource();
            _running = cts;
            int generation = ++_generation;
            PostProcessInput input = _input;
            myBtRun.IsEnabled = false;
            myBtCancel.IsEnabled = true;
            myBtSave.IsEnabled = false;
            myLbStatus.Text = $"Processing {input.Source} ...";

            PostProcessResult? result = null;
            string error = string.Empty;
            try
            {
                result = await Task.Run(() => LsoFundusPostProcessor.Run(input, options, cts.Token), cts.Token);
            }
            catch (OperationCanceledException)
            {
                error = "Cancelled.";
            }
            catch (Exception ex)
            {
                error = $"Processing failed: {ex.Message}";
            }
            finally
            {
                cts.Dispose();
            }

            if (generation != _generation)
            {
                result?.Dispose();
                return;
            }
            _running = null;
            myBtRun.IsEnabled = true;
            myBtCancel.IsEnabled = false;
            if (result == null)
            {
                myLbStatus.Text = error;
                return;
            }
            _result?.Dispose();
            _result = result;
            ShowResult(result);
        }

        private void ShowResult(PostProcessResult result)
        {
            RenderResult(result);
            myBtSave.IsEnabled = true;
        }

        // The scan window's own run after a capture: it hands the result over so both screens show
        // one processed image instead of processing the frame twice. The window owns it afterwards.
        // The input comes along so the switches show what this result was made from.
        public void ShowExternalResult(PostProcessResult result, PostProcessInput input)
        {
            SetInput(input);
            _result?.Dispose();
            _result = result;
            RenderResult(result);
            myBtSave.IsEnabled = true;
        }

        // A capture happened but the window did not ask for its result: the input (and the
        // reflected-light switch) still follow the newest frame so Run works on it.
        public void AdoptInput(PostProcessInput input) => SetInput(input);

        private void RenderResult(PostProcessResult result)
        {
            myListStages.Items.Clear();
            int index = 0;
            foreach (PostProcessStage stage in result.Stages)
            {
                ++index;
                string note = stage.Note.Length > 0 ? $"  [{stage.Note}]" : string.Empty;
                myListStages.Items.Add($"{index}. {stage.Name}{(stage.Applied ? "" : " - skipped")}  ({stage.ElapsedMs:0} ms, max {stage.MaxValue:0}){note}");
            }
            myImgOriginal.Source = Frozen(result.Stages[0].Display);
            myLbOriginal.Text = $"Original - {result.Stages[0].Name}";
            myListStages.SelectedIndex = result.Stages.Count - 1;
            myBtSave.IsEnabled = true;
            string reflected = !myCheckReflectedLight.IsEnabled ? "reflected light fixed in this input"
                : myCheckReflectedLight.IsChecked == true ? "reflected light removed" : "reflected light kept";
            myLbStatus.Text = $"{result.Source}: {result.Stages.Count} stages, {reflected}. Select a stage to compare it with the original; Save writes the selected stage.";
        }

        private void Stages_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_result == null || myListStages.SelectedIndex < 0 || myListStages.SelectedIndex >= _result.Stages.Count) return;
            PostProcessStage stage = _result.Stages[myListStages.SelectedIndex];
            myImgStage.Source = Frozen(stage.Display);
            myLbStage.Text = $"Stage {myListStages.SelectedIndex + 1} - {stage.Name}{(stage.Applied ? "" : " (skipped)")}";
        }

        private void Save_Click(object sender, RoutedEventArgs e)
        {
            if (_result == null) return;
            int index = Math.Max(0, myListStages.SelectedIndex);
            PostProcessStage stage = _result.Stages[index];
            var dialog = new SaveFileDialog
            {
                Filter = "PNG|*.png|JPEG|*.jpg",
                FileName = $"postprocess_{DateTime.Now:yyyyMMdd_HHmmss}_stage{index + 1}.png",
            };
            if (dialog.ShowDialog(this) != true) return;
            try
            {
                LsoFundusPostProcessor.Save(stage.Display, dialog.FileName);
                myLbStatus.Text = $"Saved stage {index + 1} to {dialog.FileName}";
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Save");
            }
        }

        private static BitmapSource Frozen(OpenCvSharp.Mat image)
        {
            BitmapSource bitmap = OpenCvSharp.WpfExtensions.BitmapSourceConverter.ToBitmapSource(image);
            bitmap.Freeze();
            return bitmap;
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            ++_generation;
            _running?.Cancel();
            _running = null;
            _result?.Dispose();
            _result = null;
        }
    }
}
