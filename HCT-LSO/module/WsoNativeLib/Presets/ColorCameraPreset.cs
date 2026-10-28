using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Xml;
using System.Xml.Linq;
using static WsoNativeLib.WsoLsoScan;

namespace WsoNativeLib.Presets
{
    // The two camera modes that own a preset. The mode is NOT stored in the preset file:
    // any preset can be selected for either mode, and the mode only decides which built-in
    // DevKit defaults a missing or unreadable value falls back to.
    public enum ColorCameraPresetMode
    {
        Preview = 0,
        Capture = 1,
    }

    // One GPIO line row of a camera preset (Line0..Line3).
    //
    // LineSelector is the row INDEX, never a stored duplicate of the row's values: the
    // DevKit config carries both a Lines dictionary and a loose LineMode/LineSource/
    // LineInverter triple for the selected line, and keeping both is how the two get to
    // disagree. Here the four rows are the only storage and LineSelector says which one the
    // editor was left on.
    //
    // An EMPTY Mode/Source - or a negative Inverter - means "this preset does not state
    // this node". Nothing is then written to it, so a row left alone keeps whatever the
    // device (or the start path) already had. That is what lets the shipped defaults claim
    // only the lines this instrument's trigger contract actually uses.
    public sealed class ColorCameraLinePreset
    {
        public const int NotStated = -1;

        public string Mode { get; set; } = string.Empty;

        public string Source { get; set; } = string.Empty;

        // 0 = false, 1 = true, -1 = not stated.
        public int Inverter { get; set; } = NotStated;

        public bool IsStated => Mode.Length > 0 || Source.Length > 0 || Inverter >= 0;

        public void CopyValuesFrom(ColorCameraLinePreset source)
        {
            Mode = source.Mode;
            Source = source.Source;
            Inverter = source.Inverter;
        }

        public bool ValueEquals(ColorCameraLinePreset other)
        {
            return other != null
                && string.Equals(Mode, other.Mode, StringComparison.Ordinal)
                && string.Equals(Source, other.Source, StringComparison.Ordinal)
                && Inverter == other.Inverter;
        }
    }

    // One color-camera setting preset, stored as a single XML file.
    //
    // Content = every field wso_domain::ColorCameraSettingParam carries, plus the whole A-1
    // extension (wso_domain::ColorCameraExtendedParam): ExposureAuto, the burst count, the
    // binning selector and its two modes, TestPattern, the device link throughput limit,
    // SensorShutterMode, IspEnable, the five trigger nodes, the FrameStart trigger delay and
    // the four GPIO line rows. roi_max_* are device-reported: they round-trip so a preset
    // file is a complete record, but a request built from a preset takes them from the live
    // camera instead (see ToSettingParam).
    //
    // Enum-valued fields are stored as their GenApi SYMBOLIC NAMES, never as a UI combo
    // index: the index depends on what the editor happened to list, the name is the SFNC
    // contract the DevKit configs are already written in, and a name survives a camera
    // model change that renumbers the SDK enum.
    //
    // Defaults are the DevKit configs, which are the agreed baseline for both modes:
    //   Preview - LSO_DevKit/Configuration_files/CameraConfigs/BfsCameraConfigPreview.json
    //   Capture - LSO_DevKit/Configuration_files/CameraConfigs/BfsCameraConfig.json
    // (LSO_DevKit/bin/**/CameraConfigs/* is stale build output and is deliberately not used.)
    public sealed class ColorCameraPreset
    {
        public const string FileExtension = ".xml";
        public const string RootElementName = "ColorCameraPreset";

        // v1 = the 14 base fields + TriggerDelayUs. v2 adds the A-1 extension. A v1 file is
        // NOT rewritten or discarded on load: its values are kept and only the fields it
        // cannot carry take the built-in defaults.
        public const int FormatVersion = 2;
        public const int FirstExtendedFormatVersion = 2;

        public const float DevKitPreviewGain = 10.00836543154712f;
        public const float DevKitCaptureGain = 10.048786939752075f;
        public const double DevKitPreviewTriggerDelayUs = 1011;
        public const double DevKitCaptureTriggerDelayUs = 288;

        // Both DevKit configs carry these for every mode.
        public const string DevKitExposureAuto = "Off";
        public const string DevKitBinningSelector = "All";
        public const string DevKitBinningHorizontalMode = "Average";
        public const string DevKitBinningVerticalMode = "Sum";
        public const string DevKitTestPattern = "Off";
        public const string DevKitSensorShutterMode = "Rolling";
        public const long DevKitDeviceLinkThroughput = 500000000;
        public const long DevKitAcquisitionBurstFrameCount = 1;

        // "this preset does not state the node" - the same meaning the native block gives an
        // empty name / a negative number.
        public const string NotStated = "";
        public const long NotStatedNumber = -1;
        public const double NotStatedDelay = -1.0;

