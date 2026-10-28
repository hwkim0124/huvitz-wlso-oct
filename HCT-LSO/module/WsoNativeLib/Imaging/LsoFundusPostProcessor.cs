using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using OpenCvSharp;
using WsoNativeLib.Analysis;
using WsoNativeLib.Presets;

namespace WsoNativeLib.Imaging
{
    // One run starts from the 12-bit Bayer frame the capture callback delivers (after the capture
    // path's own reflected-light / flat correction) or from an already demosaiced RGB48 image such
    // as the _rgb48.tiff companion. Values are sensor units, 0..MaxValue; nothing is rescaled here.
    public sealed class PostProcessInput
    {
        public int Width { get; }
        public int Height { get; }
        public int MaxValue { get; }
        public string Source { get; }
        public CameraHighDepthData? Bayer { get; }
        public ushort[]? Rgb48 { get; } // interleaved R, G, B per pixel
        public bool IsBayer => Bayer != null;
        // The map this frame would have been corrected with, kept so the review can switch the
        // removal on and off. Null when the frame arrived already corrected or no map fits it.
        public LsoReflectedLightCorrection? ReflectedLight { get; }
        // Why there is no map, when there is none: the review shows it instead of a dead switch.
        public string ReflectedLightNote { get; }

        private PostProcessInput(int width, int height, int maxValue, string source, CameraHighDepthData? bayer,
            ushort[]? rgb48, LsoReflectedLightCorrection? reflectedLight = null, string reflectedLightNote = "")
        {
            Width = width;
            Height = height;
            MaxValue = maxValue;
            Source = source;
            Bayer = bayer;
            Rgb48 = rgb48;
            ReflectedLight = reflectedLight;
            ReflectedLightNote = reflectedLightNote;
        }

        // unpacked: tightly packed little-endian uint16 Bayer RGGB, as the capture callback hands it over.
        // reflectedLight: the uncorrected frame's map, applied by the pipeline when the option is on.
        public static PostProcessInput FromBayer(byte[] unpacked, int width, int height, int bitDepth, string source,
            LsoReflectedLightCorrection? reflectedLight = null, string reflectedLightNote = "")
        {
            var frame = new CameraHighDepthData(unpacked, width, height, bitDepth);
            return new PostProcessInput(width, height, frame.MaxDepthValue, source, frame, null, reflectedLight, reflectedLightNote);
        }

        public static PostProcessInput FromRgb48(ushort[] rgb, int width, int height, int maxValue, string source)
        {
            if (rgb.Length != checked(width * height * 3))
                throw new ArgumentException("RGB48 buffer does not match the dimensions.", nameof(rgb));
            return new PostProcessInput(width, height, maxValue, source, null, rgb);
        }
    }

    // White balance runs right after the debayer, in float, so a channel pushed past the sensor
    // maximum keeps its value until the 8-bit step saturates it.
    public sealed record PostProcessOptions
    {
        public bool WhiteBalance { get; init; } = true;
        public double Red { get; init; } = 1.0;
        public double Green { get; init; } = 1.0;
        public double Blue { get; init; } = 1.0;
        public bool Denoise { get; init; } = true;
        public int MedianKernel { get; init; } = 3;
        // A uniform gain on R, G and B is exposure (brightness), not a colour change - WB 2/2/2 is
        // the same thing. Brightness is the gain asked for; with AutoLimit it is cut back to the
        // point where the brightest R, G or B inside a centred circle (radius = RoiRatio x width)
        // reaches Threshold, so the ROI never saturates. The whole image gets that one gain; the
        // corners outside the circle may clip at the 8-bit step.
        public double Brightness { get; init; } = 5.0;
        public bool AutoLimit { get; init; } = false;
        public double RoiRatio { get; init; } = 0.5;
        // <= 1: a fraction of MaxValue (1.0 = 4095 for 12-bit); > 1: a sensor value, clamped to MaxValue.
        public double Threshold { get; init; } = 1.0;
        public double MaxGain { get; init; } = 10.0;
        // Soft knee rolls the top ~22% off instead of clipping (the Image Analysis curve). Off = linear.
        public bool SoftKnee { get; init; } = false;
        // Display encoding at the 8-bit step: v -> MaxValue x (v / MaxValue)^(1 / Gamma) before the
        // x255/MaxValue. Full scale stays full scale, so a disc held at 90% is still unclipped while
        // the mid-tones rise (10% -> 35% at 2.2). 1.0 = linear (off); 2.2 is close to the sRGB curve
        // a monitor decodes. Same curve as LsoImageAnalysis.ApplyBoost mode 2 (gamma 2.0) but vectorised.
        public double Gamma { get; init; } = 1.0;
        // Where the gamma acts. false: on each of R, G, B (the display OETF; colour as the display
        // decodes it). true: on V = max(R,G,B) only, and R, G, B are scaled by V'/V (Zhou 2018, IEEE
        // TBME 65:521): the channel ratios stay, so no channel can clip and today's saturation is kept.
        public bool GammaOnValue { get; init; } = false;
        // Replaces the fixed Brightness gain with the gain that brings the circle median luminance
        // to LuminanceTarget (an 8-bit display value). Gain stays in 1..MaxGain. Off = the fixed gain.
        public bool MatchLuminance { get; init; } = false;
        public double LuminanceTarget { get; init; } = 110.0;
        // Replaces the fixed gain and Match circle median. The optic disc is the brightest compact
        // region of radius DiscRadiusRatio x width, found before flatten so flatten cannot hide it.
        // Gain brings that disc's p99 (max of R,G,B, top 1% ignored) to DiscHeadroom of full scale
        // and no higher. A specular point smaller than the disc does not decide the gain.
        public bool MatchDisc { get; init; } = false;
        public double DiscHeadroom { get; init; } = 0.90;
        public double DiscRadiusRatio { get; init; } = 0.035;
        // Keeps whatever global gain was chosen (fixed, Match circle median, AutoLimit) everywhere,
        // and inside the optic disc neighbourhood compresses only what rises above the local
        // retina level with a linear knee on max(R,G,B), so the disc p99 lands at DiscHeadroom.
        // Nothing can fall below the retina level, so the peripapillary retina keeps the full
        // gain and no dark ring appears; hue is kept because all three channels get the same
        // factor. The knee fades out between 1.5 x and DiscBlendRatio x the disc radius.
        public bool DiscLocal { get; init; } = false;
        public double DiscBlendRatio { get; init; } = 2.0;
        // Flatten illumination: the luminance is divided by its own low-pass estimate inside the ROI
        // circle (sigma = FlattenSigmaRatio x width), so a lamp that did not enter the eye evenly is
        // evened out per capture. The same gain goes to R, G and B (colour kept), clamped to
        // 1/FlattenMaxGain..FlattenMaxGain, scaled by FlattenStrength; outside the circle untouched.
        // Circle ROI before the debayer: everything outside the centred circle (radius = RoiRatio x
        // width) is zeroed and the image is cut to the circle's box - the product's illuminated
        // field is not a clean circle, so what lies outside is never processed.
        public bool CircleMask { get; init; } = true;
        // Reflected-light removal, applied to the uncorrected frame before the circle cut.
        // Only a frame that carries a map (a live capture) can be corrected; a TIFF cannot.
        public bool ReflectedLight { get; init; } = true;
        public bool Flatten { get; init; } = true;
        public double FlattenSigmaRatio { get; init; } = 0.045;
        public double FlattenStrength { get; init; } = 1.0;
        public double FlattenMaxGain { get; init; } = 4.0;
    }

