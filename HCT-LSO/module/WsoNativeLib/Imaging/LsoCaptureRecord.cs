using System;
using System.IO;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;
using WsoNativeLib.Presets;

namespace WsoNativeLib.Imaging
{
    // What one saved frame was captured with, written next to its TIFFs as <stem>_capture.json.
    // A TIFF alone does not carry the exposure, gain, LED, flip and focus a reflected-light map
    // has to match; this file does, so a frame opened later can pick its map again. Conditions
    // has the map's own shape, so LsoReflectedLightConditions.FirstMismatch compares them directly.
    public sealed record LsoCaptureRecord
    {
        public const int CurrentVersion = 1;
        public const string SidecarSuffix = "_capture.json";
        public const string Raw16Suffix = "_raw16";
        public const string Uncorrected16Suffix = "_uncorrected16";
        public const string Rgb48Suffix = "_rgb48";
        public const string SensorPixels = "sensor";
        public const string CorrectedPixels = "corrected";

        private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };

        public int Version { get; init; } = CurrentVersion;
        public DateTime CapturedAtUtc { get; init; }
        public string Preset { get; init; } = string.Empty;
        public string ImageMode { get; init; } = string.Empty;
        public string Illumination { get; init; } = string.Empty;
        public int BitDepth { get; init; }
        public int FrameIndex { get; init; }
        public int FrameTotal { get; init; }
        public LsoReflectedLightConditions Conditions { get; init; } = new();
        // LSO focus encoder position at capture start: the key the reflected-light maps are stored by.
        public int? FocusPosition { get; init; }
        public LsoCaptureReflectedLight ReflectedLight { get; init; } = new();
        public LsoCaptureFlatField FlatField { get; init; } = new();
        public LsoCaptureFiles Files { get; init; } = new();

        [JsonIgnore]
        public string Summary => $"{Preset}, exposure {Conditions.ExposureUs} us, gain {Conditions.Gain:0.##} dB, "
            + $"LED {Conditions.LedIntensity}, focus {(FocusPosition?.ToString() ?? "unknown")}";

        public LsoCaptureRecord ForFrame(int frameIndex, int frameTotal, int flipMode, int bitDepth,
            bool reflectedApplied, string reflectedSkipped, bool flatApplied)
        {
            LsoReflectedLightConditions c = Conditions;
            return this with
            {
                FrameIndex = frameIndex,
                FrameTotal = frameTotal,
                BitDepth = bitDepth,
                Conditions = new LsoReflectedLightConditions
                {
                    FundusMode = c.FundusMode, ExposureUs = c.ExposureUs, Gain = c.Gain, LedIntensity = c.LedIntensity,
                    Width = c.Width, Height = c.Height, OffsetX = c.OffsetX, OffsetY = c.OffsetY,
                    PixelFormat = c.PixelFormat, FlipMode = flipMode,
                },
                ReflectedLight = ReflectedLight with
                {
                    Applied = reflectedApplied,
                    Skipped = reflectedSkipped.Length > 0 ? reflectedSkipped : ReflectedLight.Skipped,
                },
                FlatField = FlatField with { Applied = flatApplied },
            };
        }

        public LsoCaptureFile? FileFor(string role) => role switch
        {
            Raw16Suffix => Files.Raw16,
            Uncorrected16Suffix => Files.Uncorrected16,
            Rgb48Suffix => Files.Rgb48,
            _ => null,
        };

