using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.utils;
using static WsoNativeLib.LsoCamera;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.controls.LsoScanImagePreview;

namespace WsoToolkit
{
    public sealed class LsoScanTestModel
    {
        public const string DefaultConfigIniPath = ".//WsoDeviceCfg.ini";
        public LsoScanProfileModel ScanProfile0 { get; } = new LsoScanProfileModel();
        public LsoScanProfileModel ScanProfile1 { get; } = new LsoScanProfileModel();
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

    public partial class LsoScanTestWindow
    {
        private readonly LsoScanTestModel _scanTestModel = new LsoScanTestModel();

        WsoCallback.ColorCameraImageCaptured _onColorCaptureImageCaptured;

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

        public void initCallbacks_()
        {
            _onColorCaptureImageCaptured = new WsoCallback.ColorCameraImageCaptured(this.OnColorCameraCaptureFrameCaptured);
        }

        //Callbacks///////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    }
}