    public sealed class PostProcessStage : IDisposable
    {
        public string Name { get; }
        public string Note { get; }
        public bool Applied { get; }
        // 8-bit BGR (or Gray8 for the Bayer input) of the working image after this stage, at the
        // final stage's scale - what the screen shows and what Save writes.
        public Mat Display { get; }
        // Largest value of the working image after this stage, in sensor units.
        public double MaxValue { get; }
        public double ElapsedMs { get; }

        internal PostProcessStage(string name, string note, bool applied, Mat display, double maxValue, double elapsedMs)
        {
            Name = name;
            Note = note;
            Applied = applied;
            Display = display;
            MaxValue = maxValue;
            ElapsedMs = elapsedMs;
        }

        public void Dispose() => Display.Dispose();
    }

    public sealed class PostProcessResult : IDisposable
    {
        public string Source { get; }
        public IReadOnlyList<PostProcessStage> Stages { get; }
        public PostProcessStage Final => Stages[Stages.Count - 1];

        internal PostProcessResult(string source, List<PostProcessStage> stages)
        {
            Source = source;
            Stages = stages.AsReadOnly();
        }

        public void Dispose()
        {
            foreach (PostProcessStage stage in Stages) stage.Dispose();
        }
    }

    // Draft pipeline: debayer -> float white balance -> median -> 8-bit BGR. Device-free; the
    // caller owns threading and passes the token. Every step leaves a stage record, skipped or not.
    public static class LsoFundusPostProcessor
    {
        public static PostProcessResult Run(PostProcessInput input, PostProcessOptions options, CancellationToken cancel = default)
        {
            if (input == null) throw new ArgumentNullException(nameof(input));
            if (options == null) throw new ArgumentNullException(nameof(options));
            var stages = new List<PostProcessStage>();
            var clock = Stopwatch.StartNew();
            double scale = 255.0 / input.MaxValue;
            Mat? work = null;
            try
            {
                cancel.ThrowIfCancellationRequested();
                if (input.Bayer != null)
                {
                    // Input is the frame as received (reflected light still in it), so the next row
                    // can be compared with it in one run.
                    stages.Add(new PostProcessStage($"Input (Bayer RGGB {input.Bayer.BitDepth}-bit)", input.Source, true,
                        GrayDisplay(input.Bayer), input.Bayer.Raw16.Max(), Lap(clock)));
                    cancel.ThrowIfCancellationRequested();
                    CameraHighDepthData bayer = input.Bayer;
                    if (input.ReflectedLight != null && options.ReflectedLight)
                    {
                        bayer = SubtractReflectedLight(bayer, input.ReflectedLight, out string reflectedNote);
                        stages.Add(new PostProcessStage("Reflected light (removed)", reflectedNote, true,
                            GrayDisplay(bayer), bayer.Raw16.Max(), Lap(clock)));
                    }
                    else
                    {
                        string reflectedSkip = input.ReflectedLight != null ? "OFF"
                            : input.ReflectedLightNote.Length > 0 ? "no map: " + input.ReflectedLightNote
                            : "no map for this input";
                        stages.Add(new PostProcessStage("Reflected light", reflectedSkip, false,
                            GrayDisplay(bayer), bayer.Raw16.Max(), Lap(clock)));
                    }
                    cancel.ThrowIfCancellationRequested();
                    if (options.CircleMask)
                    {
                        bayer = CircleCrop(bayer, options.RoiRatio, out string note);
                        stages.Add(new PostProcessStage($"Circle ROI ({options.RoiRatio:0.00} x width)", note, true,
                            GrayDisplay(bayer), bayer.Raw16.Max(), Lap(clock)));
                    }
                    else
                    {
                        stages.Add(new PostProcessStage("Circle ROI", "OFF", false, GrayDisplay(bayer), bayer.Raw16.Max(), Lap(clock)));
                    }
                    cancel.ThrowIfCancellationRequested();
                    work = Rgb48Mat(bayer.DebayerRgb48(), bayer.Width, bayer.Height);
                    stages.Add(Stage("Debayer (RGGB -> RGB48)", "CameraHighDepthData.DebayerRgb48", true, work, scale, clock));
                }
                else
                {
                    work = Rgb48Mat(input.Rgb48!, input.Width, input.Height);
                    stages.Add(Stage("Input (RGB48)", input.Source, true, work, scale, clock));
                    stages.Add(Stage("Reflected light", "skipped - the map applies to the Bayer frame, not to RGB", false, work, scale, clock));
                    if (options.CircleMask)
                    {
                        Mat cut = CircleCrop(work, options.RoiRatio, out string note);
                        work.Dispose();
                        work = cut;
                        stages.Add(Stage($"Circle ROI ({options.RoiRatio:0.00} x width)", note, true, work, scale, clock));
                    }
                    else
                    {
                        stages.Add(Stage("Circle ROI", "OFF", false, work, scale, clock));
                    }
                    stages.Add(Stage("Debayer", "skipped - input is already RGB", false, work, scale, clock));
                }

                cancel.ThrowIfCancellationRequested();
                var work32 = new Mat();
                work.ConvertTo(work32, MatType.CV_32FC3);
                work.Dispose();
                work = work32;
                if (options.WhiteBalance)
                {
                    Cv2.Split(work, out Mat[] planes);
                    double[] gains = { options.Red, options.Green, options.Blue };
                    for (int i = 0; i < 3; ++i) planes[i].ConvertTo(planes[i], MatType.CV_32FC1, gains[i]);
                    Cv2.Merge(planes, work);
                    foreach (Mat plane in planes) plane.Dispose();
                    stages.Add(Stage($"White balance (R {options.Red:0.00} / G {options.Green:0.00} / B {options.Blue:0.00})",
                        $"float RGB, values above {input.MaxValue} kept until the 8-bit step", true, work, scale, clock));
                }
                else
                {
                    stages.Add(Stage("White balance", "OFF (gains 1 / 1 / 1)", false, work, scale, clock));
                }

                cancel.ThrowIfCancellationRequested();
                if (options.Denoise)
                {
                    var blurred = new Mat();
                    Cv2.MedianBlur(work, blurred, options.MedianKernel);
                    work.Dispose();
                    work = blurred;
                    stages.Add(Stage($"Denoise (median {options.MedianKernel}x{options.MedianKernel})", string.Empty, true, work, scale, clock));
                }
                else
                {
                    stages.Add(Stage("Denoise", "OFF", false, work, scale, clock));
                }

                cancel.ThrowIfCancellationRequested();
                DiscLock discLock = default;
                if (options.MatchDisc || options.DiscLocal)
                {
                    double search = options.RoiRatio * work.Cols;
                    double discRadius = Math.Max(2.0, options.DiscRadiusRatio * work.Cols);
                    discLock = LocateDisc(work, search, discRadius);
                }
                if (options.Flatten)
                {
                    double sigma = options.FlattenSigmaRatio * work.Cols;
                    double roiRadius = options.RoiRatio * work.Cols;
                    FlattenResult f = FlattenIllumination(work, roiRadius, sigma, options.FlattenStrength, options.FlattenMaxGain);
                    stages.Add(Stage($"Flatten illumination (sigma {sigma:0} px, strength {options.FlattenStrength:0.00})",
                        $"ROI circle r={roiRadius:0} px: luminance mean {f.Mean:0}, gain {f.MinGain:0.00}..{f.MaxGain:0.00} (cap {options.FlattenMaxGain:0.#}), same gain on R/G/B, outside the ROI untouched",
                        true, work, scale, clock));
                }
                else
                {
                    stages.Add(Stage("Flatten illumination", "OFF", false, work, scale, clock));
                }

                cancel.ThrowIfCancellationRequested();
                double requested = Math.Max(1.0, options.Brightness);
                double gain = Math.Min(requested, options.MaxGain);
                string why = $"requested x{requested:0.00}";
                if (options.MatchDisc)
                {
                    double headroom = options.DiscHeadroom > 1.0
                        ? Math.Min(options.DiscHeadroom, input.MaxValue)
                        : Math.Max(0.0, options.DiscHeadroom) * input.MaxValue;
                    DiscState state = MeasureDisc(work, discLock, input.MaxValue);
                    double match = state.P99 > 0.0 ? headroom / state.P99 : options.MaxGain;
                    gain = state.P99 >= headroom ? 1.0 : Math.Min(options.MaxGain, Math.Max(1.0, match));
                    why = state.P99 >= headroom
                        ? $"disc ({state.Cx:0.0},{state.Cy:0.0}) r={state.Radius:0} px, p99 {state.P99:0}, saturated {state.SaturatedFraction * 100:0.0}%, headroom {headroom:0}, already at the headroom, not raised"
                        : $"disc ({state.Cx:0.0},{state.Cy:0.0}) r={state.Radius:0} px, p99 {state.P99:0}, saturated {state.SaturatedFraction * 100:0.0}%, headroom {headroom:0}, requested x{match:0.00}, applied x{gain:0.00}";
                    requested = gain;
                }
                else if (options.MatchLuminance)
                {
                    double radius = options.RoiRatio * work.Cols;
                    double median = CircleLuminanceMedian(work, work.Cols / 2.0, work.Rows / 2.0, radius);
                    double target = options.LuminanceTarget / 255.0 * input.MaxValue;
                    double match = median > 0.0 ? target / median : options.MaxGain;
                    gain = Math.Min(options.MaxGain, Math.Max(1.0, match));
                    why = $"circle median {median:0.0} -> target {target:0.0} ({options.LuminanceTarget:0.##}/255), requested x{match:0.00}, applied x{gain:0.00}";
                    requested = gain;
                }
                string matched = options.MatchDisc || options.MatchLuminance ? why : null;
                if (options.AutoLimit)
                {
                    double radius = options.RoiRatio * work.Cols;
                    float peak = RoiPeak(work, work.Cols / 2.0, work.Rows / 2.0, radius);
                    double threshold = options.Threshold > 1.0
                        ? Math.Min(options.Threshold, input.MaxValue)
                        : Math.Max(0.0, options.Threshold) * input.MaxValue;
                    string roi = $"ROI circle r={radius:0} px ({options.RoiRatio:0.00} x width) peak {peak:0}, threshold {threshold:0}";
                    double limit = peak > 0f ? threshold / peak : gain;
                    if (limit < gain)
                    {
                        gain = Math.Max(1.0, limit);
                        why = gain > 1.0
                            ? $"requested x{requested:0.00}, stopped at x{gain:0.00} where the {roi} saturates"
                            : $"requested x{requested:0.00}, not raised - {roi} is already at the threshold";
                    }
                    else
                    {
                        why = $"requested x{requested:0.00}, below saturation ({roi})";
                    }
                    if (matched != null) why = matched + "; " + why;
                }
                DiscState discState = default;
                double discPeak = 0.0;
                double discHeadroom = 0.0;
                if (options.DiscLocal && gain > 1.0)
                {
                    discHeadroom = options.DiscHeadroom > 1.0
                        ? Math.Min(options.DiscHeadroom, input.MaxValue)
                        : Math.Max(0.0, options.DiscHeadroom) * input.MaxValue;
                    discState = MeasureDisc(work, discLock, input.MaxValue);
                    discPeak = discState.P99 * gain;
                }
                if (gain > 1.0)
                {
                    if (options.SoftKnee)
                    {
                        // LsoImageAnalysis.ApplyBoost mode 0: the Image Analysis soft-knee curve.
                        float g = (float)gain;
                        float max = input.MaxValue;
                        int count = work.Rows * work.Cols * 3;
                        var values = new float[count];
                        Marshal.Copy(work.Data, values, 0, count);
                        for (int i = 0; i < count; ++i) values[i] = LsoImageAnalysis.ApplyBoost(values[i], g, 0, max);
                        Marshal.Copy(values, 0, work.Data, count);
                    }
                    else
                    {
                        work.ConvertTo(work, MatType.CV_32FC3, gain);
                    }
                    double knee = 0.0;
                    double slope = 1.0;
                    bool cut = options.DiscLocal && discPeak > discHeadroom
                        && CutDisc(work, discLock, Math.Max(1.0, options.DiscBlendRatio), discPeak, discHeadroom, out knee, out slope);
                    if (options.DiscLocal)
                    {
                        double outer = Math.Max(1.0, options.DiscBlendRatio) * discLock.Radius;
                        why += $"; disc ({discState.Cx:0.0},{discState.Cy:0.0}) r={discState.Radius:0} px, p99 {discState.P99:0} x{gain:0.00} = {discPeak:0}, headroom {discHeadroom:0}, "
                            + (cut ? $"knee above the retina level {knee:0}, slope {slope:0.00} inside, full gain beyond {outer:0} px"
                                : discPeak > discHeadroom ? "no room above the retina level, not cut" : "below the headroom, not cut");
                    }
                    stages.Add(Stage($"Brightness (x{gain:0.00}{(cut ? $", disc knee {slope:0.00}" : "")}, {(options.SoftKnee ? "soft knee" : "linear")})",
                        why + (options.SoftKnee
                            ? $"; knee at {Math.Round(input.MaxValue * 200.0 / 255.0)}, nothing reaches {input.MaxValue}"
                            : "; whole image scaled, outside the ROI may clip at 8-bit"),
                        true, work, scale, clock));
                }
                else
                {
                    stages.Add(Stage("Brightness", options.Brightness > 1.0 ? why : "OFF (gain 1.00)", false, work, scale, clock));
                }

                cancel.ThrowIfCancellationRequested();
                bool encodeGamma = options.Gamma > 0.0 && Math.Abs(options.Gamma - 1.0) > 1e-9;
                string encode = "RGB -> BGR for display and file";
                if (encodeGamma)
                {
                    double e = 1.0 / options.Gamma;
                    work.ConvertTo(work, MatType.CV_32FC3, 1.0 / input.MaxValue);
                    Cv2.Max(work, 0.0, work);
                    if (options.GammaOnValue)
                    {
                        Cv2.Split(work, out Mat[] rgb);
                        using var value = new Mat();
                        Cv2.Max(rgb[0], rgb[1], value);
                        Cv2.Max(value, rgb[2], value);
                        using var curved = new Mat();
                        Cv2.Pow(value, e, curved);
                        Cv2.Max(value, 1e-6, value);
                        Cv2.Divide(curved, value, curved);
                        using var ratio3 = new Mat();
                        Cv2.Merge(new[] { curved, curved, curved }, ratio3);
                        foreach (Mat channel in rgb) channel.Dispose();
                        Cv2.Multiply(work, ratio3, work);
                    }
                    else
                    {
                        Cv2.Pow(work, e, work);
                    }
                    work.ConvertTo(work, MatType.CV_32FC3, input.MaxValue);
                    encode = $"gamma {options.Gamma:0.00} on {(options.GammaOnValue ? "V = max(R,G,B), R/G/B scaled by V'/V (ratios kept)" : "each of R, G, B")}: 10% -> {Math.Pow(0.1, e) * 100:0}%, 50% -> {Math.Pow(0.5, e) * 100:0}%, 90% -> {Math.Pow(0.9, e) * 100:0}% of 255, full scale unchanged; " + encode;
                }
                stages.Add(Stage($"8-bit (x255/{input.MaxValue}, {(encodeGamma ? $"gamma {options.Gamma:0.00}{(options.GammaOnValue ? " on V" : "")}" : "linear")}, saturate)", encode, true, work, scale, clock));
                work.Dispose();
                work = null;
                return new PostProcessResult(input.Source, stages);
            }
            catch
            {
                foreach (PostProcessStage stage in stages) stage.Dispose();
                work?.Dispose();
                throw;
            }
        }

