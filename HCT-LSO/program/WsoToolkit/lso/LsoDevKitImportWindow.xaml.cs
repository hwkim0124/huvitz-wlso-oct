using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using WsoNativeLib.Presets;
using WsoToolkit.utils;

namespace WsoToolkit
{
    /// <summary>
    /// Shows what <see cref="LsoDevKitImport"/> would make of a DevKit folder - every field, warning
    /// and rejection for the new Preview and Color Capture presets - and imports them on Save.
    /// The preview imports into a scratch folder that is deleted before each preview returns, so
    /// nothing reaches the preset folder before Save, and Save never replaces an existing preset.
    /// </summary>
    public partial class LsoDevKitImportWindow : Window
    {
        // LsoDevKitImport copies the board calibration as <preset>_GalvoSlitCalibration.json and
        // refuses a name when that file or <preset>.xml already exists.
        private const string CalibrationSuffix = "_GalvoSlitCalibration.json";

        // The range of the light slider in LSO Mode Settings, the other editor of this value.
        private const int MaxLedIntensity = 255;

        private readonly string _sourceDirectory;
        private readonly string _presetDirectory;
        private readonly string _configIniPath;
        private readonly LsoModePreset _previewBase;
        private readonly LsoModePreset _captureBase;
        private readonly bool _initialized;

        // Set by a successful Save. PresetsActivated is false when the box was unchecked or the
        // INI could not be written (that failure is reported before the dialog closes).
        public string PreviewName { get; private set; } = string.Empty;
        public string CaptureName { get; private set; } = string.Empty;
        public bool PresetsActivated { get; private set; }

        // `previewBase` / `captureBase` are the presets the import starts from: their light,
        // motors and every field the DevKit files do not carry are kept.
        public LsoDevKitImportWindow(string sourceDirectory, string presetDirectory, string configIniPath,
            LsoModePreset previewBase, LsoModePreset captureBase)
        {
            _sourceDirectory = sourceDirectory;
            _presetDirectory = presetDirectory;
            _configIniPath = configIniPath;
            _previewBase = previewBase;
            _captureBase = captureBase;

            InitializeComponent();

            myTbSource.Text = sourceDirectory;
            myTbTarget.Text = presetDirectory;
            myLbPreviewBase.Text = $"from '{previewBase.Name}' - its light, motors and the fields the DevKit files lack are kept";
            myLbCaptureBase.Text = $"from '{captureBase.Name}' - its light, motors and the fields the DevKit files lack are kept";
            myLbLedHint.Text = $"0-{MaxLedIntensity}. Empty keeps each base value (Preview {previewBase.Light.Intensity}, "
                + $"Capture {captureBase.Light.Intensity}); 0 leaves the board's value.";
            myLbActivateHint.Text = $"sets [MODE_PRESET] PreviewPreset / CapturePreset (Color) in {configIniPath}";

            string stamp = sourceStamp_(sourceDirectory);
            myTbPreviewName.Text = "Preview_Bfs_" + stamp;
            myTbCaptureName.Text = "Capture_Bfs_" + stamp;

            _initialized = true;
            refresh_();
        }

        private void myTbInput_TextChanged(object sender, TextChangedEventArgs e)
        {
            refresh_();
        }