        public const int LineCount = LSO_CAMERA_LINE_COUNT;

        public string Name { get; set; } = string.Empty;

        // Device-reported maxima. Informational in a preset file.
        public uint RoiMaxWidth { get; set; }
        public uint RoiMaxHeight { get; set; }

        public uint RoiXWidth { get; set; }
        public uint RoiYHeight { get; set; }
        public uint RoiXOffset { get; set; }
        public uint RoiYOffset { get; set; }
        public uint ExposureTime { get; set; }
        public uint AcquisitionMode { get; set; }
        public uint AcquisitionFrameCount { get; set; }
        public uint PixelFormat { get; set; }
        public uint BinningHorizontal { get; set; }
        public uint BinningVertical { get; set; }
        public float Gain { get; set; }
        public uint AdcDepthIndex { get; set; }

        // FrameStart trigger delay in microseconds. Kept as a double because the camera
        // quantises it to its own increment and the editor has to be able to show - and
        // store - the value the device actually holds, not a rounded one.
        // Negative = not stated.
        public double TriggerDelayUs { get; set; } = DevKitCaptureTriggerDelayUs;

        // --- A-1 extension ------------------------------------------------------------
        public string ExposureAuto { get; set; } = NotStated;
        // White-balance ratios written to BalanceRatio (Red / Blue). Negative = not stated:
        // the camera keeps whatever it holds. 1.0 / 1.0 is what the DevKit leaves.
        public double BalanceRatioRed { get; set; } = 1.58;
        public double BalanceRatioBlue { get; set; } = 1.84;
        public string BinningSelector { get; set; } = NotStated;
        public string BinningHorizontalMode { get; set; } = NotStated;
        public string BinningVerticalMode { get; set; } = NotStated;
        public string TestPattern { get; set; } = NotStated;
        public string SensorShutterMode { get; set; } = NotStated;

        public string TriggerMode { get; set; } = NotStated;
        public string TriggerSource { get; set; } = NotStated;
        public string TriggerActivation { get; set; } = NotStated;
        public string TriggerSelector { get; set; } = NotStated;
        public string TriggerOverlap { get; set; } = NotStated;

        // Negative = not stated.
        public long AcquisitionBurstFrameCount { get; set; } = NotStatedNumber;
        public long DeviceLinkThroughput { get; set; } = NotStatedNumber;

        // 0 = false, 1 = true, -1 = not stated.
        public int IspEnable { get; set; } = (int)NotStatedNumber;

        // Which GPIO row the editor is showing. Not a value of its own - the row it names
        // holds the values.
        public string LineSelector { get; set; } = NotStated;

        public ColorCameraLinePreset[] Lines { get; } = CreateLines();

        private static ColorCameraLinePreset[] CreateLines()
        {
            var lines = new ColorCameraLinePreset[LineCount];
            for (int i = 0; i < lines.Length; ++i)
            {
                lines[i] = new ColorCameraLinePreset();
            }
            return lines;
        }

        public static string LineName(int index)
        {
            return "Line" + index.ToString(CultureInfo.InvariantCulture);
        }

