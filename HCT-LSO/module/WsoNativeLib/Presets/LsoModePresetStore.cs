using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using WsoNativeLib.Utils;

namespace WsoNativeLib.Presets
{
    // File-backed store for the per-mode presets. One preset carries camera + scan + light,
    // so Preview and Capture each need exactly one selection instead of three.
    //
    //   <app>/Presets/<Name>.xml          one file per preset
    //   <app>/WsoDeviceCfg.ini  [MODE_PRESET]
    //                             PreviewPreset=<Name>
    //                             CapturePreset=<Name>          Color capture
    //                             CapturePreset.<Mode>=<Name>   other image modes (LsoImageMode)
    //                             MigratedFromV1=1     one-shot CameraPresets/ + [LSO_SCANNER_*] merge
    //
    // The Capture role reads and writes the preset of CaptureImageMode; every Capture-mode
    // call below (Select, SaveAs, CreateDefault, Delete fallback) acts on that mode's key.
    //
    // After the merge the scanner slots 0 and 1 live in the preset only: [LSO_SCANNER_0] and
    // [LSO_SCANNER_1] are deleted and never read again. Slots 2+ stay in the INI.
    public sealed class LsoModePresetStore
    {
        public const string DirectoryName = "Presets";
        public const string IniSectionName = "MODE_PRESET";
        public const string DefaultPreviewPresetName = "Preview";
        public const string DefaultCapturePresetName = "Capture";
        public const string DevKitPreviewPresetName = "PreviewDevKit20260909";
        public const string DevKitCapturePresetName = "CaptureDevKit20260909";

        private const string PreviewPresetKey = "PreviewPreset";
        private const string CapturePresetKey = "CapturePreset";
        private const string MigratedKey = "MigratedFromV1";
        private const string DevKit20260909AppliedKey = "DevKit20260909Applied";
        private const string ColorCameraIniSection = "COLOR_CAMERA";
        private const string CaptureTriggerModeKey = "CaptureTriggerMode";
        private const int HardwareTriggerMode = 1;
        private const string MissingSelectionMarker = "*MISSING_MODE_PRESET_SELECTION*";

        // v1 layout, read once by the migration below.
        private const string LegacyCameraDirectoryName = "CameraPresets";
        private const string LegacyCameraIniSection = "CAMERA_PRESET";

        private readonly string _directory;
        private readonly string _configIniPath;
        private readonly List<string> _issues = new List<string>();

        public LsoModePresetStore(string directory, string configIniPath)
        {
            _directory = directory ?? throw new ArgumentNullException(nameof(directory));
            _configIniPath = configIniPath ?? throw new ArgumentNullException(nameof(configIniPath));
        }

        // The shared configuration location, so the Toolkit that writes these files and the
        // Agent that reads them land on the same folder (see LsoConfigLocation).
        public static string DefaultDirectory => LsoConfigLocation.PresetsDirectory();

        public string Directory => _directory;

        public LsoModePreset Preview { get; private set; } =
            LsoModePreset.DevKitDefault(ColorCameraPresetMode.Preview, DefaultPreviewPresetName);

        public LsoModePreset Capture { get; private set; } =
            LsoModePreset.DevKitDefault(ColorCameraPresetMode.Capture, DefaultCapturePresetName);

        public LsoModePreset Active(ColorCameraPresetMode mode)
        {
            return mode == ColorCameraPresetMode.Preview ? Preview : Capture;
        }

        // Which image mode the Capture role is bound to. Not persisted: the Toolkit opens on
        // Color, and the Agent names the mode per acquisition (LoadReadOnly(mode)).
        public LsoImageMode CaptureImageMode { get; private set; } = LsoImageMode.Color;

        // Toolkit: binds the Capture role to another image mode. A mode that was never set up
        // gets its seed preset (DevKit capture values + the mode's filter and lamp), the same
        // way Load() seeds "Capture".
        public void SelectCaptureImageMode(LsoImageMode mode)
        {
            CaptureImageMode = mode;
            SeedDefault(ColorCameraPresetMode.Capture, DefaultName(ColorCameraPresetMode.Capture));
            Capture = LoadSelected(ColorCameraPresetMode.Capture);
        }

        private string DefaultName(ColorCameraPresetMode mode)
        {
            return mode == ColorCameraPresetMode.Preview
                ? DefaultPreviewPresetName
                : LsoImageModes.DefaultCapturePresetName(CaptureImageMode);
        }