        private void myBtSave_Click(object sender, RoutedEventArgs e)
        {
            // Once more right before writing: the preset folder or the DevKit files may have
            // changed since the preview on screen was computed.
            if (!refresh_())
            {
                return;
            }

            string previewName = targetName_(myTbPreviewName);
            string captureName = targetName_(myTbCaptureName);
            tryReadLedIntensity_(out ushort? ledIntensity);

            // Both names were free a moment ago (refresh_). Only an import that succeeded, or one
            // that threw past the importer's own clean-up, can have left files under its name.
            var written = new List<string>();
            string current = previewName;
            string failure = string.Empty;
            try
            {
                LsoDevKitImportResult preview = LsoDevKitImport.Import(_sourceDirectory, _presetDirectory,
                    ColorCameraPresetMode.Preview, previewName, _previewBase, ledIntensity);
                if (!preview.Succeeded)
                {
                    failure = "Preview: " + string.Join(" ", preview.Rejections);
                }
                else
                {
                    written.Add(previewName);
                    current = captureName;
                    LsoDevKitImportResult capture = LsoDevKitImport.Import(_sourceDirectory, _presetDirectory,
                        ColorCameraPresetMode.Capture, captureName, _captureBase, ledIntensity);
                    if (!capture.Succeeded)
                    {
                        failure = "Capture: " + string.Join(" ", capture.Rejections);
                    }
                }
            }
            catch (Exception ex)
            {
                // The importer reports every failure it expects as a rejection. Anything else is
                // reported the same way instead of closing the Toolkit while it owns the device.
                written.Add(current);
                failure = $"{ex.GetType().Name}: {ex.Message}";
            }

            if (failure.Length > 0)
            {
                // Save is both presets or neither: a lone Preview would block the retry by name.
                MsgBoxUtil.ShowError("Nothing was imported." + Environment.NewLine + Environment.NewLine
                    + failure + removeWritten_(written));
                refresh_();
                return;
            }

            PreviewName = previewName;
            CaptureName = captureName;
            if (myCheckActivate.IsChecked == true)
            {
                try
                {
                    LsoDevKitImport.Activate(_presetDirectory, _configIniPath, previewName, captureName);
                    PresetsActivated = true;
                }
                catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException || ex is ArgumentException)
                {
                    MsgBoxUtil.ShowError($"'{previewName}' and '{captureName}' were imported, but they could not be "
                        + $"activated in '{_configIniPath}':" + Environment.NewLine + ex.Message);
                }
            }

            DialogResult = true;
        }

        // Runs both imports into scratch folders and shows what Save would write. Returns whether
        // Save may run: no rejection in either mode, two free and different names, a usable LED.
        private bool refresh_()
        {
            if (!_initialized)
            {
                return false; // the constructor is still filling the boxes
            }

            string previewName = targetName_(myTbPreviewName);
            string captureName = targetName_(myTbCaptureName);
            var issues = new List<ImportIssue>();
            if (!tryReadLedIntensity_(out ushort? ledIntensity))
            {
                issues.Add(ImportIssue.Rejection($"LED intensity '{myTbLedIntensity.Text.Trim()}' is not a whole number "
                    + $"0-{MaxLedIntensity}. Leave it empty to keep each base value."));
            }

            LsoDevKitImportResult? preview = previewImport_(ColorCameraPresetMode.Preview, previewName,
                _previewBase, ledIntensity, issues);
            LsoDevKitImportResult? capture = previewImport_(ColorCameraPresetMode.Capture, captureName,
                _captureBase, ledIntensity, issues);

            if (string.Equals(previewName, captureName, StringComparison.OrdinalIgnoreCase))
            {
                issues.Add(ImportIssue.Rejection("Preview and Capture need two different names."));
            }

            foreach (string name in new[] { previewName, captureName }.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                if (targetExists_(name))
                {
                    issues.Add(ImportIssue.Rejection($"Preset '{name}' already exists in the preset folder; existing "
                        + "presets are never overwritten. Type another name."));
                }
            }

            showFields_(myTabPreview, myGridPreview, "Preview", preview);
            showFields_(myTabCapture, myGridCapture, "Capture", capture);

            // OrderBy is stable, so each mode keeps the importer's order inside both groups.
            List<ImportIssue> ordered = issues.OrderBy(issue => issue.IsRejection ? 0 : 1).ToList();
            int rejections = ordered.Count(issue => issue.IsRejection);
            myListIssues.ItemsSource = ordered;
            myLbIssueCount.Text = $"Rejections: {rejections}    Warnings: {ordered.Count - rejections}";

            bool ready = rejections == 0 && preview?.Succeeded == true && capture?.Succeeded == true;
            myBtSave.IsEnabled = ready;
            myLbStatus.Text = ready
                ? $"Nothing is written until Save. Save creates '{previewName}' and '{captureName}' with their calibration copies in the preset folder."
                : "Save is disabled until every red item above is resolved.";
            myLbStatus.Foreground = (Brush)FindResource(ready ? "MutedForegroundBrush" : "RejectionForegroundBrush");
            return ready;
        }