        // PNG (lossless) or JPG of a stage's 8-bit image. Encoded in memory so a non-ASCII path works.
        public static void Save(Mat image8, string path)
        {
            string ext = Path.GetExtension(path).ToLowerInvariant();
            if (ext != ".png" && ext != ".jpg" && ext != ".jpeg")
                throw new ArgumentException("Only .png and .jpg are written.", nameof(path));
            Cv2.ImEncode(ext, image8, out byte[] bytes);
            File.WriteAllBytes(path, bytes);
        }

        // Gray16 is the _raw16.tiff companion: the Bayer mosaic, read as such only when the caller
        // says so (a mono Gray16 is refused in this draft). Rgb48 is the _rgb48.tiff companion.
        // A <stem>_capture.json next to the file (LsoCaptureRecord) gives the bit depth and the keys
        // of the reflected-light map, so a saved frame switches the removal like a live capture.
        public static PostProcessInput LoadTiff(string path, bool gray16IsBayer = true, int bitDepth = 12,
            string? reflectedLightDirectory = null)
        {
            string name = Path.GetFileName(path);
            BitmapSource frame = ReadSinglePageTiff(path);
            int width = frame.PixelWidth;
            int height = frame.PixelHeight;
            LsoCaptureRecord.TryLoadFor(path, out LsoCaptureRecord? record, out string role, out string recordNote);
            if (frame.Format == PixelFormats.Gray16)
            {
                if (!gray16IsBayer)
                    throw new NotSupportedException($"'{name}' is Gray16; this draft reads Gray16 only as a Bayer RGGB frame.");
                byte[] bytes = Gray16Bytes(frame);
                if (record == null)
                {
                    return PostProcessInput.FromBayer(bytes, width, height, bitDepth, name,
                        reflectedLightNote: recordNote + ", and a TIFF alone does not carry the capture's exposure, gain, LED, flip and focus");
                }
                return FromRecordedBayer(path, name, bytes, width, height, record, role,
                    reflectedLightDirectory ?? LsoConfigLocation.ReflectedLightDirectory());
            }
            if (frame.Format == PixelFormats.Rgb48)
            {
                var pixels = new ushort[checked(width * height * 3)];
                frame.CopyPixels(pixels, width * 6, 0);
                int depth = record != null && record.BitDepth > 0 ? record.BitDepth : bitDepth;
                return PostProcessInput.FromRgb48(pixels, width, height, (1 << depth) - 1,
                    record != null ? $"{name} ({record.Summary})" : name);
            }
            throw new NotSupportedException($"'{name}' is {frame.Format}; only Gray16 (Bayer) and Rgb48 TIFFs are read.");
        }

