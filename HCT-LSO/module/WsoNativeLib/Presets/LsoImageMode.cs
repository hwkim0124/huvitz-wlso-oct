using System;
using static WsoNativeLib.WsoDevice;

namespace WsoNativeLib.Presets
{
    // What a Capture images. The Preview role keeps its single aiming preset; the Capture role
    // has one preset per image mode, selected in WsoDeviceCfg.ini [MODE_PRESET]:
    //
    //   CapturePreset=<Name>             Color - the original key, so existing files keep working
    //   CapturePreset.<Mode>=<Name>      every other mode (CapturePreset.BlueFaf=..., ...)
    //
    // All modes share the Capture scanner slot: the one-call uploads the preset's profile to
    // it on every start, so the slot never has to agree with the mode that used it last.
    public enum LsoImageMode
    {
        Color,
        BlueFaf,
        GreenFaf,
        Fa,
        Icg,
        IR,
    }

    public enum ColorIllumination
    {
        White,
        Rgb,
    }

    // Blue-FAF emission. The DevKit defaults to the green plane alone (highest vessel
    // contrast) and offers the green+red composite (0.6 G + 0.4 R) for weak signal.
    // Green-FAF always takes the red plane; see LsoImageModes.Processing.
    public enum FafProcessingOption
    {
        GreenOnly = 0,
        GreenPlusRed = 1,
    }

    // What is done to a Gray-mode capture after the flat-field: each 2x2 Bayer cell (R G / G B)
    // becomes one pixel, the weighted mean of its planes (a G weight applies to both G pixels).
    // Gray = false is the Color path: the raw Bayer frame is demosaiced as before.
    public sealed record LsoModeProcessing(bool Gray, int WeightR, int WeightG, int WeightB)
    {
        public static readonly LsoModeProcessing Rgb = new(false, 0, 0, 0);
    }

    public static class LsoImageModes
    {
        public static byte FundusMode(LsoImageMode mode, ColorIllumination illumination = ColorIllumination.White)
        {
            return mode switch
            {
                LsoImageMode.Color => illumination == ColorIllumination.Rgb ? (byte)3 : (byte)0,
                LsoImageMode.IR => 1,
                LsoImageMode.Icg => 2,
                LsoImageMode.Fa => 4,
                LsoImageMode.BlueFaf => 5,
                LsoImageMode.GreenFaf => 6,
                _ => throw new ArgumentOutOfRangeException(nameof(mode), mode, null),
            };
        }

        // Every value, including the ones this instrument cannot image yet. Kept for the
        // shipped-name rules (IsDefaultName) and for configuration written by other versions.
        public static readonly LsoImageMode[] All = Enum.GetValues<LsoImageMode>();

        // The modes this instrument can image today, in the order the Toolkit lists them.
        // Icg is left out: the only near-IR lamp is the 850 nm retina IR, which sits inside
        // the ICG emission band (830-900 nm), so the barrier cannot separate the dye.
        // Fa is left out until an FA barrier (520-560 nm) is confirmed: the wheel has no FA
        // position, and the shared BlueFaf barrier may pass the tail of the blue excitation.
        public static readonly LsoImageMode[] Available =
        {
            LsoImageMode.Color,
            LsoImageMode.IR,
            LsoImageMode.BlueFaf,
            LsoImageMode.GreenFaf,
        };

        public static bool IsAvailable(LsoImageMode mode)
        {
            return Array.IndexOf(Available, mode) >= 0;
        }

        // Which Bayer planes carry each mode's signal: FAF-Blue defaults to the green plane
        // (or G+R when the preset selects the composite), FAF-Green above 600 nm (R),
        // fluorescein at 520-560 nm (G); near-IR passes every colour filter, so IR uses all
        // four pixels.
        //
        // A weight applies per plane occurrence in a 2x2 Bayer cell (R G / G B), and the
        // accumulator divides by WeightR + 2 * WeightG + WeightB. The composite therefore
        // states (4,3,0): 4 R + 3 G + 3 G over 10 = 0.4 R + 0.6 G, the DevKit's option.
        public static LsoModeProcessing Processing(LsoImageMode mode,
            FafProcessingOption fafOption = FafProcessingOption.GreenOnly)
        {
            return mode switch
            {
                LsoImageMode.Color => LsoModeProcessing.Rgb,
                LsoImageMode.IR => new LsoModeProcessing(true, 1, 1, 1),
                LsoImageMode.BlueFaf => fafOption == FafProcessingOption.GreenPlusRed
                    ? new LsoModeProcessing(true, 4, 3, 0)
                    : new LsoModeProcessing(true, 0, 1, 0),
                LsoImageMode.GreenFaf => new LsoModeProcessing(true, 1, 0, 0),
                LsoImageMode.Fa => new LsoModeProcessing(true, 0, 1, 0),
                LsoImageMode.Icg => new LsoModeProcessing(true, 1, 1, 1),
                _ => throw new ArgumentOutOfRangeException(nameof(mode), mode, null),
            };
        }

        // Frames a NEW preset of this mode acquires per capture (Scan.AcqFrameSize). The
        // autofluorescence and angiography signals are weak, so they are shot as a burst
        // and averaged; the reflectance modes stay a single frame.
        public static uint SeedFrameCount(LsoImageMode mode)
        {
            return mode switch
            {
                LsoImageMode.BlueFaf or LsoImageMode.GreenFaf => 8,
                LsoImageMode.Fa or LsoImageMode.Icg => 4,
                _ => 1,
            };
        }

        private const string ColorCaptureKey = "CapturePreset";

        public static string CaptureSelectionKey(LsoImageMode mode)
        {
            return mode == LsoImageMode.Color ? ColorCaptureKey : ColorCaptureKey + "." + mode;
        }

        // The shipped fallback preset of each mode; like "Capture" it cannot be deleted.
        public static string DefaultCapturePresetName(LsoImageMode mode)
        {
            return mode == LsoImageMode.Color
                ? LsoModePresetStore.DefaultCapturePresetName
                : LsoModePresetStore.DefaultCapturePresetName + mode;
        }

        // The optical path a NEW preset of this mode starts from: the filter wheel origin and
        // the lamp behind it. Camera, scan and intensity stay the DevKit capture defaults and
        // are tuned in the Toolkit. FA shares the Blue FAF filter and lamp (there is no FA-only
        // wheel origin); ICG images through the ICGA filter with the retina IR lamp.
        // ponytail: IR keeps the Color filter origin - confirm the IR-reflectance path on the bench.
        public static void ApplySeedOptics(LsoImageMode mode, LsoModePreset preset)
        {
            (int filter, LightType lamp) = mode switch
            {
                LsoImageMode.BlueFaf => ((int)LsoFilterMode.BlueFaf, LightType.LsoBlue),
                LsoImageMode.GreenFaf => ((int)LsoFilterMode.GreenFaf, LightType.LsoGreen),
                LsoImageMode.Fa => ((int)LsoFilterMode.BlueFaf, LightType.LsoBlue),
                LsoImageMode.Icg => ((int)LsoFilterMode.Icga, LightType.RetinaIr),
                LsoImageMode.IR => (LsoModeMotor.ColorFilterMode, LightType.RetinaIr),
                _ => (preset.Motor.LsoFilterMode, preset.Light.Led),
            };
            preset.Motor.LsoFilterMode = filter;
            preset.Light.Type = (int)lamp;
        }
    }
}
