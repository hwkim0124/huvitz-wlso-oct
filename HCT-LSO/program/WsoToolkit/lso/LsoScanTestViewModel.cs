using System;
using System.Linq;
using System.Globalization;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using System.IO;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoNativeLib.Utils;
using WsoToolkit.utils;
using static WsoNativeLib.LsoCamera;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoDomain;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
using WsoToolkit.controls;
using static WsoToolkit.controls.LsoScanImagePreview;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    public sealed class FrameRoiPosition
    {
        public int StartY { get; set; }

        public int EndY { get; set; }
    }

    public sealed class LsoScanTestModel
    {
        public const string DefaultConfigIniFileName = LsoConfigLocation.DeviceConfigFileName;

        // Resolved through LsoConfigLocation so the Agent reads the very file this screen
        // tunes. Unchanged for an existing install: the resolver answers with the app's own
        // folder until a shared location exists.
        public static string DefaultConfigIniPath => LsoConfigLocation.DeviceConfigPath();

        // One profile per native scanner slot (LSO_SCANNER_PATTERN_NUM_MAX), not just
        // COLOR/IR. Slots without an INI section load as zeros until they are read back
        // from the board or filled in.
        private readonly LsoScanProfileModel[] _scanProfiles =
            Enumerable.Range(0, LSO_SCANNER_PATTERN_NUM_MAX)
                      .Select(_ => new LsoScanProfileModel())
                      .ToArray();

        public LsoColorCameraModel ColorCamera { get; } = new LsoColorCameraModel();
        public LsoImageAdjustModel ImageAdjust { get; } = new LsoImageAdjustModel();

        // Per-mode presets (camera + scan + light), one XML file each, next to the app. The
        // INI keeps only what is not part of a mode preset - image adjust, the preview mono
        // mode, the capture trigger mode, the capture ROI table, the reserved scanner slots -
        // plus which preset each mode currently has selected.
        public LsoModePresetStore ModePresets { get; } =
            new LsoModePresetStore(LsoModePresetStore.DefaultDirectory, DefaultConfigIniPath);

        public int ScanProfileCount => _scanProfiles.Length;

        public LsoScanProfileModel GetScanProfile(int patternId)
        {
            if (patternId < 0 || patternId >= _scanProfiles.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(patternId), patternId,
                    $"LSO scanner pattern id must be 0..{_scanProfiles.Length - 1}.");
            }

            // Slots 0 and 1 belong to the active Preview / Capture preset - the preset file is
            // the only owner, so there is nothing to keep in sync.
            if (patternId == LsoScanPreviewPreset.PatternId)
            {
                return ModePresets.Preview.Scan;
            }

            if (patternId == LsoScanCapturePreset.PatternId)
            {
                return ModePresets.Capture.Scan;
            }

            return _scanProfiles[patternId];
        }

        // Reserved slots only: 0 and 1 are preset-owned (see GetScanProfile).
        private static bool IsPresetOwnedSlot(int patternId)
        {
            return patternId == LsoScanPreviewPreset.PatternId || patternId == LsoScanCapturePreset.PatternId;
        }

        public bool LoadConfigFromIniFile(string? configIniPath = null)
        {
            try
            {
                var ini = new IniFileUtil(configIniPath ?? DefaultConfigIniPath);
                for (int i = 0; i < _scanProfiles.Length; i++)
                {
                    if (IsPresetOwnedSlot(i))
                    {
                        continue;
                    }

                    _scanProfiles[i].LoadFromIni(ini, i);
                }
                ColorCamera.LoadFromIni(ini);
                ImageAdjust.LoadFromIni(ini);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        public bool SaveConfigToIniFile(string? configIniPath = null)
        {
            try
            {
                var ini = new IniFileUtil(configIniPath ?? DefaultConfigIniPath);
                for (int i = 0; i < _scanProfiles.Length; i++)
                {
                    if (IsPresetOwnedSlot(i))
                    {
                        continue;
                    }

                    _scanProfiles[i].SaveToIni(ini, i);
                }
                ColorCamera.SaveToIni(ini);
                ImageAdjust.SaveToIni(ini);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }
    }


    public sealed class LsoImageAdjustModel
    {
        public const string IniSectionName = "ImageAdjust";

        public int Brightness { get; set; }
        public bool IsRadialCorrection { get; set; }
        public float RadialCorIntensity { get; set; } = 1.0f;
        public bool IsRemoveReflectedLight { get; set; }
        public double[]? RadialCorrectionCurve { get; set; }

        internal void LoadFromIni(IniFileUtil ini)
        {
            Brightness = ini.ReadInt(IniSectionName, "Brightness", 0);
            IsRadialCorrection = ini.ReadInt(IniSectionName, "IsRadialCorrection", 0) != 0;
            RadialCorIntensity = ini.ReadFloat(IniSectionName, "RadialCorIntensity", 1.0f);
            IsRemoveReflectedLight = ini.ReadInt(IniSectionName, "IsRemoveReflectedLight", 0) != 0;

            string curveText = ini.ReadString(IniSectionName, "RadialCorrectionCurve", string.Empty);
            RadialCorrectionCurve = ParseCurve(curveText);
        }

        internal void SaveToIni(IniFileUtil ini)
        {
            ini.WriteInt(IniSectionName, "Brightness", Brightness);
            ini.WriteInt(IniSectionName, "IsRadialCorrection", IsRadialCorrection ? 1 : 0);
            ini.WriteFloat(IniSectionName, "RadialCorIntensity", RadialCorIntensity);
            ini.WriteInt(IniSectionName, "IsRemoveReflectedLight", IsRemoveReflectedLight ? 1 : 0);
            ini.WriteString(IniSectionName, "RadialCorrectionCurve", FormatCurve(RadialCorrectionCurve));
        }

        public void ApplyFrom(ImageAdjustParam param)
        {
            Brightness = param.Brightness;
            IsRadialCorrection = param.IsRadialCorrection;
            RadialCorIntensity = param.RadialCorIntensity;
            IsRemoveReflectedLight = param.IsRemoveReflectedLight;
            RadialCorrectionCurve = param.RadialCorrectionCurve?.ToArray();
        }

        public ImageAdjustParam ToParam(int nPixelDepth = 0)
        {
            var param = new ImageAdjustParam(Brightness, nPixelDepth)
            {
                IsRadialCorrection = IsRadialCorrection,
                RadialCorIntensity = RadialCorIntensity,
                IsRemoveReflectedLight = IsRemoveReflectedLight,
                RadialCorrectionCurve = RadialCorrectionCurve?.ToArray(),
            };
            return param;
        }

        public bool SavePresetToIniFile(string? configIniPath = null)
        {
            try
            {
                var ini = new IniFileUtil(configIniPath ?? LsoScanTestModel.DefaultConfigIniPath);
                SaveToIni(ini);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        private static double[]? ParseCurve(string curveText)
        {
            if (string.IsNullOrWhiteSpace(curveText))
            {
                return null;
            }

            string[] parts = curveText.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
            var values = new List<double>(parts.Length);
            foreach (string part in parts)
            {
                if (double.TryParse(part.Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out double value))
                {
                    values.Add(value);
                }
            }

            return values.Count > 0 ? values.ToArray() : null;
        }

        private static string FormatCurve(double[]? curve)
        {
            if (curve == null || curve.Length == 0)
            {
                return string.Empty;
            }

            return string.Join(",", curve.Select(v => v.ToString("G17", CultureInfo.InvariantCulture)));
        }
    }
    public sealed class LsoColorCameraModel
    {
        public const string IniSectionName = "COLOR_CAMERA";

        public uint RoiXWidth { get; set; }

        public uint RoiYHeight { get; set; }

        public uint RoiXOffset { get; set; }

        public uint RoiYOffset { get; set; }

        public uint ExposureTime { get; set; }

        public uint AcquisitionMode { get; set; }

        public uint AcquisitionFrameCount { get; set; }

        public uint PixelFormat { get; set; }

        public uint BinningHorizontal { get; set; }

        public uint BinningVertical { get; set; }

        public float Gain { get; set; }

        public uint AdcDepthIndex { get; set; }

        public int CaptureFrameRoiCount { get; set; }

        // The camera side of the aiming preview (its own exposure, gain and FrameStart
        // trigger delay) is no longer kept here: it is the Preview preset in
        // LsoModePresetStore, so both modes are edited and stored the same way.
        // The keys this section still carries from an older build (PreviewExposureTime,
        // PreviewGain, PreviewTriggerDelay) are read once by that store's migration.

        public List<FrameRoiPosition> CaptureFrameRois { get; } = new List<FrameRoiPosition>();

        internal void LoadFromIni(IniFileUtil ini)
        {
            RoiXWidth = ini.ReadUInt(IniSectionName, "RoiXWidth");
            RoiYHeight = ini.ReadUInt(IniSectionName, "RoiYHeight");
            RoiXOffset = ini.ReadUInt(IniSectionName, "RoiXOffset");
            RoiYOffset = ini.ReadUInt(IniSectionName, "RoiYOffset");
            ExposureTime = ini.ReadUInt(IniSectionName, "ExposureTime");
            AcquisitionMode = ini.ReadUInt(IniSectionName, "AcqusitionMode");
            AcquisitionFrameCount = ini.ReadUInt(IniSectionName, "AcqusitionFrameCount");
            PixelFormat = ini.ReadUInt(IniSectionName, "PixelFormat");
            BinningHorizontal = ini.ReadUInt(IniSectionName, "BinningHorizontal", 1);
            BinningVertical = ini.ReadUInt(IniSectionName, "BinningVertical", 1);
            Gain = ini.ReadFloat(IniSectionName, "Gain");
            AdcDepthIndex = ini.ReadUInt(IniSectionName, "AdcDepth");

            CaptureFrameRoiCount = ini.ReadInt(IniSectionName, "CaptureFrameRoiCount");
            CaptureFrameRois.Clear();
            for (int i = 0; i < CaptureFrameRoiCount; ++i)
            {
                CaptureFrameRois.Add(new FrameRoiPosition
                {
                    StartY = ini.ReadInt(IniSectionName, $"CaptureFrameRoi{i}_StartY"),
                    EndY = ini.ReadInt(IniSectionName, $"CaptureFrameRoi{i}_EndY"),
                });
            }
        }

        internal void SaveToIni(IniFileUtil ini)
        {
            ini.WriteInt(IniSectionName, "RoiXWidth", (int)RoiXWidth);
            ini.WriteInt(IniSectionName, "RoiYHeight", (int)RoiYHeight);
            ini.WriteInt(IniSectionName, "RoiXOffset", (int)RoiXOffset);
            ini.WriteInt(IniSectionName, "RoiYOffset", (int)RoiYOffset);
            ini.WriteInt(IniSectionName, "ExposureTime", (int)ExposureTime);
            ini.WriteInt(IniSectionName, "AcqusitionMode", (int)AcquisitionMode);
            ini.WriteInt(IniSectionName, "AcqusitionFrameCount", (int)AcquisitionFrameCount);
            ini.WriteInt(IniSectionName, "PixelFormat", (int)PixelFormat);
            ini.WriteInt(IniSectionName, "BinningHorizontal", (int)BinningHorizontal);
            ini.WriteInt(IniSectionName, "BinningVertical", (int)BinningVertical);
            ini.WriteFloat(IniSectionName, "Gain", Gain);
            ini.WriteInt(IniSectionName, "AdcDepth", (int)AdcDepthIndex);

            CaptureFrameRoiCount = CaptureFrameRois.Count;
            ini.WriteInt(IniSectionName, "CaptureFrameRoiCount", CaptureFrameRoiCount);
            for (int i = 0; i < CaptureFrameRoiCount; ++i)
            {
                ini.WriteInt(IniSectionName, $"CaptureFrameRoi{i}_StartY", CaptureFrameRois[i].StartY);
                ini.WriteInt(IniSectionName, $"CaptureFrameRoi{i}_EndY", CaptureFrameRois[i].EndY);
            }
        }

        public FrameRoiPosition[] GetCaptureFrameRoisArray()
        {
            return CaptureFrameRois.ToArray();
        }

        public void SetCaptureFrameRois(IReadOnlyList<FrameRoiPosition> rois)
        {
            CaptureFrameRois.Clear();
            foreach (FrameRoiPosition roi in rois)
            {
                CaptureFrameRois.Add(new FrameRoiPosition
                {
                    StartY = roi.StartY,
                    EndY = roi.EndY,
                });
            }

            CaptureFrameRoiCount = CaptureFrameRois.Count;
        }

        public LsoColorCameraSettingParam ToSettingParam()
        {
            return new LsoColorCameraSettingParam
            {
                roiXWidth = RoiXWidth,
                roiYHeight = RoiYHeight,
                roiXOffset = RoiXOffset,
                roiYOffset = RoiYOffset,
                exposureTime = ExposureTime,
                acquisitionMode = AcquisitionMode,
                acquisitionFrameCount = AcquisitionFrameCount,
                pixelFormat = PixelFormat,
                binningHorizontal = BinningHorizontal,
                binningVertical = BinningVertical,
                gain = Gain,
                adcDepthIndex = AdcDepthIndex,
            };
        }

        public LsoColorCameraSettingParam ToSettingParam(LsoColorCameraSettingParam baseParam)
        {
            LsoColorCameraSettingParam param = baseParam;
            param.roiXWidth = RoiXWidth;
            param.roiYHeight = RoiYHeight;
            param.roiXOffset = RoiXOffset;
            param.roiYOffset = RoiYOffset;
            param.exposureTime = ExposureTime;
            param.acquisitionMode = AcquisitionMode;
            param.acquisitionFrameCount = AcquisitionFrameCount;
            param.pixelFormat = PixelFormat;
            param.binningHorizontal = BinningHorizontal;
            param.binningVertical = BinningVertical;
            param.gain = Gain;
            param.adcDepthIndex = AdcDepthIndex;
            return param;
        }

        public void ApplySettingParam(LsoColorCameraSettingParam param)
        {
            RoiXWidth = param.roiXWidth;
            RoiYHeight = param.roiYHeight;
            RoiXOffset = param.roiXOffset;
            RoiYOffset = param.roiYOffset;
            ExposureTime = param.exposureTime;
            AcquisitionMode = param.acquisitionMode;
            AcquisitionFrameCount = param.acquisitionFrameCount;
            PixelFormat = param.pixelFormat;
            BinningHorizontal = param.binningHorizontal;
            BinningVertical = param.binningVertical;
            Gain = param.gain;
            AdcDepthIndex = param.adcDepthIndex;
        }
    }

    public partial class LsoScanTestWindow
    {
        private readonly LsoScanTestModel _scanTestModel = new LsoScanTestModel();

        WsoCallback.ColorCameraImageCaptured _onColorCaptureImageCaptured;
        WsoCallback.ColorCaptureCompleted _onColorCaptureCompleted;
        WsoCallback.ColorCameraFrameCaptured _onColorLiveFrameCaptured;

        WsoCallback.CorneaCameraFrameCaptured _onCorneaLeftFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaRightFrameCaptured;
        WsoCallback.CorneaCameraFrameCaptured _onCorneaLowerFrameCaptured;

        // Live 프레임이 UI 렌더링보다 빠르게 도착할 때 Dispatcher 큐가 쌓여 메모리가 증가하는 것을 막는 드롭 플래그.
        private volatile bool _isColorLiveFramePending;

        // LSO Focus Motor
        private StepMotorStatus _lsoFocusMotorStatus = new();
        private StepMotorPositionChanged _onLsoFocusPositionChanged;
        private bool _isLsoFocusSliderDragging; // 콜백에서 슬라이더 값을 갱신할 때 발생하는 ValueChanged가 모터를 다시 이동시키는 것을 막는 플래그.
        private bool _isLsoFocusPositionChanged;

        // Return Mirror
        private StepMotorStatus _retMirrorMotorStatus = new();
        private StepMotorPositionChanged _onRetMirrorPositionChanged;
        private bool _isRetMirrorSliderDragging;
        private bool _isRetMirrorPositionChanged;

        // LSO Filter Motor
        private StepMotorStatus _lsoFilterMotorStatus = new();
        private StepMotorPositionChanged _onLsoFilterPositionChanged;

        private volatile bool _corneaLeftPending = false;
        private volatile bool _corneaRightPending = false;
        private volatile bool _corneaLowerPending = false;


        private void initSetting_()
        {
            // Load INI
            _scanTestModel.LoadConfigFromIniFile();

            // Camera presets (one XML file per preset). Seeds what is missing and performs
            // the one-time [COLOR_CAMERA] migration; anything that had to fall back to the
            // built-in DevKit defaults is reported to the operator below.
            _scanTestModel.ModePresets.Load();
            _scanTestModel.LoadConfigFromIniFile();

            // The custom pos[] tables live in the native LsoScanner for the process lifetime
            // only. Push them now so the first Preview/Capture one-call (which resubmits the
            // profile natively) already runs the file, not the linear fallback.
            foreach (ColorCameraPresetMode mode in new[] { ColorCameraPresetMode.Preview, ColorCameraPresetMode.Capture })
            {
                if (!installTrajectoryFile_(_scanTestModel.ModePresets.Active(mode).Scan,
                        LsoModePreset.ScanPatternId(mode), out string trajectoryIssue, out string trajectoryNote))
                {
                    _startupCameraIssues.Add($"{mode} galvo trajectory: {trajectoryIssue}");
                }
                else if (trajectoryNote.Length > 0)
                {
                    _startupCameraIssues.Add($"{mode} galvo trajectory: {trajectoryNote}");
                }
            }

            // Color Camera: put the active Capture preset on the camera once, at window
            // open. This is the only place a preset is written to the camera outside a
            // Preview/Capture start - editing a preset later must not reprogram a camera
            // that is in the middle of a session.
            // The whole contract, not just the 14 fields: leaving the A-1 nodes at whatever
            // the previous session left would make the first Capture run against a trigger
            // and GPIO state no preset states.
            // The verdict is kept: this write decides what the first Capture runs against,
            // so "a setter that returned is not evidence" has to hold here too. The per-field
            // detail joins the preset issues the operator is shown a few lines below.
            var paramColorCamera = _scanTestModel.ModePresets.Capture.Camera.ToSettingParam(currentCameraParam_());
            var extColorCamera = _scanTestModel.ModePresets.Capture.Camera.ToExtendedParam();
            if (!LsoCamera.SetFullCameraParameters(ref paramColorCamera, ref extColorCamera,
                    out LsoColorCameraApplyResult startupApply))
            {
                var startupReport = new PresetApplyReport();
                collectCameraApplyRows_(startupApply, startupReport);
                _startupCameraIssues.Add("The Capture preset was not fully installed on the camera at "
                    + "startup - the first Capture will not run the values shown here:");
                _startupCameraIssues.AddRange(startupApply.fields == null || startupReport.Failures.Count == 0
                    ? new List<string> { "  no per-field detail was reported (is the camera connected?)" }
                    : startupReport.Failures.ConvertAll(f => "  " + f));
            }

            // Image Adjust preset (delegated to the inner LsoScanImagePreview)
            myColorPreview.Image.SetDefaultAdjustPreset(_scanTestModel.ImageAdjust.ToParam());
            myColorPreview.Image.AdjustPresetChanged = OnImageAdjustPresetChanged_;

            // Scan Setting
            initScannerControls_();

            // Camera presets (per-mode selection)
            initCameraPresetControls_();
            reportCameraPresetIssues_();
        }

        // The camera's own read-back. Only the fields a preset does not own (roi_max_*) are
        // taken from it; with no camera attached it is all zeros, which is exactly what the
        // pre-preset code already sent.
        private static LsoColorCameraSettingParam currentCameraParam_()
        {
            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam param);
            return param;
        }

        private void OnImageAdjustPresetChanged_(ImageAdjustParam param)
        {
            _scanTestModel.ImageAdjust.ApplyFrom(param);
            _scanTestModel.ImageAdjust.SavePresetToIniFile();
        }

        #region Captrue - Review
        // Capture - Review
        // The review needs the slot's frame layout, not just the images: the preview control
        // stitches the subframes of one acquisition back into a single image.
        private void readyToCapture_()
        {
            LsoScanProfileModel colorProfile = _scanTestModel.GetScanProfile(LsoScanCapturePreset.PatternId);

            myColorPreview.ClearReviewImages();
            myColorPreview.IsReviewMode = true;
            myColorPreview.AcqFrameCount = (int)colorProfile.AcqFrameSize;
            myColorPreview.SubFrameCount = colorProfile.SubFrameSize;
        }

        public void StartColorCameraOriginal()
        {
            // The optical path first: the filter wheel is not part of the one-call's ABI, and
            // capturing through the wrong filter is worse than refusing to start.
            if (!applyModeBoardSetup_(ColorCameraPresetMode.Capture))
            {
                return;
            }

            // Loaded per capture so a flat re-shot between two captures is picked up. The
            // preset saying "corrected" with no usable flat refuses the capture, like any
            // other preset damage.
            LsoModePreset capturePreset = _scanTestModel.ModePresets.Capture;
            if (!LsoFlatField.TryLoadFor(capturePreset, (int)capturePreset.Camera.RoiXWidth,
                    (int)capturePreset.Camera.RoiYHeight, out LsoFlatField? flat, out string flatError))
            {
                MessageBox.Show($"Capture was not started: {flatError}");
                return;
            }
            _captureFlatField = flat;
            System.Console.WriteLine(_captureFlatField == null
                ? "[LSO-FLAT] off"
                : $"[LSO-FLAT] on, flat='{_captureFlatField.Path}'");

            // One call, and nothing else: the fundus one-call owns preset apply, the scan
            // profile upload, illumination, camera arming, the board start, the bounded
            // acquisition and the teardown. This method only fills the request from the
            // knob/INI model and shows the failure text.
            // The review images are named after the preset they were shot with.
            myColorPreview.Image.CapturePresetName = _scanTestModel.ModePresets.Capture.Name;

            var request = buildFundusCaptureRequest_();
            var stage = LsoCamera.StartFundusCapture(ref request,
                _onColorCaptureImageCaptured, _onColorCaptureCompleted);
            refreshLightControls_();
            if (stage != ColorCaptureStage.Ok)
            {
                MessageBox.Show($"Capture start failed: {stage} - {LsoCamera.GetLastCaptureError()}");
            }
        }

        // Capture request: the COLOR slot profile owns the frame count, the camera model
        // owns the preset, the white LED lights the capture and aiming IR is what the
        // sequence hands back afterwards (the pre-one-call path left the white LED on).
        // Intensities are read from the board so the request states the illumination that
        // will actually run; a LED left at zero is rejected by the callee instead of
        // producing a dark capture.
        private LsoFundusCaptureRequest buildFundusCaptureRequest_()
        {
            // Assembled by the shared factory, so this screen and the Agent's measurement
            // screen send the same request built from the same preset.
            LsoFundusCaptureRequest request = LsoFundusRequestFactory.BuildCaptureRequest(
                _scanTestModel.ModePresets.Capture,
                _scanTestModel.ModePresets.Preview,
                ColorCaptureTriggerMode.HWTrigger,
                currentCameraParam_(),
                DeviceLights.GetLightIntensity(_scanTestModel.ModePresets.Capture.Light.Led),
                DeviceLights.GetLightIntensity(_scanTestModel.ModePresets.Preview.Light.Led));

            // This screen differs from the Agent in exactly one field: Capture / Slide / ROI
            // assemble the review image per frame in the preview control, so the native
            // composition path stays unused here.
            request.subFrameCompose = 0;
            return request;
        }

        // Preview request: the IR slot profile, forced continuous because an aiming view is
        // not a fixed-frame acquisition, and restore == preview light so stopping leaves the
        // illumination off (the previous behaviour).
        // The camera runs hardware-triggered off the board (DevKit preview path), with the
        // preview's own exposure/gain/trigger delay from the INI-backed model - not the
        // capture preset's values, which the camera would otherwise still be holding.
        private LsoFundusPreviewRequest buildFundusPreviewRequest_()
        {
            return LsoFundusRequestFactory.BuildPreviewRequest(
                _scanTestModel.ModePresets.Preview,
                ColorMonoPreviewMode.SensorMono8,
                currentCameraParam_(),
                DeviceLights.GetLightIntensity(_scanTestModel.ModePresets.Preview.Light.Led));
        }

        // Only the optical path (filter wheel, focus) precedes the one-call.
        // The native session owns the scan sequence and illumination, just as in the Agent.
        //
        // Blocking by nature - the native move waits on the board - so the cursor says so, the
        // same way moveGalvanoPositionY_ does on this screen.
        private bool applyModeBoardSetup_(ColorCameraPresetMode mode)
        {
            LsoModePreset preset = _scanTestModel.ModePresets.Active(mode);
            System.Console.WriteLine(
                $"[LSO-PRESET] mode={mode} preset='{preset.Name}' " +
                $"preview='{_scanTestModel.ModePresets.Preview.Name}' " +
                $"capture='{_scanTestModel.ModePresets.Capture.Name}' " +
                $"config='{LsoScanTestModel.DefaultConfigIniPath}' " +
                $"presets='{_scanTestModel.ModePresets.Directory}' trigger=HWTrigger " +
                $"sequence={preset.Scan.SequenceMode} preMove={preset.Scan.PreMoveOffset}/" +
                $"{preset.Scan.PreMoveDelayUs}us light={preset.Light.Led}/{preset.Light.Mode} " +
                $"restoreLight={_scanTestModel.ModePresets.Preview.Light.Led}/" +
                $"{_scanTestModel.ModePresets.Preview.Light.Mode}");
            Mouse.OverrideCursor = Cursors.Wait;
            try
            {
                if (!LsoModeMotorApply.Apply(preset.Motor, out string motorFailure))
                {
                    MessageBox.Show($"{mode} was not started: {motorFailure}");
                    return false;
                }

                return true;
            }
            finally
            {
                Mouse.OverrideCursor = null;
            }
        }

        // sweepGalvo: false is the slit-alignment case. The camera live path free-runs on its own
        // (native startLiveMode turns the trigger off), so skipping the grabbing start is what
        // keeps the galvo parked where the alignment put it instead of sweeping it.
        public void StartColorCameraLive(bool sweepGalvo = true)
        {
            _isColorLiveFramePending = false;

            // Normal color live always renders in color (clear any leftover aiming gray flag).
            myColorPreview.IsForceGrayLiveDisplay = false;

            myColorPreview.SetLiveMode();

            if (sweepGalvo)
            {
                LsoScanner.StartLsoScannerGrabbing(LsoScanCapturePreset.PatternId);
            }
            LsoCamera.StartLiveMode(_onColorLiveFrameCaptured);
            myColorPreview.StartTimer();
        }

        public void PauseColorCameraLive()
        {
            LsoScanner.PauseLsoScannerGrabbing(LsoScanCapturePreset.PatternId);
            LsoScanner.SetLsoScannerTriggerMode((int)LsoScannerTriggerMode.RollingShutter);

            LsoCamera.PauseLiveMode();
            myColorPreview.StopTimer();
        }

        public bool IsColorCameraLive()
        {
            return LsoCamera.IsLiveMode();
        }

        #endregion Captrue - Review

        #region Cornea Camera

        private bool isCorneaCameraPreviewing_()
        {
            if (CorneaCamera.IsPreviewing())
                return true;

            return false;
        }

        public void StartCorneaCameraPreview()
        {
            corneaPreview1.CameraType = CameraType.IrCorneaLeft;
            corneaPreview2.CameraType = CameraType.IrCorneaRight;
            corneaPreview3.CameraType = CameraType.IrCorneaLower;

            corneaPreview1.Callback = _onCorneaLeftFrameCaptured;
            corneaPreview2.Callback = _onCorneaRightFrameCaptured;
            corneaPreview3.Callback = _onCorneaLowerFrameCaptured;

            corneaPreview1.IsOverlayFocusGuide = true;
            corneaPreview2.IsOverlayFocusGuide = true;
            corneaPreview3.IsOverlayFocusGuide = true;

            corneaPreview1.IsStageControlEnabled = true;
            corneaPreview2.IsStageControlEnabled = true;
            corneaPreview3.IsStageControlEnabled = true;

            CorneaCamera.StartPreview(_onCorneaLeftFrameCaptured, _onCorneaRightFrameCaptured, _onCorneaLowerFrameCaptured);
            updateCorneaButtonLabel_();
        }

        public void CloseCorneaCameraPreview()
        {
            CorneaCamera.ClosePreview();
            updateCorneaButtonLabel_();
        }

        // The one button starts and stops the cornea preview, so its label states what the
        // next click does - read from the camera state, not assumed from the last request.
        private void updateCorneaButtonLabel_()
        {
            myBtCorneaStart.Content = isCorneaCameraPreviewing_() ? "Stop Cornea" : "Start Cornea";
        }

        #endregion Cornea Camera 

        #region Retina Camera

        // Whether the color-camera mono aiming live is active.
        private bool IsRetinaPreviewing_()
        {
            return myColorPreview.IsPreviewing();
        }

        // Start the mono aiming live. The native one-call (via ColorCameraPreview.PlayPreview)
        // owns the IR LED + IR scan pattern + camera setup; mode follows the UI option.
        private void StartRetinaPreview_()
        {
            // No scanner call here: the preview one-call uploads the IR slot profile and
            // starts the board itself. Driving it from both sides sent the board an extra
            // off/on pair on every transition, uploaded the trajectory while the galvano was
            // already sweeping, and left the scanner running when the one-call refused to
            // start (IsRetinaPreviewing_ stays false, so no stop path ever reaches it).
            if (!applyModeBoardSetup_(ColorCameraPresetMode.Preview))
            {
                return;
            }

            myColorPreview.LiveCallback = _onColorLiveFrameCaptured;
            myColorPreview.PlayPreview(ColorPreviewMonoMode.SensorMono8, buildFundusPreviewRequest_());
            refreshLightControls_();
            updateScanButtonLabel_();
        }

        // Stop the aiming live. The native one-call (via StopPreview) pauses the IR scan pattern,
        // applies the restore illumination, and restores camera params.
        private void StopRetinaPreview_()
        {
            myColorPreview.StopPreview();
            refreshLightControls_();
            updateScanButtonLabel_();
        }

        private void refreshLightControls_()
        {
            if (myLightControlPanel.IsLoaded)
            {
                myLightControlPanel.Refresh();
            }
        }

        // Start Scan doubles as Stop Scan. Every start/stop path routes through the two
        // methods above, so the label is set here once and a refused start cannot make it lie.
        private void updateScanButtonLabel_()
        {
            myBtStartScan.Content = IsRetinaPreviewing_() ? "Stop Scan" : "Start Scan";

            // Apply writes the board, so it follows the same busy state as the button.
            updateSyncControls_();
        }

        // Start Scan: show the color-camera aiming preview.
        private void ShowRetinaPreview_()
        {
            myColorPreview.Visibility = Visibility.Visible;
        }

        // Capture: show the acquired LSO color image.
        private void ShowColorPreview_()
        {
            myColorPreview.Visibility = Visibility.Visible;
        }

        #endregion Retina Camera

        #region Camera Preset

        private bool _isUpdatingCameraPresets;

        // Per-mode preset selection. The combos are editable on purpose: the text is also
        // the name "Save As" writes, so adding a preset needs no extra dialog.
        private void initCameraPresetControls_()
        {
            _isUpdatingCameraPresets = true;
            myCbCaptureImageMode.ItemsSource = LsoImageModes.All;
            _isUpdatingCameraPresets = false;
            refreshCameraPresetControls_();
        }

        private void refreshCameraPresetControls_()
        {
            _isUpdatingCameraPresets = true;
            try
            {
                myCbCaptureImageMode.SelectedItem = _scanTestModel.ModePresets.CaptureImageMode;
                IReadOnlyList<string> names = _scanTestModel.ModePresets.EnumeratePresetNames();
                fillCameraPresetCombo_(myCbPreviewPreset, names, _scanTestModel.ModePresets.Preview.Name);
                fillCameraPresetCombo_(myCbCapturePreset, names, _scanTestModel.ModePresets.Capture.Name);
            }
            finally
            {
                _isUpdatingCameraPresets = false;
            }

            // A different preset means a different slit height / line time readout.
            updateAlignControls_();
        }

        private static void fillCameraPresetCombo_(ComboBox combo, IReadOnlyList<string> names, string selected)
        {
            combo.Items.Clear();
            foreach (string name in names)
            {
                combo.Items.Add(name);
            }

            combo.SelectedItem = names.FirstOrDefault(
                name => string.Equals(name, selected, StringComparison.OrdinalIgnoreCase));
            combo.Text = selected;
        }

        private ComboBox cameraPresetCombo_(ColorCameraPresetMode mode)
        {
            return mode == ColorCameraPresetMode.Preview ? myCbPreviewPreset : myCbCapturePreset;
        }

        // Selecting a preset only changes which values the next Preview/Capture start sends.
        // It deliberately does not touch a running session's camera.
        private void commitCameraPresetSelection_(ColorCameraPresetMode mode)
        {
            if (_isUpdatingCameraPresets)
            {
                return;
            }

            if (cameraPresetCombo_(mode).SelectedItem is not string name)
            {
                return;
            }

            _scanTestModel.ModePresets.Select(mode, name);
            refreshCameraPresetControls_();
            reportCameraPresetIssues_();

            // The selected preset carries its own timing and sequence, so the inline Sync row
            // has to follow it - otherwise it keeps showing the previous preset's numbers.
            reflectScanTiming_();
            updateAlignControls_();
        }

        // Rebinds the Capture role to another image mode: the Capture combo, the Capture
        // button, Settings and the Sync block all follow the mode's own preset. A mode never
        // used before gets its seed preset (the store's rule). Refused mid-capture because the
        // mode's trajectory is installed into the Capture slot the running one-call uses.
        private void commitCaptureImageMode_()
        {
            if (_isUpdatingCameraPresets
                || myCbCaptureImageMode.SelectedItem is not LsoImageMode imageMode
                || imageMode == _scanTestModel.ModePresets.CaptureImageMode)
            {
                return;
            }

            if (LsoCamera.IsCapturing()
                || (_scanTestModel.ModePresets.HasUnsavedChanges(ColorCameraPresetMode.Capture)
                    && !MsgBoxUtil.ShowQuestion($"'{_scanTestModel.ModePresets.Capture.Name}' has changes "
                        + $"that are not saved. Discard them and switch to {imageMode}?")))
            {
                refreshCameraPresetControls_();
                return;
            }

            _scanTestModel.ModePresets.SelectCaptureImageMode(imageMode);
            if (!installTrajectoryFile_(_scanTestModel.ModePresets.Capture.Scan,
                    LsoModePreset.ScanPatternId(ColorCameraPresetMode.Capture), out string trajectoryIssue, out _))
            {
                MsgBoxUtil.ShowWarning($"{imageMode} Capture galvo trajectory: {trajectoryIssue}");
            }

            refreshCameraPresetControls_();
            reportCameraPresetIssues_();
            reflectScanTiming_();
            updateAlignControls_();
        }

        private void openModeSettings_()
        {
            if (LsoCamera.IsCapturing())
            {
                return;
            }

            var window = new LsoModeSettingWindow(this, _scanTestModel) { Owner = this };
            window.ShowDialog();
            refreshCameraPresetControls_();
            reportCameraPresetIssues_();

            // The dialog edits the same preset objects the inline Sync row shows.
            reflectScanTiming_();
            updateAlignControls_();
        }

        private void saveCameraPreset_(ColorCameraPresetMode mode)
        {
            _scanTestModel.ModePresets.Save(mode);
            refreshCameraPresetControls_();
            reportCameraPresetIssues_();
        }

        // "Save As" uses whatever name is typed in that mode's combo, so adding a preset is
        // type-a-name + click. An existing name is overwritten only after a confirmation.
        private void saveCameraPresetAs_(ColorCameraPresetMode mode)
        {
            string name = (cameraPresetCombo_(mode).Text ?? string.Empty).Trim();
            if (!LsoModePresetStore.IsValidName(name))
            {
                MsgBoxUtil.ShowWarning("Type a preset name (a valid file name) in the combo box first.");
                refreshCameraPresetControls_();
                return;
            }

            if (_scanTestModel.ModePresets.Exists(name)
                && !MsgBoxUtil.ShowQuestion($"Preset '{name}' already exists. Overwrite it?"))
            {
                refreshCameraPresetControls_();
                return;
            }

            _scanTestModel.ModePresets.SaveAs(mode, name);
            refreshCameraPresetControls_();
            reportCameraPresetIssues_();
        }

        // Every fallback the store had to make is shown, once, instead of leaving the
        // operator with a preset that silently became something else.
        // Anything the startup camera write could not install. Collected there and shown
        // here, with the preset fallbacks, so the one message the operator gets at window
        // open covers both the file and the device.
        private readonly List<string> _startupCameraIssues = new List<string>();

        private void reportCameraPresetIssues_()
        {
            var lines = new List<string>(_scanTestModel.ModePresets.DrainIssues());
            if (_startupCameraIssues.Count > 0)
            {
                lines.AddRange(_startupCameraIssues);
                _startupCameraIssues.Clear();
            }

            if (lines.Count == 0)
            {
                return;
            }

            MsgBoxUtil.ShowWarning("Camera presets:" + Environment.NewLine + Environment.NewLine
                + string.Join(Environment.NewLine, lines));
        }

        #endregion Camera Preset

        #region Scan Setting

        // When the Capture slot starts under Normal (software) trigger, derive the camera's
        // AcquisitionMode / AcquisitionFrameCount from the scan profile (AcqFrame x SubFrame)
        // at Apply. Exposure is NOT part of this any more: the camera preset's exposure is
        // the authority and the scan profile only mirrors it (DevKit rule).
        internal bool DeriveAcquisitionFromScan { get; set; } = true;

        internal bool ApplyScannerControlParam(int patternId) => applyScannerControlParam_(patternId);

        // Writes one mode preset to the hardware: camera (the 14 structural fields AND the
        // A-1 extension), illumination and the scanner slot. Each layer is checked on its
        // own, and every failure is named - the caller saves the preset file only on a clean
        // Success, so a failed Apply leaves the previously saved file untouched.
        internal sealed class PresetApplyReport
        {
            public bool Rejected { get; set; }
            public bool CameraApplied { get; set; }
            public bool LightApplied { get; set; }
            public bool ScanApplied { get; set; }
            public bool MotorApplied { get; set; }
            // The aiming preview or the color live was left stopped on purpose - the reason
            // is in Warnings. Not a failure: the hardware took the settings, the session
            // just cannot run against them.
            public bool PreviewLeftStopped { get; set; }
            public bool LiveLeftStopped { get; set; }
            // The preset is valid to store and was written to the hardware, but Preview or
            // Capture will refuse to START with it. Tuning is allowed to produce such a
            // preset, so this does NOT fail the apply and does not block saving it.
            public bool StartBlocked { get; set; }

            // What the restart needs, carried here because the restart is no longer part of
            // the apply: the caller refreshes its UI while the camera is still stopped and
            // then asks for the session back. The panel's refresh re-queries the GenICam
            // node map, and doing that against a streaming, freshly armed camera fights the
            // live worker for the node map and the USB control channel - which is what hung
            // the UI thread.
            internal bool WasPreviewing { get; set; }
            internal bool WasLive { get; set; }
            internal LsoColorCameraExtendedParam CameraExt { get; set; }
            internal bool RestartDone { get; set; }

            public List<string> Failures { get; } = new List<string>();

            // Things the operator must see but which did not stop anything: a device that
            // quantised a value, a node it does not implement, a session left stopped.
            public List<string> Warnings { get; } = new List<string>();

            public bool Success => !Rejected && CameraApplied && LightApplied && ScanApplied
                && MotorApplied;

            // The camera and the scanner slot took the values - what the preset file and a
            // restarted session need. A lamp or a motor that refused is reported in Failures,
            // but does not keep a verified camera/galvo tuning off disk or the session stopped.
            public bool CanSave => !Rejected && CameraApplied && ScanApplied;

            public string Describe()
            {
                var lines = new List<string>(Failures);
                if (Warnings.Count > 0)
                {
                    if (lines.Count > 0)
                    {
                        lines.Add(string.Empty);
                    }
                    lines.Add("Notes:");
                    lines.AddRange(Warnings.ConvertAll(w => "  " + w));
                }
                return string.Join(Environment.NewLine, lines);
            }
        }

        // This screen tunes the sensor, so a running aiming preview or color live is stopped,
        // reprogrammed and restarted instead of being refused - the same stop/apply/restart
        // the DevKit camera config does. A capture one-call is never interrupted.
        //
        // One camera cannot hold two modes' settings at once, so applying the Capture preset
        // while the (Preview-driven) aiming view is running leaves the preview STOPPED rather
        // than restarting it on top of the settings just written: restarting would reinstall
        // the Preview preset and quietly undo the Apply the operator just asked for. Starting
        // Preview again explicitly reinstalls the Preview preset, which is where that belongs.
        internal PresetApplyReport ApplyPresetToBoard(ColorCameraPresetMode mode)
        {
            var report = new PresetApplyReport();

            if (LsoCamera.IsCapturing())
            {
                report.Rejected = true;
                report.Failures.Add("A capture is in progress - nothing was applied.");
                return report;
            }

            LsoModePreset preset = _scanTestModel.ModePresets.Active(mode);

            // The galvo exposure sync used to run AFTER the checked camera apply, writing
            // exposure / acquisition mode / frame count straight to the camera with no
            // read-back and mutating the preset behind the report. It is folded into the
            // preset here instead, so those values go out through the SAME checked apply and
            // the report describes the camera that is actually running.
            applyExposureSyncToPreset_(mode, report);

            // A stated camera configuration that the board wiring cannot drive is reported
            // BEFORE anything is written, with the conflicting field named. It does NOT
            // block the write or the save: tuning is allowed to produce a preset that the
            // instrument cannot currently start, and the operator is told exactly that.
            LsoColorCameraExtendedParam cameraExt = preset.Camera.ToExtendedParam();
            CameraStartContract contract = LsoCamera.ValidateStartContract(ref cameraExt,
                startsHardwareTriggered_(mode));
            if (contract != CameraStartContract.Ok)
            {
                report.StartBlocked = true;
                report.Warnings.Add($"{mode} cannot START with this camera configuration: "
                    + LsoCamera.DescribeStartContract(contract)
                    + ". The values were still written to the camera and the preset can be saved.");
            }

            bool wasPreviewing = IsRetinaPreviewing_();
            bool wasLive = IsColorCameraLive();

            if (wasPreviewing)
            {
                StopRetinaPreview_();
            }
            else if (wasLive)
            {
                PauseColorCameraLive();
            }

            // Camera: the whole contract in one checked apply. True only when the structural
            // read-back matched AND every stated A-1 field was written and read back equal
            // (or was quantised by the device, which is reported as a note, not a failure).
            LsoColorCameraSettingParam cameraParam = preset.Camera.ToSettingParam(currentCameraParam_());
            report.CameraApplied = LsoCamera.SetFullCameraParameters(ref cameraParam, ref cameraExt,
                out LsoColorCameraApplyResult applyResult);

            collectCameraApplyRows_(applyResult, report);
            adoptDeviceGain_(preset);
            if (!report.CameraApplied && report.Failures.Count == 0)
            {
                report.Failures.Add("Camera: the apply reported a failure with no field detail "
                    + "(is the camera connected?).");
            }

            // Illumination. Previously written without looking at the answer; a light that
            // refuses the write now fails the Apply instead of being reported as success.
            LightType light = preset.Light.Led;
            report.LightApplied = true;
            if (preset.Light.Intensity > 0
                && !DeviceLights.SetLightIntensity(light, preset.Light.Intensity))
            {
                report.LightApplied = false;
                report.Failures.Add($"Light: intensity {preset.Light.Intensity} was not accepted by {light}.");
            }
            if (!DeviceLights.SetLightMode(light, (LightMode)preset.Light.Mode))
            {
                report.LightApplied = false;
                report.Failures.Add($"Light: mode {(LightMode)preset.Light.Mode} was not accepted by {light}.");
            }
            refreshLightControls_();

            // syncCamera: false - the exposure sync already went out with the checked apply
            // above. Letting it write the camera again here is what used to invalidate the
            // report a line after it was produced.
            report.ScanApplied = applyScannerControlParam_(LsoModePreset.ScanPatternId(mode), syncCamera: false);
            if (report.ScanApplied && _lastTrajectoryNote.Length > 0)
            {
                report.Warnings.Add("Galvo: " + _lastTrajectoryNote);
            }
            if (!report.ScanApplied)
            {
                report.Failures.Add(_lastTrajectoryIssue.Length > 0
                    ? "Galvo: the scan profile was not written - " + _lastTrajectoryIssue
                    : "Galvo: the scan profile was not written. "
                        + "Pattern Size must be non-zero - press Load Board to read the current profile first.");
            }

            // Motors last: the optical path a mode images through is independent of the board
            // writes above, and the move blocks for seconds, so nothing else waits on it.
            // This screen is the sensor tuning tool and already blocks on device operations.
            report.MotorApplied = LsoModeMotorApply.Apply(preset.Motor, out string motorFailure);
            if (!report.MotorApplied)
            {
                report.Failures.Add("Motor: " + motorFailure);
            }

            // The session is deliberately left stopped here. RestartSessionAfterApply puts
            // it back once the caller has finished touching the camera from the UI thread.
            report.WasPreviewing = wasPreviewing;
            report.WasLive = wasLive;
            report.CameraExt = cameraExt;
            return report;
        }

        // Second half of Apply: start the interrupted session again. Idempotent, so a caller
        // that asks twice cannot open two sessions, and a caller that forgets leaves the
        // instrument stopped rather than streaming against an unrefreshed screen.
        internal void RestartSessionAfterApply(ColorCameraPresetMode mode, PresetApplyReport report)
        {
            if (report.RestartDone)
            {
                return;
            }
            report.RestartDone = true;

            LsoColorCameraExtendedParam cameraExt = report.CameraExt;
            restartAfterApply_(mode, report, report.WasPreviewing, report.WasLive, ref cameraExt);
        }

        // Puts back the session the apply interrupted - but only when there is something
        // safe to put it back into. A restart is itself a device operation, so its outcome
        // is part of the verdict instead of being assumed.
        private void restartAfterApply_(ColorCameraPresetMode mode, PresetApplyReport report,
            bool wasPreviewing, bool wasLive, ref LsoColorCameraExtendedParam cameraExt)
        {
            if (!wasPreviewing && !wasLive)
            {
                return;
            }

            // A half-written camera must not be handed straight back to a running session:
            // the operator would be looking at frames from a configuration nobody verified.
            if (!report.CanSave)
            {
                report.PreviewLeftStopped = wasPreviewing;
                report.LiveLeftStopped = wasLive;
                report.Warnings.Add("The imaging session was left stopped because the apply did not "
                    + "complete. Fix the reported fields and start it again.");
                return;
            }

            if (wasPreviewing)
            {
                // One camera cannot hold two modes' settings at once. Restarting the aiming
                // preview after a Capture apply would reinstall the Preview preset and
                // quietly undo what was just written.
                if (mode != ColorCameraPresetMode.Preview)
                {
                    report.PreviewLeftStopped = true;
                    report.Warnings.Add("The aiming preview was left stopped: one camera cannot hold the "
                        + "Preview and Capture settings at the same time. Press Preview again to run it "
                        + "with the Preview preset.");
                    return;
                }

                StartRetinaPreview_();
                if (!IsRetinaPreviewing_())
                {
                    report.PreviewLeftStopped = true;
                    report.Warnings.Add("The aiming preview did not restart after the apply: "
                        + LsoCamera.GetLastCaptureError());
                }
                return;
            }

            // Color live is free-running, so starting it writes TriggerMode Off. Against a
            // preset that states the trigger On that is a silent overwrite of the value this
            // apply just verified - so the session is left stopped and said so instead.
            if (LsoCamera.ValidateStartContract(ref cameraExt, false) != CameraStartContract.Ok)
            {
                report.LiveLeftStopped = true;
                report.Warnings.Add("Color live was left stopped: it runs free-running, and starting it "
                    + "would overwrite the trigger configuration this preset states. Use Preview or a "
                    + "hardware-triggered Capture instead.");
                return;
            }

            StartColorCameraLive();
            if (!IsColorCameraLive())
            {
                report.LiveLeftStopped = true;
                report.Warnings.Add("Color live did not restart after the apply.");
            }
        }

        // Folds the galvo exposure sync into the preset BEFORE the checked camera apply, so
        // the values it derives are verified with everything else instead of being written
        // afterwards through an unchecked path.
        // Precedence: the CAMERA preset owns exposure and the scan profile mirrors it, for
        // both modes (the DevKit rule - its scan form only displays the camera config's
        // exposure). Under Normal trigger on the Capture slot the scan profile additionally
        // owns acquisition mode/count when DeriveAcquisitionFromScan is on; HW Trigger keeps
        // the preset's values and says nothing, because nothing was replaced.
        private void applyExposureSyncToPreset_(ColorCameraPresetMode mode, PresetApplyReport report)
        {
            LsoModePreset preset = _scanTestModel.ModePresets.Active(mode);
            mirrorCameraExposureIntoScan_(preset);

            int patternId = LsoModePreset.ScanPatternId(mode);
            if (!DeriveAcquisitionFromScan
                || patternId != LsoScanCapturePreset.PatternId
                || startsHardwareTriggered_(mode))
            {
                return;
            }

            uint frameCount = calcAcquisitionFrameCount_(patternId);
            if (frameCount == 0)
            {
                return;
            }

            ColorCameraPreset camera = preset.Camera;
            uint previousMode = camera.AcquisitionMode;
            uint previousCount = camera.AcquisitionFrameCount;
            camera.AcquisitionFrameCount = frameCount;
            camera.AcquisitionMode = frameCount == 1 ? 1u : 2u;
            _scanTestModel.ColorCamera.ApplySettingParam(camera.ToSettingParam());

            if (previousMode != camera.AcquisitionMode || previousCount != camera.AcquisitionFrameCount)
            {
                report.Warnings.Add($"Normal trigger: the scan profile set the camera acquisition mode/count "
                    + $"to {camera.AcquisitionMode}/{frameCount} (AcqFrame x SubFrame); the preset had "
                    + $"{previousMode}/{previousCount}.");
            }
        }

        // Scan.ExposureTimeUs is a mirror of Camera.ExposureTime. Copy it at every point
        // the scan profile is about to leave for the board, so the two cannot drift.
        private static void mirrorCameraExposureIntoScan_(LsoModePreset preset)
        {
            if (preset.Camera.ExposureTime > 0)
            {
                preset.Scan.ExposureTimeUs = (int)preset.Camera.ExposureTime;
            }
        }

        // Whether starting this mode drives the camera from the board trigger. Both modes do:
        // Preview always has, and Capture is hardware-triggered now that the selector is gone.
        private static bool startsHardwareTriggered_(ColorCameraPresetMode mode)
        {
            return true;
        }

        // One line per field that did not end up holding exactly the requested value, with
        // the requested and the actual side by side - "it failed" is not an answer an
        // operator can act on.
        //
        // Adjusted rows are notes, not failures: the write succeeded and the device holds
        // the nearest value it can, which is a configuration it can run. Everything else -
        // a node this camera does not implement, one it refused, one that read back
        // different - is a failure, because the operator explicitly asked for that value.
        // An explicitly requested unsupported value is never reported as success.
        private static void collectCameraApplyRows_(LsoColorCameraApplyResult result, PresetApplyReport report)
        {
            if (result.fields == null)
            {
                return;
            }

            int count = Math.Min(result.fieldCount, result.fields.Length);
            for (int i = 0; i < count; ++i)
            {
                LsoColorCameraFieldResult field = result.fields[i];
                var status = (ColorCameraFieldStatus)field.status;
                if (status == ColorCameraFieldStatus.Applied || status == ColorCameraFieldStatus.NotRequested)
                {
                    continue;
                }

                string line = $"{field.node}: {status} (requested '{field.requested}', actual '{field.actual}')";
                if (status == ColorCameraFieldStatus.Adjusted)
                {
                    report.Warnings.Add("Camera " + line
                        + " - the device does not have the requested value and used the nearest one it has.");
                }
                else
                {
                    report.Failures.Add("Camera " + line);
                }
            }
        }

        // The camera quantises Gain to its own grid - a typed 27.04 dB is held as 27.045664 -
        // and GenICam does not publish that grid, because the Gain node reports no increment.
        // So the preset adopts what the device actually holds after the apply: what gets saved
        // is then always a gain this camera can reproduce, and the next apply compares the
        // device against a value that exists instead of against a typed approximation.
        // No camera, or a node this model does not have, leaves the preset untouched.
        private static void adoptDeviceGain_(LsoModePreset preset)
        {
            if (LsoCamera.GetNodeValue("Gain", out double actual) && actual >= 0.0)
            {
                preset.Camera.Gain = (float)actual;
            }
        }

        internal int PreviewBoardExposureUs() => previewBoardExposureUs_();

        internal static string ScannerPatternName(int patternId) => scannerPatternName_(patternId);

        private void initScannerControls_()
        {
            for (int i = 0; i < _scanTestModel.ScanProfileCount; i++)
            {
                applyScannerControlParam_(i);
            }
        }

        internal static string scannerPatternName_(int patternId)
        {
            return patternId switch
            {
                LsoScanPreviewPreset.PatternId => $"{patternId}: Preview",
                LsoScanCapturePreset.PatternId => $"{patternId}: Capture",
                _ => $"{patternId}: (reserved)",
            };
        }

        // Board exposure for the preview profile follows the camera (see
        // LsoScanPreviewPreset.ApplyTo). While the aiming preview is running the camera IS
        // holding the preview exposure, so its read-back is the exact value the board is
        // being driven with; otherwise the camera still holds the capture preset, and the
        // value the preview will install is the configured preview exposure - reading the
        // camera there would show the capture exposure instead.
        // Either way this is only what the panel displays: startLsoFundusPreview overwrites
        // the uploaded profile with the camera's real read-back after applying the preset.
        private int previewBoardExposureUs_()
        {
            if (IsRetinaPreviewing_())
            {
                LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam cameraParam);
                if (cameraParam.exposureTime > 0)
                {
                    return (int)cameraParam.exposureTime;
                }
            }

            return (int)_scanTestModel.ModePresets.Preview.Camera.ExposureTime;
        }

        // syncCamera: whether this call may also push the derived acquisition mode/count at
        // the camera. ApplyPresetToBoard passes false because it has already folded them
        // into its single checked apply; the other callers (window open, the Scan Setting
        // screen) keep the original behaviour. Exposure is never pushed from here: the camera
        // preset owns it and the scan profile is refreshed from it before the upload.
        private bool applyScannerControlParam_(int nPatternId, bool syncCamera = true)
        {
            LsoScanProfileModel profile = _scanTestModel.GetScanProfile(nPatternId);
            _lastTrajectoryIssue = string.Empty;
            _lastTrajectoryNote = string.Empty;

            // A zero pattern size is not a scan - the native side rejects it in
            // generateGalvanoPositions() but still stores the (zeroed) trajectory. Refuse
            // here instead, so selecting an empty reserved slot cannot wipe it on the board.
            if (profile.GalvoPatternSize == 0)
            {
                return false;
            }

            ColorCameraPresetMode? mode = nPatternId switch
            {
                LsoScanPreviewPreset.PatternId => ColorCameraPresetMode.Preview,
                LsoScanCapturePreset.PatternId => ColorCameraPresetMode.Capture,
                _ => null,
            };
            if (mode.HasValue)
            {
                mirrorCameraExposureIntoScan_(_scanTestModel.ModePresets.Active(mode.Value));
            }

            if (syncCamera
                && nPatternId == LsoScanCapturePreset.PatternId
                && DeriveAcquisitionFromScan
                && !startsHardwareTriggered_(ColorCameraPresetMode.Capture))
            {
                LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam tempCameraParam);
                tempCameraParam.acquisitionFrameCount = calcAcquisitionFrameCount_(nPatternId);
                tempCameraParam.acquisitionMode = tempCameraParam.acquisitionFrameCount == 1 ? 1u : 2u;

                LsoCamera.SetCameraParameters(ref tempCameraParam);
                _scanTestModel.ColorCamera.ApplySettingParam(tempCameraParam);

                // Keep the Capture preset in step: the capture one-call sends the preset,
                // so leaving it behind here would undo the derivation on the next Capture.
                // The file is written by Apply (myBtApply_Click), like the INI.
                _scanTestModel.ModePresets.Capture.Camera.ApplySettingParam(tempCameraParam);
            }

            if (!installTrajectoryFile_(profile, nPatternId, out _lastTrajectoryIssue, out _lastTrajectoryNote))
            {
                return false;
            }

            LsoScannerControlParam param = profile.ToControlParam();
            return LsoScanner.SubmitLsoScannerControlParam(nPatternId, param);
        }

        // Why the last applyScannerControlParam_ refused, when the CSV was the reason - and,
        // when it did not refuse, what it had to do to the table (resampling).
        private string _lastTrajectoryIssue = string.Empty;
        private string _lastTrajectoryNote = string.Empty;

        // Loads the preset's CSV (if any) into the native per-slot override, or clears the
        // override when the preset has none. Pattern Size is a DERIVED value - the timing
        // calculator rewrites it from line rate / exposure / galvo delay - so a table whose
        // row count no longer matches is resampled to the live size (shape kept) and the
        // operator is told, rather than the apply failing every time the calculator moves N.
        private static bool installTrajectoryFile_(LsoScanProfileModel profile, int nPatternId,
            out string issue, out string note)
        {
            issue = string.Empty;
            note = string.Empty;
            if (!profile.HasTrajectoryFile)
            {
                return LsoScanner.SetLsoScannerTrajectory(nPatternId, null);
            }

            short[]? pos = LsoScanProfileModel.LoadTrajectoryFile(profile.TrajectoryFile, out string error);
            if (pos == null)
            {
                issue = $"'{profile.TrajectoryFile}': {error}";
                return false;
            }
            if (pos.Length != profile.GalvoPatternSize)
            {
                note = $"'{System.IO.Path.GetFileName(profile.TrajectoryFile)}' has {pos.Length} samples; "
                    + $"resampled to the live Pattern Size {profile.GalvoPatternSize} (shape kept). "
                    + "Re-export from the Trajectory Editor to make the file match.";
                pos = LsoScanProfileModel.ResampleTrajectory(pos, profile.GalvoPatternSize);
            }
            if (!LsoScanner.SetLsoScannerTrajectory(nPatternId, pos))
            {
                issue = "the native scanner did not accept the table.";
                return false;
            }
            return true;
        }

        private uint calcAcquisitionFrameCount_(int nPatternId)
        {
            LsoScanProfileModel profile = _scanTestModel.GetScanProfile(nPatternId);

            uint nAcqFrameCount = profile.AcqFrameSize;
            uint nSubFrameCount = profile.SubFrameSize;

            if (nAcqFrameCount > 0 && nSubFrameCount > 0)
            {
                return nAcqFrameCount * nSubFrameCount;
            }

            return 0;
        }

        #endregion Scan Setting

        #region Galvano Move

        private void moveGalvanoPositionY_(int pos)
        {
            setGalvanoYPosition_(pos);
            Mouse.OverrideCursor = Cursors.Wait;
            try
            {
                if (!LsoScanner.MoveLsoScannerYposition(pos))
                {
                    MessageBox.Show("Move galvano position Y failed!");
                }
            }
            finally
            {
                Mouse.OverrideCursor = null;
            }
        }

        #endregion Galvano Move

        #region Internal Fixation

        private void initFixation_()
        {
            mySliderFixationRow.Minimum = FIXATION_ROW_MIN;
            mySliderFixationRow.Maximum = FIXATION_ROW_MAX;
            mySliderFixationCol.Minimum = FIXATION_COL_MIN;
            mySliderFixationCol.Maximum = FIXATION_COL_MAX;

            setPresetInternalFixation_(FixationTarget.FUNDUS);
        }

        private void getCurrentInternalFixation_()
        {
            if (Fixation.GetCurrentInternalFixation(out int row, out int col))
            {
                mySliderFixationRow.Value = row;
                mySliderFixationCol.Value = col;
                myTbFixationRow.Text = row.ToString();
                myTbFixationCol.Text = col.ToString();
            }
        }

        private void setCurrentInternalFixation_()
        {
            int row = (int)mySliderFixationRow.Value;
            int col = (int)mySliderFixationCol.Value;
            Fixation.TurnOnInternalFixation(row, col);
            getCurrentInternalFixation_();
        }

        private void setPresetInternalFixation_(FixationTarget target)
        {
            EyeSide side = myRbSideOD.IsChecked == true ? EyeSide.OD : EyeSide.OS;
            Fixation.TurnOnInternalFixationWithTarget(side, target);
            getCurrentInternalFixation_();
        }

        #endregion Internal Fixation

        #region LSO Focus Motor

        private void initLsoFocusMotor_()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.LsoFocus, out _lsoFocusMotorStatus);

            mySliderLsoFocus.Minimum = _lsoFocusMotorStatus.rangeMin;
            mySliderLsoFocus.Maximum = _lsoFocusMotorStatus.rangeMax;
            mySliderLsoFocus.TickFrequency = 100;

            myLbLsoFocusMin.Content = _lsoFocusMotorStatus.rangeMin.ToString();
            myLbLsoFocusMax.Content = _lsoFocusMotorStatus.rangeMax.ToString();

            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.LsoFocus, _onLsoFocusPositionChanged);

            DeviceMotors.MoveStepMotorPositionAsync(
                MotorType.LsoFocus, mapLsoFocusValueToPosition_(0.0));
        }

        private void releaseLsoFocusMotor_()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.LsoFocus);
        }

        private void OnLsoFocusPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                myTbLsoFocusPos.Text = pos.ToString();
                myTbLsoFocusValue.Text = value.ToString("N1");

                if (pos != mySliderLsoFocus.Value)
                {
                    mySliderLsoFocus.Value = pos;
                    _isLsoFocusPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        #endregion LSO Focus Motor

        #region Return Mirror

        private void initRetMirrorMotor_()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.RetMirror, out _retMirrorMotorStatus);

            // Min/Max/Value 설정이 ValueChanged를 올려도 모터는 그대로 둔다. (Filter와 같음)
            _isRetMirrorSliderDragging = true;
            mySliderRetMirror.Minimum = _retMirrorMotorStatus.rangeMin;
            mySliderRetMirror.Maximum = _retMirrorMotorStatus.rangeMax;
            mySliderRetMirror.TickFrequency = 10;
            myTbRetMirrorPos.Text = _retMirrorMotorStatus.currPos.ToString();
            mySliderRetMirror.Value = _retMirrorMotorStatus.currPos;
            _isRetMirrorSliderDragging = false;

            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.RetMirror, _onRetMirrorPositionChanged);
        }

        private void releaseRetMirrorMotor_()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.RetMirror);
        }

        private void OnRetMirrorPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                myTbRetMirrorPos.Text = pos.ToString();

                if (pos != mySliderRetMirror.Value)
                {
                    mySliderRetMirror.Value = pos;
                    _isRetMirrorPositionChanged = true;
                }
            }, DispatcherPriority.Normal);
        }

        #endregion Return Mirror

        #region LSO Filter Motor

        private void initLsoFilterMotor_()
        {
            DeviceMotors.FetchStepMotorStatus(MotorType.LsoFilter, out _lsoFilterMotorStatus);

            myTbLsoFilterPos.Text = _lsoFilterMotorStatus.currPos.ToString();

            DeviceMotors.ConnectStepMotorPositionChanged(MotorType.LsoFilter, _onLsoFilterPositionChanged);

            // 창을 열 때 필터 휠을 움직이지 않는다. (현재 위치 표시만)
        }

        private void releaseLsoFilterMotor_()
        {
            DeviceMotors.ReleaseStepMotorPositionChanged(MotorType.LsoFilter);
        }

        private void OnLsoFilterPositionChanged(int pos, float value)
        {
            Dispatcher.BeginInvoke(() =>
            {
                myTbLsoFilterPos.Text = pos.ToString();
            }, DispatcherPriority.Normal);
        }

        #endregion LSO Filter Motor

        #region Callbacks

        // Color Live 
        private void OnColorCameraFrameCaptured(IntPtr data, int width, int height, int frameCount, int nFlipMode, int nPixelFormat, int nBytesPerPixel)
        {
            // 이전 프레임이 아직 UI에서 처리 중이면 이번 프레임은 버려서 Dispatcher 큐 적체를 방지한다.
            if (_isColorLiveFramePending)
            {
                return;
            }

            int totalBytes = width * height * nBytesPerPixel;

            byte[] frameData = new byte[totalBytes];
            Marshal.Copy(data, frameData, 0, totalBytes);
            rememberAlignFrame_(frameData, width, height, nBytesPerPixel);

            int channel = 1;

            _isColorLiveFramePending = true;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() =>
            {
                try
                {
                    myColorPreview.UpdateFramerate();
                    myColorPreview.CallbackLsoScanFrameImage(frameData, width, height, channel, nFlipMode, nPixelFormat, nBytesPerPixel);
                }
                finally
                {
                    _isColorLiveFramePending = false;
                }
            }, DispatcherPriority.Background);
            return;
        }

        // Flat of the capture in progress (null = not corrected). Set before the one-call starts,
        // read on the native capture thread.
        private volatile LsoFlatField? _captureFlatField;

        // Color Capture
        private void OnColorCameraCaptureFrameCaptured(IntPtr data, int width, int height, int frameCount, int totalFrameCount, int nFlipMode, int nPixelFormat, int nBytesPerPixel)
        {
            int totalBytes = width * height * nBytesPerPixel;
            byte[] frameData = new byte[totalBytes];
            Marshal.Copy(data, frameData, 0, totalBytes);

            // On the raw Bayer copy, so the review, the stitch and every save see the same
            // corrected frame.
            LsoFlatField? flat = _captureFlatField;
            if (flat != null && !flat.Apply(frameData, width, height, nBytesPerPixel))
            {
                System.Console.WriteLine($"[LSO-FLAT] frame {frameCount} left uncorrected: {width}x{height}x{nBytesPerPixel} "
                    + $"does not match the flat {flat.Width}x{flat.Height}");
            }

            Dispatcher.BeginInvoke(() =>
            {
                myColorPreview.UpdateFramerate();
                myColorPreview.CallbackLsoScanCaptureFrameImage(frameData, width, height, frameCount,
                    totalFrameCount, 1, nFlipMode, nPixelFormat, nBytesPerPixel);
            }, DispatcherPriority.Background);
        }

        // Capture one-call completion notice (native worker thread) - marshal before UI state.
        private void OnColorCaptureCompleted(int result, int framesAcquired)
        {
            Dispatcher.BeginInvoke(() =>
            {
                var stage = (ColorCaptureStage)result;
                System.Console.WriteLine($"Color capture completed: {stage}, frames={framesAcquired}");
                if (stage != ColorCaptureStage.Ok && stage != ColorCaptureStage.Cancelled)
                {
                    MessageBox.Show($"Capture failed: {stage} ({framesAcquired} frames) - {LsoCamera.GetLastCaptureError()}");
                }
                refreshLightControls_();
            }, DispatcherPriority.Background);
        }

        // Cornea Cameras

        private void OnCorneaCameraLeftFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            corneaPreview1.MarkFrameCaptured();
            if (_corneaLeftPending) return;

            _corneaLeftPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview1.CallbackCorneaCameraFrame(data, width, height);
                _corneaLeftPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraRightFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            corneaPreview2.MarkFrameCaptured();
            if (_corneaRightPending) return;

            _corneaRightPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview2.CallbackCorneaCameraFrame(data, width, height);
                _corneaRightPending = false;
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraLowerFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            corneaPreview3.MarkFrameCaptured();
            if (_corneaLowerPending) return;

            _corneaLowerPending = true;
            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview3.CallbackCorneaCameraFrame(data, width, height);
                _corneaLowerPending = false;
            }, DispatcherPriority.Background);
        }

        #endregion Callbacks
    }
}
