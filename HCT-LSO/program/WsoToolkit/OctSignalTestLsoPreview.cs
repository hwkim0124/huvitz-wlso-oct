using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.controls;
using WsoToolkit.utils;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoToolkit
{
    // LSO aiming preview with the Preview preset, the same one-call LsoScanTestWindow's Start Scan runs.
    public partial class OctSignalTestWindow
    {
        private readonly LsoScanTestModel _lsoModel = new LsoScanTestModel();
        private WsoCallback.ColorCameraFrameCaptured? _onLsoFrameCaptured;
        private volatile bool _isLsoFramePending = false;

        private void initLsoPreview_()
        {
            var issues = new List<string>();
            string? seeded = LsoPreviewPresetSeed.Load(_lsoModel.ModePresets, LsoScanTestModel.DefaultConfigIniPath);
            if (seeded != null)
            {
                issues.Add(seeded);
            }
            issues.AddRange(_lsoModel.ModePresets.DrainIssues());
            _lsoModel.LoadConfigFromIniFile();

            lsoPreview.Image.SetDefaultAdjustPreset(_lsoModel.ImageAdjust.ToParam());
            _onLsoFrameCaptured = new WsoCallback.ColorCameraFrameCaptured(this.OnLsoFrameCaptured);
            lsoPreview.LiveCallback = _onLsoFrameCaptured;

            if (issues.Count > 0)
            {
                MsgBoxUtil.ShowWarning("LSO Preview preset:" + Environment.NewLine + Environment.NewLine
                    + string.Join(Environment.NewLine, issues));
            }
        }

        private void releaseLsoPreview_()
        {
            lsoPreview.StopPreview();
            lsoPreview.CloseAdjustmentWindow();
        }

        private void BtnLsoPreview_Click(object sender, RoutedEventArgs e)
        {
            if (lsoPreview.IsPreviewing())
            {
                lsoPreview.StopPreview();
            }
            else
            {
                startLsoPreview_();
            }
            btnLsoPreview.Content = lsoPreview.IsPreviewing() ? "Stop Preview" : "Start Preview";
        }

        private void startLsoPreview_()
        {
            LsoModePreset preset = _lsoModel.ModePresets.Preview;
            if (!LsoModeTrajectory.TryLoad(preset.Scan, out short[]? table, out string issue, out _)
                || !LsoModeTrajectory.Install(LsoScanPreviewPreset.PatternId, table, out issue))
            {
                MsgBoxUtil.ShowWarning($"LSO preview was not started: {issue}");
                return;
            }

            Mouse.OverrideCursor = Cursors.Wait;
            try
            {
                if (!LsoModeMotorApply.Apply(preset.Motor, out string motorFailure))
                {
                    MsgBoxUtil.ShowWarning($"LSO preview was not started: {motorFailure}");
                    return;
                }
            }
            finally
            {
                Mouse.OverrideCursor = null;
            }

            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam cameraReadBack);
            lsoPreview.PlayPreview(ColorPreviewMonoMode.SensorMono8,
                LsoFundusRequestFactory.BuildPreviewRequest(preset, ColorMonoPreviewMode.SensorMono8,
                    cameraReadBack, DeviceLights.GetLightIntensity(preset.Light.Led)));
        }

        private void OnLsoFrameCaptured(IntPtr data, int width, int height, int frameCount, int nFlipMode, int nPixelFormat, int nBytesPerPixel)
        {
            if (_isLsoFramePending)
            {
                return;
            }

            byte[] frameData = new byte[width * height * nBytesPerPixel];
            Marshal.Copy(data, frameData, 0, frameData.Length);
            _isLsoFramePending = true;

            Dispatcher.BeginInvoke(() =>
            {
                try
                {
                    lsoPreview.UpdateFramerate();
                    lsoPreview.CallbackLsoScanFrameImage(frameData, width, height, 1, nFlipMode, nPixelFormat, nBytesPerPixel);
                }
                finally
                {
                    _isLsoFramePending = false;
                }
            }, DispatcherPriority.Background);
        }
    }
}