        // Next to the companions SaveTiffCompanions already wrote for pngPath: the uncorrected
        // sensor frame (only when the caller passes one) and the sidecar with a pixel hash per file.
        public static void Write(string pngPath, LsoCaptureRecord record, CameraHighDepthData frame, byte[]? uncorrected)
        {
            string stem = Path.Combine(Path.GetDirectoryName(pngPath)!, Path.GetFileNameWithoutExtension(pngPath));
            string name = Path.GetFileName(stem);
            var raw = new byte[checked(frame.Raw16.Length * 2)];
            Buffer.BlockCopy(frame.Raw16, 0, raw, 0, raw.Length);
            if (uncorrected != null)
            {
                if (uncorrected.Length != raw.Length)
                    throw new ArgumentException("The uncorrected frame does not match the saved frame.", nameof(uncorrected));
                var pixels = new ushort[frame.Raw16.Length];
                Buffer.BlockCopy(uncorrected, 0, pixels, 0, uncorrected.Length);
                CameraHighDepthData.SaveTiff(stem + Uncorrected16Suffix + ".tiff", frame.Width, frame.Height, pixels, false);
            }
            bool corrected = record.ReflectedLight.Applied || record.FlatField.Applied;
            LsoCaptureRecord written = record with
            {
                Files = new LsoCaptureFiles
                {
                    Raw16 = new LsoCaptureFile(name + Raw16Suffix + ".tiff", corrected ? CorrectedPixels : SensorPixels, Sha256Hex(raw)),
                    Uncorrected16 = uncorrected == null ? null
                        : new LsoCaptureFile(name + Uncorrected16Suffix + ".tiff", SensorPixels, Sha256Hex(uncorrected)),
                    Rgb48 = frame.IsMono ? null
                        : new LsoCaptureFile(name + Rgb48Suffix + ".tiff", "debayered from raw16", null),
                },
            };
            File.WriteAllText(stem + SidecarSuffix, JsonSerializer.Serialize(written, JsonOptions));
        }

        // False with the reason when there is no usable sidecar. role is the companion suffix of
        // tiffPath (_raw16 ...), even when the sidecar is missing.
        public static bool TryLoadFor(string tiffPath, out LsoCaptureRecord? record, out string role, out string note)
        {
            record = null;
            string? stem = StemOf(tiffPath, out role);
            if (stem == null)
            {
                note = $"'{Path.GetFileName(tiffPath)}' is not named like a saved frame (*{Raw16Suffix}, *{Uncorrected16Suffix}, *{Rgb48Suffix})";
                return false;
            }
            string sidecar = stem + SidecarSuffix;
            if (!File.Exists(sidecar))
            {
                note = $"no {Path.GetFileName(sidecar)} next to the file";
                return false;
            }
            try
            {
                record = JsonSerializer.Deserialize<LsoCaptureRecord>(File.ReadAllText(sidecar), JsonOptions);
            }
            catch (Exception e) when (e is JsonException or IOException or UnauthorizedAccessException or NotSupportedException)
            {
                note = $"{Path.GetFileName(sidecar)} is unreadable: {e.Message}";
                return false;
            }
            if (record == null || record.Version != CurrentVersion)
            {
                note = $"{Path.GetFileName(sidecar)} has version {record?.Version.ToString() ?? "none"}; {CurrentVersion} is read";
                record = null;
                return false;
            }
            note = string.Empty;
            return true;
        }

        public static string? StemOf(string tiffPath, out string role)
        {
            role = string.Empty;
            string ext = Path.GetExtension(tiffPath);
            if (!ext.Equals(".tiff", StringComparison.OrdinalIgnoreCase) && !ext.Equals(".tif", StringComparison.OrdinalIgnoreCase))
                return null;
            string name = Path.GetFileNameWithoutExtension(tiffPath);
            foreach (string suffix in new[] { Raw16Suffix, Uncorrected16Suffix, Rgb48Suffix })
            {
                if (name.EndsWith(suffix, StringComparison.OrdinalIgnoreCase))
                {
                    role = suffix;
                    return Path.Combine(Path.GetDirectoryName(tiffPath)!, name[..^suffix.Length]);
                }
            }
            return null;
        }

        // Over the unpacked little-endian 16-bit pixels, not the TIFF bytes: the encoder's layout
        // does not matter, only what the pipeline will read.
        public static string Sha256Hex(byte[] pixels) => Convert.ToHexString(SHA256.HashData(pixels));
    }

    public sealed record LsoCaptureReflectedLight
    {
        public bool Enabled { get; init; }
        public bool Applied { get; init; }
        // Created stamp of the map set the capture used: a later recalibration shows up as a difference.
        public DateTime? SetCreated { get; init; }
        public int? Position { get; init; }
        public int? Lower { get; init; }
        public int? Upper { get; init; }
        public double? Weight { get; init; }
        public string Skipped { get; init; } = string.Empty;
    }

    public sealed record LsoCaptureFlatField
    {
        public bool Applied { get; init; }
        public string File { get; init; } = string.Empty;
    }

    public sealed record LsoCaptureFile(string Name, string Pixels, string? Sha256);

    public sealed record LsoCaptureFiles
    {
        public LsoCaptureFile? Raw16 { get; init; }
        public LsoCaptureFile? Uncorrected16 { get; init; }
        public LsoCaptureFile? Rgb48 { get; init; }
    }
}
