using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using WsoNativeLib;
using WsoNativeLib.Presets;
using WsoToolkit.utils;
using static WsoNativeLib.WsoLsoDefs;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    // Editor for ONE camera preset - whichever mode (Preview / Capture) the caller opened it
    // for. It reads and writes the preset, never the live camera: a preset edited in the
    // middle of a session must not reprogram a camera that is streaming, and the values are
    // installed by Apply, or by the Preview/Capture one-call when that mode next starts.
    //
    // Editing changes the preset OBJECT only. The file is written by Save As, or by Apply
    // once the hardware confirmed it - which is what lets a failed Apply leave the previously
    // saved preset on disk untouched. (Before A-1 every keystroke saved, so a value the
    // camera then rejected was already the stored one.)
    //
    // The camera is asked two things, and only two: its ROI maxima (Center ROI / Max Size
    // need the sensor's real limits) and, for each enum node, which entries it actually
    // implements. With no camera attached the preset's own maxima are used and the combos
    // fall back to the DevKit configs' vocabulary, which the note under them says out loud
    // rather than presenting as a device-confirmed list.
    public partial class ColorCameraSettingWindow
    {
        bool bInit = false;

        private List<TextBox> _imageFormatInputBoxsList = new List<TextBox>();
        private LsoColorCameraSettingParam _colorCameraParam = new LsoColorCameraSettingParam();

        private ColorCameraPreset Preset => _scanTestModel.ModePresets.Active(_mode).Camera;

        // The entry that stores nothing. A node left on this keeps whatever the device - or
        // the start path that owns it - already had.
        private const string DeviceDefaultLabel = "(device default)";

        // One camera per process, and what it offers only changes when the configuration
        // changes - so the node map is read once per Apply instead of ~31 nodes on every
        // reload (a reload runs on every commit, mode switch and preset selection, and
        // reading the node map while the camera streams stalls the UI thread).
        private static readonly Dictionary<string, IReadOnlyList<string>> _enumEntryCache
            = new Dictionary<string, IReadOnlyList<string>>();
        private static readonly Dictionary<string, string> _nodeRangeCache
            = new Dictionary<string, string>();

        // Called by the hosting window after a successful Apply, while the camera is stopped.
        internal static void InvalidateCapabilityCache()
        {
            _enumEntryCache.Clear();
            _nodeRangeCache.Clear();
        }

        // A camera that answered nothing is NOT cached: that is the "no camera / node not
        // implemented" case, and caching it would keep the fallback vocabulary in place
        // after the device shows up.
        private static IReadOnlyList<string> enumEntries_(string nodeName)
        {
            if (_enumEntryCache.TryGetValue(nodeName, out IReadOnlyList<string>? cached))
            {
                return cached;
            }

            IReadOnlyList<string> entries = LsoCamera.GetEnumEntries(nodeName);
            if (entries.Count > 0)
            {
                _enumEntryCache[nodeName] = entries;
            }
            return entries;
        }

        private List<string> _AcqModeList = new List<string>() { "Continuous", "Single Frame", "Multi Frame"};
        private SortedDictionary<string, uint> _PixelFormatList = new SortedDictionary<string, uint>() 
        {
            { "Mono8", (uint)ColorPixelFormat.Mono8 },
            { "Mono16", (uint)ColorPixelFormat.Mono16 },
            //{ "RGB8Packed", (uint)ColorPixelFormat.RGB8Packed },
            { "BayerRG8", (uint)ColorPixelFormat.BayerRG8 },
            { "BayerRG16", (uint)ColorPixelFormat.BayerRG16 },
            //{ "BGR8", (uint)ColorPixelFormat.BGR8 },
        };
        private SortedDictionary<string, uint> _AdcBitDepthList = new SortedDictionary<string, uint>()
        {
            { "10 Bit", (uint)ColorAdcBitDepth.Bit10 },
            { "12 Bit", (uint)ColorAdcBitDepth.Bit12 },
        };

        private SortedDictionary<string, uint> _BinningValueList = new SortedDictionary<string, uint>()
        {
            { "1", 1 },
            { "2", 2 },
        };

        // Vocabulary used when the camera cannot be asked. These are the values the DevKit
        // configs and setupHwTriggerSetting() use on this instrument - a starting point for
        // an operator with the camera unplugged, NOT a claim that a given camera supports
        // them. Whether a value exists is settled by the apply's per-field read-back.
        private static readonly string[] FallbackExposureAuto = { "Off", "Once", "Continuous" };
        private static readonly string[] FallbackBinningSelector = { "All", "Sensor", "ISP" };
        private static readonly string[] FallbackBinningMode = { "Sum", "Average" };
        private static readonly string[] FallbackTestPattern = { "Off", "GreyDiagonalRamp", "SensorTestPattern" };
        private static readonly string[] FallbackSensorShutterMode = { "Global", "Rolling", "GlobalReset" };
        private static readonly string[] FallbackTriggerMode = { "Off", "On" };
        private static readonly string[] FallbackTriggerSource = { "Software", "Line0", "Line1", "Line2", "Line3" };
        private static readonly string[] FallbackTriggerActivation = { "LevelLow", "LevelHigh", "FallingEdge", "RisingEdge", "AnyEdge" };
        private static readonly string[] FallbackTriggerSelector = { "AcquisitionStart", "FrameStart", "FrameBurstStart" };
        private static readonly string[] FallbackTriggerOverlap = { "Off", "ReadOut", "PreviousFrame" };
        private static readonly string[] FallbackLineMode = { "Input", "Output" };
        private static readonly string[] FallbackLineSource = { "Off", "ExposureActive", "UserOutput0", "UserOutput1", "FrameTriggerWait" };
        private static readonly string[] LineSelectorNames = { "Line0", "Line1", "Line2", "Line3" };

        // True when at least one combo was filled from the camera's own node map.
        private bool _capabilityFromDevice;

        // Fields the last commit could not parse. Emptied at the start of every commit, so
        // it always describes the current contents of the editor.
        private readonly List<string> _inputErrors = new List<string>();

        /// <summary>Why the last CommitToPreset refused, one line per field. Empty when it
        /// succeeded.</summary>
        internal IReadOnlyList<string> InputErrors => _inputErrors;

        private ComboBox[] LineModeCombos => new[] { myComboLine0Mode, myComboLine1Mode, myComboLine2Mode, myComboLine3Mode };
        private ComboBox[] LineSourceCombos => new[] { myComboLine0Source, myComboLine1Source, myComboLine2Source, myComboLine3Source };
        private ComboBox[] LineInverterCombos => new[] { myComboLine0Inverter, myComboLine1Inverter, myComboLine2Inverter, myComboLine3Inverter };

        private void initControls_()
        {
            _imageFormatInputBoxsList.Add(myTbROIXWidth);
            _imageFormatInputBoxsList.Add(myTbROIYHeight);
            _imageFormatInputBoxsList.Add(myTbROIXOffset);
            _imageFormatInputBoxsList.Add(myTbROIYOffset);
            _imageFormatInputBoxsList.Add(myTbExposureTime);
            _imageFormatInputBoxsList.Add(myTbAcquisitionFrmaeCount);
            _imageFormatInputBoxsList.Add(myTbGain);
            _imageFormatInputBoxsList.Add(myTbTriggerDelay);
            _imageFormatInputBoxsList.Add(myTbBurstFrameCount);
            _imageFormatInputBoxsList.Add(myTbDeviceLinkThroughput);

            myComboAcquisitionMode.ItemsSource = _AcqModeList;

            foreach (var pixelformat in _PixelFormatList)
            {
                var item = new ComboBoxItem { Content = pixelformat.Key, Tag = pixelformat.Value };
                myComboPixelFormat.Items.Add(item);
            }

            foreach (var depth in _AdcBitDepthList)
            {
                var item = new ComboBoxItem { Content = depth.Key, Tag = depth.Value };
                myComboADCBitDepth.Items.Add(item);
            }

            foreach (var value in _BinningValueList)
            {
                var item = new ComboBoxItem { Content = value.Key, Tag = value.Value };
                myComboBinningHorizontal.Items.Add(item);
            }

            foreach (var value in _BinningValueList)
            {
                var item = new ComboBoxItem { Content = value.Key, Tag = value.Value };
                myComboBinningVertical.Items.Add(item);
            }
        }

        // Fills the editor from the preset. Only the ROI maxima come from the camera, and
        // only when it reports them - they describe the sensor, not the preset.
        private void loadParams_()
        {
            _colorCameraParam = Preset.ToSettingParam();

            LsoCamera.GetCameraParameters(out LsoColorCameraSettingParam live);
            if (live.roiMaxWidth > 0 && live.roiMaxHeight > 0)
            {
                _colorCameraParam.roiMaxWidth = live.roiMaxWidth;
                _colorCameraParam.roiMaxHeight = live.roiMaxHeight;
            }
        }

        // Repopulating the enum combos raises SelectionChanged for every Clear/Add, and that
        // handler writes the controls back into the preset. Without this guard a refresh
        // would commit a half-filled combo over the value it is in the middle of showing.
        private void updateControlDatas_()
        {
            bool wasUpdating = bInit;
            bInit = true;
            try
            {
                updateControlDatasCore_();
            }
            finally
            {
                bInit = wasUpdating;
            }
        }

        private void updateControlDatasCore_()
        {
            myTbROIXWidth.Text = _colorCameraParam.roiXWidth.ToString();
            myTbROIYHeight.Text = _colorCameraParam.roiYHeight.ToString();
            myTbROIXOffset.Text = _colorCameraParam.roiXOffset.ToString();
            myTbROIYOffset.Text = _colorCameraParam.roiYOffset.ToString();
            myTbExposureTime.Text = _colorCameraParam.exposureTime.ToString();
            myTbGain.Text = _colorCameraParam.gain.ToString();
            myTbTriggerDelay.Text = optionalNumberText_(Preset.TriggerDelayUs);

            myComboAcquisitionMode.SelectedIndex = (int)_colorCameraParam.acquisitionMode;

            myTbAcquisitionFrmaeCount.Text = _colorCameraParam.acquisitionFrameCount.ToString();

            uint nPixelFormat = _colorCameraParam.pixelFormat;
            foreach (ComboBoxItem item in myComboPixelFormat.Items)
            {
                if (item.Tag is uint value && value == nPixelFormat)
                {
                    myComboPixelFormat.SelectedItem = item;
                }
            }

            uint nBinningHorizontal = _colorCameraParam.binningHorizontal;
            foreach (ComboBoxItem item in myComboBinningHorizontal.Items)
            {
                if (item.Tag is uint value && value == nBinningHorizontal)
                {
                    myComboBinningHorizontal.SelectedItem = item;
                }
            }

            uint nBinningVertical = _colorCameraParam.binningVertical;
            foreach (ComboBoxItem item in myComboBinningVertical.Items)
            {
                if (item.Tag is uint value && value == nBinningVertical)
                {
                    myComboBinningVertical.SelectedItem = item;
                }
            }

            uint nAdcBitDepth = _colorCameraParam.adcDepthIndex;
            foreach (ComboBoxItem item in myComboADCBitDepth.Items)
            {
                if (item.Tag is uint value && value == nAdcBitDepth)
                {
                    myComboADCBitDepth.SelectedItem = item;
                }
            }

            updateExtendedControlDatas_();
        }

        // --- A-1 extension ----------------------------------------------------------------

        private void updateExtendedControlDatas_()
        {
            _capabilityFromDevice = false;

            fillEnumCombo_(myComboExposureAuto, "ExposureAuto", FallbackExposureAuto, Preset.ExposureAuto);
            fillEnumCombo_(myComboBinningSelector, "BinningSelector", FallbackBinningSelector, Preset.BinningSelector);
            fillEnumCombo_(myComboBinningHorizontalMode, "BinningHorizontalMode", FallbackBinningMode, Preset.BinningHorizontalMode);
            fillEnumCombo_(myComboBinningVerticalMode, "BinningVerticalMode", FallbackBinningMode, Preset.BinningVerticalMode);
            fillEnumCombo_(myComboTestPattern, "TestPattern", FallbackTestPattern, Preset.TestPattern);
            fillEnumCombo_(myComboSensorShutterMode, "SensorShutterMode", FallbackSensorShutterMode, Preset.SensorShutterMode);
            fillEnumCombo_(myComboTriggerMode, "TriggerMode", FallbackTriggerMode, Preset.TriggerMode);
            fillEnumCombo_(myComboTriggerSource, "TriggerSource", FallbackTriggerSource, Preset.TriggerSource);
            fillEnumCombo_(myComboTriggerActivation, "TriggerActivation", FallbackTriggerActivation, Preset.TriggerActivation);
            fillEnumCombo_(myComboTriggerSelector, "TriggerSelector", FallbackTriggerSelector, Preset.TriggerSelector);
            fillEnumCombo_(myComboTriggerOverlap, "TriggerOverlap", FallbackTriggerOverlap, Preset.TriggerOverlap);
            fillEnumCombo_(myComboLineSelector, "LineSelector", LineSelectorNames, Preset.LineSelector);

            fillTriStateCombo_(myComboIspEnable, Preset.IspEnable);

            ComboBox[] modes = LineModeCombos;
            ComboBox[] sources = LineSourceCombos;
            ComboBox[] inverters = LineInverterCombos;
            for (int i = 0; i < ColorCameraPreset.LineCount; ++i)
            {
                fillEnumCombo_(modes[i], "LineMode", FallbackLineMode, Preset.Lines[i].Mode);
                fillEnumCombo_(sources[i], "LineSource", FallbackLineSource, Preset.Lines[i].Source);
                fillTriStateCombo_(inverters[i], Preset.Lines[i].Inverter);
            }

            myTbBurstFrameCount.Text = optionalNumberText_(Preset.AcquisitionBurstFrameCount);
            myTbDeviceLinkThroughput.Text = optionalNumberText_(Preset.DeviceLinkThroughput);

            // The numeric nodes get the same capability treatment the combos get: the range
            // and increment come from the device, so a value it cannot hold is visible
            // before Apply rather than after. With no camera the label says so.
            myLbBurstFrameCountRange.Content = describeNodeRange_("AcquisitionBurstFrameCount");
            myLbDeviceLinkThroughputRange.Content = describeNodeRange_("DeviceLinkThroughputLimit");
            myLbTriggerDelayRange.Content = describeNodeRange_("TriggerDelay");

            myTbCapabilityNote.Text = _capabilityFromDevice
                ? "Choices and ranges come from this camera's node map."
                : "No camera answered - the choices are the DevKit vocabulary, not a device-confirmed list, "
                  + "and no ranges are available. Apply reports per field what the camera accepted.";
        }

        // "min .. max (step N)" as the camera reports it, or an explicit "no camera" so an
        // empty label is never mistaken for "unlimited". A node the camera does not
        // implement says so here instead of only failing at Apply.
        private string describeNodeRange_(string nodeName)
        {
            // Cached with the enum vocabulary, so the "read-only now" hint is as of the last
            // Apply rather than of this instant - the price of not touching the device on
            // every reload.
            if (_nodeRangeCache.TryGetValue(nodeName, out string? cachedText))
            {
                _capabilityFromDevice = true;
                return cachedText;
            }

            if (!LsoCamera.GetNodeRange(nodeName, out double min, out double max,
                    out double increment, out bool writable))
            {
                return _capabilityFromDevice
                    ? "not implemented by this camera"
                    : "range: no camera";
            }

            _capabilityFromDevice = true;

            string text = $"{min.ToString("G", CultureInfo.InvariantCulture)} .. "
                + max.ToString("G", CultureInfo.InvariantCulture);
            if (increment > 0)
            {
                text += " (step " + increment.ToString("G", CultureInfo.InvariantCulture) + ")";
            }
            if (!writable)
            {
                text += " - read-only now";
            }

            _nodeRangeCache[nodeName] = text;
            return text;
        }

        // Fills one enum combo with "(device default)" plus the entries the CAMERA reports,
        // or the fallback vocabulary when it reports none. A stored value the camera does not
        // list is added and marked rather than dropped: opening the editor must not silently
        // rewrite a preset that was tuned on another camera.
        private void fillEnumCombo_(ComboBox combo, string nodeName, IReadOnlyList<string> fallback, string current)
        {
            IReadOnlyList<string> entries = enumEntries_(nodeName);
            if (entries.Count > 0)
            {
                _capabilityFromDevice = true;
            }
            else
            {
                entries = fallback;
            }

            var desired = new List<KeyValuePair<string, string>>(entries.Count + 2)
            {
                new KeyValuePair<string, string>(string.Empty, DeviceDefaultLabel),
            };

            bool currentListed = false;
            foreach (string entry in entries)
            {
                desired.Add(new KeyValuePair<string, string>(entry, entry));
                if (string.Equals(entry, current, StringComparison.Ordinal))
                {
                    currentListed = true;
                }
            }

            if (!currentListed && !string.IsNullOrEmpty(current))
            {
                desired.Add(new KeyValuePair<string, string>(current,
                    current + " (not offered by this camera)"));
            }

            replaceItemsIfChanged_(combo, desired);
            selectByTag_(combo, current ?? string.Empty);
        }

        // Rebuilding the items is what raises SelectionChanged - and the ComboBoxItem
        // containers are generated at layout time, so that event can land AFTER the bInit
        // guard has been cleared. The handler then repopulates the whole panel, which
        // rebuilds the items again: an endless loop that spins the UI thread and freezes the
        // window. Identical content is therefore left in place, which also makes a reload
        // idempotent instead of a full teardown of every combo.
        private static void replaceItemsIfChanged_(ComboBox combo,
            List<KeyValuePair<string, string>> desired)
        {
            if (itemsMatch_(combo, desired))
            {
                return;
            }

            combo.Items.Clear();
            foreach (KeyValuePair<string, string> entry in desired)
            {
                combo.Items.Add(new ComboBoxItem { Content = entry.Value, Tag = entry.Key });
            }
        }

        private static bool itemsMatch_(ComboBox combo, List<KeyValuePair<string, string>> desired)
        {
            if (combo.Items.Count != desired.Count)
            {
                return false;
            }

            for (int i = 0; i < desired.Count; ++i)
            {
                if (combo.Items[i] is not ComboBoxItem item
                    || !string.Equals(item.Tag as string, desired[i].Key, StringComparison.Ordinal)
                    || !string.Equals(item.Content as string, desired[i].Value, StringComparison.Ordinal))
                {
                    return false;
                }
            }

            return true;
        }

        // "(device default)" / False / True.
        private void fillTriStateCombo_(ComboBox combo, int current)
        {
            var desired = new List<KeyValuePair<string, string>>(3)
            {
                new KeyValuePair<string, string>(string.Empty, DeviceDefaultLabel),
                new KeyValuePair<string, string>("0", "False"),
                new KeyValuePair<string, string>("1", "True"),
            };

            replaceItemsIfChanged_(combo, desired);
            selectByTag_(combo, current < 0 ? string.Empty : current.ToString(CultureInfo.InvariantCulture));
        }

        private static void selectByTag_(ComboBox combo, string tag)
        {
            foreach (ComboBoxItem item in combo.Items)
            {
                if (item.Tag is string value && string.Equals(value, tag, StringComparison.Ordinal))
                {
                    combo.SelectedItem = item;
                    return;
                }
            }

            combo.SelectedIndex = 0;
        }

        private static string tagOf_(ComboBox combo)
        {
            return combo.SelectedItem is ComboBoxItem item && item.Tag is string value ? value : string.Empty;
        }

        private static int triStateOf_(ComboBox combo)
        {
            string tag = tagOf_(combo);
            return tag.Length == 0 ? ColorCameraLinePreset.NotStated : (tag == "1" ? 1 : 0);
        }

        // An empty box means "(device default)" - the same thing the combos' first entry
        // means - so it stores the negative sentinel rather than a zero the camera would
        // actually apply.
        private static string optionalNumberText_(long value)
        {
            return value < 0 ? string.Empty : value.ToString(CultureInfo.InvariantCulture);
        }

        private static string optionalNumberText_(double value)
        {
            return value < 0 ? string.Empty : value.ToString("R", CultureInfo.InvariantCulture);
        }

        // Input that cannot be parsed is REFUSED, not quietly replaced by the previous
        // value: the operator typed something, and silently keeping the old number means
        // applying a configuration nobody asked for. The offending field is named in
        // `_inputErrors` and the apply stops before any hardware call.
        private long readOptionalLong_(string label, string text, long fallback)
        {
            string trimmed = (text ?? string.Empty).Trim();
            if (trimmed.Length == 0)
            {
                return ColorCameraPreset.NotStatedNumber;
            }
            if (!long.TryParse(trimmed, NumberStyles.Integer, CultureInfo.InvariantCulture, out long value))
            {
                _inputErrors.Add($"{label}: '{trimmed}' is not a whole number. "
                    + "Leave it empty for the device default.");
                return fallback;
            }
            return value;
        }

        private double readOptionalDouble_(string label, string text, double fallback)
        {
            string trimmed = (text ?? string.Empty).Trim();
            if (trimmed.Length == 0)
            {
                return ColorCameraPreset.NotStatedDelay;
            }
            if (!double.TryParse(trimmed, NumberStyles.Float, CultureInfo.InvariantCulture, out double value))
            {
                _inputErrors.Add($"{label}: '{trimmed}' is not a number. "
                    + "Leave it empty for the device default.");
                return fallback;
            }
            return value;
        }

        private void clearTextBoxFocus_()
        {
            // 1) TextBox 포커스 잃기
            Keyboard.ClearFocus();

            foreach (TextBox Tb in _imageFormatInputBoxsList)
            {
                var be = Tb.GetBindingExpression(TextBox.TextProperty); be?.UpdateSource();
            }
        }

        // Writes the editor's values into the preset OBJECT. The file is NOT written here:
        // Apply writes it once the hardware confirmed, Save As writes it on request. Nothing
        // is sent to the camera here either - see the class comment.
        private bool applyControlsToPreset_()
        {
            bool bRet = false;
            _inputErrors.Clear();

            try
            {
                LsoColorCameraSettingParam colorCameraParam = _colorCameraParam;

                colorCameraParam.roiXWidth = ToUInt(myTbROIXWidth.Text);
                colorCameraParam.roiYHeight = ToUInt(myTbROIYHeight.Text);
                colorCameraParam.roiXOffset = ToUInt(myTbROIXOffset.Text);
                colorCameraParam.roiYOffset = ToUInt(myTbROIYOffset.Text);
                colorCameraParam.exposureTime = ToUInt(myTbExposureTime.Text);
                colorCameraParam.gain = ToFloat(myTbGain.Text);

                colorCameraParam.acquisitionMode = (uint)myComboAcquisitionMode.SelectedIndex;

                colorCameraParam.acquisitionFrameCount = ToUInt(myTbAcquisitionFrmaeCount.Text);

                ComboBoxItem item = (ComboBoxItem)myComboPixelFormat.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.pixelFormat = (uint)item.Tag;
                }

                item = (ComboBoxItem)myComboBinningHorizontal.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.binningHorizontal = (uint)item.Tag;
                }

                item = (ComboBoxItem)myComboBinningVertical.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.binningVertical = (uint)item.Tag;
                }

                item = (ComboBoxItem)myComboADCBitDepth.SelectedItem;
                if (item != null)
                {
                    colorCameraParam.adcDepthIndex = (uint)item.Tag;
                }

                Preset.ApplySettingParam(colorCameraParam);
                Preset.TriggerDelayUs = readOptionalDouble_("Trigger Delay (us)", myTbTriggerDelay.Text, Preset.TriggerDelayUs);

                Preset.ExposureAuto = tagOf_(myComboExposureAuto);
                Preset.BinningSelector = tagOf_(myComboBinningSelector);
                Preset.BinningHorizontalMode = tagOf_(myComboBinningHorizontalMode);
                Preset.BinningVerticalMode = tagOf_(myComboBinningVerticalMode);
                Preset.TestPattern = tagOf_(myComboTestPattern);
                Preset.SensorShutterMode = tagOf_(myComboSensorShutterMode);
                Preset.TriggerMode = tagOf_(myComboTriggerMode);
                Preset.TriggerSource = tagOf_(myComboTriggerSource);
                Preset.TriggerActivation = tagOf_(myComboTriggerActivation);
                Preset.TriggerSelector = tagOf_(myComboTriggerSelector);
                Preset.TriggerOverlap = tagOf_(myComboTriggerOverlap);
                Preset.LineSelector = tagOf_(myComboLineSelector);
                Preset.IspEnable = triStateOf_(myComboIspEnable);
                Preset.AcquisitionBurstFrameCount = readOptionalLong_("Acq. Burst Frame Count",
                    myTbBurstFrameCount.Text, Preset.AcquisitionBurstFrameCount);
                Preset.DeviceLinkThroughput = readOptionalLong_("Device Link Throughput",
                    myTbDeviceLinkThroughput.Text, Preset.DeviceLinkThroughput);

                ComboBox[] modes = LineModeCombos;
                ComboBox[] sources = LineSourceCombos;
                ComboBox[] inverters = LineInverterCombos;
                for (int i = 0; i < ColorCameraPreset.LineCount; ++i)
                {
                    Preset.Lines[i].Mode = tagOf_(modes[i]);
                    Preset.Lines[i].Source = tagOf_(sources[i]);
                    Preset.Lines[i].Inverter = triStateOf_(inverters[i]);
                }

                // Refuse the commit when a field could not be read. The caller stops before
                // any hardware call, which is what D-6 step 4 requires of invalid input.
                bRet = _inputErrors.Count == 0;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                bRet = false;
            }

            return bRet;
        }

        // "Default" = this mode's DevKit baseline (BfsCameraConfigPreview.json for Preview,
        // BfsCameraConfig.json for Capture), written into the preset object.
        private bool setDefaultCameraParameters_()
        {
            try
            {
                Preset.CopyValuesFrom(ColorCameraPreset.DevKitDefault(_mode, Preset.Name));
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                return false;
            }
        }

        // Read-back into the preset: what the camera is actually holding right now. Nodes it
        // cannot report are left as the preset has them, so this can never blank a value the
        // operator typed.
        private bool readExtendedFromCamera_()
        {
            if (!LsoCamera.GetExtendedParameters(out LsoColorCameraExtendedParam param))
            {
                MsgBoxUtil.ShowWarning("The camera did not report its extended settings "
                    + "(is it connected and idle?). Nothing was changed.");
                return false;
            }

            Preset.ApplyExtendedParam(param);
            return true;
        }

        private void reportPresetIssues_()
        {
            IReadOnlyList<string> issues = _scanTestModel.ModePresets.DrainIssues();
            if (issues.Count == 0)
            {
                return;
            }

            MsgBoxUtil.ShowWarning("Camera presets:" + Environment.NewLine + Environment.NewLine
                + string.Join(Environment.NewLine, issues));
        }

        private void setDefaultColorMode_()
        {
            uint nMode = (uint)ColorPixelFormat.BayerRG8;
            foreach (ComboBoxItem item in myComboPixelFormat.Items)
            {
                if (item.Tag is uint value && value == nMode)
                {
                    myComboPixelFormat.SelectedItem = item;
                    break;
                }
            }

            uint nBinningHorizontal = 1;
            foreach (ComboBoxItem item in myComboBinningHorizontal.Items)
            {
                if (item.Tag is uint value && value == nBinningHorizontal)
                {
                    myComboBinningHorizontal.SelectedItem = item;
                    break;
                }
            }

            uint nBinningVertical = 1;
            foreach (ComboBoxItem item in myComboBinningVertical.Items)
            {
                if (item.Tag is uint value && value == nBinningVertical)
                {
                    myComboBinningVertical.SelectedItem = item;
                    break;
                }
            }

            uint nMaxWidth = _colorCameraParam.roiMaxWidth;
            uint nMaxHeight = _colorCameraParam.roiMaxHeight;

            myTbROIXWidth.Text = nMaxWidth.ToString();
            myTbROIYHeight.Text = nMaxHeight.ToString();
            myTbROIXOffset.Text = "0";
            myTbROIYOffset.Text = "0";
        }

        private void setDefaultMonoMode_()
        {
            uint nMode = (uint)ColorPixelFormat.Mono8;
            foreach (ComboBoxItem item in myComboPixelFormat.Items)
            {
                if (item.Tag is uint value && value == nMode)
                {
                    myComboPixelFormat.SelectedItem = item;
                    break;
                }
            }

            uint nBinningHorizontal = 2;
            foreach (ComboBoxItem item in myComboBinningHorizontal.Items)
            {
                if (item.Tag is uint value && value == nBinningHorizontal)
                {
                    myComboBinningHorizontal.SelectedItem = item;
                    break;
                }
            }

            uint nBinningVertical = 2;
            foreach (ComboBoxItem item in myComboBinningVertical.Items)
            {
                if (item.Tag is uint value && value == nBinningVertical)
                {
                    myComboBinningVertical.SelectedItem = item;
                    break;
                }
            }

            uint nMaxWidth = _colorCameraParam.roiMaxWidth;
            uint nMaxHeight = _colorCameraParam.roiMaxHeight;

            myTbROIXWidth.Text = nMaxWidth.ToString();
            myTbROIYHeight.Text = nMaxHeight.ToString();
            myTbROIXOffset.Text = "0";
            myTbROIYOffset.Text = "0";
        }
    }
}