        // BfsCameraConfigPreview.json / BfsCameraConfig.json, field for field.
        //
        // Preview states every key in BfsCameraConfigPreview.json. This is intentionally a
        // strict promise: if the connected camera cannot write and read back any requested
        // node, Apply/Preview reports that field and Preview does not pretend to have parity.
        public static ColorCameraPreset DevKitDefault(ColorCameraPresetMode mode, string name)
        {
            ColorCameraPreset preset;
            if (mode == ColorCameraPresetMode.Preview)
            {
                preset = new ColorCameraPreset
                {
                    Name = name,
                    RoiMaxWidth = 2736,          // binning-relative max at 2x2
                    RoiMaxHeight = 1824,
                    RoiXWidth = 2736,
                    RoiYHeight = 1824,
                    RoiXOffset = 0,
                    RoiYOffset = 0,
                    ExposureTime = 1495,
                    AcquisitionMode = 0,         // Continuous
                    AcquisitionFrameCount = 2,
                    PixelFormat = 0,             // Mono8
                    BinningHorizontal = 2,
                    BinningVertical = 2,
                    Gain = DevKitPreviewGain,
                    AdcDepthIndex = 2,           // Bit12
                    TriggerDelayUs = DevKitPreviewTriggerDelayUs,
                    LineSelector = "Line2",
                };
            }
            else
            {
                preset = new ColorCameraPreset
                {
                    Name = name,
                    RoiMaxWidth = 5472,          // binning-relative max at 1x1
                    RoiMaxHeight = 3648,
                    RoiXWidth = 3648,
                    RoiYHeight = 3648,
                    RoiXOffset = 912,
                    RoiYOffset = 0,
                    ExposureTime = 5629,
                    AcquisitionMode = 0,         // Continuous
                    AcquisitionFrameCount = 2,
                    PixelFormat = 4,             // BayerRG8
                    BinningHorizontal = 1,
                    BinningVertical = 1,
                    Gain = DevKitCaptureGain,
                    AdcDepthIndex = 2,           // Bit12
                    TriggerDelayUs = DevKitCaptureTriggerDelayUs,
                    LineSelector = "Line2",
                };
            }

            preset.ExposureAuto = DevKitExposureAuto;

            preset.BinningSelector = DevKitBinningSelector;
            preset.BinningHorizontalMode = DevKitBinningHorizontalMode;
            preset.BinningVerticalMode = DevKitBinningVerticalMode;
            preset.TestPattern = DevKitTestPattern;
            preset.SensorShutterMode = DevKitSensorShutterMode;
            preset.AcquisitionBurstFrameCount = DevKitAcquisitionBurstFrameCount;
            preset.DeviceLinkThroughput = DevKitDeviceLinkThroughput;
            preset.IspEnable = 0;
            preset.TriggerMode = "On";

            preset.Lines[0].Mode = "Input";
            preset.Lines[0].Source = "Off";
            preset.Lines[0].Inverter = 0;
            preset.Lines[1].Mode = "Output";
            preset.Lines[1].Source = "ExposureActive";
            preset.Lines[1].Inverter = 0;

            preset.TriggerSource = "Line3";
            preset.TriggerActivation = "RisingEdge";
            preset.TriggerSelector = "FrameStart";
            preset.TriggerOverlap = "ReadOut";

            // Line2 = the exposure strobe out, Line3 = the board trigger in.
            preset.Lines[2].Mode = "Output";
            preset.Lines[2].Source = "ExposureActive";
            preset.Lines[2].Inverter = 0;
            preset.Lines[3].Mode = "Input";
            preset.Lines[3].Source = "Off";
            preset.Lines[3].Inverter = 0;

            return preset;
        }

        public ColorCameraPreset Clone(string name)
        {
            var copy = new ColorCameraPreset { Name = name };
            copy.CopyValuesFrom(this);
            return copy;
        }

        // Value comparison, Name excluded: a preset saved under a new name still carries the
        // same settings, and that is what a round-trip or a defaults check is about.
        public bool ValueEquals(ColorCameraPreset other)
        {
            if (other == null)
            {
                return false;
            }

            bool equal = RoiMaxWidth == other.RoiMaxWidth
                && RoiMaxHeight == other.RoiMaxHeight
                && RoiXWidth == other.RoiXWidth
                && RoiYHeight == other.RoiYHeight
                && RoiXOffset == other.RoiXOffset
                && RoiYOffset == other.RoiYOffset
                && ExposureTime == other.ExposureTime
                && AcquisitionMode == other.AcquisitionMode
                && AcquisitionFrameCount == other.AcquisitionFrameCount
                && PixelFormat == other.PixelFormat
                && BinningHorizontal == other.BinningHorizontal
                && BinningVertical == other.BinningVertical
                && Gain.Equals(other.Gain)
                && AdcDepthIndex == other.AdcDepthIndex
                && TriggerDelayUs.Equals(other.TriggerDelayUs)
                && string.Equals(ExposureAuto, other.ExposureAuto, StringComparison.Ordinal)
                && BalanceRatioRed.Equals(other.BalanceRatioRed)
                && BalanceRatioBlue.Equals(other.BalanceRatioBlue)
                && string.Equals(BinningSelector, other.BinningSelector, StringComparison.Ordinal)
                && string.Equals(BinningHorizontalMode, other.BinningHorizontalMode, StringComparison.Ordinal)
                && string.Equals(BinningVerticalMode, other.BinningVerticalMode, StringComparison.Ordinal)
                && string.Equals(TestPattern, other.TestPattern, StringComparison.Ordinal)
                && string.Equals(SensorShutterMode, other.SensorShutterMode, StringComparison.Ordinal)
                && string.Equals(TriggerMode, other.TriggerMode, StringComparison.Ordinal)
                && string.Equals(TriggerSource, other.TriggerSource, StringComparison.Ordinal)
                && string.Equals(TriggerActivation, other.TriggerActivation, StringComparison.Ordinal)
                && string.Equals(TriggerSelector, other.TriggerSelector, StringComparison.Ordinal)
                && string.Equals(TriggerOverlap, other.TriggerOverlap, StringComparison.Ordinal)
                && string.Equals(LineSelector, other.LineSelector, StringComparison.Ordinal)
                && AcquisitionBurstFrameCount == other.AcquisitionBurstFrameCount
                && DeviceLinkThroughput == other.DeviceLinkThroughput
                && IspEnable == other.IspEnable;

            for (int i = 0; equal && i < LineCount; ++i)
            {
                equal = Lines[i].ValueEquals(other.Lines[i]);
            }

            return equal;
        }