        // The sidecar must describe these very pixels (size and hash) before its map keys are used.
        // A frame whose removal ran at capture is reopened on its _uncorrected16.tiff, so switching
        // the removal never subtracts the map a second time.
        private static PostProcessInput FromRecordedBayer(string path, string name, byte[] bytes, int width, int height,
            LsoCaptureRecord record, string role, string reflectedLightDirectory)
        {
            int bitDepth = record.BitDepth > 0 ? record.BitDepth : 12;
            LsoCaptureFile? entry = record.FileFor(role);
            if (entry?.Sha256 == null || (uint)width != record.Conditions.Width || (uint)height != record.Conditions.Height
                || !string.Equals(entry.Sha256, LsoCaptureRecord.Sha256Hex(bytes), StringComparison.OrdinalIgnoreCase))
            {
                return PostProcessInput.FromBayer(bytes, width, height, bitDepth, name,
                    reflectedLightNote: "the capture sidecar does not describe these pixels (renamed, edited or from another frame), so its map keys are not used");
            }
            string source = $"{name} ({record.Summary})";
            if (record.FlatField.Applied)
            {
                return PostProcessInput.FromBayer(bytes, width, height, bitDepth, source,
                    reflectedLightNote: "a flat field was applied at capture and post processing cannot re-apply it, so reflected-light removal is not offered for this frame");
            }
            bool removedAtCapture = entry.Pixels == LsoCaptureRecord.CorrectedPixels;
            LsoReflectedLightCorrection? correction = ResolveReflectedLight(record, reflectedLightDirectory, out string reflectedNote);
            if (correction == null)
            {
                return PostProcessInput.FromBayer(bytes, width, height, bitDepth, source,
                    reflectedLightNote: removedAtCapture
                        ? $"removed at capture (focus {record.FocusPosition}) and cannot be switched: {reflectedNote}"
                        : reflectedNote);
            }
            if (removedAtCapture)
            {
                if (!TryReadUncorrected(path, record, width, height, out byte[] sensor, out string why))
                {
                    return PostProcessInput.FromBayer(bytes, width, height, bitDepth, source,
                        reflectedLightNote: $"removed at capture (focus {record.FocusPosition}); {why}");
                }
                bytes = sensor;
                source += $"; sensor pixels from {record.Files.Uncorrected16!.Name}";
            }
            if (reflectedNote.Length > 0) source += "; " + reflectedNote;
            return PostProcessInput.FromBayer(bytes, width, height, bitDepth, source, correction);
        }