        private string SelectionKey(ColorCameraPresetMode mode)
        {
            return mode == ColorCameraPresetMode.Preview
                ? PreviewPresetKey
                : LsoImageModes.CaptureSelectionKey(CaptureImageMode);
        }

        // A new preset for the role; a Capture preset of a non-Color mode starts on that
        // mode's optical path instead of the white-light one.
        private LsoModePreset NewSeed(ColorCameraPresetMode mode, string presetName)
        {
            LsoModePreset seed = LsoModePreset.DevKitDefault(mode, presetName);
            if (mode == ColorCameraPresetMode.Capture && CaptureImageMode != LsoImageMode.Color)
            {
                LsoImageModes.ApplySeedOptics(CaptureImageMode, seed);
            }
            return seed;
        }

        public IReadOnlyList<string> Issues => _issues;

        public bool HasIssues => _issues.Count > 0;

        // Status of the most recent read-only load. Unlike Issues, this excludes harmless
        // legacy migration and exposure-normalisation notes, so a consumer can reject only
        // loads that actually substituted unselected or damaged configuration.
        public bool HasLoadFallback { get; private set; }

        public IReadOnlyList<string> DrainIssues()
        {
            var drained = _issues.ToArray();
            _issues.Clear();
            return drained;
        }

        public void Load()
        {
            CaptureImageMode = LsoImageMode.Color;
            EnsureDirectory();
            MigrateV1();
            SeedDefault(ColorCameraPresetMode.Preview, DefaultPreviewPresetName);
            SeedDefault(ColorCameraPresetMode.Capture, DefaultCapturePresetName);
            InstallDevKit20260909Once();

            Preview = LoadSelected(ColorCameraPresetMode.Preview);
            Capture = LoadSelected(ColorCameraPresetMode.Capture);
        }

        // Read-only consumption for a process that does NOT own these files - the acquisition
        // Agent. It creates no folder, seeds no preset, runs no v1 migration and writes no
        // selection back, because the Toolkit is the only author of this configuration.
        //
        // Anything unreadable falls back to the built-in DevKit contract IN MEMORY and is
        // reported through Issues: a consumer must never image with values nobody chose, and
        // never do it silently.
        public void LoadReadOnly()
        {
            LoadReadOnly(LsoImageMode.Color);
        }

        // Capture is read from the given image mode's key. A mode the Toolkit never mapped is
        // a missing selection, so it falls back and HasLoadFallback refuses it.
        public void LoadReadOnly(LsoImageMode captureImageMode)
        {
            CaptureImageMode = captureImageMode;
            HasLoadFallback = false;
            Preview = LoadSelectedReadOnly(ColorCameraPresetMode.Preview);
            Capture = LoadSelectedReadOnly(ColorCameraPresetMode.Capture);
        }

        private LsoModePreset LoadSelectedReadOnly(ColorCameraPresetMode mode)
        {
            string defaultName = DefaultName(mode);

            string selected = ReadSelectedNameReadOnly(mode, defaultName, out bool selectionFallback);
            HasLoadFallback |= selectionFallback;
            if (!IsValidName(selected))
            {
                _issues.Add($"The selected {mode} preset name '{selected}' is not usable - "
                    + $"'{defaultName}' is read instead.");
                HasLoadFallback = true;
                selected = defaultName;
            }

            string path = PathOf(selected);
            if (!File.Exists(path))
            {
                _issues.Add($"The {mode} preset '{selected}' is not in '{_directory}' - the "
                    + $"built-in DevKit {mode} defaults are used for this session. Create and "
                    + "tune it in the Toolkit so this instrument runs approved values.");
                HasLoadFallback = true;
                return LsoModePreset.DevKitDefault(mode, selected);
            }

            LsoModePreset preset = LsoModePreset.Load(path, mode, _issues, out bool presetFallback);
            HasLoadFallback |= presetFallback;
            return preset;
        }

