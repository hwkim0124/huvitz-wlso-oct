using System;
using System.Windows;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;

namespace WsoToolkit
{
    // "Import DevKit..." in the Preset group: a DevKit folder becomes two new presets - Preview and
    // the Color Capture - through LsoDevKitImport, previewed in LsoDevKitImportWindow before
    // anything is written.
    public partial class LsoScanTestWindow
    {
        // The folder picked last, so a retry or a second import starts where the first one did.
        private static string _devKitImportFolder = string.Empty;

        private void myBtImportDevKit_Click(object sender, RoutedEventArgs e)
        {
            importDevKitPresets_();
        }

        private void importDevKitPresets_()
        {
            // Activation swaps the presets a running capture reads - refused like Settings is.
            if (LsoCamera.IsCapturing())
            {
                return;
            }

            using var picker = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select the DevKit folder to import (the one holding LsoPatternCfg and CameraConfigs)",
                UseDescriptionForTitle = true,
                ShowNewFolderButton = false,
                SelectedPath = _devKitImportFolder,
            };
            if (picker.ShowDialog() != System.Windows.Forms.DialogResult.OK)
            {
                return;
            }
            _devKitImportFolder = picker.SelectedPath;

            LsoModePresetStore store = _scanTestModel.ModePresets;
            var dialog = new LsoDevKitImportWindow(picker.SelectedPath, store.Directory,
                LsoScanTestModel.DefaultConfigIniPath, store.Preview, devKitCaptureBase_()) { Owner = this };
            if (dialog.ShowDialog() != true)
            {
                return;
            }

            // Activate rewrote [MODE_PRESET]; Select loads the same names into the presets this
            // window runs. Activate names the Color capture, so another bound Mode keeps its own.
            if (dialog.PresetsActivated)
            {
                store.Select(ColorCameraPresetMode.Preview, dialog.PreviewName);
                if (store.CaptureImageMode == LsoImageMode.Color)
                {
                    store.Select(ColorCameraPresetMode.Capture, dialog.CaptureName);
                }
            }

            refreshCameraPresetControls_();
            reportCameraPresetIssues_();
            reflectScanTiming_();
            updateAlignControls_();

            MsgBoxUtil.ShowInfo(dialog.PresetsActivated
                ? $"Imported and activated Preview '{dialog.PreviewName}' and Capture '{dialog.CaptureName}'."
                    + (store.CaptureImageMode == LsoImageMode.Color
                        ? string.Empty
                        : " The Capture preset is the Color mode's - switch Mode to Color to run it.")
                : $"Imported '{dialog.PreviewName}' and '{dialog.CaptureName}' into {store.Directory}. They are not "
                    + "active - pick them in the Preview / Capture combos to use them.");
        }

        // The DevKit BFS capture is the Color capture. While Mode shows another image mode, the
        // bound Capture preset carries that mode's lamp and filter, so the Color selection as it
        // is saved becomes the base instead.
        private LsoModePreset devKitCaptureBase_()
        {
            LsoModePresetStore store = _scanTestModel.ModePresets;
            if (store.CaptureImageMode == LsoImageMode.Color)
            {
                return store.Capture;
            }

            var saved = new LsoModePresetStore(store.Directory, LsoScanTestModel.DefaultConfigIniPath);
            saved.LoadReadOnly(LsoImageMode.Color);
            if (saved.HasIssues)
            {
                MsgBoxUtil.ShowWarning("Color Capture preset used as the import base:" + Environment.NewLine
                    + Environment.NewLine + string.Join(Environment.NewLine, saved.Issues));
            }
            return saved.Capture;
        }
    }
}
