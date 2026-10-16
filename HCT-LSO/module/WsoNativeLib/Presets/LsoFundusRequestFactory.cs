using System;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // Turns a mode preset into the request the fundus one-call takes. Both consumers - the
    // Toolkit's scan-test screen and the Agent's measurement screen - assemble the SAME
    // request, so it is assembled once, here, and neither has its own copy to drift.
    //
    // Device-free on purpose: everything that needs the instrument (the camera read-back, the
    // board's current LED intensity) is passed IN, so this can be checked without hardware.
    public static class LsoFundusRequestFactory
    {
        // The A-1 extension's "this caller does not state this node" value for a number.
        private const double NotStatedDelay = -1.0;

        // Capture: the Capture preset owns the camera, the profile, the frame count AND the
        // lamp it images with; what the sequence hands back afterwards is the PREVIEW preset's
        // lamp at the intensity that preset states - the preview is what runs next.
        //
        // Both presets naming the same lamp means "leave it off after the capture": that is the
        // native restore rule (working light off, restore light applied only when it differs),
        // and the next preview start switches it on again.
        //
        // A preset intensity of 0 is "never tuned", and the native request reads 0 as "leave
        // the board's value". Substituting the board read-back here keeps that meaning while
        // stating the illumination the run will actually use.
        //
        // cameraReadBack supplies only what a preset does not own (roi_max_*, which the device
        // reports and which halves under binning).
        public static LsoFundusCaptureRequest BuildCaptureRequest(
            LsoModePreset capture,
            LsoModePreset preview,
            ColorCaptureTriggerMode triggerMode,
            LsoColorCameraSettingParam cameraReadBack,
            ushort captureLightFallback,
            ushort restoreLightFallback)
        {
            if (capture == null) { throw new ArgumentNullException(nameof(capture)); }
            if (preview == null) { throw new ArgumentNullException(nameof(preview)); }

            var request = LsoFundusCaptureRequest.Create();
            request.triggerMode = (int)triggerMode;
            request.scanPatternId = LsoModePreset.ScanPatternId(ColorCameraPresetMode.Capture);
            request.sequenceMode = (int)capture.Scan.SequenceMode;
            request.preMoveOffset = capture.Scan.PreMoveOffset;
            request.preMoveDelayUs = capture.Scan.PreMoveDelayUs;
            request.captureLight = capture.Light.Type;
            request.captureLightMode = capture.Light.Mode;
            request.restoreLight = preview.Light.Type;
            request.restoreLightMode = preview.Light.Mode;
            request.captureLightIntensity = Intensity(capture.Light, captureLightFallback);
            request.restoreLightIntensity = Intensity(preview.Light, restoreLightFallback);

            // Native composition: one image per capture instead of one per band. The band
            // count is the preset's own subframe count, so the consumer receives exactly the
            // image the preset describes.
            request.subFrameCompose = capture.Scan.SubFrameSize;

            request.profile = capture.Scan.ToControlParam();
            request.camera = capture.Camera.ToSettingParam(cameraReadBack);
            request.cameraExt = capture.Camera.ToExtendedParam();
            return request;
        }

        // Preview: hardware-triggered by contract, continuous by nature. Exposure, gain and
        // trigger delay are the validated top-level fields, so the A-1 extension deliberately
        // leaves the delay unstated - one value, one owner.
        public static LsoFundusPreviewRequest BuildPreviewRequest(
            LsoModePreset preview,
            ColorMonoPreviewMode monoMode,
            LsoColorCameraSettingParam cameraReadBack,
            ushort previewLightFallback)
        {
            if (preview == null) { throw new ArgumentNullException(nameof(preview)); }

            var request = LsoFundusPreviewRequest.Create();
            request.monoMode = (int)monoMode;
            request.scanPatternId = LsoModePreset.ScanPatternId(ColorCameraPresetMode.Preview);
            request.sequenceMode = (int)preview.Scan.SequenceMode;
            request.preMoveOffset = preview.Scan.PreMoveOffset;
            request.preMoveDelayUs = preview.Scan.PreMoveDelayUs;
            request.camera = preview.Camera.ToSettingParam(cameraReadBack);
            request.cameraExt = preview.Camera.ToExtendedParam(includeTriggerDelay: false);
            request.cameraExt.triggerDelayUs = NotStatedDelay;
            request.exposureTimeUs = preview.Camera.ExposureTime;
            request.gain = preview.Camera.Gain;
            request.triggerDelayUs = preview.Camera.TriggerDelayUs > 0.0
                ? (uint)Math.Round(preview.Camera.TriggerDelayUs)
                : 0u;

            request.previewLight = preview.Light.Type;
            request.previewLightMode = preview.Light.Mode;
            request.restoreLight = request.previewLight;
            request.restoreLightMode = request.previewLightMode;
            request.previewLightIntensity = Intensity(preview.Light, previewLightFallback);
            request.restoreLightIntensity = request.previewLightIntensity;

            // An aiming view is not a fixed-frame acquisition, whatever the preset stored.
            LsoScannerControlParam profile = preview.Scan.ToControlParam();
            profile.acquisitionMode = 0;
            request.profile = profile;
            return request;
        }

        // Whether this preset can drive its mode at all. Checked BEFORE anything is armed, so
        // a preset that would produce a black frame is refused with the field named instead of
        // being started and explained afterwards.
        //
        // These are the gates a request cannot express: the native validator sees the assembled
        // request and answers with a reason code, but by then the operator has already pressed
        // the button on a preset the store may have fallen back to.
        public static bool IsUsable(LsoModePreset preset, ColorCameraPresetMode mode, out string reason)
        {
            if (preset == null) { throw new ArgumentNullException(nameof(preset)); }

            if (preset.Scan.GalvoPatternSize == 0)
            {
                reason = $"the {mode} preset '{preset.Name}' has a zero galvo pattern size, "
                    + "so the scanner would not sweep. Load and apply the scan profile in the Toolkit.";
                return false;
            }

            if (preset.Scan.SubFrameSize == 0)
            {
                reason = $"the {mode} preset '{preset.Name}' has a zero subframe count, "
                    + "so no frame would be acquired.";
                return false;
            }

            if (mode == ColorCameraPresetMode.Capture && preset.Scan.AcqFrameSize == 0)
            {
                reason = $"the Capture preset '{preset.Name}' has a zero acquisition frame count.";
                return false;
            }

            if (mode == ColorCameraPresetMode.Preview && preset.Camera.ExposureTime == 0)
            {
                reason = $"the Preview preset '{preset.Name}' has a zero camera exposure, "
                    + "which the preview request rejects.";
                return false;
            }

            if (preset.Camera.RoiXWidth == 0 || preset.Camera.RoiYHeight == 0)
            {
                reason = $"the {mode} preset '{preset.Name}' has a zero-sized camera ROI.";
                return false;
            }

            reason = string.Empty;
            return true;
        }

        // Bytes per pixel the composed image will carry, for sizing the consumer's buffer.
        public static int BytesPerPixel(uint pixelFormat)
        {
            return pixelFormat switch
            {
                (uint)ColorPixelFormat.Mono8 or (uint)ColorPixelFormat.BayerRG8 => 1,
                (uint)ColorPixelFormat.Mono16 or (uint)ColorPixelFormat.BayerRG16 => 2,
                (uint)ColorPixelFormat.RGB8Packed or (uint)ColorPixelFormat.BGR8 => 3,
                _ => 1,
            };
        }

        private static ushort Intensity(LsoModeLight light, ushort boardFallback)
        {
            return light.Intensity > 0 ? light.Intensity : boardFallback;
        }
    }
}