        // Never create an INI, even if it disappears between the existence check and open.
        private string ReadSelectedNameReadOnly(ColorCameraPresetMode mode, string defaultName,
            out bool hasFallback)
        {
            if (!File.Exists(_configIniPath))
            {
                _issues.Add($"'{_configIniPath}' does not exist - the {mode} preset selection "
                    + $"falls back to '{defaultName}'.");
                hasFallback = true;
                return defaultName;
            }

            try
            {
                var ini = new IniFileUtil(_configIniPath, createIfMissing: false);
                string value = ini.ReadString(IniSectionName, SelectionKey(mode), MissingSelectionMarker);
                if (string.Equals(value, MissingSelectionMarker, StringComparison.Ordinal)
                    || string.IsNullOrWhiteSpace(value))
                {
                    _issues.Add($"The {mode} preset selection is missing from '{_configIniPath}' - "
                        + $"it falls back to '{defaultName}'.");
                    hasFallback = true;
                    return defaultName;
                }

                hasFallback = false;
                return value.Trim();
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException)
            {
                _issues.Add($"The active {mode} preset name could not be read from '{_configIniPath}': {e.Message}");
                hasFallback = true;
                return defaultName;
            }
        }

        public IReadOnlyList<string> EnumeratePresetNames()
        {
            try
            {
                if (!System.IO.Directory.Exists(_directory))
                {
                    return Array.Empty<string>();
                }

                return System.IO.Directory
                    .EnumerateFiles(_directory, "*" + LsoModePreset.FileExtension)
                    .Select(Path.GetFileNameWithoutExtension)
                    .Where(name => !string.IsNullOrEmpty(name))
                    .Select(name => name!)
                    .OrderBy(name => name, StringComparer.OrdinalIgnoreCase)
                    .ToArray();
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                _issues.Add($"Preset folder '{_directory}' could not be listed: {e.Message}");
                return Array.Empty<string>();
            }
        }

        public string PathOf(string presetName)
        {
            return Path.Combine(_directory, presetName + LsoModePreset.FileExtension);
        }

        public bool Exists(string presetName)
        {
            return IsValidName(presetName) && File.Exists(PathOf(presetName));
        }

        public static bool IsDefaultName(string? presetName)
        {
            return string.Equals(presetName, DefaultPreviewPresetName, StringComparison.OrdinalIgnoreCase)
                || LsoImageModes.All.Any(mode => string.Equals(presetName,
                    LsoImageModes.DefaultCapturePresetName(mode), StringComparison.OrdinalIgnoreCase))
                || IsVersionedDevKitName(presetName, DevKitPreviewPresetName)
                || IsVersionedDevKitName(presetName, DevKitCapturePresetName);
        }