        // The map the capture would use today: the set in the config folder, checked against the
        // recorded conditions (FirstMismatch names the field) and interpolated at the recorded focus.
        private static LsoReflectedLightCorrection? ResolveReflectedLight(LsoCaptureRecord record, string directory, out string note)
        {
            note = string.Empty;
            if (record.FocusPosition is not int focus)
            {
                note = "the capture did not record the LSO focus position";
                return null;
            }
            if (!LsoReflectedLightSet.TryLoad(directory, out LsoReflectedLightSet? set, out string error) || set == null)
            {
                note = "no reflected-light map: " + error;
                return null;
            }
            string? mismatch = set.Conditions.FirstMismatch(record.Conditions);
            if (mismatch != null)
            {
                note = "the map does not fit this capture: " + mismatch;
                return null;
            }
            if (!set.TryForFocus(focus, out LsoReflectedLightCorrection? correction, out error) || correction == null)
            {
                note = error;
                return null;
            }
            if (record.ReflectedLight.SetCreated is DateTime created && created != set.Created)
            {
                note = $"map set recalibrated since the capture ({created:O} -> {set.Created:O})";
            }
            return correction;
        }

        private static bool TryReadUncorrected(string path, LsoCaptureRecord record, int width, int height,
            out byte[] sensor, out string why)
        {
            sensor = Array.Empty<byte>();
            LsoCaptureFile? entry = record.Files.Uncorrected16;
            if (entry?.Sha256 == null)
            {
                why = "no _uncorrected16.tiff was kept for this frame";
                return false;
            }
            string file = LsoCaptureRecord.StemOf(path, out _)! + LsoCaptureRecord.Uncorrected16Suffix + ".tiff";
            if (!File.Exists(file))
            {
                why = $"{Path.GetFileName(file)} is missing";
                return false;
            }
            BitmapSource frame = ReadSinglePageTiff(file);
            if (frame.Format != PixelFormats.Gray16 || frame.PixelWidth != width || frame.PixelHeight != height)
            {
                why = $"{Path.GetFileName(file)} is not a {width}x{height} Gray16 frame";
                return false;
            }
            byte[] bytes = Gray16Bytes(frame);
            if (!string.Equals(entry.Sha256, LsoCaptureRecord.Sha256Hex(bytes), StringComparison.OrdinalIgnoreCase))
            {
                why = $"{Path.GetFileName(file)} does not match its recorded hash";
                return false;
            }
            sensor = bytes;
            why = string.Empty;
            return true;
        }

        private static BitmapSource ReadSinglePageTiff(string path)
        {
            using FileStream stream = File.OpenRead(path);
            var decoder = new TiffBitmapDecoder(stream, BitmapCreateOptions.PreservePixelFormat, BitmapCacheOption.OnLoad);
            if (decoder.Frames.Count != 1)
                throw new NotSupportedException($"'{Path.GetFileName(path)}' has {decoder.Frames.Count} pages; only a single-page TIFF is read.");
            return decoder.Frames[0];
        }

