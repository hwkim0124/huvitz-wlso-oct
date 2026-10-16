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

    public static class LsoImageModes
    {
        public static readonly LsoImageMode[] All = Enum.GetValues<LsoImageMode>();

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