        private static bool IsVersionedDevKitName(string? presetName, string baseName)
        {
            if (string.Equals(presetName, baseName, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            if (presetName == null
                || !presetName.StartsWith(baseName, StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }

            string suffix = presetName.Substring(baseName.Length);
            return suffix.Length > 0 && suffix.All(char.IsDigit);
        }

        // A preset name is a file name: reject anything that is not one instead of letting
        // Path.Combine turn it into a write somewhere else.
        public static bool IsValidName(string? presetName)
        {
            if (string.IsNullOrWhiteSpace(presetName) || presetName!.Length > 64)
            {
                return false;
            }

            if (presetName != presetName.Trim())
            {
                return false;
            }

            return presetName.IndexOfAny(Path.GetInvalidFileNameChars()) < 0;
        }

        public bool Select(ColorCameraPresetMode mode, string presetName)
        {
            if (!IsValidName(presetName))
            {
                _issues.Add($"'{presetName}' is not a usable preset name.");
                return false;
            }

            string path = PathOf(presetName);
            if (!File.Exists(path))
            {
                _issues.Add($"Preset '{presetName}' does not exist in '{_directory}'.");
                return false;
            }

            LsoModePreset selected = LsoModePreset.Load(path, mode, _issues);
            if (!WriteSelectedName(mode, presetName))
            {
                return false;
            }
            SetActive(mode, selected);
            return true;
        }

        public bool Save(ColorCameraPresetMode mode)
        {
            LsoModePreset preset = Active(mode);
            return WritePreset(preset, preset.Name);
        }

        // Adds a preset: writes the mode's current values under a new name and selects it.
        public bool SaveAs(ColorCameraPresetMode mode, string presetName)
        {
            if (!IsValidName(presetName))
            {
                _issues.Add($"'{presetName}' is not a usable preset name.");
                return false;
            }

            LsoModePreset copy = Active(mode).Clone(presetName);
            if (!WritePreset(copy, presetName))
            {
                return false;
            }

            if (!WriteSelectedName(mode, presetName))
            {
                return false;
            }
            SetActive(mode, copy);
            return true;
        }

        // Adds a preset seeded from the mode's built-in defaults instead of the current values.
        public bool CreateDefault(ColorCameraPresetMode mode, string presetName)
        {
            if (!IsValidName(presetName))
            {
                _issues.Add($"'{presetName}' is not a usable preset name.");
                return false;
            }

            LsoModePreset seed = NewSeed(mode, presetName);
            if (!WritePreset(seed, presetName))
            {
                return false;
            }

            if (!WriteSelectedName(mode, presetName))
            {
                return false;
            }
            SetActive(mode, seed);
            return true;
        }

        // The two shipped names are the fallback every other path lands on, so they stay.
        public bool Delete(string presetName)
        {
            if (!IsValidName(presetName))
            {
                _issues.Add($"'{presetName}' is not a usable preset name.");
                return false;
            }

            if (IsDefaultName(presetName))
            {
                _issues.Add($"'{presetName}' is a built-in preset and cannot be deleted.");
                return false;
            }

            try
            {
                File.Delete(PathOf(presetName));
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                _issues.Add($"Preset '{presetName}' could not be deleted: {e.Message}");
                return false;
            }

            // A mode left pointing at the deleted file falls back to its shipped preset.
            FallBackIfSelected(ColorCameraPresetMode.Preview, presetName, DefaultPreviewPresetName);
            FallBackIfSelected(ColorCameraPresetMode.Capture, presetName,
                DefaultName(ColorCameraPresetMode.Capture));
            return true;
        }

        // True when the mode's in-memory preset differs from its file, i.e. selecting another
        // preset would throw edits away. ROI maxima are the camera's read-back, not an edit,
        // so they are excluded the same way HasDevKitCameraAndScanValues excludes them.
        public bool HasUnsavedChanges(ColorCameraPresetMode mode)
        {
            LsoModePreset active = Active(mode);
            string path = PathOf(active.Name);
            if (!IsValidName(active.Name) || !File.Exists(path))
            {
                return true;
            }

            LsoModePreset saved = LsoModePreset.Load(path, mode, new List<string>());
            saved.Camera.RoiMaxWidth = active.Camera.RoiMaxWidth;
            saved.Camera.RoiMaxHeight = active.Camera.RoiMaxHeight;
            return !active.ValueEquals(saved);
        }

        public bool Reload(ColorCameraPresetMode mode)
        {
            return Select(mode, Active(mode).Name);
        }

        private void FallBackIfSelected(ColorCameraPresetMode mode, string deletedName, string defaultName)
        {
            if (!string.Equals(Active(mode).Name, deletedName, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            SeedDefault(mode, defaultName);
            Select(mode, defaultName);
        }

        private void SetActive(ColorCameraPresetMode mode, LsoModePreset preset)
        {
            if (mode == ColorCameraPresetMode.Preview)
            {
                Preview = preset;
            }
            else
            {
                Capture = preset;
            }
        }

        private bool WritePreset(LsoModePreset preset, string presetName)
        {
            try
            {
                preset.Save(PathOf(presetName));
                return true;
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                _issues.Add($"Preset '{presetName}' could not be written: {e.Message}");
                return false;
            }
        }

        private void EnsureDirectory()
        {
            try
            {
                if (!System.IO.Directory.Exists(_directory))
                {
                    System.IO.Directory.CreateDirectory(_directory);
                }
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                _issues.Add($"Preset folder '{_directory}' could not be created: {e.Message}");
            }
        }

        private void SeedDefault(ColorCameraPresetMode mode, string presetName)
        {
            if (File.Exists(PathOf(presetName)))
            {
                return;
            }

            LsoModePreset seed = NewSeed(mode, presetName);
            if (WritePreset(seed, presetName))
            {
                _issues.Add($"Preset '{presetName}' was missing and has been recreated from the built-in {mode} (DevKit) defaults.");
            }
        }

        private void InstallDevKit20260909Once()
        {
            try
            {
                var ini = new IniFileUtil(_configIniPath);
                if (ini.ReadInt(IniSectionName, DevKit20260909AppliedKey, 0) != 0)
                {
                    return;
                }

                string previewName = FindOrCreateExactDevKitPreset(
                    ColorCameraPresetMode.Preview, DevKitPreviewPresetName);
                if (previewName.Length == 0)
                {
                    _issues.Add("The DevKit 20260909 Preview preset could not be installed; existing selections were preserved.");
                    return;
                }

                string captureName = FindOrCreateExactDevKitPreset(
                    ColorCameraPresetMode.Capture, DevKitCapturePresetName);
                if (captureName.Length == 0)
                {
                    _issues.Add("The DevKit 20260909 Capture preset could not be installed; existing selections were preserved.");
                    return;
                }

                if (!WriteDevKitSelectionsAtomically(previewName, captureName))
                {
                    _issues.Add($"The DevKit 20260909 presets were created, but their selections could not be fully saved to '{_configIniPath}'.");
                    return;
                }

                _issues.Add($"Preview='{previewName}' and Capture='{captureName}' now use the DevKit 20260909 presets; Capture uses HWTrigger. Existing preset files were preserved.");
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException)
            {
                _issues.Add($"The DevKit 20260909 presets could not be installed: {e.Message}");
            }
        }

        private bool WriteDevKitSelectionsAtomically(string previewName, string captureName)
        {
            string stagedPath = _configIniPath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                File.Copy(_configIniPath, stagedPath, true);
                var staged = new IniFileUtil(stagedPath);
                bool written = staged.WriteString(IniSectionName, PreviewPresetKey, previewName)
                    && staged.WriteString(IniSectionName, CapturePresetKey, captureName)
                    && staged.WriteInt(ColorCameraIniSection, CaptureTriggerModeKey, HardwareTriggerMode);
                bool matches = written
                    && string.Equals(staged.ReadString(IniSectionName, PreviewPresetKey), previewName,
                        StringComparison.Ordinal)
                    && string.Equals(staged.ReadString(IniSectionName, CapturePresetKey), captureName,
                        StringComparison.Ordinal)
                    && staged.ReadInt(ColorCameraIniSection, CaptureTriggerModeKey, 0) == HardwareTriggerMode;
                if (!matches || !staged.WriteInt(IniSectionName, DevKit20260909AppliedKey, 1))
                {
                    return false;
                }

                File.Replace(stagedPath, _configIniPath, null);
                return true;
            }
            finally
            {
                try
                {
                    File.Delete(stagedPath);
                }
                catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
                {
                    _issues.Add($"Temporary preset configuration '{stagedPath}' could not be removed: {e.Message}");
                }
            }
        }

        private string FindOrCreateExactDevKitPreset(ColorCameraPresetMode mode, string baseName)
        {
            for (int suffix = 1; suffix <= 100; ++suffix)
            {
                string name = suffix == 1 ? baseName : baseName + suffix;
                string path = PathOf(name);
                LsoModePreset exact = LsoModePreset.DevKitDefault(mode, name);

                if (!File.Exists(path))
                {
                    if (!WritePreset(exact, name))
                    {
                        return string.Empty;
                    }
                }

                var loadIssues = new List<string>();
                LsoModePreset existing = LsoModePreset.Load(path, mode, loadIssues);
                if (loadIssues.Count == 0 && IsExactDevKitPreset(existing, exact))
                {
                    return name;
                }
            }

            _issues.Add($"No unused built-in preset name was available for '{baseName}'.");
            return string.Empty;
        }

        private static bool IsExactDevKitPreset(LsoModePreset candidate, LsoModePreset exact)
        {
            // roi_max_* is the camera's own read-back - it halves with 2x2 binning - so it is
            // not part of preset identity. Comparing it made this check fail on every launch
            // (binned vs unbinned max), which installed and selected a new versioned copy each
            // time and silently dropped the previous session's preset.
            return HasDevKitCameraAndScanValues(candidate, exact)
                && candidate.Light.Intensity == exact.Light.Intensity
                && candidate.Light.Mode == exact.Light.Mode
                && candidate.Motor.ValueEquals(exact.Motor);
        }

        // Runtime verification compares the selected preset with the authoritative values
        // for that mode while leaving operator-owned light mode and motor positions alone.
        // ROI maxima are device read-back, so they are excluded exactly as they are from the
        // installer's semantic identity check above.
        public static bool HasDevKitCameraAndScanValues(
            ColorCameraPresetMode mode, LsoModePreset candidate)
        {
            if (candidate == null)
            {
                return false;
            }

            return HasDevKitCameraAndScanValues(
                candidate, LsoModePreset.DevKitDefault(mode, candidate.Name));
        }

        private static bool HasDevKitCameraAndScanValues(
            LsoModePreset candidate, LsoModePreset exact)
        {
            LsoModePreset normalized = exact.Clone(candidate.Name);
            normalized.Camera.RoiMaxWidth = candidate.Camera.RoiMaxWidth;
            normalized.Camera.RoiMaxHeight = candidate.Camera.RoiMaxHeight;

            return candidate.Camera.ValueEquals(normalized.Camera)
                && candidate.Scan.TimeStepUs == exact.Scan.TimeStepUs
                && candidate.Scan.ExposureTimeUs == exact.Scan.ExposureTimeUs
                && candidate.Scan.TriggerSource == exact.Scan.TriggerSource
                && candidate.Scan.AcquisitionMode == exact.Scan.AcquisitionMode
                && candidate.Scan.AcqFrameSize == exact.Scan.AcqFrameSize
                && candidate.Scan.SubFrameSize == exact.Scan.SubFrameSize
                && candidate.Scan.GalvoPatternSize == exact.Scan.GalvoPatternSize
                && candidate.Scan.GalvanoRewindOffset == exact.Scan.GalvanoRewindOffset
                && candidate.Scan.LedOnPosIndex == exact.Scan.LedOnPosIndex
                && candidate.Scan.LedOffPosIndex == exact.Scan.LedOffPosIndex
                && candidate.Scan.GalvoStartPos == exact.Scan.GalvoStartPos
                && candidate.Scan.GalvoEndPos == exact.Scan.GalvoEndPos
                && candidate.Scan.LeadInSamples == exact.Scan.LeadInSamples
                && candidate.Scan.LeadOutSamples == exact.Scan.LeadOutSamples
                && candidate.Scan.LineRateHz.Equals(exact.Scan.LineRateHz)
                && candidate.Scan.SensorRows == exact.Scan.SensorRows
                && candidate.Scan.GalvoDelayUs.Equals(exact.Scan.GalvoDelayUs)
                && candidate.Scan.GalvoDecelUs.Equals(exact.Scan.GalvoDecelUs)
                && candidate.Scan.SlitHeightRows == exact.Scan.SlitHeightRows;
        }

        private LsoModePreset LoadSelected(ColorCameraPresetMode mode)
        {
            string defaultName = DefaultName(mode);

            string selected = ReadSelectedName(mode, defaultName);
            if (!IsValidName(selected) || !File.Exists(PathOf(selected)))
            {
                if (!string.Equals(selected, defaultName, StringComparison.OrdinalIgnoreCase))
                {
                    _issues.Add($"The selected {mode} preset '{selected}' is missing - '{defaultName}' is used instead.");
                }
                selected = defaultName;
                WriteSelectedName(mode, selected);
            }

            string path = PathOf(selected);
            if (!File.Exists(path))
            {
                return LsoModePreset.DevKitDefault(mode, selected);
            }

            return LsoModePreset.Load(path, mode, _issues);
        }

        private string ReadSelectedName(ColorCameraPresetMode mode, string defaultName)
        {
            try
            {
                var ini = new IniFileUtil(_configIniPath);
                string value = ini.ReadString(IniSectionName, SelectionKey(mode), defaultName);
                return string.IsNullOrWhiteSpace(value) ? defaultName : value.Trim();
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException)
            {
                _issues.Add($"The active {mode} preset name could not be read from '{_configIniPath}': {e.Message}");
                return defaultName;
            }
        }

        private bool WriteSelectedName(ColorCameraPresetMode mode, string presetName)
        {
            try
            {
                var ini = new IniFileUtil(_configIniPath);
                if (ini.WriteString(IniSectionName, SelectionKey(mode), presetName))
                {
                    return true;
                }
                _issues.Add($"The active {mode} preset name could not be written to '{_configIniPath}'.");
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException)
            {
                _issues.Add($"The active {mode} preset name could not be written to '{_configIniPath}': {e.Message}");
            }
            return false;
        }

        // One-shot merge of the v1 layout: a camera preset folder plus the scanner slots that
        // lived in WsoDeviceCfg.ini. The tuned slot values are what matters here - they are
        // moved into the two active presets and the INI sections are then removed, so there is
        // exactly one owner afterwards.
        private void MigrateV1()
        {
            IniFileUtil ini;
            try
            {
                ini = new IniFileUtil(_configIniPath);
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException)
            {
                _issues.Add($"'{_configIniPath}' could not be opened for the one-time preset merge: {e.Message}");
                return;
            }

            if (ini.ReadInt(IniSectionName, MigratedKey, 0) != 0)
            {
                return;
            }

            string activePreview = ini.ReadString(LegacyCameraIniSection, PreviewPresetKey, DefaultPreviewPresetName).Trim();
            string activeCapture = ini.ReadString(LegacyCameraIniSection, CapturePresetKey, DefaultCapturePresetName).Trim();
            if (!IsValidName(activePreview)) { activePreview = DefaultPreviewPresetName; }
            if (!IsValidName(activeCapture)) { activeCapture = DefaultCapturePresetName; }

            string legacyDir = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, LegacyCameraDirectoryName));
            foreach (string name in EnumerateLegacyCameraPresets(legacyDir))
            {
                if (File.Exists(PathOf(name)))
                {
                    continue;
                }

                // The mode only picks which built-in scan/camera defaults fill what the old
                // camera-only file could not carry.
                ColorCameraPresetMode mode = string.Equals(name, activePreview, StringComparison.OrdinalIgnoreCase)
                    || name.StartsWith(DefaultPreviewPresetName, StringComparison.OrdinalIgnoreCase)
                    ? ColorCameraPresetMode.Preview
                    : ColorCameraPresetMode.Capture;

                LsoModePreset merged = LsoModePreset.DevKitDefault(mode, name);
                merged.Camera.CopyValuesFrom(
                    ColorCameraPreset.Load(Path.Combine(legacyDir, name + ColorCameraPreset.FileExtension), mode, _issues));
                merged.Camera.Name = name;
                WritePreset(merged, name);
            }

            MergeLegacyScanSlot(ini, ColorCameraPresetMode.Preview, activePreview);
            MergeLegacyScanSlot(ini, ColorCameraPresetMode.Capture, activeCapture);

            // A안: the slots now live in the preset only.
            ini.DeleteSection(LsoScanProfileModel.GetIniSectionName(LegacyCaptureScanSlot));
            ini.DeleteSection(LsoScanProfileModel.GetIniSectionName(LegacyPreviewScanSlot));

            ini.WriteString(IniSectionName, PreviewPresetKey, activePreview);
            ini.WriteString(IniSectionName, CapturePresetKey, activeCapture);
            ini.WriteInt(IniSectionName, MigratedKey, 1);

            _issues.Add("Camera presets and the [LSO_SCANNER_0]/[LSO_SCANNER_1] scan settings were merged into "
                + $"'{DirectoryName}\\<name>.xml'. Preview='{activePreview}', Capture='{activeCapture}'.");
        }

        private IEnumerable<string> EnumerateLegacyCameraPresets(string legacyDir)
        {
            try
            {
                if (!System.IO.Directory.Exists(legacyDir))
                {
                    return Array.Empty<string>();
                }

                return System.IO.Directory
                    .EnumerateFiles(legacyDir, "*" + ColorCameraPreset.FileExtension)
                    .Select(Path.GetFileNameWithoutExtension)
                    .Where(name => IsValidName(name))
                    .Select(name => name!)
                    .ToArray();
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                _issues.Add($"Legacy camera preset folder '{legacyDir}' could not be listed: {e.Message}");
                return Array.Empty<string>();
            }
        }

        // v1 slot numbering, which is the reverse of the DevKit order the app runs now:
        // [LSO_SCANNER_0] carried the color capture profile and [LSO_SCANNER_1] the aiming
        // preview. Pinned here so the one-shot migration keeps reading the right section.
        private const int LegacyCaptureScanSlot = 0;
        private const int LegacyPreviewScanSlot = 1;

        // Moves one tuned [LSO_SCANNER_n] section into the preset that mode is running.
        private void MergeLegacyScanSlot(IniFileUtil ini, ColorCameraPresetMode mode, string presetName)
        {
            int legacySlot = mode == ColorCameraPresetMode.Preview
                ? LegacyPreviewScanSlot
                : LegacyCaptureScanSlot;

            var legacyScan = new LsoScanProfileModel();
            legacyScan.LoadFromIni(ini, legacySlot);
            if (legacyScan.GalvoPatternSize == 0)
            {
                return; // never configured - the preset keeps its built-in defaults
            }

            LsoModePreset preset = File.Exists(PathOf(presetName))
                ? LsoModePreset.Load(PathOf(presetName), mode, _issues)
                : LsoModePreset.DevKitDefault(mode, presetName);

            preset.Scan.CopyValuesFrom(legacyScan);
            WritePreset(preset, presetName);
        }
    }
}