        public void CopyValuesFrom(ColorCameraPreset source)
        {
            RoiMaxWidth = source.RoiMaxWidth;
            RoiMaxHeight = source.RoiMaxHeight;
            RoiXWidth = source.RoiXWidth;
            RoiYHeight = source.RoiYHeight;
            RoiXOffset = source.RoiXOffset;
            RoiYOffset = source.RoiYOffset;
            ExposureTime = source.ExposureTime;
            AcquisitionMode = source.AcquisitionMode;
            AcquisitionFrameCount = source.AcquisitionFrameCount;
            PixelFormat = source.PixelFormat;
            BinningHorizontal = source.BinningHorizontal;
            BinningVertical = source.BinningVertical;
            Gain = source.Gain;
            AdcDepthIndex = source.AdcDepthIndex;
            TriggerDelayUs = source.TriggerDelayUs;

            ExposureAuto = source.ExposureAuto;
            BalanceRatioRed = source.BalanceRatioRed;
            BalanceRatioBlue = source.BalanceRatioBlue;
            BinningSelector = source.BinningSelector;
            BinningHorizontalMode = source.BinningHorizontalMode;
            BinningVerticalMode = source.BinningVerticalMode;
            TestPattern = source.TestPattern;
            SensorShutterMode = source.SensorShutterMode;
            TriggerMode = source.TriggerMode;
            TriggerSource = source.TriggerSource;
            TriggerActivation = source.TriggerActivation;
            TriggerSelector = source.TriggerSelector;
            TriggerOverlap = source.TriggerOverlap;
            LineSelector = source.LineSelector;
            AcquisitionBurstFrameCount = source.AcquisitionBurstFrameCount;
            DeviceLinkThroughput = source.DeviceLinkThroughput;
            IspEnable = source.IspEnable;

            for (int i = 0; i < LineCount; ++i)
            {
                Lines[i].CopyValuesFrom(source.Lines[i]);
            }
        }

        // Native camera struct for a request. roi_max_* come from `baseParam` - the live
        // camera read-back - because they describe the sensor, not a user choice.
        public LsoColorCameraSettingParam ToSettingParam(LsoColorCameraSettingParam baseParam)
        {
            LsoColorCameraSettingParam param = baseParam;
            param.roiXWidth = RoiXWidth;
            param.roiYHeight = RoiYHeight;
            param.roiXOffset = RoiXOffset;
            param.roiYOffset = RoiYOffset;
            param.exposureTime = ExposureTime;
            param.acquisitionMode = AcquisitionMode;
            param.acquisitionFrameCount = AcquisitionFrameCount;
            param.pixelFormat = PixelFormat;
            param.binningHorizontal = BinningHorizontal;
            param.binningVertical = BinningVertical;
            param.gain = Gain;
            param.adcDepthIndex = AdcDepthIndex;
            return param;
        }

        public LsoColorCameraSettingParam ToSettingParam()
        {
            var baseParam = new LsoColorCameraSettingParam
            {
                roiMaxWidth = RoiMaxWidth,
                roiMaxHeight = RoiMaxHeight,
            };
            return ToSettingParam(baseParam);
        }

        // The A-1 half of the same contract.
        //
        // includeTriggerDelay is false for the preview request, whose own validated
        // triggerDelayUs field already owns that node: one value, one owner, so the two can
        // never disagree about what the camera should hold.
        public LsoColorCameraExtendedParam ToExtendedParam(bool includeTriggerDelay = true)
        {
            LsoColorCameraExtendedParam param = LsoColorCameraExtendedParam.Create();
            param.exposureAuto = ExposureAuto;
            param.balanceRatioRed = BalanceRatioRed;
            param.balanceRatioBlue = BalanceRatioBlue;
            param.binningSelector = BinningSelector;
            param.binningHorizontalMode = BinningHorizontalMode;
            param.binningVerticalMode = BinningVerticalMode;
            param.testPattern = TestPattern;
            param.sensorShutterMode = SensorShutterMode;
            param.triggerMode = TriggerMode;
            param.triggerSource = TriggerSource;
            param.triggerActivation = TriggerActivation;
            param.triggerSelector = TriggerSelector;
            param.triggerOverlap = TriggerOverlap;
            param.lineSelector = LineSelector;
            param.acquisitionBurstFrameCount = AcquisitionBurstFrameCount;
            param.deviceLinkThroughput = DeviceLinkThroughput;
            param.ispEnable = IspEnable;
            param.triggerDelayUs = includeTriggerDelay ? TriggerDelayUs : NotStatedDelay;

            for (int i = 0; i < LineCount; ++i)
            {
                param.lines[i].mode = Lines[i].Mode;
                param.lines[i].source = Lines[i].Source;
                param.lines[i].inverter = Lines[i].Inverter;
            }

            return param;
        }

