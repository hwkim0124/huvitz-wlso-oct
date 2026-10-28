using System;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // Everything one capture of an image mode needs, assembled in one place so the Toolkit's
    // scan-test screen and the Agent prepare it the same way: the mode must be one this
    // instrument can image, the preset must be able to drive a capture, the request comes from
    // the shared factory and the flat-field is loaded for the request's own geometry.
    //
    // Device-free: the camera read-back and the board's lamp intensities are passed in. The
    // caller still owns the device steps, in this order - LsoModeMotorApply.Apply(Motor) off
    // the UI thread, InstallTrajectory(), then LsoCamera.StartFundusCapture(ref Request, ...).
    public sealed class LsoFundusModeCapture
    {
        // A field, not a property: LsoCamera.StartFundusCapture takes it by ref.
        public LsoFundusCaptureRequest Request;

        private LsoFundusModeCapture(LsoImageMode mode, string presetName, LsoModeMotor motor,
            LsoFlatField? flatField, LsoReflectedLightSet? reflectedLight, string reflectedLightNote, LsoModeProcessing processing, uint totalFrames,
            short[]? trajectory, string trajectoryNote, LsoFundusCaptureRequest request)
        {
            Mode = mode;
            PresetName = presetName;
            Motor = motor;
            FlatField = flatField;
            ReflectedLight = reflectedLight;
            ReflectedLightNote = reflectedLightNote;
            Processing = processing;
            TotalFrames = totalFrames;
            Trajectory = trajectory;
            TrajectoryNote = trajectoryNote;
            Request = request;
        }

        // The preset's galvo table, or null for the linear sweep (LsoModeTrajectory).
        public short[]? Trajectory { get; }

        // Non-empty when the table was resampled to the live Pattern Size.
        public string TrajectoryNote { get; }

        // Device step, after the motors and right before the one-call.
        public bool InstallTrajectory(out string failure)
        {
            return LsoModeTrajectory.Install(Request.scanPatternId, Trajectory, out failure);
        }

        public LsoImageMode Mode { get; }

        public string PresetName { get; }

        // A copy: the preset can be edited while the motors move.
        public LsoModeMotor Motor { get; }

        public LsoFlatField? FlatField { get; }

        public LsoReflectedLightSet? ReflectedLight { get; }

        // Non-empty when the preset asks for reflected-light removal but no map fits this
        // capture: the capture runs without it.
        public string ReflectedLightNote { get; }

        public LsoModeProcessing Processing { get; }

        // Frames the callbacks deliver: one per sweep, AcqFrameSize x SubFrameSize.
        public uint TotalFrames { get; }

        // perFrame true delivers every frame through the frame callback - what a burst and the
        // Toolkit review need. False keeps the native composition's single image, which is only
        // right for a one-frame capture.
        public static bool TryPrepare(LsoImageMode mode, LsoModePreset capture, LsoModePreset preview,
            ColorCaptureTriggerMode trigger, LsoColorCameraSettingParam cameraReadBack,
            Func<LightType, ushort> boardIntensity, bool perFrame,
            out LsoFundusModeCapture? prepared, out string failure)
        {
            return TryPrepare(mode, capture, preview, trigger, cameraReadBack, boardIntensity, perFrame,
                LsoConfigLocation.ReflectedLightDirectory(), out prepared, out failure);
        }

        public static bool TryPrepare(LsoImageMode mode, LsoModePreset capture, LsoModePreset preview,
            ColorCaptureTriggerMode trigger, LsoColorCameraSettingParam cameraReadBack,
            Func<LightType, ushort> boardIntensity, bool perFrame, string reflectedLightDirectory,
            out LsoFundusModeCapture? prepared, out string failure)
        {
            if (capture == null) { throw new ArgumentNullException(nameof(capture)); }
            if (preview == null) { throw new ArgumentNullException(nameof(preview)); }
            if (boardIntensity == null) { throw new ArgumentNullException(nameof(boardIntensity)); }

            prepared = null;

            if (!LsoImageModes.IsAvailable(mode))
            {
                failure = $"{mode} cannot be imaged on this instrument (not in LsoImageModes.Available).";
                return false;
            }

            if (!LsoFundusRequestFactory.IsUsable(capture, ColorCameraPresetMode.Capture, out failure))
            {
                return false;
            }

            LsoModeProcessing processing = LsoImageModes.Processing(mode, capture.FafOption);
            if (processing.Gray && capture.Scan.SubFrameSize != 1)
            {
                failure = $"the {mode} preset '{capture.Name}' splits each frame into {capture.Scan.SubFrameSize} "
                    + "subframes; gray processing averages whole frames, so SubFrameSize must be 1.";
                return false;
            }

            if (!perFrame && capture.Scan.AcqFrameSize > 1)
            {
                failure = $"the preset '{capture.Name}' acquires {capture.Scan.AcqFrameSize} frames, and the "
                    + "composed delivery keeps only the first - a burst needs per-frame delivery.";
                return false;
            }

            LsoFundusCaptureRequest request = LsoFundusRequestFactory.BuildCaptureRequest(
                capture, preview, trigger, cameraReadBack,
                boardIntensity(capture.Light.Led), boardIntensity(preview.Light.Led), mode);
            if (perFrame)
            {
                request.subFrameCompose = 0;
            }

            LsoReflectedLightSet? reflectedLight = LoadReflectedLight(mode, capture, request,
                reflectedLightDirectory, out string reflectedLightNote);

            if (!LsoFlatField.TryLoadFor(capture, (int)request.camera.roiXWidth, (int)request.camera.roiYHeight,
                    out LsoFlatField? flat, out failure))
            {
                return false;
            }

            if (!LsoModeTrajectory.TryLoad(capture.Scan, out short[]? trajectory, out failure, out string trajectoryNote))
            {
                return false;
            }

            var motor = new LsoModeMotor();
            motor.CopyValuesFrom(capture.Motor);
            prepared = new LsoFundusModeCapture(mode, capture.Name, motor, flat, reflectedLight, reflectedLightNote, processing,
                capture.Scan.AcqFrameSize * capture.Scan.SubFrameSize, trajectory, trajectoryNote, request);
            failure = string.Empty;
            return true;
        }

        public static LsoReflectedLightConditions ReflectedLightConditions(in LsoFundusCaptureRequest request,
            int flipMode)
        {
            return new LsoReflectedLightConditions
            {
                FundusMode = request.fundusMode,
                ExposureUs = request.camera.exposureTime,
                Gain = request.camera.gain,
                LedIntensity = request.captureLightIntensity,
                Width = request.camera.roiXWidth,
                Height = request.camera.roiYHeight,
                OffsetX = request.camera.roiXOffset,
                OffsetY = request.camera.roiYOffset,
                PixelFormat = request.camera.pixelFormat,
                FlipMode = flipMode,
            };
        }

        // Null when the switch is off or no map fits this capture - another mode or illumination,
        // a non-12-bit format, a missing or unreadable set, or a changed LED or camera setting.
        // skipped then says why, and the capture runs without the removal.
        public static LsoReflectedLightSet? LoadReflectedLight(LsoImageMode mode, LsoModePreset capture,
            in LsoFundusCaptureRequest request, string directory, out string skipped)
        {
            skipped = string.Empty;
            if (!capture.PostProcess.ReflectedLightEnabled) return null;

            string reason;
            if (mode != LsoImageMode.Color || capture.ColorIllumination != ColorIllumination.White)
            {
                reason = "reflected-light removal is calibrated for White capture only";
            }
            else if (request.camera.pixelFormat != (uint)ColorPixelFormat.BayerRG12Packed)
            {
                reason = "reflected-light removal requires BayerRG12Packed";
            }
            else if (LsoReflectedLightSet.TryLoad(directory, out LsoReflectedLightSet? loaded, out reason))
            {
                // Flip is known only in the frame callback, where it must be checked before applying.
                string? mismatch = loaded!.Conditions.FirstMismatch(
                    ReflectedLightConditions(request, loaded.Conditions.FlipMode));
                if (mismatch == null)
                {
                    return loaded;
                }
                reason = mismatch;
            }
            skipped = $"the preset '{capture.Name}': {reason}; reflected-light removal skipped.";
            return null;
        }

        // Null for Color: its frames are demosaiced, not averaged.
        public LsoModeImageAccumulator? CreateAccumulator()
        {
            return Processing.Gray ? new LsoModeImageAccumulator(Processing) : null;
        }
    }
}