        // One import into its own scratch folder, removed again before this returns - including
        // when the import throws - so the preview never leaves files behind.
        private LsoDevKitImportResult? previewImport_(ColorCameraPresetMode mode, string presetName,
            LsoModePreset basePreset, ushort? ledIntensity, List<ImportIssue> issues)
        {
            string scratch = Path.Combine(Path.GetTempPath(), "HctLsoDevKitPreview_" + Guid.NewGuid().ToString("N"));
            LsoDevKitImportResult? report = null;
            try
            {
                report = LsoDevKitImport.Import(_sourceDirectory, scratch, mode, presetName, basePreset, ledIntensity);
            }
            catch (Exception e)
            {
                // Same reason as in Save: shown as a rejection, never a crash of the Toolkit.
                issues.Add(ImportIssue.Rejection($"{mode}: {e.GetType().Name}: {e.Message}"));
            }
            finally
            {
                deleteScratch_(scratch, issues);
            }

            if (report != null)
            {
                issues.AddRange(report.Rejections.Select(text => ImportIssue.Rejection($"{mode}: {text}")));
                issues.AddRange(report.Warnings.Select(text => ImportIssue.Warning($"{mode}: {text}")));
            }
            return report;
        }

        private static void deleteScratch_(string scratch, List<ImportIssue> issues)
        {
            try
            {
                if (Directory.Exists(scratch))
                {
                    Directory.Delete(scratch, recursive: true);
                }
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                issues.Add(ImportIssue.Warning($"The preview scratch folder '{scratch}' could not be removed: {e.Message}"));
            }
        }

        private static void showFields_(TabItem tab, DataGrid grid, string mode, LsoDevKitImportResult? report)
        {
            grid.ItemsSource = report?.Fields;
            tab.Header = report == null ? mode : $"{mode} ({report.Fields.Count} fields)";
        }

        // Removes what a failed Save wrote under the given names, so the retry finds them free.
        // Returns the files that could not be removed, ready to append to the error message.
        private string removeWritten_(IEnumerable<string> names)
        {
            var left = new List<string>();
            foreach (string name in names.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                foreach (string path in new[] { Path.Combine(_presetDirectory, name + LsoModePreset.FileExtension),
                    Path.Combine(_presetDirectory, name + CalibrationSuffix) })
                {
                    try
                    {
                        File.Delete(path);
                    }
                    catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
                    {
                        left.Add($"{path}: {e.Message}");
                    }
                }
            }

            return left.Count == 0
                ? string.Empty
                : Environment.NewLine + Environment.NewLine + "These files could not be removed:"
                    + Environment.NewLine + string.Join(Environment.NewLine, left);
        }

        private bool targetExists_(string presetName)
        {
            return LsoModePresetStore.IsValidName(presetName)
                && (File.Exists(Path.Combine(_presetDirectory, presetName + LsoModePreset.FileExtension))
                    || File.Exists(Path.Combine(_presetDirectory, presetName + CalibrationSuffix)));
        }

        private bool tryReadLedIntensity_(out ushort? intensity)
        {
            intensity = null;
            string text = myTbLedIntensity.Text.Trim();
            if (text.Length == 0)
            {
                return true;
            }

            if (ushort.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out ushort value)
                && value <= MaxLedIntensity)
            {
                intensity = value;
                return true;
            }
            return false;
        }

        private static string targetName_(TextBox box)
        {
            return box.Text.Trim();
        }

        // MMdd of the DevKit's Capture pattern file, so the default names say which DevKit save
        // they came from. The pattern folder is resolved the way LsoDevKitImport resolves it.
        private static string sourceStamp_(string sourceDirectory)
        {
            string patternFolder = Path.Combine(sourceDirectory, "LsoPatternCfg");
            string pattern = Path.Combine(Directory.Exists(patternFolder) ? patternFolder : sourceDirectory,
                "LsoPattern_Capture_Bfs.json");
            DateTime written = File.Exists(pattern) ? File.GetLastWriteTime(pattern) : DateTime.Now;
            return written.ToString("MMdd", CultureInfo.InvariantCulture);
        }

        // One line of the issue list; IsRejection selects the red card in the XAML template.
        private sealed record ImportIssue(string Text, bool IsRejection)
        {
            public static ImportIssue Warning(string text) => new ImportIssue(text, false);

            public static ImportIssue Rejection(string text) => new ImportIssue(text, true);
        }
    }
}