        public void ApplySettingParam(LsoColorCameraSettingParam param)
        {
            RoiMaxWidth = param.roiMaxWidth;
            RoiMaxHeight = param.roiMaxHeight;
            RoiXWidth = param.roiXWidth;
            RoiYHeight = param.roiYHeight;
            RoiXOffset = param.roiXOffset;
            RoiYOffset = param.roiYOffset;
            ExposureTime = param.exposureTime;
            AcquisitionMode = param.acquisitionMode;
            AcquisitionFrameCount = param.acquisitionFrameCount;
            PixelFormat = param.pixelFormat;
            BinningHorizontal = param.binningHorizontal;
            BinningVertical = param.binningVertical;
            Gain = param.gain;
            AdcDepthIndex = param.adcDepthIndex;
        }

        // Takes a device read-back into the preset. Nodes the camera could not report come
        // back unstated and are left alone here too, so a read-back can never turn a value
        // the operator wrote into a blank.
        public void ApplyExtendedParam(LsoColorCameraExtendedParam param)
        {
            if (param.balanceRatioRed >= 0.0) { BalanceRatioRed = param.balanceRatioRed; }
            if (param.balanceRatioBlue >= 0.0) { BalanceRatioBlue = param.balanceRatioBlue; }
            ExposureAuto = Stated(param.exposureAuto, ExposureAuto);
            BinningSelector = Stated(param.binningSelector, BinningSelector);
            BinningHorizontalMode = Stated(param.binningHorizontalMode, BinningHorizontalMode);
            BinningVerticalMode = Stated(param.binningVerticalMode, BinningVerticalMode);
            TestPattern = Stated(param.testPattern, TestPattern);
            SensorShutterMode = Stated(param.sensorShutterMode, SensorShutterMode);
            TriggerMode = Stated(param.triggerMode, TriggerMode);
            TriggerSource = Stated(param.triggerSource, TriggerSource);
            TriggerActivation = Stated(param.triggerActivation, TriggerActivation);
            TriggerSelector = Stated(param.triggerSelector, TriggerSelector);
            TriggerOverlap = Stated(param.triggerOverlap, TriggerOverlap);
            LineSelector = Stated(param.lineSelector, LineSelector);

            if (param.acquisitionBurstFrameCount >= 0) { AcquisitionBurstFrameCount = param.acquisitionBurstFrameCount; }
            if (param.deviceLinkThroughput >= 0) { DeviceLinkThroughput = param.deviceLinkThroughput; }
            if (param.ispEnable >= 0) { IspEnable = param.ispEnable; }
            if (param.triggerDelayUs >= 0.0) { TriggerDelayUs = param.triggerDelayUs; }

            if (param.lines == null)
            {
                return;
            }

            for (int i = 0; i < LineCount && i < param.lines.Length; ++i)
            {
                Lines[i].Mode = Stated(param.lines[i].mode, Lines[i].Mode);
                Lines[i].Source = Stated(param.lines[i].source, Lines[i].Source);
                if (param.lines[i].inverter >= 0) { Lines[i].Inverter = param.lines[i].inverter; }
            }
        }

        private static string Stated(string? read, string current)
        {
            return string.IsNullOrEmpty(read) ? current : read!;
        }

        public void Save(string path)
        {
            string? directory = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }

            XElement root = ToElement(RootElementName);
            root.Add(new XAttribute("version", FormatVersion));
            root.Add(new XAttribute("name", Name));

            var document = new XDocument(new XDeclaration("1.0", "utf-8", null), root);
            document.Save(path);
        }

