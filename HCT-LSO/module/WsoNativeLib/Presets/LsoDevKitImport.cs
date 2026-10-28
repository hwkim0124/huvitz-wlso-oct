using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Nodes;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    public sealed record LsoDevKitImportField(string Source, string SourceValue, string Destination, string ResultValue);

    public sealed class LsoDevKitImportResult
    {
        public LsoModePreset? Preset { get; internal set; }
        public string SavedPath { get; internal set; } = string.Empty;
        public List<LsoDevKitImportField> Fields { get; } = new List<LsoDevKitImportField>();
        public List<string> Warnings { get; } = new List<string>();
        public List<string> Rejections { get; } = new List<string>();
        public bool Succeeded => SavedPath.Length > 0 && Rejections.Count == 0;
    }

    // DevKit 파일만 읽고 지정한 preset 폴더에 새 파일을 만든다. 활성화는 별도 호출이다.
    public static class LsoDevKitImport
    {
        public static LsoDevKitImportResult Import(string sourceDirectory, string presetDirectory,
            ColorCameraPresetMode mode, string presetName, LsoModePreset? currentPreset = null,
            ushort? ledIntensity = null)
        {
            var report = new LsoDevKitImportResult();
            if (!LsoModePresetStore.IsValidName(presetName))
            {
                report.Rejections.Add("Invalid preset name.");
                return report;
            }
            string output = Path.Combine(presetDirectory, presetName + LsoModePreset.FileExtension);
            string calibrationName = presetName + "_GalvoSlitCalibration.json";
            string calibrationOutput = Path.Combine(presetDirectory, calibrationName);
            if (File.Exists(output) || File.Exists(calibrationOutput))
            {
                report.Rejections.Add($"Preset '{presetName}' already exists; no files were changed.");
                return report;
            }

            string? createdCalibration = null;
            string? createdPreset = null;
            try
            {
                string patternDirectory = Directory.Exists(Path.Combine(sourceDirectory, "LsoPatternCfg"))
                    ? Path.Combine(sourceDirectory, "LsoPatternCfg") : sourceDirectory;
                string cameraDirectory = new[] { "CameraConfigs", "CameraConfig" }
                    .Select(d => Path.Combine(sourceDirectory, d)).FirstOrDefault(Directory.Exists) ?? sourceDirectory;
                string activePath = Path.Combine(patternDirectory, "ActivePatterns.json");
                if (!File.Exists(activePath)) { activePath = Path.Combine(patternDirectory, "ActiveSlots.json"); }
                using JsonDocument active = JsonDocument.Parse(File.ReadAllText(activePath));
                string selectionKey = mode == ColorCameraPresetMode.Preview ? "BfsPreviewPattern" : "BfsCapturePattern";
                string selected = active.RootElement.GetProperty(selectionKey).GetString() ?? string.Empty;
                if (!LsoModePresetStore.IsValidName(selected)) { throw new FormatException("Invalid active pattern name."); }
                string patternPath = Path.Combine(patternDirectory, "LsoPattern_" + selected + ".json");
                string cameraPath = Path.Combine(cameraDirectory,
                    mode == ColorCameraPresetMode.Preview ? "BfsCameraConfigPreview.json" : "BfsCameraConfig.json");
                using JsonDocument patternDocument = JsonDocument.Parse(File.ReadAllText(patternPath));
                using JsonDocument cameraDocument = JsonDocument.Parse(File.ReadAllText(cameraPath));
                JsonElement pattern = patternDocument.RootElement;
                JsonElement camera = cameraDocument.RootElement;
                LsoModePreset preset = currentPreset?.Clone(presetName) ?? LsoModePreset.DevKitDefault(mode, presetName);
                report.Preset = preset;
                preset.Scan.TrajectoryFile = string.Empty;
                preset.Scan.CalibrationFile = string.Empty;

                T Read<T>(JsonElement json, string key, string destination, Func<JsonElement, T>? convert = null)
                {
                    JsonElement value = json.GetProperty(key);
                    T result = convert == null ? value.Deserialize<T>()! : convert(value);
                    string file = json.Equals(pattern) ? Path.GetFileName(patternPath) : Path.GetFileName(cameraPath);
                    report.Fields.Add(new LsoDevKitImportField(file + "/" + key, value.ToString(), destination,
                        Convert.ToString(result, CultureInfo.InvariantCulture) ?? string.Empty));
                    return result;
                }
                void Row(string source, object before, string destination, object after) =>
                    report.Fields.Add(new LsoDevKitImportField(source, Convert.ToString(before, CultureInfo.InvariantCulture)!,
                        destination, Convert.ToString(after, CultureInfo.InvariantCulture)!));

                var scan = preset.Scan;
                scan.TimeStepUs = Read<int>(pattern, "TimeStepUs", "Scan/TimeStepUs");
                scan.TriggerSource = Read<ushort>(pattern, "TrigSrc", "Scan/TriggerSource");
                scan.AcquisitionMode = Read<ushort>(pattern, "AcqMode", "Scan/AcquisitionMode");
                scan.AcqFrameSize = Read<uint>(pattern, "AcqFrameSize", "Scan/AcqFrameSize");
                scan.SubFrameSize = Read<ushort>(pattern, "SubframeSize", "Scan/SubFrameSize");
                scan.GalvanoRewindOffset = Read<short>(pattern, "PrescanPosRewindOffset", "Scan/GalvanoRewindOffset");
                scan.GalvoPatternSize = Read<ushort>(pattern, "SampleSize", "Scan/GalvoPatternSize");
                scan.LedOnPosIndex = Read<ushort>(pattern, "LedOnPosIndex", "Scan/LedOnPosIndex");
                scan.LedOffPosIndex = Read<ushort>(pattern, "LedOffPosIndex", "Scan/LedOffPosIndex");
                scan.GalvoStartPos = Read<short>(pattern, "GalvanoStartPos", "Scan/GalvoStartPos");
                scan.GalvoEndPos = Read<short>(pattern, "GalvanoEndPos", "Scan/GalvoEndPos");
                scan.LeadInSamples = Read<ushort>(pattern, "LeadInSamples", "Scan/LeadInSamples");
                scan.LeadOutSamples = Read<ushort>(pattern, "LeadOutSamples", "Scan/LeadOutSamples");
                scan.LineRateHz = Read<double>(pattern, "AcquisitionLineRateHz", "Scan/LineRateHz");
                scan.SensorRows = Read<int>(pattern, "SensorPhysicalRows", "Scan/SensorRows");
                if (mode == ColorCameraPresetMode.Preview && scan.SensorRows != 3648)
                {
                    Row("Physical Preview sensor", scan.SensorRows, "Scan/SensorRows", 3648);
                    scan.SensorRows = 3648;
                }
                scan.SensorBlankingRows = Read<int>(pattern, "SensorBlankingRows", "Scan/SensorBlankingRows");
                scan.SlitHeightRows = Read<int>(pattern, "SlitHeightRows", "Scan/SlitHeightRows");
                scan.GalvoDelayUs = Read<double>(pattern, "GalvanoDelayUs", "Scan/GalvoDelayUs");
                scan.GalvoDecelUs = Read<double>(pattern, "GalvanoDecelUs", "Scan/GalvoDecelUs");
                scan.SequenceMode = Read<LsoScanSequenceMode>(pattern, "SequenceModeSelection", "Scan/SequenceMode");
                scan.PreMoveOffset = Read<short>(pattern, "PreMoveOffset", "Scan/PreMoveOffset");
                scan.PreMoveDelayUs = Read<ushort>(pattern, "PreMoveDelayUs", "Scan/PreMoveDelayUs");
                scan.TrajectoryMode = Read<LsoTrajectoryMode>(pattern, "TrajectoryMode", "Scan/TrajectoryMode");
                if (!Enum.IsDefined(scan.SequenceMode) || !Enum.IsDefined(scan.TrajectoryMode))
                {
                    throw new FormatException("Unsupported sequence or trajectory mode.");
                }

                var cam = preset.Camera;
                cam.RoiXWidth = Read<uint>(camera, "Width", "Camera/RoiXWidth");
                cam.RoiYHeight = Read<uint>(camera, "Height", "Camera/RoiYHeight");
                cam.RoiXOffset = Read<uint>(camera, "OffsetX", "Camera/RoiXOffset");
                cam.RoiYOffset = Read<uint>(camera, "OffsetY", "Camera/RoiYOffset");
                cam.ExposureTime = Read<uint>(camera, "ExposureTime", "Camera/ExposureTime",
                    v => checked((uint)Math.Round(v.GetDouble(), MidpointRounding.AwayFromZero)));
                scan.ExposureTimeUs = checked((int)cam.ExposureTime);
                Row(Path.GetFileName(patternPath) + "/ExposureTimeUs", pattern.GetProperty("ExposureTimeUs"),
                    "Scan/ExposureTimeUs (camera authority)", scan.ExposureTimeUs);
                cam.AcquisitionMode = Read<uint>(camera, "AcquisitionMode", "Camera/AcquisitionMode",
                    v => SymbolIndex(v, "Continuous", "SingleFrame", "MultiFrame"));
                cam.AcquisitionFrameCount = Read<uint>(camera, "AcquisitionFrameCount", "Camera/AcquisitionFrameCount");
                cam.BinningHorizontal = Read<uint>(camera, "BinningHorizontal", "Camera/BinningHorizontal");
                cam.BinningVertical = Read<uint>(camera, "BinningVertical", "Camera/BinningVertical");
                cam.Gain = Read<float>(camera, "Gain", "Camera/Gain");
                cam.AdcDepthIndex = Read<uint>(camera, "AdcBitDepth", "Camera/AdcDepthIndex",
                    v => SymbolIndex(v, "Bit8", "Bit10", "Bit12", "Bit14"));
                cam.TriggerDelayUs = Read<double>(camera, "TriggerDelay", "Camera/TriggerDelayUs");
                cam.PixelFormat = Read<uint>(camera, "PixelFormat", "Camera/PixelFormat", v =>
                {
                    string symbol = v.GetString()!;
                    if (!Enum.TryParse(symbol, out ColorPixelFormat format) || format == ColorPixelFormat.UNKNOWN
                        || !Enum.IsDefined(format)) { throw new FormatException("Unsupported PixelFormat: " + symbol); }
                    return (uint)format;
                });
                cam.ExposureAuto = Read<string>(camera, "ExposureAuto", "Camera/ExposureAuto");
                // The DevKit only switches BalanceWhiteAuto off and leaves both ratios at 1.0.
                double UnityRatio(System.Text.Json.JsonElement v) => v.GetString() == "Off" ? 1.0
                    : throw new FormatException("Unsupported BalanceWhiteAuto: " + v.GetString());
                cam.BalanceRatioRed = Read<double>(camera, "BalanceWhiteAuto", "Camera/BalanceRatioRed", UnityRatio);
                cam.BalanceRatioBlue = Read<double>(camera, "BalanceWhiteAuto", "Camera/BalanceRatioBlue", UnityRatio);
                cam.BinningSelector = Read<string>(camera, "BinningSelector", "Camera/BinningSelector");
                cam.BinningHorizontalMode = Read<string>(camera, "BinningHorizontalMode", "Camera/BinningHorizontalMode");
                cam.BinningVerticalMode = Read<string>(camera, "BinningVerticalMode", "Camera/BinningVerticalMode");
                cam.TestPattern = Read<string>(camera, "TestPattern", "Camera/TestPattern");
                cam.SensorShutterMode = Read<string>(camera, "SensorShutterMode", "Camera/SensorShutterMode");
                cam.TriggerMode = Read<string>(camera, "TriggerMode", "Camera/TriggerMode");
                cam.TriggerSource = Read<string>(camera, "TriggerSource", "Camera/TriggerSource");
                cam.TriggerActivation = Read<string>(camera, "TriggerActivation", "Camera/TriggerActivation");
                cam.TriggerSelector = Read<string>(camera, "TriggerSelector", "Camera/TriggerSelector");
                cam.TriggerOverlap = Read<string>(camera, "TriggerOverlap", "Camera/TriggerOverlap");
                cam.LineSelector = Read<string>(camera, "LineSelector", "Camera/LineSelector");
                cam.DeviceLinkThroughput = Read<long>(camera, "DeviceLinkThroughput", "Camera/DeviceLinkThroughput");
                cam.AcquisitionBurstFrameCount = Read<long>(camera, "AcquisitionBurstFrameCount", "Camera/AcquisitionBurstFrameCount");
                cam.IspEnable = Read<int>(camera, "IspEnable", "Camera/IspEnable", v => v.GetBoolean() ? 1 : 0);
                for (int i = 0; i < cam.Lines.Length; i++)
                {
                    string lineName = ColorCameraPreset.LineName(i);
                    JsonElement line = camera.GetProperty("Lines").GetProperty(lineName);
                    cam.Lines[i].Mode = line.GetProperty("LineMode").GetString()!;
                    cam.Lines[i].Source = line.GetProperty("LineSource").GetString()!;
                    cam.Lines[i].Inverter = line.GetProperty("LineInverter").GetBoolean() ? 1 : 0;
                    Row("Lines/" + lineName + "/LineMode", line.GetProperty("LineMode"), "Camera/Lines/" + lineName + "/Mode", cam.Lines[i].Mode);
                    Row("Lines/" + lineName + "/LineSource", line.GetProperty("LineSource"), "Camera/Lines/" + lineName + "/Source", cam.Lines[i].Source);
                    Row("Lines/" + lineName + "/LineInverter", line.GetProperty("LineInverter"), "Camera/Lines/" + lineName + "/Inverter", cam.Lines[i].Inverter);
                }
                foreach (string key in new[] { "DebayerAlgorithm", "GainAuto", "GammaEnable", "AwbWhitePatchEnable",
                    "BalanceWhiteEnable", "BalanceRatioSelector", "BalanceRatioRed", "BalanceRatioGreen",
                    "BalanceRatioBlue", "BalanceRatios", "BalanceWhiteAutoAnchorSelector" })
                {
                    if (camera.TryGetProperty(key, out JsonElement ignored)) { Row("Camera/" + key, ignored, "Not persisted", "Ignored (Toolkit processing)"); }
                }
                foreach (string key in new[] { "SensorLineTimeUs", "RecommendedExposureUs", "PreTrigSampleIndex",
                    "AutoCalculateSync", "AutoCalculatePreGalvo", "FrameTimeSafetyMarginPercent", "PatternID", "CameraTriggerDelayUs" })
                {
                    if (pattern.TryGetProperty(key, out JsonElement ignored)) { Row("Pattern/" + key, ignored, "Not persisted", "Derived/UI/slot or camera authority"); }
                }
                ushort oldIntensity = preset.Light.Intensity;
                preset.Light.Intensity = ledIntensity ?? oldIntensity;
                Row("Caller LED intensity", ledIntensity?.ToString() ?? "null (keep current)", "Light/Intensity", preset.Light.Intensity);
                Row(Path.GetFileName(activePath) + "/" + selectionKey, selected, "Preset/Name", presetName);

                string calibrationPath = Path.Combine(patternDirectory, LsoGalvoSlitCalibration.DefaultFileName);
                if (scan.IsCalibratedTrajectory)
                {
                    if (!LsoGalvoSlitCalibration.TryLoad(calibrationPath, out var calibration, out string error)
                        || calibration == null || !calibration.TryFit(out error)) { throw new FormatException(error); }
                    string bfsPath = Path.Combine(patternDirectory, "GalvoSlitCalibration_BFS.json");
                    if (File.Exists(bfsPath) && !JsonNode.DeepEquals(JsonNode.Parse(File.ReadAllText(calibrationPath)),
                        JsonNode.Parse(File.ReadAllText(bfsPath))))
                    {
                        report.Warnings.Add("GalvoSlitCalibration_BFS.json differs from GalvoSlitCalibration.json; the board's default file is used.");
                    }
                    calibration.SweepEnds(scan.SensorRows, out short start, out short end);
                    if (Math.Abs(scan.GalvoStartPos - start) > 2 || Math.Abs(scan.GalvoEndPos - end) > 2)
                    {
                        report.Warnings.Add($"Stale Start/End {scan.GalvoStartPos}/{scan.GalvoEndPos} -> calibrated {start}/{end}.");
                    }
                    Row("Calibration DAC(0)", scan.GalvoStartPos, "Scan/GalvoStartPos", start);
                    Row("Calibration DAC(rows-1)", scan.GalvoEndPos, "Scan/GalvoEndPos", end);
                    scan.GalvoStartPos = start;
                    scan.GalvoEndPos = end;
                    scan.CalibrationFile = calibrationName;
                    Row(calibrationPath, "Board calibration", "Scan/CalibrationFile", calibrationName);
                    if (calibration.GenerateTrajectory(start, end, scan.GalvoPatternSize, scan.LeadInSamples,
                        scan.LeadOutSamples, scan.SensorRows, out error) == null) { throw new FormatException(error); }
                }
                if (!LsoScanTimingCalculator.TryCalculate(scan, cam.TriggerDelayUs, cam.ExposureTime,
                    out var timing, out string timingError, desiredPreMoveDelayUs: scan.TimingPreMoveDelayUs)) { throw new FormatException(timingError); }
                foreach (var pair in new[] { ("LeadInSamples", (int)scan.LeadInSamples, timing.LeadInSamples),
                    ("LeadOutSamples", (int)scan.LeadOutSamples, timing.LeadOutSamples),
                    ("SampleSize", (int)scan.GalvoPatternSize, timing.PatternSize),
                    ("LedOnPosIndex", (int)scan.LedOnPosIndex, timing.LedOnIndex),
                    ("LedOffPosIndex", (int)scan.LedOffPosIndex, timing.LedOffIndex) })
                {
                    if (pair.Item2 != pair.Item3) { report.Rejections.Add($"{pair.Item1}: JSON {pair.Item2} != calculated {pair.Item3}."); }
                }
                if (report.Rejections.Count > 0) { return report; }
                Directory.CreateDirectory(presetDirectory);
                if (scan.IsCalibratedTrajectory)
                {
                    File.Copy(calibrationPath, calibrationOutput, overwrite: false);
                    createdCalibration = calibrationOutput;
                }
                // CreateNew closes the race with another importer; Save never overwrites an existing preset.
                using (var reserved = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None)) { }
                createdPreset = output;
                preset.Save(output);
                report.SavedPath = Path.GetFullPath(output);
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is JsonException
                || e is FormatException || e is InvalidOperationException || e is KeyNotFoundException
                || e is OverflowException || e is ArgumentException)
            {
                if (createdPreset != null) { File.Delete(createdPreset); }
                if (createdCalibration != null) { File.Delete(createdCalibration); }
                report.Rejections.Add(e.Message);
            }
            return report;
        }

        private static uint SymbolIndex(JsonElement value, params string[] names)
        {
            int index = Array.IndexOf(names, value.GetString());
            return index >= 0 ? (uint)index : throw new FormatException("Unsupported symbolic value: " + value);
        }

        // INI 역시 managed 파일 처리만 사용한다. 두 선택을 한 번의 교체로 반영한다.
        public static void Activate(string presetDirectory, string iniPath, string previewName, string captureName)
        {
            foreach (string name in new[] { previewName, captureName })
            {
                if (!LsoModePresetStore.IsValidName(name)
                    || !File.Exists(Path.Combine(presetDirectory, name + LsoModePreset.FileExtension)))
                {
                    throw new ArgumentException("Activation requires an existing preset: " + name);
                }
            }
            var lines = File.Exists(iniPath) ? File.ReadAllLines(iniPath).ToList() : new List<string>();
            int section = lines.FindIndex(l => l.Trim().Equals("[" + LsoModePresetStore.IniSectionName + "]", StringComparison.OrdinalIgnoreCase));
            if (section < 0) { section = lines.Count; lines.Add("[" + LsoModePresetStore.IniSectionName + "]"); }
            int end = section + 1;
            while (end < lines.Count && !lines[end].TrimStart().StartsWith("[", StringComparison.Ordinal)) { end++; }
            foreach (var pair in new[] { ("PreviewPreset", previewName), ("CapturePreset", captureName) })
            {
                for (int i = end - 1; i > section; i--)
                {
                    if (lines[i].Split('=')[0].Trim().Equals(pair.Item1, StringComparison.OrdinalIgnoreCase))
                    {
                        lines.RemoveAt(i);
                        end--;
                    }
                }
                lines.Insert(end++, pair.Item1 + "=" + pair.Item2);
            }
            string temporary = iniPath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                File.WriteAllLines(temporary, lines);
                File.Move(temporary, iniPath, overwrite: true);
            }
            finally
            {
                if (File.Exists(temporary)) { File.Delete(temporary); }
            }
        }
    }
}
