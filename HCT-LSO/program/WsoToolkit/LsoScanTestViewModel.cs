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
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.controls.LsoScanImagePreview;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
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

        public float Gain { get; set; }

        public uint AdcDepthIndex { get; set; }

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
            Gain = ini.ReadFloat(IniSectionName, "Gain");
            AdcDepthIndex = ini.ReadUInt(IniSectionName, "AdcDepth");
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
            ini.WriteFloat(IniSectionName, "Gain", Gain);
            ini.WriteInt(IniSectionName, "AdcDepth", (int)AdcDepthIndex);
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
            Gain = param.gain;
            AdcDepthIndex = param.adcDepthIndex;
        }
    }

    public partial class LsoScanTestWindow
    {
        private readonly LsoScanTestModel _scanTestModel = new LsoScanTestModel();

        WsoCallback.ColorCameraImageCaptured _onColorCaptureImageCaptured;

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
                        //myColorPreview.IsReviewSliceMode = false;
                        //myColorPreview.IsReviewROIMode = false;
                        myColorPreview.AcqFrameCount = 0;
                        myColorPreview.SubFrameCount = 0;

                        //LsoConfig.ImageAdjustPreset param = new();
                        //if (!Configuration.ObtainImageAdjustPreset(out param))
                        //{
                        //    MessageBox.Show("Image Adjust Preset을 가져오지 못함.", "오류", MessageBoxButton.OK, MessageBoxImage.Warning);
                        //    return;
                        //}

                        //myColorPreview.ImageAdjustBrightness = param.Brightness;

                    }
                    break;
                //case PreviewDisplayMode.REVIEW_SLICE:
                //    {
                //        myColorPreview.ClearReviewImages();
                //        myColorPreview.IsReviewMode = false;
                //        myColorPreview.IsReviewSliceMode = true;
                //        myColorPreview.IsReviewROIMode = false;
                //        myColorPreview.AcqFrameCount = ToInt(myTbAcqFrame.Text);
                //        myColorPreview.SubFrameCount = ToInt(myTbSubFrame.Text);

                //        LsoConfig.ImageAdjustPreset param = new();
                //        if (!Configuration.ObtainImageAdjustPreset(out param))
                //        {
                //            MessageBox.Show("Image Adjust Preset을 가져오지 못함.", "오류", MessageBoxButton.OK, MessageBoxImage.Warning);
                //            return;
                //        }

                //        myColorPreview.ImageAdjustBrightness = param.Brightness;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROI:
                //    {
                //        myColorPreview.ClearReviewImages();
                //        myColorPreview.IsReviewMode = false;
                //        myColorPreview.IsReviewSliceMode = false;
                //        myColorPreview.IsReviewROIMode = true;
                //        myColorPreview.AcqFrameCount = ToInt(myTbAcqFrame.Text);
                //        myColorPreview.SubFrameCount = ToInt(myTbSubFrame.Text);


                //        LsoConfig.CaptureFrameROIPreset param = new LsoConfig.CaptureFrameROIPreset();
                //        if (!Configuration.ObtainCaptureFrameROIPreset(out param))
                //        {
                //            return;
                //        }

                //        CaptureFrameROIPresetCS presetCS = StructCaptureFrameROIConverter.ToManaged(in param);
                //        myColorPreview.FrameROIs = presetCS.FrameROIPositionArray;

                //        LsoConfig.ImageAdjustPreset ImageParam = new();
                //        if (!Configuration.ObtainImageAdjustPreset(out ImageParam))
                //        {
                //            MessageBox.Show("Image Adjust Preset을 가져오지 못함.", "오류", MessageBoxButton.OK, MessageBoxImage.Warning);
                //            return;
                //        }

                //        myColorPreview.ImageAdjustBrightness = ImageParam.Brightness;
                //    }
                //    break;
                //case PreviewDisplayMode.LIVE_SEQ_ROI:
                //    {
                //        myColorPreview.ClearReviewImages();
                //        myColorPreview.IsReviewMode = false;
                //        myColorPreview.IsReviewSliceMode = false;
                //        myColorPreview.IsReviewROIMode = false;
                //        myColorPreview.IsLiveSeqROIMode = true;

                //        LsoConfig.CaptureFrameSeqROIPreset param = new LsoConfig.CaptureFrameSeqROIPreset();
                //        if (!Configuration.ObtainCaptureFrameSeqROIPreset(out param))
                //        {
                //            return;
                //        }

                //        CaptureFrameSeqROIPresetCS presetCS = StructCaptureFrameSeqROIConverter.ToManaged(in param);
                //        myColorPreview.FrameSeqROIs = presetCS.FrameSeqROIParamArray;
                //        myColorPreview.SeqTargetImageMaxWidth = (int)_colorCameraParam.roiMaxWidth;
                //        myColorPreview.SeqTargetImageMaxHeight = (int)_colorCameraParam.roiMaxHeight;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_OFFSET_ROI:
                //    {
                //        myColorPreview.ClearReviewImages();
                //        myColorPreview.IsReviewMode = false;
                //        myColorPreview.IsReviewSliceMode = false;
                //        myColorPreview.IsReviewROIMode = false;
                //        myColorPreview.IsLiveSeqROIMode = false;
                //        myColorPreview.IsLiveOffsetROIMode = true;

                //        LsoConfig.CaptureFrameOffsetROIPreset param = new LsoConfig.CaptureFrameOffsetROIPreset();
                //        if (!Configuration.ObtainCaptureFrameOffsetROIPreset(out param))
                //        {
                //            return;
                //        }

                //        CaptureFrameOffsetROIPresetCS presetCS = StructCaptureFrameOffsetROIConverter.ToManaged(in param);
                //        myColorPreview.FrameOffsetROIs = presetCS.FrameOffsetROIParamArray;
                //        myColorPreview.OffsetRoiImageWidth = presetCS.RoiWidth;
                //        myColorPreview.OffsetRoiImageHeight = presetCS.RoiHeight;
                //        myColorPreview.OffsetRoiTargetImageMaxWidth = (int)_colorCameraParam.roiMaxWidth;
                //        myColorPreview.OffsetRoiTargetImageMaxHeight = (int)_colorCameraParam.roiMaxHeight;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_OVERLAP:
                //    {
                //        myColorPreview.ClearReviewImages();
                //        myColorPreview.IsReviewMode = false;
                //        myColorPreview.IsReviewSliceMode = false;
                //        myColorPreview.IsReviewROIMode = false;
                //        myColorPreview.IsLiveSeqROIMode = false;
                //        myColorPreview.IsLiveOffsetROIMode = false;
                //        myColorPreview.IsReviewRollSWTrigOverlapMode = true;

                //        LsoConfig.CaptureFrameRollSWTrigOverlapPreset param = new LsoConfig.CaptureFrameRollSWTrigOverlapPreset();
                //        if (!Configuration.ObtainCaptureFrameRollSWTrigOverlapPreset(out param))
                //        {
                //            return;
                //        }

                //        myColorPreview.ReviewRollSWTrigOverlapBrightness = param.Brightness;
                //    }
                //    break;
                //case PreviewDisplayMode.REVIEW_ROLLING_SW_TRIGGER_MANUAL:
                //    {
                //        myColorPreview.ClearReviewImages();
                //        myColorPreview.IsReviewMode = true;
                //        myColorPreview.IsReviewSliceMode = false;
                //        myColorPreview.IsReviewROIMode = false;
                //        myColorPreview.AcqFrameCount = 0;
                //        myColorPreview.SubFrameCount = 0;

                //        LsoConfig.ImageAdjustPreset ImageParam = new();
                //        if (!Configuration.ObtainImageAdjustPreset(out ImageParam))
                //        {
                //            MessageBox.Show("Image Adjust Preset을 가져오지 못함.", "오류", MessageBoxButton.OK, MessageBoxImage.Warning);
                //            return;
                //        }

                //        myColorPreview.ImageAdjustBrightness = ImageParam.Brightness;
                //    }
                //    break;
            }

            //if (IsRetinaCameraPreviewing())
            //{
            //    //myBtStartScan.Content = "Start Scan";
            //    myBtSWTriggerManualIrLive.Content = "IR Live";
            //    myBtRollShutSWTriggerOverlabIrLive.Content = "IR Live";
            //    myBtHWTriggerIrLive.Content = "IR Live";
            //    CloseRetinaCameraPreview();
            //}
            //if (IsRetinaCameraPreviewing())
            //{
            //    //myBtStartScan.Content = "Start Scan";
            //    myBtSWTriggerManualIrLive.Content = "IR Live";
            //    myBtRollShutSWTriggerOverlabIrLive.Content = "IR Live";
            //    myBtHWTriggerIrLive.Content = "IR Live";
            //    CloseRetinaCameraPreview();
            //}
        }

        public void StartColorCameraOriginal()
        {
            LsoScanner.StartLsoScannerGrabbing((int)LsoScannerPatternId.COLOR);
            LsoCamera.StartOriginalMode(_onColorCaptureImageCaptured);
        }

        private void initCallbacks_()
        {
            _onColorCaptureImageCaptured = new WsoCallback.ColorCameraImageCaptured(this.OnColorCameraCaptureFrameCaptured);
        }

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

        #endregion Captrue - Review

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

            LsoScannerControlParam param = profile.ToControlParam();
            return LsoScanner.SubmitLsoScannerControlParam(nPatternId, param);
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

        #region Callbacks
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
        #endregion Callbacks
    }
}