        // The value fields as one element, so the same field list serves a standalone camera
        // preset file and the <Camera> section of a mode preset.
        public XElement ToElement(string elementName)
        {
            var lines = new XElement("Lines");
            for (int i = 0; i < LineCount; ++i)
            {
                lines.Add(new XElement("Line",
                    new XAttribute("index", i.ToString(CultureInfo.InvariantCulture)),
                    Text("Mode", Lines[i].Mode),
                    Text("Source", Lines[i].Source),
                    Number("Inverter", Lines[i].Inverter)));
            }

            return new XElement(elementName,
                UInt(nameof(RoiMaxWidth), RoiMaxWidth),
                UInt(nameof(RoiMaxHeight), RoiMaxHeight),
                UInt(nameof(RoiXWidth), RoiXWidth),
                UInt(nameof(RoiYHeight), RoiYHeight),
                UInt(nameof(RoiXOffset), RoiXOffset),
                UInt(nameof(RoiYOffset), RoiYOffset),
                UInt(nameof(ExposureTime), ExposureTime),
                UInt(nameof(AcquisitionMode), AcquisitionMode),
                UInt(nameof(AcquisitionFrameCount), AcquisitionFrameCount),
                UInt(nameof(PixelFormat), PixelFormat),
                UInt(nameof(BinningHorizontal), BinningHorizontal),
                UInt(nameof(BinningVertical), BinningVertical),
                // G9 round-trips a float exactly, so a save/load pair cannot drift the gain.
                new XElement(nameof(Gain), Gain.ToString("G9", CultureInfo.InvariantCulture)),
                UInt(nameof(AdcDepthIndex), AdcDepthIndex),
                // R round-trips a double exactly, for the same reason.
                new XElement(nameof(TriggerDelayUs), TriggerDelayUs.ToString("R", CultureInfo.InvariantCulture)),
                Text(nameof(ExposureAuto), ExposureAuto),
                new XElement(nameof(BalanceRatioRed), BalanceRatioRed.ToString("R", CultureInfo.InvariantCulture)),
                new XElement(nameof(BalanceRatioBlue), BalanceRatioBlue.ToString("R", CultureInfo.InvariantCulture)),
                Text(nameof(BinningSelector), BinningSelector),
                Text(nameof(BinningHorizontalMode), BinningHorizontalMode),
                Text(nameof(BinningVerticalMode), BinningVerticalMode),
                Text(nameof(TestPattern), TestPattern),
                Text(nameof(SensorShutterMode), SensorShutterMode),
                Text(nameof(TriggerMode), TriggerMode),
                Text(nameof(TriggerSource), TriggerSource),
                Text(nameof(TriggerActivation), TriggerActivation),
                Text(nameof(TriggerSelector), TriggerSelector),
                Text(nameof(TriggerOverlap), TriggerOverlap),
                Number(nameof(AcquisitionBurstFrameCount), AcquisitionBurstFrameCount),
                Number(nameof(DeviceLinkThroughput), DeviceLinkThroughput),
                Number(nameof(IspEnable), IspEnable),
                Text(nameof(LineSelector), LineSelector),
                lines);
        }

        public void ReadFrom(XElement root, string presetName, IList<string> issues)
        {
            ReadFrom(root, presetName, issues, FormatVersion, out _);
        }

        // fileVersion is the <... version="N"> the document declared. A pre-A-1 file (v1)
        // could not carry the extension, so its absence is a MIGRATION, not damage: those
        // fields silently take the built-in defaults and one note says so, instead of
        // burying the operator in nineteen "field is missing" warnings on every load. Every
        // value the old file DOES carry is preserved.
        public void ReadFrom(XElement root, string presetName, IList<string> issues, int fileVersion)
        {
            ReadFrom(root, presetName, issues, fileVersion, out _);
        }