        private static byte[] Gray16Bytes(BitmapSource frame)
        {
            var pixels = new ushort[checked(frame.PixelWidth * frame.PixelHeight)];
            frame.CopyPixels(pixels, frame.PixelWidth * 2, 0);
            var bytes = new byte[pixels.Length * 2];
            Buffer.BlockCopy(pixels, 0, bytes, 0, bytes.Length);
            return bytes;
        }

        private static Mat Rgb48Mat(ushort[] rgb, int width, int height)
        {
            var mat = new Mat(height, width, MatType.CV_16UC3);
            var bytes = new byte[rgb.Length * 2];
            Buffer.BlockCopy(rgb, 0, bytes, 0, bytes.Length);
            Marshal.Copy(bytes, 0, mat.Data, bytes.Length);
            return mat;
        }

        private static PostProcessStage Stage(string name, string note, bool applied, Mat workingRgb, double scale, Stopwatch clock)
        {
            using var rgb8 = new Mat();
            workingRgb.ConvertTo(rgb8, MatType.CV_8UC3, scale);
            var bgr8 = new Mat();
            Cv2.CvtColor(rgb8, bgr8, ColorConversionCodes.RGB2BGR);
            using Mat flat = workingRgb.Reshape(1);
            Cv2.MinMaxLoc(flat, out _, out double max);
            return new PostProcessStage(name, note, applied, bgr8, max, Lap(clock));
        }

        // The map is subtracted from the unpacked 16-bit Bayer frame, in place on a copy: the same
        // arithmetic the capture callback runs (LsoReflectedLightCorrection.Apply).
        private static CameraHighDepthData SubtractReflectedLight(CameraHighDepthData frame,
            LsoReflectedLightCorrection correction, out string note)
        {
            var bytes = new byte[checked(frame.Raw16.Length * 2)];
            Buffer.BlockCopy(frame.Raw16, 0, bytes, 0, bytes.Length);
            note = $"map at position {correction.Position} (weight {correction.Weight:0.00}), "
                + $"ROI {correction.Roi.X},{correction.Roi.Y} {correction.Roi.Width}x{correction.Roi.Height}";
            if (!correction.Apply(bytes, frame.Width, frame.Height, 2))
            {
                throw new InvalidOperationException(
                    $"The reflected-light map is {correction.Conditions.Width}x{correction.Conditions.Height}; the frame is {frame.Width}x{frame.Height}.");
            }
            return new CameraHighDepthData(bytes, frame.Width, frame.Height, frame.BitDepth, frame.IsMono);
        }

        private static Mat GrayDisplay(CameraHighDepthData bayer)
        {
            var gray = new Mat(bayer.Height, bayer.Width, MatType.CV_8UC1);
            Marshal.Copy(bayer.ToDisplay8(), 0, gray.Data, bayer.Width * bayer.Height);
            return gray;
        }

        // The circle of radius ratio x width at the image centre and its bounding box, clipped to
        // the image. Offsets and sizes are even so a Bayer mosaic keeps its RGGB phase.
        private static (int X, int Y, int Width, int Height, double Cx, double Cy, double R) CircleBox(int width, int height, double ratio)
        {
            double r = ratio * width;
            double cx = width / 2.0;
            double cy = height / 2.0;
            int x0 = Math.Max(0, (int)Math.Floor(cx - r)) & ~1;
            int y0 = Math.Max(0, (int)Math.Floor(cy - r)) & ~1;
            int x1 = Math.Min(width, (int)Math.Ceiling(cx + r));
            int y1 = Math.Min(height, (int)Math.Ceiling(cy + r));
            int w = Math.Max(2, (x1 - x0) & ~1);
            int h = Math.Max(2, (y1 - y0) & ~1);
            return (x0, y0, w, h, cx - x0, cy - y0, r);
        }

        // Bayer mosaic: pixels whose centre lies outside the circle become 0, then the box is cut.
        private static CameraHighDepthData CircleCrop(CameraHighDepthData bayer, double ratio, out string note)
        {
            var box = CircleBox(bayer.Width, bayer.Height, ratio);
            var raw = new ushort[box.Width * box.Height];
            double r2 = box.R * box.R;
            for (int y = 0; y < box.Height; ++y)
            {
                double dy = y + 0.5 - box.Cy;
                for (int x = 0; x < box.Width; ++x)
                {
                    double dx = x + 0.5 - box.Cx;
                    raw[y * box.Width + x] = dx * dx + dy * dy <= r2
                        ? bayer.Raw16[(y + box.Y) * bayer.Width + x + box.X]
                        : (ushort)0;
                }
            }
            var bytes = new byte[raw.Length * 2];
            Buffer.BlockCopy(raw, 0, bytes, 0, bytes.Length);
            note = $"r={box.R:0} px at ({box.Cx:0},{box.Cy:0}); outside zeroed, cut to {box.Width}x{box.Height} (was {bayer.Width}x{bayer.Height})";
            return new CameraHighDepthData(bytes, box.Width, box.Height, bayer.BitDepth, bayer.IsMono);
        }

        // RGB image (already demosaiced): same circle and box, via a mask copy.
        private static Mat CircleCrop(Mat rgb, double ratio, out string note)
        {
            var box = CircleBox(rgb.Cols, rgb.Rows, ratio);
            using var mask = new Mat(box.Height, box.Width, MatType.CV_8UC1, Scalar.All(0));
            Cv2.Circle(mask, new Point(box.Cx, box.Cy), (int)Math.Round(box.R), Scalar.All(255), -1);
            var cut = new Mat(box.Height, box.Width, rgb.Type(), Scalar.All(0));
            using (var view = new Mat(rgb, new Rect(box.X, box.Y, box.Width, box.Height)))
            {
                view.CopyTo(cut, mask);
            }
            note = $"r={box.R:0} px at ({box.Cx:0},{box.Cy:0}); outside zeroed, cut to {box.Width}x{box.Height} (was {rgb.Cols}x{rgb.Rows})";
            return cut;
        }

        private readonly record struct FlattenResult(double Mean, double MinGain, double MaxGain);
        private readonly record struct DiscLock(double Cx, double Cy, double Radius);
        private readonly record struct DiscState(double Cx, double Cy, double Radius, double P99, double SaturatedFraction);

