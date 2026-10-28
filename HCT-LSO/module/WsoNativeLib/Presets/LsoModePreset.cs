using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Xml;
using System.Xml.Linq;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // The light a mode drives, as the preset stores it. Intensity 0 means "leave whatever
    // the board already has" - the same rule the capture/preview request uses, so a preset
    // that was never tuned cannot darken a working instrument.
    public sealed class LsoModeLight
    {
        public ushort Intensity { get; set; }

        // (int)LightMode - 0 = Trigger, 1 = Continuous.
        public int Mode { get; set; } = (int)LightMode.Trigger;

        // (int)LightType - WHICH lamp this mode switches on. Stored per preset because the
        // optical path is: a FAF preset already moves the filter wheel to Blue/Green, and
        // lighting that path with the white LED produces a black image. Files written before
        // this field take the mode's built-in light (white for Capture, retina IR for
        // Preview), so an existing preset behaves exactly as it did.
        public int Type { get; set; } = (int)LightType.LsoWhite;

        public LightType Led => (LightType)Type;

        public void CopyValuesFrom(LsoModeLight source)
        {
            Intensity = source.Intensity;
            Mode = source.Mode;
            Type = source.Type;
        }
    }

    // The motor positions a mode images through. The LSO filter wheel selects the optical
    // path (0 = Color, 1 = BlueFaf, 2 = GreenFaf, 3 = Icga), and it is moved by mode origin,
    // not by step position, so the calibrated position the board holds stays the authority.
    //
    // NotStated (-1) means "leave this motor where it is": a preset that was never tuned must
    // not drag the focus motor to 0 on the next Preview. The filter wheel defaults to Color
    // instead, because that is the path both shipped modes image through.
    public sealed class LsoModeMotor
    {
        public const int NotStated = -1;
        public const int ColorFilterMode = 0;

        // (int)LsoFilterMode - 0 = Color, 1 = BlueFaf, 2 = GreenFaf, 3 = Icga, -1 = do not move.
        public int LsoFilterMode { get; set; } = ColorFilterMode;

        // Step position for MotorType.LsoFocus, or -1 to leave the current position.
        public int LsoFocusPosition { get; set; } = NotStated;

        public void CopyValuesFrom(LsoModeMotor source)
        {
            LsoFilterMode = source.LsoFilterMode;
            LsoFocusPosition = source.LsoFocusPosition;
        }

        public bool ValueEquals(LsoModeMotor other)
        {
            return LsoFilterMode == other.LsoFilterMode
                && LsoFocusPosition == other.LsoFocusPosition;
        }
    }

    // What is done to a captured frame after it leaves the camera. Only Capture uses it; the
    // aiming Preview is never corrected. Off unless the preset turns it on, so every file
    // written before this section existed keeps producing the raw image it always did.
    public sealed class LsoModePostProcess
    {
        // Divide the raw Bayer frame by a flat reference (LsoFlatField).
        public bool FlatFieldEnabled { get; set; }

        // Raw Bayer PGM of a uniform target, shot with this preset's geometry.
        public string FlatFieldFile { get; set; } = string.Empty;

        // Subtract this instrument's measured reflected-light map from White captures; the
        // maps live in <config dir>\ReflectedLight (LsoReflectedLight).
        public bool ReflectedLightEnabled { get; set; }

        public void CopyValuesFrom(LsoModePostProcess source)
        {
            FlatFieldEnabled = source.FlatFieldEnabled;
            FlatFieldFile = source.FlatFieldFile;
            ReflectedLightEnabled = source.ReflectedLightEnabled;
        }
    }

    // One mode preset = everything a Preview or a Capture run needs: the color camera
    // geometry, the scanner (galvo) profile, the illumination and the motor positions.
    // One XML file per preset.
    //
    // The mode is NOT stored in the file: any preset can be selected for either mode. The
    // mode alone decides which scanner slot it is uploaded to and which LED it drives, so a
    // preset never has to agree with the slot it happens to be sitting in.
    public sealed class LsoModePreset
    {
        public const string FileExtension = ".xml";
        public const string RootElementName = "LsoModePreset";

        // v2 = camera(14 + trigger delay) + scan + light. v3 adds the A-1 camera extension.
        // v4 adds the <Motor> section, v5 the light's own <Type>. An older file is not
        // rewritten or discarded on load:
        // everything it carries is kept and only the fields it could not carry take the
        // built-in DevKit defaults, reported as one migration note per missing section rather
        // than a wall of per-field warnings.
        public const int FormatVersion = 5;

        // The first version whose document carries <Motor>. Below it the section's absence is
        // a migration, not damage.
        public const int FirstMotorFormatVersion = 4;

        // The first mode-preset version whose <Camera> section carries the A-1 extension.
        // The two files version independently, so a mode preset older than this holds a
        // camera section equivalent to a v1 standalone camera preset - and that is what has
        // to be handed to the camera reader, not this file's own number.
        public const int FirstExtendedFormatVersion = 3;

        // Ramps were added during v4 without a format bump. Only v5 and later are
        // guaranteed to contain them; pre-ramp v4 files still need migration defaults.
        private const int FirstScanRampFormatVersion = 5;

        public string Name { get; set; } = string.Empty;
        public ColorIllumination ColorIllumination { get; set; } = ColorIllumination.White;

        // Blue-FAF emission composite. Absent in every file written before the option
        // existed, and absence is the GreenOnly default - a migration, not damage.
        public FafProcessingOption FafOption { get; set; } = FafProcessingOption.GreenOnly;

        public ColorCameraPreset Camera { get; } = new ColorCameraPreset();

        public LsoScanProfileModel Scan { get; } = new LsoScanProfileModel();

        public LsoModeLight Light { get; } = new LsoModeLight();

        public LsoModeMotor Motor { get; } = new LsoModeMotor();

        public LsoModePostProcess PostProcess { get; } = new LsoModePostProcess();

        // Scanner slot the mode runs on: preview = 0, capture = 1, the same arrangement the
        // DevKit writes and starts (LsoScanFormUnified.selected_tarj_profile_id).
        public static int ScanPatternId(ColorCameraPresetMode mode)
        {
            return mode == ColorCameraPresetMode.Preview
                ? LsoScanPreviewPreset.PatternId
                : LsoScanCapturePreset.PatternId;
        }

        // The light a mode drives when its preset does not state one: the built-in default a
        // new or pre-v5 preset starts from. What actually runs is the preset's Light.Type.
        public static LightType ModeLight(ColorCameraPresetMode mode)
        {
            return mode == ColorCameraPresetMode.Preview ? LightType.RetinaIr : LightType.LsoWhite;
        }

        public static LsoModePreset DevKitDefault(ColorCameraPresetMode mode, string name)
        {
            var preset = new LsoModePreset { Name = name };
            preset.Camera.CopyValuesFrom(ColorCameraPreset.DevKitDefault(mode, name));
            preset.Camera.Name = name;

            if (mode == ColorCameraPresetMode.Preview)
            {
                // The shipped aiming profile; its board exposure follows the camera preset.
                LsoScanPreviewPreset.ApplyTo(preset.Scan, (int)preset.Camera.ExposureTime);
            }
            else
            {
                LsoScanCapturePreset.ApplyTo(preset.Scan, (int)preset.Camera.ExposureTime);
            }

            preset.Light.Type = (int)ModeLight(mode);
            return preset;
        }

        // The capture scanner profile as WsoDeviceCfg.ini shipped it in [LSO_SCANNER_0].
        // Kept here because that section is no longer read once a preset exists.
        public static void ApplyCaptureScanDefaults(LsoScanProfileModel scan)
        {
            LsoScanCapturePreset.ApplyTo(scan, 5629);
        }

        public LsoModePreset Clone(string name)
        {
            var copy = new LsoModePreset { Name = name };
            copy.CopyValuesFrom(this);
            copy.Camera.Name = name;
            return copy;
        }

        public void CopyValuesFrom(LsoModePreset source)
        {
            ColorIllumination = source.ColorIllumination;
            FafOption = source.FafOption;
            Camera.CopyValuesFrom(source.Camera);
            Scan.CopyValuesFrom(source.Scan);
            Light.CopyValuesFrom(source.Light);
            Motor.CopyValuesFrom(source.Motor);
            PostProcess.CopyValuesFrom(source.PostProcess);
        }

        // A referenced file (trajectory CSV, flat PGM) that sits in the preset's own folder is
        // written by name only, so the Presets folder can move with its files - another PC, or
        // the shared folder in the repository. Load resolves such a name against the folder
        // the preset is read from, so in memory the path is always absolute.
        public void Save(string path)
        {
            string? directory = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }

            string? folder = string.IsNullOrEmpty(directory) ? null : Path.GetFullPath(directory);
            new XDocument(new XDeclaration("1.0", "utf-8", null), ToElement(folder)).Save(path);
        }

        // Same values as the file would store them - the comparison the "unsaved changes"
        // check needs, without a second field list that could fall out of step with Save.
        public bool ValueEquals(LsoModePreset other)
        {
            return XNode.DeepEquals(ToElement(null), other.ToElement(null));
        }

        private XElement ToElement(string? folder)
        {
            return new XElement(RootElementName,
                new XAttribute("version", FormatVersion),
                new XAttribute("name", Name),
                new XElement(nameof(ColorIllumination), ColorIllumination),
                new XElement(nameof(FafOption), FafOption),
                Camera.ToElement("Camera"),
                ScanElement(folder),
                LightElement(),
                MotorElement(),
                PostProcessElement(folder));
        }

        private static string RelativeToFolder(string file, string? folder)
        {
            if (file.Length == 0 || folder == null || !Path.IsPathRooted(file))
            {
                return file;
            }

            string fileFolder = Path.GetDirectoryName(Path.GetFullPath(file)) ?? string.Empty;
            return string.Equals(Path.TrimEndingDirectorySeparator(fileFolder),
                    Path.TrimEndingDirectorySeparator(folder), StringComparison.OrdinalIgnoreCase)
                ? Path.GetFileName(file)
                : file;
        }

        private static string ResolveInFolder(string file, string? folder)
        {
            return file.Length == 0 || folder == null || Path.IsPathRooted(file)
                ? file
                : Path.GetFullPath(Path.Combine(folder, file));
        }

        // Anything unreadable falls back to the mode's built-in default and is appended to
        // `issues` - a preset that silently became something else is the failure this store
        // exists to avoid.
        public static LsoModePreset Load(string path, ColorCameraPresetMode fallbackMode, IList<string> issues)
        {
            return Load(path, fallbackMode, issues, out _);
        }

        // `hasFallback` is narrower than `issues`: it reports values substituted because the
        // selected file is damaged, while declared legacy migrations and exposure
        // normalisation remain informational notes.
        internal static LsoModePreset Load(string path, ColorCameraPresetMode fallbackMode,
            IList<string> issues, out bool hasFallback)
        {
            string name = Path.GetFileNameWithoutExtension(path);
            LsoModePreset preset = DevKitDefault(fallbackMode, name);
            hasFallback = false;

            XElement? root;
            try
            {
                root = XDocument.Load(path).Root;
            }
            catch (Exception e) when (e is XmlException || e is IOException || e is UnauthorizedAccessException)
            {
                issues?.Add($"'{name}': {e.Message} - the built-in {fallbackMode} defaults are used instead.");
                hasFallback = true;
                return preset;
            }

            if (root == null || root.Name.LocalName != RootElementName)
            {
                issues?.Add($"'{name}': not a {RootElementName} document - the built-in {fallbackMode} defaults are used instead.");
                hasFallback = true;
                return preset;
            }

            // The mode preset's own version decides how its <Camera> section is read: a v2
            // document has no A-1 extension, and that is a migration rather than damage.
            // Translated into the camera preset's own numbering, because the two files
            // version independently.
            int fileVersion = ColorCameraPreset.ReadFormatVersion(root);
            XElement? illumination = root.Element(nameof(ColorIllumination));
            if (illumination != null)
            {
                if (Enum.TryParse(illumination.Value.Trim(), out ColorIllumination parsedIllumination)
                    && Enum.IsDefined(parsedIllumination))
                {
                    preset.ColorIllumination = parsedIllumination;
                }
                else
                {
                    issues?.Add($"'{name}': ColorIllumination is invalid - White is used.");
                    hasFallback = true;
                }
            }
            // Absent in every file written before the Blue-FAF option existed: the mode's
            // GreenOnly default, without a migration note.
            XElement? fafOption = root.Element(nameof(FafOption));
            if (fafOption != null)
            {
                if (Enum.TryParse(fafOption.Value.Trim(), out FafProcessingOption parsedFafOption)
                    && Enum.IsDefined(parsedFafOption))
                {
                    preset.FafOption = parsedFafOption;
                }
                else
                {
                    issues?.Add($"'{name}': FafOption is invalid - GreenOnly is used.");
                    hasFallback = true;
                }
            }
            XAttribute? declaredVersion = root.Attribute("version");
            if (declaredVersion != null
                && (!int.TryParse(declaredVersion.Value.Trim(), NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out int parsedVersion) || parsedVersion <= 0))
            {
                issues?.Add($"'{name}': the declared format version is invalid.");
                hasFallback = true;
                return preset;
            }
            int cameraVersion = fileVersion < FirstExtendedFormatVersion
                ? 1
                : ColorCameraPreset.FormatVersion;

            XElement? camera = root.Element("Camera");
            if (camera == null)
            {
                issues?.Add($"'{name}': <Camera> is missing - the built-in {fallbackMode} camera defaults are used.");
                hasFallback = true;
            }
            else
            {
                preset.Camera.ReadFrom(camera, name, issues, cameraVersion, out bool cameraFallback);
                hasFallback |= cameraFallback;
            }

            XElement? scan = root.Element("Scan");
            if (scan == null)
            {
                issues?.Add($"'{name}': <Scan> is missing - the built-in {fallbackMode} scan defaults are used.");
                hasFallback = true;
            }
            else
            {
                preset.ReadScan(scan, name, issues, fileVersion, out bool scanFallback);
                hasFallback |= scanFallback;
            }

            XElement? light = root.Element("Light");
            if (light == null)
            {
                issues?.Add($"'{name}': <Light> is missing - the built-in {fallbackMode} light defaults are used.");
                hasFallback = true;
            }
            else
            {
                int issueCountBeforeLight = issues?.Count ?? 0;
                preset.Light.Intensity = (ushort)ReadInt(light, nameof(LsoModeLight.Intensity), preset.Light.Intensity, name, issues);
                preset.Light.Mode = ReadInt(light, nameof(LsoModeLight.Mode), preset.Light.Mode, name, issues);

                // A pre-v5 file may omit Type, but an explicitly damaged value must never
                // silently select another lamp. Current files must state the lamp.
                if (fileVersion >= 5 || light.Element(nameof(LsoModeLight.Type)) != null)
                {
                    preset.Light.Type = ReadInt(light, nameof(LsoModeLight.Type), preset.Light.Type, name, issues);
                }
                hasFallback |= (issues?.Count ?? 0) > issueCountBeforeLight;
            }

            XElement? motor = root.Element("Motor");
            if (motor == null)
            {
                issues?.Add($"'{name}': <Motor> is missing - the built-in {fallbackMode} motor "
                    + "defaults are used (LSO filter to Color, LSO focus left where it is).");
                hasFallback |= fileVersion >= FirstMotorFormatVersion;
            }
            else
            {
                int issueCountBeforeMotor = issues?.Count ?? 0;
                preset.Motor.LsoFilterMode = ReadInt(motor, nameof(LsoModeMotor.LsoFilterMode),
                    preset.Motor.LsoFilterMode, name, issues);
                preset.Motor.LsoFocusPosition = ReadInt(motor, nameof(LsoModeMotor.LsoFocusPosition),
                    preset.Motor.LsoFocusPosition, name, issues);
                hasFallback |= (issues?.Count ?? 0) > issueCountBeforeMotor;
            }

            // Optional, like TrajectoryFile: absent in every file written before it existed and
            // means "off". A stated but unreadable switch is damage, not a migration - it must
            // not silently turn the correction on or off.
            XElement? postProcess = root.Element("PostProcess");
            if (postProcess != null)
            {
                int issueCountBeforePostProcess = issues?.Count ?? 0;
                preset.PostProcess.FlatFieldEnabled = ReadInt(postProcess, nameof(LsoModePostProcess.FlatFieldEnabled),
                    0, name, issues) != 0;
                preset.PostProcess.FlatFieldFile =
                    postProcess.Element(nameof(LsoModePostProcess.FlatFieldFile))?.Value?.Trim() ?? string.Empty;

                // Optional like FlatFieldEnabled: absent in files written before the switch
                // existed and means "off", not a fallback.
                if (postProcess.Element(nameof(LsoModePostProcess.ReflectedLightEnabled)) != null)
                {
                    preset.PostProcess.ReflectedLightEnabled = ReadInt(postProcess,
                        nameof(LsoModePostProcess.ReflectedLightEnabled), 0, name, issues) != 0;
                }
                hasFallback |= (issues?.Count ?? 0) > issueCountBeforePostProcess;
            }

            string? folder = Path.GetDirectoryName(Path.GetFullPath(path));
            preset.Scan.TrajectoryFile = ResolveInFolder(preset.Scan.TrajectoryFile, folder);
            preset.Scan.CalibrationFile = ResolveInFolder(preset.Scan.CalibrationFile, folder);
            preset.PostProcess.FlatFieldFile = ResolveInFolder(preset.PostProcess.FlatFieldFile, folder);

            preset.Camera.Name = name;

            // The camera exposure is the authority (same rule as the DevKit, whose scan form
            // only displays the camera config's exposure). A scan value that disagrees is a
            // file from before that rule; it is normalised here and reported once so nothing
            // is silently replaced.
            if (preset.Camera.ExposureTime > 0 && preset.Scan.ExposureTimeUs != (int)preset.Camera.ExposureTime)
            {
                issues?.Add($"'{name}': <Scan> ExposureTimeUs {preset.Scan.ExposureTimeUs} replaced by the camera "
                    + $"exposure {preset.Camera.ExposureTime}us - the camera value is the authority.");
                preset.Scan.ExposureTimeUs = (int)preset.Camera.ExposureTime;
            }

            return preset;
        }

        private XElement MotorElement()
        {
            return new XElement("Motor",
                Int(nameof(LsoModeMotor.LsoFilterMode), Motor.LsoFilterMode),
                Int(nameof(LsoModeMotor.LsoFocusPosition), Motor.LsoFocusPosition));
        }

        private XElement PostProcessElement(string? folder)
        {
            return new XElement("PostProcess",
                Int(nameof(LsoModePostProcess.FlatFieldEnabled), PostProcess.FlatFieldEnabled ? 1 : 0),
                new XElement(nameof(LsoModePostProcess.FlatFieldFile), RelativeToFolder(PostProcess.FlatFieldFile, folder)),
                Int(nameof(LsoModePostProcess.ReflectedLightEnabled), PostProcess.ReflectedLightEnabled ? 1 : 0));
        }

        private XElement ScanElement(string? folder)
        {
            return new XElement("Scan",
                Int("TimeStepUs", Scan.TimeStepUs),
                Int("ExposureTimeUs", Scan.ExposureTimeUs),
                Int("TriggerSource", Scan.TriggerSource),
                Int("AcquisitionMode", Scan.AcquisitionMode),
                Int("AcqFrameSize", (int)Scan.AcqFrameSize),
                Int("SubFrameSize", Scan.SubFrameSize),
                Int("GalvoPatternSize", Scan.GalvoPatternSize),
                Int("GalvanoRewindOffset", Scan.GalvanoRewindOffset),
                Int("LedOnPosIndex", Scan.LedOnPosIndex),
                Int("LedOffPosIndex", Scan.LedOffPosIndex),
                Int("GalvoStartPos", Scan.GalvoStartPos),
                Int("GalvoEndPos", Scan.GalvoEndPos),
                Int("LeadInSamples", Scan.LeadInSamples),
                Int("LeadOutSamples", Scan.LeadOutSamples),
                Dbl("LineRateHz", Scan.LineRateHz),
                Int("SensorRows", Scan.SensorRows),
                Dbl("GalvoDelayUs", Scan.GalvoDelayUs),
                Dbl("GalvoDecelUs", Scan.GalvoDecelUs),
                Int("SlitHeightRows", Scan.SlitHeightRows),
                Int("SensorBlankingRows", Scan.SensorBlankingRows),
                Int("SequenceMode", (int)Scan.SequenceMode),
                Int("PreMoveOffset", Scan.PreMoveOffset),
                Int("PreMoveDelayUs", Scan.PreMoveDelayUs),
                Scan.RequestedPreMoveDelayUs.HasValue
                    ? Int("RequestedPreMoveDelayUs", Scan.RequestedPreMoveDelayUs.Value) : null,
                new XElement("TrajectoryFile", RelativeToFolder(Scan.TrajectoryFile, folder)),
                Int("TrajectoryMode", (int)Scan.TrajectoryMode),
                new XElement("CalibrationFile", RelativeToFolder(Scan.CalibrationFile, folder)));
        }

        private void ReadScan(XElement scan, string presetName, IList<string>? issues,
            int fileVersion, out bool hasFallback)
        {
            int issueCountBeforeFields = issues?.Count ?? 0;
            Scan.TimeStepUs = ReadInt(scan, "TimeStepUs", Scan.TimeStepUs, presetName, issues);
            Scan.ExposureTimeUs = ReadInt(scan, "ExposureTimeUs", Scan.ExposureTimeUs, presetName, issues);
            Scan.TriggerSource = (ushort)ReadInt(scan, "TriggerSource", Scan.TriggerSource, presetName, issues);
            Scan.AcquisitionMode = (ushort)ReadInt(scan, "AcquisitionMode", Scan.AcquisitionMode, presetName, issues);
            Scan.AcqFrameSize = (uint)ReadInt(scan, "AcqFrameSize", (int)Scan.AcqFrameSize, presetName, issues);
            Scan.SubFrameSize = (ushort)ReadInt(scan, "SubFrameSize", Scan.SubFrameSize, presetName, issues);
            Scan.GalvoPatternSize = (ushort)ReadInt(scan, "GalvoPatternSize", Scan.GalvoPatternSize, presetName, issues);
            Scan.GalvanoRewindOffset = (short)ReadInt(scan, "GalvanoRewindOffset", Scan.GalvanoRewindOffset, presetName, issues);
            Scan.LedOnPosIndex = (ushort)ReadInt(scan, "LedOnPosIndex", Scan.LedOnPosIndex, presetName, issues);
            Scan.LedOffPosIndex = (ushort)ReadInt(scan, "LedOffPosIndex", Scan.LedOffPosIndex, presetName, issues);
            Scan.GalvoStartPos = (short)ReadInt(scan, "GalvoStartPos", Scan.GalvoStartPos, presetName, issues);
            Scan.GalvoEndPos = (short)ReadInt(scan, "GalvoEndPos", Scan.GalvoEndPos, presetName, issues);
            hasFallback = (issues?.Count ?? 0) > issueCountBeforeFields;

            // Files written before the ramps existed report the fallback (30/30) as an issue,
            // on purpose: the trajectory they produce is no longer the plain sweep they were
            // tuned with, and the operator should see that once and re-save.
            int issueCountBeforeRamp = issues?.Count ?? 0;
            Scan.LeadInSamples = (ushort)ReadInt(scan, "LeadInSamples", Scan.LeadInSamples, presetName, issues);
            if ((issues?.Count ?? 0) > issueCountBeforeRamp
                && (fileVersion >= FirstScanRampFormatVersion || scan.Element("LeadInSamples") != null))
            {
                hasFallback = true;
            }

            issueCountBeforeRamp = issues?.Count ?? 0;
            Scan.LeadOutSamples = (ushort)ReadInt(scan, "LeadOutSamples", Scan.LeadOutSamples, presetName, issues);
            if ((issues?.Count ?? 0) > issueCountBeforeRamp
                && (fileVersion >= FirstScanRampFormatVersion || scan.Element("LeadOutSamples") != null))
            {
                hasFallback = true;
            }

            // The timing calculator's inputs, read QUIETLY: unlike everything above they never
            // reach the board and drive nothing on their own, so a file that predates them is
            // not a migration worth a note per field - it just starts from the defaults.
            Scan.LineRateHz = ReadDoubleQuiet(scan, "LineRateHz", Scan.LineRateHz);
            Scan.SensorRows = ReadIntQuiet(scan, "SensorRows", Scan.SensorRows);
            Scan.GalvoDelayUs = ReadDoubleQuiet(scan, "GalvoDelayUs", Scan.GalvoDelayUs);
            Scan.GalvoDecelUs = ReadDoubleQuiet(scan, "GalvoDecelUs", Scan.GalvoDecelUs);
            Scan.SlitHeightRows = ReadIntQuiet(scan, "SlitHeightRows", Scan.SlitHeightRows);
            Scan.SensorBlankingRows = ReadIntQuiet(scan, "SensorBlankingRows", Scan.SensorBlankingRows);

            // Sequence fields arrived during v5 without a format bump. Absence in an old
            // file means TRIG_FIRST, but malformed stated fields are acquisition fallbacks.
            int issueCountBeforeSequence = issues?.Count ?? 0;
            if (scan.Element("SequenceMode") != null)
            {
                Scan.SequenceMode = (LsoScanSequenceMode)ReadInt(scan, "SequenceMode",
                    (int)Scan.SequenceMode, presetName, issues);
            }
            if (scan.Element("PreMoveOffset") != null || Scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst)
            {
                Scan.PreMoveOffset = (short)ReadInt(scan, "PreMoveOffset", Scan.PreMoveOffset, presetName, issues);
            }
            if (scan.Element("PreMoveDelayUs") != null || Scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst)
            {
                Scan.PreMoveDelayUs = (ushort)ReadInt(scan, "PreMoveDelayUs", Scan.PreMoveDelayUs, presetName, issues);
            }
            Scan.RequestedPreMoveDelayUs = scan.Element("RequestedPreMoveDelayUs") != null
                ? (ushort)ReadInt(scan, "RequestedPreMoveDelayUs", Scan.PreMoveDelayUs, presetName, issues)
                : null;
            hasFallback |= (issues?.Count ?? 0) > issueCountBeforeSequence;

            // Optional; absent in every file written before it existed and means "linear".
            Scan.TrajectoryFile = scan.Element("TrajectoryFile")?.Value?.Trim() ?? string.Empty;

            // Optional as well; absent means Linear. A stated mode that is not one of the two is
            // damage - it must not silently decide which table the galvo sweeps.
            if (scan.Element("TrajectoryMode") != null)
            {
                int issueCountBeforeMode = issues?.Count ?? 0;
                int trajectoryMode = ReadInt(scan, "TrajectoryMode", (int)LsoTrajectoryMode.Linear, presetName, issues);
                if (!Enum.IsDefined(typeof(LsoTrajectoryMode), trajectoryMode))
                {
                    issues?.Add($"'{presetName}': <TrajectoryMode> {trajectoryMode} is not 0 (Linear) or 1 (CalibratedFov) "
                        + "- Linear is used.");
                    trajectoryMode = (int)LsoTrajectoryMode.Linear;
                }
                Scan.TrajectoryMode = (LsoTrajectoryMode)trajectoryMode;
                hasFallback |= (issues?.Count ?? 0) > issueCountBeforeMode;
            }
            Scan.CalibrationFile = scan.Element("CalibrationFile")?.Value?.Trim() ?? string.Empty;
        }

        private XElement LightElement()
        {
            return new XElement("Light",
                Int(nameof(LsoModeLight.Type), Light.Type),
                Int(nameof(LsoModeLight.Intensity), Light.Intensity),
                Int(nameof(LsoModeLight.Mode), Light.Mode));
        }

        private static XElement Int(string elementName, int value)
        {
            return new XElement(elementName, value.ToString(CultureInfo.InvariantCulture));
        }

        // "R" so a rate like 84889.6 survives the round trip unchanged.
        private static XElement Dbl(string elementName, double value)
        {
            return new XElement(elementName, value.ToString("R", CultureInfo.InvariantCulture));
        }

        private static int ReadIntQuiet(XElement parent, string elementName, int fallback)
        {
            XElement? element = parent.Element(elementName);
            return element != null
                && int.TryParse(element.Value.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int value)
                ? value
                : fallback;
        }

        private static double ReadDoubleQuiet(XElement parent, string elementName, double fallback)
        {
            XElement? element = parent.Element(elementName);
            return element != null
                && double.TryParse(element.Value.Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out double value)
                ? value
                : fallback;
        }

        internal static int ReadInt(XElement parent, string elementName, int fallback, string presetName, IList<string>? issues)
        {
            XElement? element = parent.Element(elementName);
            if (element == null)
            {
                issues?.Add($"'{presetName}': <{elementName}> is missing - the built-in default {fallback} is used.");
                return fallback;
            }

            if (!int.TryParse(element.Value.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int value))
            {
                issues?.Add($"'{presetName}': <{elementName}> is not a number ('{element.Value.Trim()}') - the built-in default {fallback} is used.");
                return fallback;
            }

            return value;
        }
    }
}