        // Reports only substitutions caused by damaged fields. The one note emitted for a
        // declared legacy format is deliberately added after that status is captured: it is
        // useful migration information, but not a reason for a read-only consumer to reject
        // an otherwise valid preset.
        internal void ReadFrom(XElement root, string presetName, IList<string> issues,
            int fileVersion, out bool hasFallback)
        {
            int issueCountBeforeFields = issues?.Count ?? 0;

            RoiMaxWidth = ReadUInt(root, nameof(RoiMaxWidth), RoiMaxWidth, presetName, issues);
            RoiMaxHeight = ReadUInt(root, nameof(RoiMaxHeight), RoiMaxHeight, presetName, issues);
            RoiXWidth = ReadUInt(root, nameof(RoiXWidth), RoiXWidth, presetName, issues);
            RoiYHeight = ReadUInt(root, nameof(RoiYHeight), RoiYHeight, presetName, issues);
            RoiXOffset = ReadUInt(root, nameof(RoiXOffset), RoiXOffset, presetName, issues);
            RoiYOffset = ReadUInt(root, nameof(RoiYOffset), RoiYOffset, presetName, issues);
            ExposureTime = ReadUInt(root, nameof(ExposureTime), ExposureTime, presetName, issues);
            AcquisitionMode = ReadUInt(root, nameof(AcquisitionMode), AcquisitionMode, presetName, issues);
            AcquisitionFrameCount = ReadUInt(root, nameof(AcquisitionFrameCount), AcquisitionFrameCount, presetName, issues);
            PixelFormat = ReadUInt(root, nameof(PixelFormat), PixelFormat, presetName, issues);
            BinningHorizontal = ReadUInt(root, nameof(BinningHorizontal), BinningHorizontal, presetName, issues);
            BinningVertical = ReadUInt(root, nameof(BinningVertical), BinningVertical, presetName, issues);
            Gain = ReadFloat(root, nameof(Gain), Gain, presetName, issues);
            AdcDepthIndex = ReadUInt(root, nameof(AdcDepthIndex), AdcDepthIndex, presetName, issues);
            TriggerDelayUs = ReadDouble(root, nameof(TriggerDelayUs), TriggerDelayUs, presetName, issues);

            bool migrating = fileVersion < FirstExtendedFormatVersion;
            ExposureAuto = ReadText(root, nameof(ExposureAuto), ExposureAuto, presetName, issues, migrating);
            if (root.Element(nameof(BalanceRatioRed)) != null)
            {
                BalanceRatioRed = ReadDouble(root, nameof(BalanceRatioRed), BalanceRatioRed, presetName, issues);
            }
            if (root.Element(nameof(BalanceRatioBlue)) != null)
            {
                BalanceRatioBlue = ReadDouble(root, nameof(BalanceRatioBlue), BalanceRatioBlue, presetName, issues);
            }
            BinningSelector = ReadText(root, nameof(BinningSelector), BinningSelector, presetName, issues, migrating);
            BinningHorizontalMode = ReadText(root, nameof(BinningHorizontalMode), BinningHorizontalMode, presetName, issues, migrating);
            BinningVerticalMode = ReadText(root, nameof(BinningVerticalMode), BinningVerticalMode, presetName, issues, migrating);
            TestPattern = ReadText(root, nameof(TestPattern), TestPattern, presetName, issues, migrating);
            SensorShutterMode = ReadText(root, nameof(SensorShutterMode), SensorShutterMode, presetName, issues, migrating);
            TriggerMode = ReadText(root, nameof(TriggerMode), TriggerMode, presetName, issues, migrating);
            TriggerSource = ReadText(root, nameof(TriggerSource), TriggerSource, presetName, issues, migrating);
            TriggerActivation = ReadText(root, nameof(TriggerActivation), TriggerActivation, presetName, issues, migrating);
            TriggerSelector = ReadText(root, nameof(TriggerSelector), TriggerSelector, presetName, issues, migrating);
            TriggerOverlap = ReadText(root, nameof(TriggerOverlap), TriggerOverlap, presetName, issues, migrating);
            LineSelector = ReadText(root, nameof(LineSelector), LineSelector, presetName, issues, migrating);
            AcquisitionBurstFrameCount = ReadLong(root, nameof(AcquisitionBurstFrameCount), AcquisitionBurstFrameCount, presetName, issues, migrating);
            DeviceLinkThroughput = ReadLong(root, nameof(DeviceLinkThroughput), DeviceLinkThroughput, presetName, issues, migrating);
            IspEnable = (int)ReadLong(root, nameof(IspEnable), IspEnable, presetName, issues, migrating);

            XElement? lines = root.Element("Lines");
            if (lines == null)
            {
                if (!migrating)
                {
                    issues?.Add($"'{presetName}': <Lines> is missing - the built-in GPIO defaults are used.");
                }
            }
            else
            {
                var seen = new bool[LineCount];
                foreach (XElement line in lines.Elements("Line"))
                {
                    XAttribute? index = line.Attribute("index");
                    if (index == null
                        || !int.TryParse(index.Value.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int i)
                        || i < 0 || i >= LineCount)
                    {
                        issues?.Add($"'{presetName}': a <Line> row has no usable index attribute - it is ignored.");
                        continue;
                    }

                    if (seen[i])
                    {
                        issues?.Add($"'{presetName}': duplicate GPIO Line{i} row.");
                        continue;
                    }
                    seen[i] = true;
                    Lines[i].Mode = ReadText(line, "Mode", Lines[i].Mode, presetName, issues, migrating);
                    Lines[i].Source = ReadText(line, "Source", Lines[i].Source, presetName, issues, migrating);
                    Lines[i].Inverter = (int)ReadLong(line, "Inverter", Lines[i].Inverter, presetName, issues, migrating);
                }
                if (!migrating)
                {
                    for (int i = 0; i < LineCount; i++)
                    {
                        if (!seen[i])
                        {
                            issues?.Add($"'{presetName}': GPIO Line{i} is missing - its built-in defaults are used.");
                        }
                    }
                }
            }

            if (migrating)
            {
                hasFallback = (issues?.Count ?? 0) > issueCountBeforeFields;
                issues?.Add($"'{presetName}': the preset predates the extended camera settings "
                    + $"(format {fileVersion} < {FirstExtendedFormatVersion}). Its stored values are kept and the "
                    + "trigger, GPIO and sensor fields take the built-in DevKit defaults.");
                return;
            }

            hasFallback = (issues?.Count ?? 0) > issueCountBeforeFields;
        }

