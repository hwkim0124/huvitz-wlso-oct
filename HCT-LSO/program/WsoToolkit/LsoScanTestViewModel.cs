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
        public const string DefaultConfigIniFileName = "WsoDeviceCfg.ini";
        public static string DefaultConfigIniPath =>
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, DefaultConfigIniFileName));

        // One profile per native scanner slot (LSO_SCANNER_PATTERN_NUM_MAX), not just
        // COLOR/IR. Slots without an INI section load as zeros until they are read back
        // from the board or filled in.
        private readonly LsoScanProfileModel[] _scanProfiles =
            Enumerable.Range(0, LSO_SCANNER_PATTERN_NUM_MAX)
                      .Select(_ => new LsoScanProfileModel())
                      .ToArray();

        public LsoColorCameraModel ColorCamera { get; } = new LsoColorCameraModel();
        public LsoImageAdjustModel ImageAdjust { get; } = new LsoImageAdjustModel();

        public int ScanProfileCount => _scanProfiles.Length;

        public LsoScanProfileModel GetScanProfile(int patternId)
        {
            if (patternId < 0 || patternId >= _scanProfiles.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(patternId), patternId,
                    $"LSO scanner pattern id must be 0..{_scanProfiles.Length - 1}.");
            }

            return _scanProfiles[patternId];
        }

        public bool LoadConfigFromIniFile(string? configIniPath = null)
        {
            try
            {
                var ini = new IniFileUtil(configIniPath ?? DefaultConfigIniPath);
                for (int i = 0; i < _scanProfiles.Length; i++)
                {
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
    public sealed class LsoScanProfileModel
    {
        // The native struct carries these as float, but the board and the INI both work in
        // whole microseconds - keeping them int stops a fractional value from surviving in
        // memory only to be truncated on the next INI round-trip.
        public int ExposureTimeUs { get; set; }

        public ushort TriggerSource { get; set; }

        public ushort AcquisitionMode { get; set; }

        public uint AcqFrameSize { get; set; }

        public ushort SubFrameSize { get; set; }

        public ushort LedOnPosIndex { get; set; }

        public ushort LedOffPosIndex { get; set; }

        public short GalvanoRewindOffset { get; set; }

        public ushort GalvoPatternSize { get; set; }

        public int TimeStepUs { get; set; }

        public short GalvoStartPos { get; set; }

        public short GalvoEndPos { get; set; }

        public static string GetIniSectionName(int patternId)
        {
            return $"LSO_SCANNER_{patternId}";
        }

        internal void LoadFromIni(IniFileUtil ini, int patternId)
        {
            string section = GetIniSectionName(patternId);

            ExposureTimeUs = ini.ReadInt(section, "ExposureTime");
            TriggerSource = (ushort)ini.ReadInt(section, "TrgSrc");
            AcquisitionMode = (ushort)ini.ReadInt(section, "AcqMode");
            AcqFrameSize = (uint)ini.ReadInt(section, "AcqFrameSize");
            SubFrameSize = (ushort)ini.ReadInt(section, "SubFrameSize");
            LedOnPosIndex = (ushort)ini.ReadInt(section, "LedOnPosIndex");
            LedOffPosIndex = (ushort)ini.ReadInt(section, "LedOffPosIndex");
            GalvanoRewindOffset = (short)ini.ReadInt(section, "GalvanoRewindOffset");
            GalvoPatternSize = (ushort)ini.ReadInt(section, "GalvoPatternSize");
            TimeStepUs = ini.ReadInt(section, "TimeStep");
            GalvoStartPos = (short)ini.ReadInt(section, "GalvoStartPos");
            GalvoEndPos = (short)ini.ReadInt(section, "GalvoEndPos");
        }

        internal void SaveToIni(IniFileUtil ini, int patternId)
        {
            string section = GetIniSectionName(patternId);

            ini.WriteInt(section, "ExposureTime", ExposureTimeUs);
            ini.WriteInt(section, "TrgSrc", TriggerSource);
            ini.WriteInt(section, "AcqMode", AcquisitionMode);
            ini.WriteInt(section, "AcqFrameSize", (int)AcqFrameSize);
            ini.WriteInt(section, "SubFrameSize", SubFrameSize);
            ini.WriteInt(section, "LedOnPosIndex", LedOnPosIndex);
            ini.WriteInt(section, "LedOffPosIndex", LedOffPosIndex);
            ini.WriteInt(section, "GalvanoRewindOffset", GalvanoRewindOffset);
            ini.WriteInt(section, "GalvoPatternSize", GalvoPatternSize);
            ini.WriteInt(section, "TimeStep", TimeStepUs);
            ini.WriteInt(section, "GalvoStartPos", GalvoStartPos);
            ini.WriteInt(section, "GalvoEndPos", GalvoEndPos);
        }

        // Note: the galvano position table is not built here. Only start/end/sampleSize
        // cross the ABI; the native LsoScanner::generateGalvanoPositions() interpolates the
        // actual pos[] before it is written to the board.
        public LsoScannerControlParam ToControlParam()
        {
            return new LsoScannerControlParam
            {
                timeStepUs = TimeStepUs,
                exposureTimeUs = ExposureTimeUs,
                triggerSource = TriggerSource,
                acquisitionMode = AcquisitionMode,
                acqFrameSize = (int)AcqFrameSize,
                sampleSize = GalvoPatternSize,
                subFrameSize = SubFrameSize,
                prescanPosRewindOffset = GalvanoRewindOffset,
                ledOnPosIndex = LedOnPosIndex,
                ledOffPosIndex = LedOffPosIndex,
                yGalvoStartPos = GalvoStartPos,
                yGalvoEndPos = GalvoEndPos,
            };
        }

        public void ApplyControlParam(LsoScannerControlParam param)
        {
            TimeStepUs = (int)Math.Round(param.timeStepUs);
            ExposureTimeUs = (int)Math.Round(param.exposureTimeUs);
            TriggerSource = param.triggerSource;
            AcquisitionMode = param.acquisitionMode;
            AcqFrameSize = (uint)param.acqFrameSize;
            GalvoPatternSize = param.sampleSize;
            SubFrameSize = param.subFrameSize;
            GalvanoRewindOffset = param.prescanPosRewindOffset;
            LedOnPosIndex = param.ledOnPosIndex;
            LedOffPosIndex = param.ledOffPosIndex;
            GalvoStartPos = param.yGalvoStartPos;
            GalvoEndPos = param.yGalvoEndPos;
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

        // Mono aiming-preview drive mode. (int)ColorPreviewMonoMode - 0=SensorMono8, 1=BayerSwGray.
        // Not a camera HW parameter, so excluded from ToSettingParam/ApplySettingParam.
        public int PreviewMonoMode { get; set; }

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
            PreviewMonoMode = ini.ReadInt(IniSectionName, "PreviewMonoMode", (int)ColorPreviewMonoMode.SensorMono8);

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
            ini.WriteInt(IniSectionName, "PreviewMonoMode", PreviewMonoMode);

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

        StageMotorWindow? _stageMotorWindow = null;
        LightControlWindow? _lightControlWindow = null;

        // LSO Focus Motor
        private StepMotorStatus _lsoFocusMotorStatus = new();
        private StepMotorPositionChanged _onLsoFocusPositionChanged;
        private bool _isLsoFocusSliderDragging; // 콜백에서 슬라이더 값을 갱신할 때 발생하는 ValueChanged가 모터를 다시 이동시키는 것을 막는 플래그.
        private bool _isLsoFocusPositionChanged;

        private volatile bool _corneaLeftPending = false;
        private volatile bool _corneaRightPending = false;
        private volatile bool _corneaLowerPending = false;


        private void initSetting_()
        {
            // Load INI
            _scanTestModel.LoadConfigFromIniFile();

            // Color Camera
            var paramColorCamera = _scanTestModel.ColorCamera.ToSettingParam();
            LsoCamera.SetCameraParameters(ref paramColorCamera);

            // Image Adjust preset (delegated to the inner LsoScanImagePreview)
            myColorPreview.Image.SetDefaultAdjustPreset(_scanTestModel.ImageAdjust.ToParam());
            myColorPreview.Image.AdjustPresetChanged = OnImageAdjustPresetChanged_;

            // Scan Setting
            initScannerControls_();

            // Preview Mono Mode
            initPreviewMonoModeControls_();
        }

        private void OnImageAdjustPresetChanged_(ImageAdjustParam param)
        {
            _scanTestModel.ImageAdjust.ApplyFrom(param);
            _scanTestModel.ImageAdjust.SavePresetToIniFile();
        }

        #region Captrue - Review
        // Capture - Review
        private void readyToCapture_(PreviewDisplayMode mode)
        {
            switch (mode)
            {
                case PreviewDisplayMode.REVIEW:
                    {
                        myColorPreview.ClearReviewImages();
                        myColorPreview.IsReviewMode = true;
                        myColorPreview.IsReviewSliceMode = false;
                        myColorPreview.IsReviewROIMode = false;
                        myColorPreview.AcqFrameCount = 0;
                        myColorPreview.SubFrameCount = 0;

                    }
                    break;
                case PreviewDisplayMode.REVIEW_SLICE:
                    {
                        myColorPreview.ClearReviewImages();
                        myColorPreview.IsReviewMode = false;
                        myColorPreview.IsReviewSliceMode = true;
                        myColorPreview.IsReviewROIMode = false;
                        myColorPreview.AcqFrameCount = ToInt(myTbAcqFrame.Text);
                        myColorPreview.SubFrameCount = ToInt(myTbSubFrame.Text);
                    }
                    break;
                case PreviewDisplayMode.REVIEW_ROI:
                    {
                        myColorPreview.ClearReviewImages();
                        myColorPreview.IsReviewMode = false;
                        myColorPreview.IsReviewSliceMode = false;
                        myColorPreview.IsReviewROIMode = true;
                        myColorPreview.AcqFrameCount = ToInt(myTbAcqFrame.Text);
                        myColorPreview.SubFrameCount = ToInt(myTbSubFrame.Text);
                        myColorPreview.Image.FrameROIs = _scanTestModel.ColorCamera.GetCaptureFrameRoisArray();
                    }
                    break;

            }
        }

        public void StartColorCameraOriginal()
        {
            // The capture one-call owns the device sequence (IR off -> checked preset apply
            // -> scanner COLOR grab -> worker acquisition -> scanner pause -> completion).
            // The preset is the knob/INI model value, same source as before.
            var captureParam = _scanTestModel.ColorCamera.ToSettingParam();
            var stage = LsoCamera.StartColorCapture(ref captureParam, _onColorCaptureImageCaptured, _onColorCaptureCompleted);
            if (stage != ColorCaptureStage.Ok)
            {
                MessageBox.Show($"Capture start failed: {stage} - {LsoCamera.GetLastCaptureError()}");
            }
        }

        public void StartColorCameraHWTriggerLive()
        {
            // Ensure the IR aiming light is off during capture. (White LED is scan-gated / manual.)
            DeviceLights.TurnOffLight(LightType.RetinaIr);

            // Apply known-good color(Bayer) settings explicitly, same as the Original path.
            // StopPreview's restore can fail silently inside native's single try/catch (void ABI),
            // so relying on it could leave a Mono8/half-FOV state at capture.
            var captureParam = _scanTestModel.ColorCamera.ToSettingParam();
            LsoCamera.SetCameraParameters(ref captureParam);

            //myColorPreview.SetLiveMode();
            LsoScanner.StartLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            LsoCamera.StartColorCameraHwTriggerLive(_onColorCaptureImageCaptured);
        }

        public void StopColorCameraHWTriggerLive()
        {
            LsoScanner.PauseLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            LsoCamera.StopColorCameraHwTriggerLive();
        }

        public void StartColorCameraLive()
        {
            _isColorLiveFramePending = false;

            // Normal color live always renders in color (clear any leftover aiming gray flag).
            myColorPreview.IsForceGrayLiveDisplay = false;

            myColorPreview.SetLiveMode();

            LsoScanner.StartLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            LsoCamera.StartLiveMode(_onColorLiveFrameCaptured);
            myColorPreview.StartTimer();
        }

        public void PauseColorCameraLive()
        {
            LsoScanner.PauseLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
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
        }

        public void CloseCorneaCameraPreview()
        {
            CorneaCamera.ClosePreview();
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
            LsoScanner.StartLsoScannerGrabbing((int)LsoScannerPatternId.IR);
            myColorPreview.LiveCallback = _onColorLiveFrameCaptured;
            myColorPreview.PlayPreview(previewMonoMode_());
        }

        // Stop the aiming live. The native one-call (via StopPreview) pauses the IR scan pattern,
        // turns off the IR LED, and restores camera params.
        private void StopRetinaPreview_()
        {
            LsoScanner.PauseLsoScannerGrabbing((int)LsoScannerPatternId.IR);
            myColorPreview.StopPreview();
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

        #region Preview Mono Mode

        private bool _isUpdatingPreviewMonoMode;

        private void initPreviewMonoModeControls_()
        {
            _isUpdatingPreviewMonoMode = true;

            myCbPreviewMonoMode.Items.Clear();
            myCbPreviewMonoMode.Items.Add("Sensor Mono8");    // index 0 = SensorMono8
            myCbPreviewMonoMode.Items.Add("Bayer + SW Gray"); // index 1 = BayerSwGray
            myCbPreviewMonoMode.SelectedIndex =
                _scanTestModel.ColorCamera.PreviewMonoMode == (int)ColorPreviewMonoMode.BayerSwGray ? 1 : 0;

            _isUpdatingPreviewMonoMode = false;
        }

        private ColorPreviewMonoMode previewMonoMode_()
        {
            return myCbPreviewMonoMode.SelectedIndex == 1
                ? ColorPreviewMonoMode.BayerSwGray
                : ColorPreviewMonoMode.SensorMono8;
        }

        private void commitPreviewMonoMode_()
        {
            if (_isUpdatingPreviewMonoMode)
            {
                return;
            }

            _scanTestModel.ColorCamera.PreviewMonoMode = (int)previewMonoMode_();
            _scanTestModel.SaveConfigToIniFile();

            // If the aiming preview is running, restart it in the new mode (else apply on next start).
            if (IsRetinaPreviewing_())
            {
                StopRetinaPreview_();
                StartRetinaPreview_();
            }
        }

        #endregion Preview Mono Mode

        #region Scan Setting

        private bool _isUpdatingScannerControls;

        private void initScannerControls_()
        {
            _isUpdatingScannerControls = true;

            // All native scanner slots are selectable, not just COLOR/IR. Ids covered by
            // LsoScannerPatternId show their enum name; the remaining slots are marked
            // reserved so it is clear they carry no agreed meaning yet.
            myCbPatternID.Items.Clear();
            for (int i = 0; i < _scanTestModel.ScanProfileCount; i++)
            {
                myCbPatternID.Items.Add(scannerPatternName_(i));
            }

            myCbTriggerSrc.Items.Clear();
            myCbTriggerSrc.Items.Add("Color Camera");
            myCbTriggerSrc.Items.Add("IR Camera");

            myCbPatternID.SelectedIndex = 0;

            _isUpdatingScannerControls = false;

            updateScannerControls_();
        }

        private static string scannerPatternName_(int patternId)
        {
            return Enum.IsDefined((LsoScannerPatternId)patternId)
                ? $"{patternId}: {(LsoScannerPatternId)patternId}"
                : $"{patternId}: (reserved)";
        }

        private void updateScannerControls_()
        {
            int nPatternId = myCbPatternID.SelectedIndex;
            if (nPatternId < 0)
            {
                return;
            }

            LsoScanProfileModel profile = _scanTestModel.GetScanProfile(nPatternId);

            _isUpdatingScannerControls = true;

            myCbTriggerSrc.SelectedIndex = profile.TriggerSource;
            myCheckBoxFixedFrame.IsChecked = profile.AcquisitionMode != 0;
            myTbCameraTimeStep.Text = profile.TimeStepUs.ToString();
            myTbCameraExposureTime.Text = profile.ExposureTimeUs.ToString();
            myTbAcqFrame.Text = profile.AcqFrameSize.ToString();
            myTbSubFrame.Text = profile.SubFrameSize.ToString();
            myTbGalvoPatternSize.Text = profile.GalvoPatternSize.ToString();
            myTbLightIndexOn.Text = profile.LedOnPosIndex.ToString();
            myTbLightIndexOff.Text = profile.LedOffPosIndex.ToString();
            myTbGalvoStartPos.Text = profile.GalvoStartPos.ToString();
            myTbGalvoEndPos.Text = profile.GalvoEndPos.ToString();
            myTbGalvoRewindOffset.Text = profile.GalvanoRewindOffset.ToString();

            _isUpdatingScannerControls = false;
        }

        // Reads back what the board is actually holding for this pattern and shows it.
        // Model/UI only - press Apply afterwards to also persist it to the INI.
        private bool loadScannerControlParam_(int nPatternId)
        {
            LsoScannerControlParam param = default;
            if (!LsoScanner.ObtainLsoScannerControlParam(nPatternId, ref param))
            {
                MessageBox.Show($"Load scanner parameters from board failed! (patternId={nPatternId})");
                return false;
            }

            _scanTestModel.GetScanProfile(nPatternId).ApplyControlParam(param);
            updateScannerControls_();
            return true;
        }

        private bool applyScannerControlParam_(int nPatternId)
        {
            LsoScanProfileModel profile = _scanTestModel.GetScanProfile(nPatternId);

            // A zero pattern size is not a scan - the native side rejects it in
            // generateGalvanoPositions() but still stores the (zeroed) trajectory. Refuse
            // here instead, so selecting an empty reserved slot cannot wipe it on the board.
            if (profile.GalvoPatternSize == 0)
            {
                return false;
            }

            // Color 패턴이라면 Color Camera에도 Exposure time을 동기화해야 한다.
            if (nPatternId == (int)LsoScannerPatternId.COLOR && myCheckBoxUseSyncExposureTime.IsChecked == true)
            {
                LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam tempCameraParam);

                tempCameraParam.exposureTime = (uint)profile.ExposureTimeUs;
                tempCameraParam.acquisitionFrameCount = calcAcquisitionFrameCount_(nPatternId);

                // 1: Single Frame, 2: Multi Frame
                tempCameraParam.acquisitionMode = tempCameraParam.acquisitionFrameCount == 1 ? 1u : 2u;

                LsoCamera.SetCameraParameters(ref tempCameraParam);
                _scanTestModel.ColorCamera.ApplySettingParam(tempCameraParam);
            }

            LsoScannerControlParam param = profile.ToControlParam();
            return LsoScanner.SubmitLsoScannerControlParam(nPatternId, param);
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

        private void commitScanSettingTextBox_(TextBox? textbox)
        {
            if (_isUpdatingScannerControls || textbox == null)
            {
                return;
            }

            int nPatternId = myCbPatternID.SelectedIndex;
            if (nPatternId < 0)
            {
                return;
            }

            LsoScanProfileModel profile = _scanTestModel.GetScanProfile(nPatternId);

            switch (textbox.Name)
            {
                case "myTbCameraTimeStep":
                    profile.TimeStepUs = ToInt(textbox.Text);
                    break;
                case "myTbCameraExposureTime":
                    profile.ExposureTimeUs = ToInt(textbox.Text);
                    break;
                case "myTbAcqFrame":
                    profile.AcqFrameSize = (uint)ToInt(textbox.Text);
                    break;
                case "myTbSubFrame":
                    profile.SubFrameSize = ToUshort(textbox.Text);
                    break;
                case "myTbGalvoPatternSize":
                    profile.GalvoPatternSize = ToUshort(textbox.Text);
                    break;
                case "myTbLightIndexOn":
                    profile.LedOnPosIndex = ToUshort(textbox.Text);
                    break;
                case "myTbLightIndexOff":
                    profile.LedOffPosIndex = ToUshort(textbox.Text);
                    break;
                case "myTbGalvoStartPos":
                    profile.GalvoStartPos = ToShort(textbox.Text);
                    break;
                case "myTbGalvoEndPos":
                    profile.GalvoEndPos = ToShort(textbox.Text);
                    break;
                case "myTbGalvoRewindOffset":
                    profile.GalvanoRewindOffset = ToShort(textbox.Text);
                    break;
            }

            applyScannerControlParam_(nPatternId);
        }

        #endregion Scan Setting

        #region Galvano Move

        private void moveGalvanoPositionY_()
        {
            Mouse.OverrideCursor = Cursors.Wait;
            try
            {
                int pos = ToInt(myTbGalvanoYPos.Text);
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

            DeviceMotors.MoveStepMotorPosition(MotorType.LsoFocus, _lsoFocusMotorStatus.currPos);
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

        // Color Capture
        private void OnColorCameraCaptureFrameCaptured(IntPtr data, int width, int height, int frameCount, int totalFrameCount, int nFlipMode, int nPixelFormat, int nBytesPerPixel)
        {
            int totalBytes = width * height * nBytesPerPixel;

            byte[] frameData = new byte[totalBytes];
            Marshal.Copy(data, frameData, 0, totalBytes);

            int channel = 1;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() =>
            {
                myColorPreview.UpdateFramerate();
                myColorPreview.CallbackLsoScanCaptureFrameImage(frameData, width, height, frameCount, totalFrameCount, channel, nFlipMode, nPixelFormat, nBytesPerPixel);
            }, DispatcherPriority.Background);

            // HWTrig 경로 전용 잔존: 원콜 캡처는 native 워커가 스캐너 정지와 완료 통지를
            // 소유하므로 제외한다. HWTrig 원콜 전환 시 이 블록은 삭제된다.
            if (frameCount == totalFrameCount - 1 && !LsoCamera.IsCapturing())
            {
                LsoScanner.PauseLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            }
            return;
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