        // Brightest compact region inside the search circle. Blur sigma is a quarter of the disc
        // radius, so a specular point smaller than the disc loses to the disc body.
        private static DiscLock LocateDisc(Mat rgb32, double searchRadius, double discRadius)
        {
            int width = rgb32.Cols;
            int height = rgb32.Rows;
            Cv2.Split(rgb32, out Mat[] channels);
            using var luma = new Mat();
            channels[0].ConvertTo(luma, MatType.CV_32FC1, 0.2126);
            using (var term = new Mat())
            {
                channels[1].ConvertTo(term, MatType.CV_32FC1, 0.7152);
                Cv2.Add(luma, term, luma);
                channels[2].ConvertTo(term, MatType.CV_32FC1, 0.0722);
                Cv2.Add(luma, term, luma);
            }
            foreach (Mat channel in channels) channel.Dispose();
            using var blurred = new Mat();
            Cv2.GaussianBlur(luma, blurred, new Size(0, 0), Math.Max(1.0, discRadius * 0.25));
            using var mask = new Mat(height, width, MatType.CV_8UC1, Scalar.All(0));
            int search = (int)Math.Floor(searchRadius - discRadius);
            if (search > 2)
                Cv2.Circle(mask, new Point(width / 2, height / 2), search, Scalar.All(255), -1);
            else
                mask.SetTo(Scalar.All(255));
            Cv2.MinMaxLoc(blurred, out _, out _, out _, out Point peak, mask);
            return new DiscLock(peak.X + 0.5, peak.Y + 0.5, discRadius);
        }

        // Linear knee on max(R,G,B) in the disc neighbourhood, after the global gain, in place on
        // the bounding box only: m <= T untouched; m > T -> T + (m - T) * s, and R, G, B are all
        // scaled by the same factor so the hue is kept. T = median of max(R,G,B) in the ring
        // between 1.5 x radius and blend x radius (the local retina level). slope = (headroom - T)
        // / (p99 - T) puts the disc p99 at the headroom; s = 1 - spatial * (1 - slope) with
        // spatial = 1 inside 1.5 x radius falling to 0 at blend x radius (smoothstep). Returns
        // false when the ring is empty or the knee has no room (T >= headroom or p99 <= T).
        private static bool CutDisc(Mat rgb32, DiscLock disc, double blend, double p99, double headroom, out double knee, out double slope)
        {
            knee = 0.0;
            slope = 1.0;
            double outer = disc.Radius * blend;
            double plateau = Math.Min(1.5 * disc.Radius, 0.75 * outer);
            if (outer <= plateau + 1.0) return false;
            int x0 = Math.Max(0, (int)Math.Floor(disc.Cx - outer));
            int y0 = Math.Max(0, (int)Math.Floor(disc.Cy - outer));
            int x1 = Math.Min(rgb32.Cols, (int)Math.Ceiling(disc.Cx + outer) + 1);
            int y1 = Math.Min(rgb32.Rows, (int)Math.Ceiling(disc.Cy + outer) + 1);
            if (x1 <= x0 || y1 <= y0) return false;
            var rect = new Rect(x0, y0, x1 - x0, y1 - y0);
            using var roi = new Mat(rgb32, rect);
            using var box = roi.Clone();
            var values = new float[rect.Width * rect.Height * 3];
            Marshal.Copy(box.Data, values, 0, values.Length);
            double plateau2 = plateau * plateau;
            double outer2 = outer * outer;
            var ring = new List<float>();
            for (int y = 0; y < rect.Height; ++y)
            {
                double dy = y0 + y + 0.5 - disc.Cy;
                for (int x = 0; x < rect.Width; ++x)
                {
                    double dx = x0 + x + 0.5 - disc.Cx;
                    double d2 = dx * dx + dy * dy;
                    if (d2 <= plateau2 || d2 > outer2) continue;
                    int i = (y * rect.Width + x) * 3;
                    ring.Add(Math.Max(values[i], Math.Max(values[i + 1], values[i + 2])));
                }
            }
            if (ring.Count == 0) return false;
            ring.Sort();
            knee = ring[ring.Count / 2];
            if (knee >= headroom || p99 <= knee) return false;
            slope = (headroom - knee) / (p99 - knee);
            double span = outer - plateau;
            for (int y = 0; y < rect.Height; ++y)
            {
                double dy = y0 + y + 0.5 - disc.Cy;
                for (int x = 0; x < rect.Width; ++x)
                {
                    int i = (y * rect.Width + x) * 3;
                    float m = Math.Max(values[i], Math.Max(values[i + 1], values[i + 2]));
                    if (m <= knee) continue;
                    double dx = x0 + x + 0.5 - disc.Cx;
                    double t = Math.Clamp((Math.Sqrt(dx * dx + dy * dy) - plateau) / span, 0.0, 1.0);
                    double spatial = 1.0 - t * t * (3.0 - 2.0 * t);
                    double s = 1.0 - spatial * (1.0 - slope);
                    float factor = (float)((knee + (m - knee) * s) / m);
                    values[i] *= factor;
                    values[i + 1] *= factor;
                    values[i + 2] *= factor;
                }
            }
            Marshal.Copy(values, 0, box.Data, values.Length);
            box.CopyTo(roi);
            return true;
        }

        // p99 of max(R,G,B) inside the disc, and the fraction already at 98% of full scale.
        private static DiscState MeasureDisc(Mat rgb32, DiscLock disc, float fullScale)
        {
            int width = rgb32.Cols;
            int height = rgb32.Rows;
            var values = new float[width * height * 3];
            Marshal.Copy(rgb32.Data, values, 0, values.Length);
            double r2 = disc.Radius * disc.Radius;
            int x0 = Math.Max(0, (int)Math.Floor(disc.Cx - disc.Radius));
            int x1 = Math.Min(width - 1, (int)Math.Ceiling(disc.Cx + disc.Radius));
            int y0 = Math.Max(0, (int)Math.Floor(disc.Cy - disc.Radius));
            int y1 = Math.Min(height - 1, (int)Math.Ceiling(disc.Cy + disc.Radius));
            var peaks = new List<float>();
            int saturated = 0;
            float ceiling = fullScale * 0.98f;
            for (int y = y0; y <= y1; ++y)
            {
                double dy = y + 0.5 - disc.Cy;
                for (int x = x0; x <= x1; ++x)
                {
                    double dx = x + 0.5 - disc.Cx;
                    if (dx * dx + dy * dy > r2) continue;
                    int i = (y * width + x) * 3;
                    float m = Math.Max(values[i], Math.Max(values[i + 1], values[i + 2]));
                    peaks.Add(m);
                    if (m >= ceiling) ++saturated;
                }
            }
            if (peaks.Count == 0) return new DiscState(disc.Cx, disc.Cy, disc.Radius, 0, 0);
            peaks.Sort();
            double p99 = peaks[(int)Math.Round(0.99 * (peaks.Count - 1))];
            return new DiscState(disc.Cx, disc.Cy, disc.Radius, p99, (double)saturated / peaks.Count);
        }