        // Reads a preset file. Anything unreadable falls back to the mode's DevKit default
        // and is APPENDED to `issues` - a preset that silently became something else is the
        // failure mode this whole store exists to avoid, so every fallback is reportable.
        public static ColorCameraPreset Load(string path, ColorCameraPresetMode fallbackMode, IList<string> issues)
        {
            string name = Path.GetFileNameWithoutExtension(path);
            ColorCameraPreset preset = DevKitDefault(fallbackMode, name);

            XElement? root;
            try
            {
                root = XDocument.Load(path).Root;
            }
            catch (Exception e) when (e is XmlException || e is IOException || e is UnauthorizedAccessException)
            {
                issues?.Add($"'{name}': {e.Message} - the built-in {fallbackMode} defaults are used instead.");
                return preset;
            }

            if (root == null || root.Name.LocalName != RootElementName)
            {
                issues?.Add($"'{name}': not a {RootElementName} document - the built-in {fallbackMode} defaults are used instead.");
                return preset;
            }

            preset.ReadFrom(root, name, issues, ReadFormatVersion(root));
            return preset;
        }

        // The declared format version, defaulting to 1: a document with no version attribute
        // is older than the attribute, so treating it as v1 is what makes the migration path
        // cover hand-written files too.
        public static int ReadFormatVersion(XElement root)
        {
            XAttribute? version = root.Attribute("version");
            if (version == null
                || !int.TryParse(version.Value.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int value))
            {
                return 1;
            }
            return value;
        }

        private static XElement UInt(string elementName, uint value)
        {
            return new XElement(elementName, value.ToString(CultureInfo.InvariantCulture));
        }

        private static XElement Number(string elementName, long value)
        {
            return new XElement(elementName, value.ToString(CultureInfo.InvariantCulture));
        }

        private static XElement Text(string elementName, string value)
        {
            return new XElement(elementName, value ?? string.Empty);
        }

        private static uint ReadUInt(XElement root, string elementName, uint fallback, string presetName, IList<string>? issues)
        {
            XElement? element = root.Element(elementName);
            if (element == null)
            {
                issues?.Add($"'{presetName}': <{elementName}> is missing - the built-in default {fallback} is used.");
                return fallback;
            }

            if (!uint.TryParse(element.Value.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out uint value))
            {
                issues?.Add($"'{presetName}': <{elementName}> is not a number ('{element.Value.Trim()}') - the built-in default {fallback} is used.");
                return fallback;
            }

            return value;
        }

        private static long ReadLong(XElement root, string elementName, long fallback, string presetName,
            IList<string>? issues, bool allowMissing = false)
        {
            XElement? element = root.Element(elementName);
            if (element == null)
            {
                if (!allowMissing)
                {
                    issues?.Add($"'{presetName}': <{elementName}> is missing - the built-in default {fallback} is used.");
                }
                return fallback;
            }

            if (!long.TryParse(element.Value.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out long value))
            {
                issues?.Add($"'{presetName}': <{elementName}> is not a number ('{element.Value.Trim()}') - the built-in default {fallback} is used.");
                return fallback;
            }

            return value;
        }

        // An empty element is a VALUE here, not a failure: it is how a preset says it does
        // not state that node. Only a missing element falls back to the default.
        private static string ReadText(XElement root, string elementName, string fallback, string presetName,
            IList<string>? issues, bool allowMissing = false)
        {
            XElement? element = root.Element(elementName);
            if (element == null)
            {
                if (!allowMissing)
                {
                    issues?.Add($"'{presetName}': <{elementName}> is missing - the built-in default "
                        + (fallback.Length == 0 ? "(device value)" : "'" + fallback + "'") + " is used.");
                }
                return fallback;
            }

            return element.Value.Trim();
        }

        private static float ReadFloat(XElement root, string elementName, float fallback, string presetName, IList<string>? issues)
        {
            XElement? element = root.Element(elementName);
            if (element == null)
            {
                issues?.Add($"'{presetName}': <{elementName}> is missing - the built-in default {fallback.ToString("G9", CultureInfo.InvariantCulture)} is used.");
                return fallback;
            }

            if (!float.TryParse(element.Value.Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out float value))
            {
                issues?.Add($"'{presetName}': <{elementName}> is not a number ('{element.Value.Trim()}') - the built-in default {fallback.ToString("G9", CultureInfo.InvariantCulture)} is used.");
                return fallback;
            }

            return value;
        }

        private static double ReadDouble(XElement root, string elementName, double fallback, string presetName, IList<string>? issues)
        {
            XElement? element = root.Element(elementName);
            if (element == null)
            {
                issues?.Add($"'{presetName}': <{elementName}> is missing - the built-in default {fallback.ToString("R", CultureInfo.InvariantCulture)} is used.");
                return fallback;
            }

            if (!double.TryParse(element.Value.Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out double value))
            {
                issues?.Add($"'{presetName}': <{elementName}> is not a number ('{element.Value.Trim()}') - the built-in default {fallback.ToString("R", CultureInfo.InvariantCulture)} is used.");
                return fallback;
            }

            return value;
        }
    }
}
