using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.utils;
using static WsoNativeLib.LsoCamera;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoDomain;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
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
        public const string DefaultConfigIniPath = ".//WsoDeviceCfg.ini";
        public LsoScanProfileModel ScanProfile0 { get; } = new LsoScanProfileModel();
        public LsoScanProfileModel ScanProfile1 { get; } = new LsoScanProfileModel();
        public LsoColorCameraModel ColorCamera { get; } = new LsoColorCameraModel();
        public LsoScanProfileModel GetScanProfile(int patternId)
        {
            return patternId switch
            {
                0 => ScanProfile0,
                1 => ScanProfile1,
                _ => throw new ArgumentOutOfRangeException(nameof(patternId), patternId, "LSO scanner pattern id must be 0 or 1."),
            };
        }

        public bool LoadConfigFromIniFile(string? configIniPath = null)
        {
            try
            {
                var ini = new IniFileUtil(configIniPath ?? DefaultConfigIniPath);
                ScanProfile0.LoadFromIni(ini, 0);
                ScanProfile1.LoadFromIni(ini, 1);
                ColorCamera.LoadFromIni(ini);
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
                ScanProfile0.SaveToIni(ini, 0);
                ScanProfile1.SaveToIni(ini, 1);
                ColorCamera.SaveToIni(ini);
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }
    }

    public sealed class LsoScanProfileModel
    {
        public float ExposureTimeUs { get; set; }

        public ushort TriggerSource { get; set; }

        public ushort AcquisitionMode { get; set; }

        public uint AcqFrameSize { get; set; }

        public ushort SubFrameSize { get; set; }

        public ushort LedOnPosIndex { get; set; }

        public ushort LedOffPosIndex { get; set; }

        public short GalvanoRewindOffset { get; set; }

        public ushort GalvoPatternSize { get; set; }

        public float TimeStepUs { get; set; }

        public short GalvoStartPos { get; set; }

        public short GalvoEndPos { get; set; }

        public short[] GalvanoPositions { get; private set; } = new short[LSO_SCANNER_SAMPLE_SIZE_MAX];

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

            CalcGalvanoPositions();
        }

        internal void SaveToIni(IniFileUtil ini, int patternId)
        {
            string section = GetIniSectionName(patternId);

            ini.WriteInt(section, "ExposureTime", (int)ExposureTimeUs);
            ini.WriteInt(section, "TrgSrc", TriggerSource);
            ini.WriteInt(section, "AcqMode", AcquisitionMode);
            ini.WriteInt(section, "AcqFrameSize", (int)AcqFrameSize);
            ini.WriteInt(section, "SubFrameSize", SubFrameSize);
            ini.WriteInt(section, "LedOnPosIndex", LedOnPosIndex);
            ini.WriteInt(section, "LedOffPosIndex", LedOffPosIndex);
            ini.WriteInt(section, "GalvanoRewindOffset", GalvanoRewindOffset);
            ini.WriteInt(section, "GalvoPatternSize", GalvoPatternSize);
            ini.WriteInt(section, "TimeStep", (int)TimeStepUs);
            ini.WriteInt(section, "GalvoStartPos", GalvoStartPos);
            ini.WriteInt(section, "GalvoEndPos", GalvoEndPos);
        }

        public void CalcGalvanoPositions()
        {
            if (GalvanoPositions.Length != LSO_SCANNER_SAMPLE_SIZE_MAX)
            {
                GalvanoPositions = new short[LSO_SCANNER_SAMPLE_SIZE_MAX];
            }
            else
            {
                Array.Clear(GalvanoPositions, 0, GalvanoPositions.Length);
            }

            int sampleSize = GalvoPatternSize;
            if (sampleSize <= 0 || sampleSize >= LSO_SCANNER_SAMPLE_SIZE_MAX)
            {
                return;
            }

            double delta = GalvoEndPos - GalvoStartPos;
            double step = sampleSize > 1 ? delta / (sampleSize - 1) : 0.0;

            for (int i = 0; i < sampleSize; i++)
            {
                double value = GalvoStartPos + step * i;
                GalvanoPositions[i] = (short)Math.Round(value);
            }
        }

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
            TimeStepUs = param.timeStepUs;
            ExposureTimeUs = param.exposureTimeUs;
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

            CalcGalvanoPositions();
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

        private void initSetting_()
        {
            // Load INI
            _scanTestModel.LoadConfigFromIniFile();

            // Color Camera
            var paramColorCamera = _scanTestModel.ColorCamera.ToSettingParam();
            LsoCamera.SetCameraParameters(ref paramColorCamera);

            // Scan Setting
            initScannerControls_();
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
                        myColorPreview.FrameROIs = _scanTestModel.ColorCamera.GetCaptureFrameRoisArray();
                    }
                    break;

            }
        }

        public void StartColorCameraOriginal()
        {
            LsoScanner.StartLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            LsoCamera.StartOriginalMode(_onColorCaptureImageCaptured);
        }

        public void StartColorCameraHWTriggerLive()
        {
            //myColorPreview.SetLiveMode();
            LsoCamera.StartColorCameraHwTriggerLive(_onColorCaptureImageCaptured);
        }

        public void StopColorCameraHWTriggerLive()
        {
            LsoCamera.StopColorCameraHwTriggerLive();
        }

        public void StartColorCameraLive()
        {
            _isColorLiveFramePending = false;

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
            if (CorneaCamera.IsPreviewing(CameraType.IrCorneaLeft) || CorneaCamera.IsPreviewing(CameraType.IrCorneaRight))
                return true;

            return false;
        }

        public void StartCorneaCameraPreview()
        {
            CorneaCamera.StartPreview(CameraType.IrCorneaLeft, _onCorneaLeftFrameCaptured);
            CorneaCamera.StartPreview(CameraType.IrCorneaRight, _onCorneaRightFrameCaptured);
            // CorneaCamera.StartPreview(CameraType.IrCorneaLower, _onCorneaLowerFrameCaptured);
        }

        public void CloseCorneaCameraPreview()
        {
            CorneaCamera.ClosePreview(CameraType.IrCorneaLeft);
            CorneaCamera.ClosePreview(CameraType.IrCorneaRight);
            // CorneaCamera.ClosePreview(CameraType.IrCorneaLower);
        }

        #endregion Cornea Camera 

        #region Scan Setting

        private bool _isUpdatingScannerControls;

        private void initScannerControls_()
        {
            _isUpdatingScannerControls = true;

            myCbPatternID.Items.Clear();
            myCbPatternID.Items.Add("Color");
            myCbPatternID.Items.Add("IR");

            myCbTriggerSrc.Items.Clear();
            myCbTriggerSrc.Items.Add("Color Camera");
            myCbTriggerSrc.Items.Add("IR Camera");

            myCbPatternID.SelectedIndex = 0;

            _isUpdatingScannerControls = false;

            updateScannerControls_();
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

        private bool applyScannerControlParam_(int nPatternId)
        {
            LsoScanProfileModel profile = _scanTestModel.GetScanProfile(nPatternId);
            profile.CalcGalvanoPositions();

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
                    profile.TimeStepUs = ToFloat(textbox.Text);
                    break;
                case "myTbCameraExposureTime":
                    profile.ExposureTimeUs = ToFloat(textbox.Text);
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

            if (frameCount == totalFrameCount - 1)
            {
                LsoScanner.PauseLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            }
            return;
        }

        // Cornea Cameras

        private void OnCorneaCameraLeftFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview1.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraRightFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview2.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }

        private void OnCorneaCameraLowerFrameCaptured(nint data, int width, int height)
        {
            if (data == 0) return;

            // Update GUI preview control asynchronously.
            Dispatcher.BeginInvoke(() => {
                corneaPreview3.CallbackCorneaCameraFrame(data, width, height);
            }, DispatcherPriority.Background);
        }
        #endregion Callbacks
    }
}