        // Normalised convolution: blur(L * mask) / blur(mask), so the black corners outside the
        // circle do not pull the estimate down near the rim. The blur runs on a downscaled copy
        // (sigma ~12 px there) and is interpolated back - a 300 px sigma at full size is too slow.
        private static FlattenResult FlattenIllumination(Mat rgb32, double radius, double sigma, double strength, double maxGain)
        {
            int width = rgb32.Cols;
            int height = rgb32.Rows;
            Cv2.Split(rgb32, out Mat[] planes);
            using var luma = new Mat();
            Cv2.AddWeighted(planes[0], 0.2126, planes[1], 0.7152, 0.0, luma);
            Cv2.AddWeighted(luma, 1.0, planes[2], 0.0722, 0.0, luma);
            foreach (Mat plane in planes) plane.Dispose();

            using var mask8 = new Mat(height, width, MatType.CV_8UC1, Scalar.All(0));
            Cv2.Circle(mask8, new Point(width / 2, height / 2), (int)Math.Round(radius), Scalar.All(255), -1);
            double mean = Cv2.Mean(luma, mask8).Val0;
            using var mask32 = new Mat();
            mask8.ConvertTo(mask32, MatType.CV_32FC1, 1.0 / 255.0);
            using var masked = new Mat();
            Cv2.Multiply(luma, mask32, masked);

            int factor = Math.Clamp((int)Math.Floor(sigma / 12.0), 1, 16);
            var small = new Size(Math.Max(1, width / factor), Math.Max(1, height / factor));
            using var smallMasked = new Mat();
            using var smallMask = new Mat();
            Cv2.Resize(masked, smallMasked, small, 0, 0, InterpolationFlags.Area);
            Cv2.Resize(mask32, smallMask, small, 0, 0, InterpolationFlags.Area);
            double smallSigma = Math.Max(0.5, sigma / factor);
            Cv2.GaussianBlur(smallMasked, smallMasked, new Size(0, 0), smallSigma);
            Cv2.GaussianBlur(smallMask, smallMask, new Size(0, 0), smallSigma);
            using var smallBackground = new Mat();
            Cv2.Divide(smallMasked, smallMask, smallBackground);
            using var background = new Mat();
            Cv2.Resize(smallBackground, background, new Size(width, height), 0, 0, InterpolationFlags.Linear);

            // gain = 1 + strength x (mean / background - 1), clamped, inside the circle; 1 outside.
            using var gain = new Mat();
            Cv2.Divide(mean, background, gain);
            gain.ConvertTo(gain, MatType.CV_32FC1, strength, 1.0 - strength);
            Cv2.Min(gain, maxGain, gain);
            Cv2.Max(gain, 1.0 / maxGain, gain);
            Cv2.MinMaxLoc(gain, out double minGain, out double maxGainSeen, out _, out _, mask8);
            using var outside = new Mat();
            mask32.ConvertTo(outside, MatType.CV_32FC1, -1.0, 1.0);
            Cv2.Multiply(gain, mask32, gain);
            Cv2.Add(gain, outside, gain);
            using var gain3 = new Mat();
            Cv2.Merge(new[] { gain, gain, gain }, gain3);
            Cv2.Multiply(rgb32, gain3, rgb32);
            return new FlattenResult(mean, minGain, maxGainSeen);
        }

        // Median of Y = 0.2126 R + 0.7152 G + 0.0722 B inside the circle. Work is RGB, not BGR.
        private static double CircleLuminanceMedian(Mat rgb32, double cx, double cy, double radius)
        {
            int width = rgb32.Cols;
            int height = rgb32.Rows;
            var values = new float[width * height * 3];
            Marshal.Copy(rgb32.Data, values, 0, values.Length);
            double r2 = radius * radius;
            var luma = new List<float>();
            for (int y = 0; y < height; ++y)
            {
                double dy = y + 0.5 - cy;
                for (int x = 0; x < width; ++x)
                {
                    double dx = x + 0.5 - cx;
                    if (dx * dx + dy * dy > r2) continue;
                    int i = (y * width + x) * 3;
                    luma.Add((float)(0.2126 * values[i] + 0.7152 * values[i + 1] + 0.0722 * values[i + 2]));
                }
            }
            if (luma.Count == 0) return 0.0;
            luma.Sort();
            int mid = luma.Count / 2;
            return luma.Count % 2 == 1 ? luma[mid] : 0.5 * (luma[mid - 1] + luma[mid]);
        }

        // Largest R, G or B value inside the circle (pixel centres), from a float RGB Mat.
        private static float RoiPeak(Mat rgb32, double cx, double cy, double radius)
        {
            int width = rgb32.Cols;
            int height = rgb32.Rows;
            var values = new float[width * height * 3];
            Marshal.Copy(rgb32.Data, values, 0, values.Length);
            double r2 = radius * radius;
            float peak = 0f;
            for (int y = 0; y < height; ++y)
            {
                double dy = y + 0.5 - cy;
                for (int x = 0; x < width; ++x)
                {
                    double dx = x + 0.5 - cx;
                    if (dx * dx + dy * dy > r2) continue;
                    int i = (y * width + x) * 3;
                    float m = Math.Max(values[i], Math.Max(values[i + 1], values[i + 2]));
                    if (m > peak) peak = m;
                }
            }
            return peak;
        }

        private static double Lap(Stopwatch clock)
        {
            double ms = clock.Elapsed.TotalMilliseconds;
            clock.Restart();
            return ms;
        }
    }
}
