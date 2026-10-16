using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;
using OxyPlot;
using OxyPlot.Annotations;
using OxyPlot.Axes;
using OxyPlot.Legends;
using OxyPlot.Series;
using WsoNativeLib.Presets;
using WsoToolkit.utils;
using static WsoNativeLib.WsoLsoScan;
using static WsoToolkit.utils.NumberUtil;

namespace WsoToolkit
{
    // Shows and edits the per-sample galvo table (pos[]) of one mode preset. Two sources:
    // the generated linear sweep (what the board gets without a file - read-only mirror of
    // the native generator) and a CSV table. Only the CSV can be edited. Save to preset
    // writes <Preset>_traj.csv next to the preset and points the preset at it; the board is
    // still written by the Setting window's Apply, like every other scan field.
    public partial class LsoTrajectoryEditorWindow : Window
    {
        private readonly LsoScanProfileModel _scan;
        private readonly string _modeName;
        private readonly string _defaultCsvPath;

        private short[] _generated = Array.Empty<short>();
        private short[]? _csv;
        private string _csvPath = string.Empty;
        private bool _useCsv;
        private readonly Stack<short[]> _undo = new Stack<short[]>();
        private const int MaxUndo = 20;

        private bool _updating;
        private bool _syncingAxes;

        private PlotModel _posModel = null!;
        private PlotModel _velModel = null!;
        private LinearAxis _xPos = null!, _yPos = null!, _xVel = null!, _yVel = null!;
        private LineSeries _seriesCsv = null!, _seriesGen = null!, _seriesVel = null!;
        private AreaSeries _ledBandPos = null!, _ledBandVel = null!;
        private LineAnnotation _annStart = null!, _annEnd = null!, _annVelMean = null!;
        private LineAnnotation _cursorPos = null!, _cursorVel = null!, _cursorShutter = null!;

        // Shutter alignment plot
        private PlotModel _shutterModel = null!;
        private LinearAxis _xShutter = null!, _yShutter = null!;
        private AreaSeries _expCorridor = null!, _slitBand = null!;
        private LineSeries _expMid = null!, _slitCentre = null!;
        private LineAnnotation _annLedOn = null!, _annLedOff = null!;
        private double[] _predBrightUs = Array.Empty<double>();   // per sensor row, from the last computeShutter_

        // Camera-side numbers the simulation needs; the camera preset is the authority for
        // them, so the Setting window hands them over instead of the scan model guessing.
        private readonly int _sensorRows;
        private readonly double _cameraTrigDelayUs;
        private readonly double _exposureUs;

        private static readonly string[] EditOps = { "Set", "Offset", "Scale", "Linear fill", "Smooth" };
        private static readonly string[] EditHints =
        {
            "pos[i] = value",
            "pos[i] += value",
            "pos[i] = c + (pos[i] - c) * value, c = (pos[a] + pos[b]) / 2",
            "pos[a]..pos[b] replaced by the straight line between the two ends (no value)",
            "moving average, window = value (odd, >= 3); the two ends keep their values",
        };

        public LsoTrajectoryEditorWindow(LsoScanProfileModel scan, string modeName, int patternId,
            string presetName, string presetDirectory, int sensorRows, double cameraTrigDelayUs, double exposureUs)
        {
            _scan = scan;
            _modeName = modeName;
            _defaultCsvPath = Path.Combine(presetDirectory, presetName + "_traj.csv");
            _sensorRows = sensorRows > 0 ? sensorRows : 3648;
            _cameraTrigDelayUs = Math.Max(0.0, cameraTrigDelayUs);
            _exposureUs = exposureUs > 0 ? exposureUs : scan.ExposureTimeUs;
            InitializeComponent();
            initSimulation_();
            Title = $"Galvo Trajectory Editor - {modeName} (slot {patternId}, preset '{presetName}')";

            initPlots_();
            _updating = true;
            foreach (string op in EditOps)
            {
                myCbEditOp.Items.Add(op);
            }
            myCbEditOp.SelectedIndex = 0;
            _updating = false;

            _generated = _scan.GenerateLinearTrajectory();
            showGeometry_();

            if (_scan.HasTrajectoryFile)
            {
                short[]? pos = LsoScanProfileModel.LoadTrajectoryFile(_scan.TrajectoryFile, out string error);
                if (pos != null)
                {
                    _csv = pos;
                    _csvPath = _scan.TrajectoryFile;
                    _useCsv = true;
                }
                else
                {
                    MsgBoxUtil.ShowWarning($"The preset's trajectory file could not be loaded - showing the generated sweep.\n"
                        + $"'{_scan.TrajectoryFile}': {error}");
                }
            }

            myTbRangeFrom.Text = "0";
            myTbRangeTo.Text = Math.Max(0, Current.Length - 1).ToString(CultureInfo.InvariantCulture);
            syncSourceControls_();
            refresh_();
        }

        private short[] Current => _useCsv && _csv != null ? _csv : _generated;

        #region Plot setup

        private void initPlots_()
        {
            _posModel = new PlotModel { Title = "Position - pos[i] (DAC, int16)", TitleFontSize = 12 };
            _posModel.Legends.Add(new Legend
            {
                LegendPosition = LegendPosition.TopRight,
                LegendPlacement = LegendPlacement.Inside,
                LegendOrientation = LegendOrientation.Horizontal,
                LegendBackground = OxyColor.FromArgb(220, 255, 255, 255),
                LegendFontSize = 9,
            });
            _xPos = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                MajorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColor.FromRgb(235, 238, 242),
                MinimumPadding = 0.02,
                MaximumPadding = 0.02,
            };
            _yPos = new LinearAxis
            {
                Position = AxisPosition.Left,
                Title = "Galvo position (DAC)",
                MajorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColor.FromRgb(235, 238, 242),
                MinimumPadding = 0.05,
                MaximumPadding = 0.05,
            };
            _posModel.Axes.Add(_xPos);
            _posModel.Axes.Add(_yPos);

            _ledBandPos = new AreaSeries
            {
                Title = "LED on..off",
                Color = OxyColor.FromArgb(120, 214, 158, 46),
                Fill = OxyColor.FromArgb(40, 236, 201, 75),
                StrokeThickness = 1.0,
            };
            _seriesGen = new LineSeries
            {
                Title = "Generated (linear)",
                Color = OxyColor.FromRgb(160, 174, 192),
                StrokeThickness = 1.5,
                LineStyle = LineStyle.Dash,
                TrackerFormatString = "Generated\ni={2:0}\npos={4:0}",
            };
            _seriesCsv = new LineSeries
            {
                Title = "CSV",
                Color = OxyColor.FromRgb(43, 108, 176),
                StrokeThickness = 2.0,
                TrackerFormatString = "CSV\ni={2:0}\npos={4:0}",
            };
            _posModel.Series.Add(_ledBandPos);
            _posModel.Series.Add(_seriesGen);
            _posModel.Series.Add(_seriesCsv);

            _annStart = new LineAnnotation
            {
                Type = LineAnnotationType.Horizontal,
                Color = OxyColor.FromRgb(47, 133, 90),
                LineStyle = LineStyle.Dash,
                StrokeThickness = 1.2,
            };
            _annEnd = new LineAnnotation
            {
                Type = LineAnnotationType.Horizontal,
                Color = OxyColor.FromRgb(197, 48, 48),
                LineStyle = LineStyle.Dash,
                StrokeThickness = 1.2,
            };
            _posModel.Annotations.Add(_annStart);
            _posModel.Annotations.Add(_annEnd);
            _cursorPos = makeCursor_();
            _posModel.Annotations.Add(_cursorPos);
            plotPos.Model = _posModel;

            _velModel = new PlotModel { Title = "Velocity - pos[i+1] - pos[i] (DAC / step)", TitleFontSize = 12 };
            _xVel = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                MajorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColor.FromRgb(235, 238, 242),
                MinimumPadding = 0.02,
                MaximumPadding = 0.02,
            };
            _yVel = new LinearAxis
            {
                Position = AxisPosition.Left,
                Title = "DAC / step",
                MajorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColor.FromRgb(235, 238, 242),
                MinimumPadding = 0.1,
                MaximumPadding = 0.1,
            };
            _velModel.Axes.Add(_xVel);
            _velModel.Axes.Add(_yVel);
            _ledBandVel = new AreaSeries
            {
                Color = OxyColor.FromArgb(120, 214, 158, 46),
                Fill = OxyColor.FromArgb(40, 236, 201, 75),
                StrokeThickness = 1.0,
            };
            _seriesVel = new LineSeries
            {
                Color = OxyColor.FromRgb(43, 108, 176),
                StrokeThickness = 1.5,
                TrackerFormatString = "i={2:0}\ndelta={4:0}",
            };
            _velModel.Series.Add(_ledBandVel);
            _velModel.Series.Add(_seriesVel);
            _annVelMean = new LineAnnotation
            {
                Type = LineAnnotationType.Horizontal,
                Color = OxyColor.FromRgb(47, 133, 90),
                LineStyle = LineStyle.Dot,
                StrokeThickness = 1.0,
            };
            _velModel.Annotations.Add(_annVelMean);
            _cursorVel = makeCursor_();
            _velModel.Annotations.Add(_cursorVel);
            plotVel.Model = _velModel;

            // --- Shutter alignment (row vs time, row 0 at the top like the camera image) ---
            _shutterModel = new PlotModel { Title = "Shutter alignment - sensor row vs time", TitleFontSize = 12 };
            _shutterModel.Legends.Add(new Legend
            {
                LegendPosition = LegendPosition.BottomRight,
                LegendPlacement = LegendPlacement.Inside,
                LegendOrientation = LegendOrientation.Horizontal,
                LegendBackground = OxyColor.FromArgb(220, 255, 255, 255),
                LegendFontSize = 9,
            });
            _xShutter = new LinearAxis
            {
                Position = AxisPosition.Bottom,
                MajorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColor.FromRgb(235, 238, 242),
                MinimumPadding = 0.02,
                MaximumPadding = 0.02,
            };
            _yShutter = new LinearAxis
            {
                Position = AxisPosition.Left,
                Title = "sensor row",
                StartPosition = 1.0,   // row 0 at the top
                EndPosition = 0.0,
                MajorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColor.FromRgb(235, 238, 242),
                MinimumPadding = 0.05,
                MaximumPadding = 0.05,
            };
            _shutterModel.Axes.Add(_xShutter);
            _shutterModel.Axes.Add(_yShutter);
            _expCorridor = new AreaSeries
            {
                Title = "exposure open",
                Color = OxyColor.FromArgb(180, 56, 189, 248),
                Fill = OxyColor.FromArgb(50, 56, 189, 248),
                StrokeThickness = 1.2,
            };
            _expMid = new LineSeries
            {
                Title = "exposure midpoint",
                Color = OxyColor.FromRgb(2, 132, 199),
                StrokeThickness = 1.0,
                LineStyle = LineStyle.Dash,
            };
            _slitBand = new AreaSeries
            {
                Title = "slit (LED on)",
                Color = OxyColor.FromArgb(200, 245, 158, 11),
                Fill = OxyColor.FromArgb(70, 245, 158, 11),
                StrokeThickness = 1.0,
            };
            _slitCentre = new LineSeries
            {
                Title = "slit centre",
                Color = OxyColor.FromRgb(180, 83, 9),
                StrokeThickness = 1.5,
                TrackerFormatString = "slit centre\nx={2:0}\nrow={4:0}",
            };
            _shutterModel.Series.Add(_expCorridor);
            _shutterModel.Series.Add(_expMid);
            _shutterModel.Series.Add(_slitBand);
            _shutterModel.Series.Add(_slitCentre);
            _annLedOn = new LineAnnotation { Type = LineAnnotationType.Vertical, Color = OxyColor.FromRgb(214, 158, 46), LineStyle = LineStyle.Dash, Text = "LED on", TextLinePosition = 0.05 };
            _annLedOff = new LineAnnotation { Type = LineAnnotationType.Vertical, Color = OxyColor.FromRgb(214, 158, 46), LineStyle = LineStyle.Dash, Text = "LED off", TextLinePosition = 0.05 };
            _shutterModel.Annotations.Add(_annLedOn);
            _shutterModel.Annotations.Add(_annLedOff);
            _cursorShutter = makeCursor_();
            _shutterModel.Annotations.Add(_cursorShutter);
            plotShutter.Model = _shutterModel;

            // Both X axes follow each other (DevKit "Lock Time", always on here). OxyPlot 2.2
            // marks every model/axis event obsolete (#111) without a replacement short of a
            // custom PlotController; AxisChanged is the precise hook (zoom/pan only), same as
            // the DevKit uses.
#pragma warning disable CS0618
            _xPos.AxisChanged += (s, e) => { syncAxis_(_xPos, _xVel, plotVel); syncAxis_(_xPos, _xShutter, plotShutter); };
            _xVel.AxisChanged += (s, e) => { syncAxis_(_xVel, _xPos, plotPos); syncAxis_(_xVel, _xShutter, plotShutter); };
            _xShutter.AxisChanged += (s, e) => { syncAxis_(_xShutter, _xPos, plotPos); syncAxis_(_xShutter, _xVel, plotVel); };
#pragma warning restore CS0618
        }

        private static LineAnnotation makeCursor_()
        {
            return new LineAnnotation
            {
                Type = LineAnnotationType.Vertical,
                Color = OxyColor.FromRgb(221, 107, 32),
                StrokeThickness = 1.5,
                X = double.NaN,
            };
        }

        private void syncAxis_(LinearAxis from, LinearAxis to, OxyPlot.Wpf.PlotView view)
        {
            if (_syncingAxes || double.IsNaN(from.ActualMinimum) || double.IsNaN(from.ActualMaximum))
            {
                return;
            }
            if (from.ActualMinimum == to.ActualMinimum && from.ActualMaximum == to.ActualMaximum)
            {
                return;
            }
            _syncingAxes = true;
            try
            {
                to.Zoom(from.ActualMinimum, from.ActualMaximum);
                view.InvalidatePlot(false);
            }
            finally
            {
                _syncingAxes = false;
            }
        }

        #endregion Plot setup

        #region Refresh

        private double XScale => myCheckTimeAxis.IsChecked == true && _scan.TimeStepUs > 0 ? _scan.TimeStepUs : 1.0;

        private void showGeometry_()
        {
            myTbGeoN.Text = _scan.GalvoPatternSize.ToString(CultureInfo.InvariantCulture);
            myTbGeoStep.Text = _scan.TimeStepUs.ToString(CultureInfo.InvariantCulture) + " us";
            myTbGeoStart.Text = _scan.GalvoStartPos.ToString(CultureInfo.InvariantCulture);
            myTbGeoEnd.Text = _scan.GalvoEndPos.ToString(CultureInfo.InvariantCulture);
            myTbGeoLeadIn.Text = _scan.LeadInSamples.ToString(CultureInfo.InvariantCulture);
            myTbGeoLeadOut.Text = _scan.LeadOutSamples.ToString(CultureInfo.InvariantCulture);
            myTbGeoLedOn.Text = _scan.LedOnPosIndex.ToString(CultureInfo.InvariantCulture);
            myTbGeoLedOff.Text = _scan.LedOffPosIndex.ToString(CultureInfo.InvariantCulture);
        }

        private void syncSourceControls_()
        {
            _updating = true;
            rbSourceCsv.IsChecked = _useCsv;
            rbSourceGenerated.IsChecked = !_useCsv;
            rbSourceCsv.IsEnabled = _csv != null;
            _updating = false;

            bool editable = _useCsv && _csv != null;
            myGbEdit.IsEnabled = editable;
            myBtUndo.IsEnabled = editable && _undo.Count > 0;
            myBtClear.IsEnabled = _csv != null;

            myTbCurrentFile.Text = !_useCsv
                ? "(generated - no file)"
                : _csvPath.Length > 0 ? _csvPath : "(unsaved CSV - Save to preset writes " + Path.GetFileName(_defaultCsvPath) + ")";
        }

        private void refresh_()
        {
            double xs = XScale;
            string xTitle = xs == 1.0 ? "sample index i" : "time (us)";
            _xPos.Title = xTitle;
            _xVel.Title = xTitle;
            _xShutter.Title = xTitle;

            short[] shown = Current;

            _seriesGen.Points.Clear();
            _seriesGen.IsVisible = myCheckShowGenerated.IsChecked == true || !_useCsv;
            for (int i = 0; i < _generated.Length; i++)
            {
                _seriesGen.Points.Add(new DataPoint(i * xs, _generated[i]));
            }

            _seriesCsv.Points.Clear();
            _seriesCsv.IsVisible = _useCsv && _csv != null;
            if (_seriesCsv.IsVisible)
            {
                for (int i = 0; i < _csv!.Length; i++)
                {
                    _seriesCsv.Points.Add(new DataPoint(i * xs, _csv[i]));
                }
            }

            _seriesVel.Points.Clear();
            for (int i = 0; i + 1 < shown.Length; i++)
            {
                _seriesVel.Points.Add(new DataPoint(i * xs, shown[i + 1] - shown[i]));
            }

            // LED window as a band across the full Y extent of each panel.
            int ledOn = _scan.LedOnPosIndex, ledOff = _scan.LedOffPosIndex;
            bool ledValid = shown.Length > 0 && ledOff > ledOn && ledOff <= shown.Length;
            fillBand_(_ledBandPos, ledValid, ledOn * xs, ledOff * xs,
                shown.Length > 0 ? shown.Min() : 0, shown.Length > 0 ? shown.Max() : 0, 0.05);
            double velMin = 0, velMax = 0;
            if (_seriesVel.Points.Count > 0)
            {
                velMin = _seriesVel.Points.Min(p => p.Y);
                velMax = _seriesVel.Points.Max(p => p.Y);
            }
            fillBand_(_ledBandVel, ledValid, ledOn * xs, ledOff * xs, velMin, velMax, 0.1);

            _annStart.Y = _scan.GalvoStartPos;
            _annStart.Text = $"Start {_scan.GalvoStartPos}";
            _annEnd.Y = _scan.GalvoEndPos;
            _annEnd.Text = $"End {_scan.GalvoEndPos}";

            updateStatus_(shown, ledValid, ledOn, ledOff);
            computeShutter_(shown);

            plotPos.InvalidatePlot(true);
            plotVel.InvalidatePlot(true);
            plotShutter.InvalidatePlot(true);

            // The table (or its length) changed: the playback range follows, the position is kept.
            syncSimRange_();
            drawSimulation_();
        }

        private static void fillBand_(AreaSeries band, bool visible, double x0, double x1, double yMin, double yMax, double pad)
        {
            band.Points.Clear();
            band.Points2.Clear();
            band.IsVisible = visible;
            if (!visible)
            {
                return;
            }
            double span = Math.Max(1.0, yMax - yMin);
            double lo = yMin - span * pad, hi = yMax + span * pad;
            band.Points.Add(new DataPoint(x0, lo));
            band.Points.Add(new DataPoint(x1, lo));
            band.Points2.Add(new DataPoint(x0, hi));
            band.Points2.Add(new DataPoint(x1, hi));
        }

        private void updateStatus_(short[] shown, bool ledValid, int ledOn, int ledOff)
        {
            if (shown.Length == 0)
            {
                myTbStatus.Text = "No samples - Pattern Size must be 1..8191.";
                myTbStatus.Foreground = Brushes.DarkRed;
                _annVelMean.Text = string.Empty;
                return;
            }

            int deltaMax = 0;
            for (int i = 0; i + 1 < shown.Length; i++)
            {
                deltaMax = Math.Max(deltaMax, Math.Abs(shown[i + 1] - shown[i]));
            }

            string exposure;
            if (ledValid && ledOff - ledOn >= 2)
            {
                var deltas = new List<double>(ledOff - ledOn);
                for (int i = ledOn; i + 1 < ledOff; i++)
                {
                    deltas.Add(shown[i + 1] - shown[i]);
                }
                double mean = deltas.Average();
                double sigma = Math.Sqrt(deltas.Sum(d => (d - mean) * (d - mean)) / deltas.Count);
                _annVelMean.Y = mean;
                _annVelMean.Text = $"exposure mean {mean:F2}";
                exposure = $"exposure [{ledOn}..{ledOff}) velocity mean {mean:F2}, sigma {sigma:F2}";
            }
            else
            {
                _annVelMean.Text = string.Empty;
                exposure = "exposure window invalid (LED on/off outside the table)";
            }

            bool sizeOk = shown.Length == _scan.GalvoPatternSize;
            string sizeText = sizeOk
                ? $"N = Pattern Size ({shown.Length}) OK"
                : $"N {shown.Length} != Pattern Size {_scan.GalvoPatternSize} - Save/Apply resample it to {_scan.GalvoPatternSize} (shape kept)";

            myTbStatus.Text = $"N={shown.Length} · range {shown[0]} -> {shown[shown.Length - 1]} (min {shown.Min()}, max {shown.Max()}) · "
                + $"delta max {deltaMax} DAC/step · {exposure} · {sizeText}";
            myTbStatus.Foreground = !sizeOk ? Brushes.DarkGoldenrod : Brushes.DarkGreen;
        }

        #endregion Refresh

        #region Source / file

        private void Source_Checked(object sender, RoutedEventArgs e)
        {
            if (_updating)
            {
                return;
            }
            _useCsv = rbSourceCsv.IsChecked == true && _csv != null;
            syncSourceControls_();
            refresh_();
        }

        private void myBtCopyGenerated_Click(object sender, RoutedEventArgs e)
        {
            if (_generated.Length == 0)
            {
                MsgBoxUtil.ShowWarning("Nothing to copy - the preset's Pattern Size gives no trajectory.");
                return;
            }
            _csv = (short[])_generated.Clone();
            _csvPath = string.Empty;
            _undo.Clear();
            _useCsv = true;
            syncSourceControls_();
            refresh_();
        }

        private void myBtImport_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Title = "Import galvo trajectory",
                Filter = "CSV / text (*.csv;*.txt)|*.csv;*.txt|All files (*.*)|*.*",
                CheckFileExists = true,
                InitialDirectory = Path.GetDirectoryName(_defaultCsvPath),
            };
            if (dialog.ShowDialog(this) != true)
            {
                return;
            }
            short[]? pos = LsoScanProfileModel.LoadTrajectoryFile(dialog.FileName, out string error);
            if (pos == null)
            {
                MsgBoxUtil.ShowWarning($"'{dialog.FileName}' was not loaded: {error}");
                return;
            }
            _csv = pos;
            _csvPath = dialog.FileName;
            _undo.Clear();
            _useCsv = true;
            syncSourceControls_();
            refresh_();
            if (pos.Length != _scan.GalvoPatternSize)
            {
                MsgBoxUtil.ShowWarning($"The file holds {pos.Length} samples but Pattern Size is {_scan.GalvoPatternSize}. "
                    + $"Save to preset writes it resampled to {_scan.GalvoPatternSize} samples (shape kept); "
                    + "the editor keeps showing the file as imported until then.");
            }
        }

        private void myBtExport_Click(object sender, RoutedEventArgs e)
        {
            short[] shown = Current;
            if (shown.Length == 0)
            {
                MsgBoxUtil.ShowWarning("Nothing to export.");
                return;
            }
            var dialog = new Microsoft.Win32.SaveFileDialog
            {
                Title = "Export galvo trajectory",
                Filter = "CSV (*.csv)|*.csv|All files (*.*)|*.*",
                FileName = Path.GetFileName(_csvPath.Length > 0 ? _csvPath : _defaultCsvPath),
                InitialDirectory = Path.GetDirectoryName(_defaultCsvPath),
            };
            if (dialog.ShowDialog(this) != true)
            {
                return;
            }
            if (!LsoScanProfileModel.SaveTrajectoryFile(dialog.FileName, shown, header_(shown), out string error))
            {
                MsgBoxUtil.ShowWarning($"'{dialog.FileName}' was not written: {error}");
            }
        }

        private void myBtClear_Click(object sender, RoutedEventArgs e)
        {
            _csv = null;
            _csvPath = string.Empty;
            _undo.Clear();
            _useCsv = false;
            syncSourceControls_();
            refresh_();
        }

        private string header_(short[] pos)
        {
            string source = _useCsv ? "csv" : "generated-linear";
            return $"Galvo trajectory  mode={_modeName}  source={source}  N={pos.Length}  timeStepUs={_scan.TimeStepUs}  "
                + $"start={_scan.GalvoStartPos} end={_scan.GalvoEndPos} leadIn={_scan.LeadInSamples} leadOut={_scan.LeadOutSamples}  "
                + $"exported={DateTime.Now:yyyy-MM-ddTHH:mm}";
        }

        // Writes the preset's own file and points the preset at it. The preset XML itself is
        // saved by the Setting window's Apply, like every other scan field.
        private void myBtSave_Click(object sender, RoutedEventArgs e)
        {
            if (!_useCsv || _csv == null)
            {
                _scan.TrajectoryFile = string.Empty;
                DialogResult = true;
                return;
            }
            // Written at the LIVE Pattern Size: the calculator may have moved it since the
            // import, and a file that matches spares the resample-and-warn path at Apply.
            short[] table = LsoScanProfileModel.ResampleTrajectory(_csv, _scan.GalvoPatternSize);
            if (!LsoScanProfileModel.SaveTrajectoryFile(_defaultCsvPath, table, header_(table), out string error))
            {
                MsgBoxUtil.ShowWarning($"'{_defaultCsvPath}' was not written: {error}\nThe preset was NOT changed.");
                return;
            }
            _csv = table;
            _csvPath = _defaultCsvPath;
            _scan.TrajectoryFile = _defaultCsvPath;
            DialogResult = true;
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        #endregion Source / file

        #region Edit

        private void myCbEditOp_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_updating || myCbEditOp.SelectedIndex < 0)
            {
                return;
            }
            int op = myCbEditOp.SelectedIndex;
            myTbEditHint.Text = EditHints[op];
            myLbEditValue.Content = op == 4 ? "Window" : "Value";
            myTbEditValue.IsEnabled = op != 3;
        }

        private void pushUndo_()
        {
            if (_csv == null)
            {
                return;
            }
            if (_undo.Count >= MaxUndo)
            {
                // Stack has no bounded mode; rebuild without the oldest entry.
                var keep = _undo.Take(MaxUndo - 1).Reverse().ToArray();
                _undo.Clear();
                foreach (short[] s in keep)
                {
                    _undo.Push(s);
                }
            }
            _undo.Push((short[])_csv.Clone());
        }

        private void myBtUndo_Click(object sender, RoutedEventArgs e)
        {
            if (_undo.Count == 0)
            {
                return;
            }
            _csv = _undo.Pop();
            syncSourceControls_();
            refresh_();
        }

        private void myBtApplyEdit_Click(object sender, RoutedEventArgs e)
        {
            if (_csv == null || _csv.Length == 0)
            {
                return;
            }
            int last = _csv.Length - 1;
            int a = Math.Clamp(ToInt(myTbRangeFrom.Text), 0, last);
            int b = Math.Clamp(ToInt(myTbRangeTo.Text), 0, last);
            if (a > b)
            {
                (a, b) = (b, a);
            }
            int op = myCbEditOp.SelectedIndex;
            double value = ToDouble(myTbEditValue.Text);

            pushUndo_();
            short[] p = _csv;
            switch (op)
            {
                case 0: // Set
                    for (int i = a; i <= b; i++) p[i] = LsoScanProfileModel.ClampToShort(value);
                    break;
                case 1: // Offset
                    for (int i = a; i <= b; i++) p[i] = LsoScanProfileModel.ClampToShort(p[i] + value);
                    break;
                case 2: // Scale about the range's midpoint value
                {
                    double c = (p[a] + p[b]) / 2.0;
                    for (int i = a; i <= b; i++) p[i] = LsoScanProfileModel.ClampToShort(c + (p[i] - c) * value);
                    break;
                }
                case 3: // Linear fill between the two ends
                    if (b > a)
                    {
                        double pa = p[a], pb = p[b];
                        for (int i = a + 1; i < b; i++)
                        {
                            p[i] = LsoScanProfileModel.ClampToShort(pa + (pb - pa) * (i - a) / (b - a));
                        }
                    }
                    break;
                case 4: // Smooth - moving average, ends kept
                {
                    int window = Math.Max(3, (int)value);
                    if (window % 2 == 0) window++;
                    int half = window / 2;
                    short[] src = (short[])p.Clone();
                    for (int i = a + 1; i < b; i++)
                    {
                        int lo = Math.Max(0, i - half), hi = Math.Min(last, i + half);
                        double sum = 0;
                        for (int j = lo; j <= hi; j++) sum += src[j];
                        p[i] = LsoScanProfileModel.ClampToShort(sum / (hi - lo + 1));
                    }
                    break;
                }
            }
            syncSourceControls_();
            refresh_();
        }

        #endregion Edit

        #region View

        private void View_Changed(object sender, RoutedEventArgs e)
        {
            if (_posModel == null)
            {
                return;
            }
            refresh_();
        }

        private void myBtFitSweep_Click(object sender, RoutedEventArgs e)
        {
            int ledOn = _scan.LedOnPosIndex, ledOff = _scan.LedOffPosIndex;
            if (ledOff <= ledOn)
            {
                return;
            }
            double xs = XScale;
            _xPos.Zoom(ledOn * xs, ledOff * xs);
            plotPos.InvalidatePlot(false);
        }

        private void myBtFitAll_Click(object sender, RoutedEventArgs e)
        {
            _posModel.ResetAllAxes();
            _velModel.ResetAllAxes();
            plotPos.InvalidatePlot(false);
            plotVel.InvalidatePlot(false);
        }

        #endregion View

        #region Simulation

        // Time base: t = 0 is the first DAC sample (EXPOSURE_ACTIVE in TRIG_FIRST, DAC start in
        // GALVANO_FIRST), one sample per Time Step - the same model as the DevKit timing view.
        //   * physical mirror = command delayed by Galvo Delay (transport delay only; no 2nd
        //     order response - the ramp shape is the operator's business here);
        //   * row r exposes during [tRow0 + r * lineTime, + exposure], tRow0 = blanking rows
        //     after the camera trigger (TRIG_FIRST: trigger was before t = 0, so tRow0 =
        //     blanking; GALVANO_FIRST: preMoveDelay + camTrigDelay + blanking);
        //   * slit = Slit Height rows centred on the row the mirror illuminates, lit while
        //     LED on <= i < LED off.
        // DAC -> sensor row: the calibration table when loaded (angle, row-from-centre, DAC),
        // else a linear map Start -> row 0, End -> last row (DevKit mappedRow).
        private readonly DispatcherTimer _simTimer = new DispatcherTimer(DispatcherPriority.Render);
        private DateTime _simLastTick;
        private double _simTimeUs;
        private bool _simPlaying;
        private bool _simSliderUpdating;
        private double[]? _calDacAsc;      // ascending DAC
        private double[]? _calRowByDac;    // row-from-centre at each _calDacAsc
        private static string s_lastCalPath = string.Empty;   // remembered for the session

        private double TotalUs => Current.Length * Math.Max(1, _scan.TimeStepUs);

        private void initSimulation_()
        {
            _simTimer.Interval = TimeSpan.FromMilliseconds(33);
            _simTimer.Tick += simTick_;
            Closed += (s, e) => _simTimer.Stop();
            if (s_lastCalPath.Length > 0 && File.Exists(s_lastCalPath))
            {
                loadCalibration_(s_lastCalPath, quiet: true);
            }
        }

        private void syncSimRange_()
        {
            _simSliderUpdating = true;
            mySliderSim.Maximum = Math.Max(1.0, TotalUs);
            _simTimeUs = Math.Min(_simTimeUs, TotalUs);
            mySliderSim.Value = _simTimeUs;
            _simSliderUpdating = false;
        }

        private double simDurationSec_()
        {
            switch (myCbSimDuration.SelectedIndex)
            {
                case 0: return 2;
                case 2: return 10;
                case 3: return 30;
                default: return 5;
            }
        }

        private void myBtSimPlay_Click(object sender, RoutedEventArgs e)
        {
            if (_simPlaying)
            {
                _simTimer.Stop();
                _simPlaying = false;
                myBtSimPlay.Content = "Play";
                return;
            }
            if (Current.Length == 0)
            {
                return;
            }
            if (_simTimeUs >= TotalUs)
            {
                _simTimeUs = 0;
            }
            _simLastTick = DateTime.UtcNow;
            _simPlaying = true;
            myBtSimPlay.Content = "Pause";
            _simTimer.Start();
        }

        private void myBtSimStop_Click(object sender, RoutedEventArgs e)
        {
            _simTimer.Stop();
            _simPlaying = false;
            myBtSimPlay.Content = "Play";
            _simTimeUs = 0;
            syncSimRange_();
            drawSimulation_();
        }

        private void simTick_(object? sender, EventArgs e)
        {
            DateTime now = DateTime.UtcNow;
            double realSec = (now - _simLastTick).TotalSeconds;
            _simLastTick = now;
            _simTimeUs += realSec * TotalUs / simDurationSec_();
            if (_simTimeUs >= TotalUs)
            {
                _simTimeUs = TotalUs;
                _simTimer.Stop();
                _simPlaying = false;
                myBtSimPlay.Content = "Play";
            }
            _simSliderUpdating = true;
            mySliderSim.Value = _simTimeUs;
            _simSliderUpdating = false;
            drawSimulation_();
        }

        private void mySliderSim_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_simSliderUpdating || _cursorPos == null)
            {
                return;
            }
            _simTimeUs = e.NewValue;
            drawSimulation_();
        }

        private void SensorCanvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            drawBrightnessColumn_();
            drawSimulation_();
        }

        private void myTabLower_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // A PlotView that was hidden in a tab has no size yet; give it one pass.
            if (_shutterModel == null) return;
            plotShutter.InvalidatePlot(false);
            plotVel.InvalidatePlot(false);
        }

        private void myBtSimLoadCal_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Title = "Galvo angle -> sensor row calibration (CSV: angle, row, DAC)",
                Filter = "CSV / text (*.csv;*.txt)|*.csv;*.txt|All files (*.*)|*.*",
                CheckFileExists = true,
            };
            if (dialog.ShowDialog(this) == true)
            {
                loadCalibration_(dialog.FileName, quiet: false);
                refresh_();   // the mapping changed: shutter plot, brightness column and strip follow
            }
        }

        // Three numeric columns per line (angle, sensor row from centre, DAC); header and '#'
        // lines are skipped. Sorted by DAC so the lookup can interpolate.
        private void loadCalibration_(string path, bool quiet)
        {
            var pairs = new List<(double dac, double row)>();
            try
            {
                foreach (string raw in File.ReadLines(path))
                {
                    string line = raw.Trim();
                    if (line.Length == 0 || line[0] == '#')
                    {
                        continue;
                    }
                    string[] tok = line.Split(new[] { ',', ';', '\t' }, StringSplitOptions.RemoveEmptyEntries);
                    if (tok.Length < 3
                        || !double.TryParse(tok[1], NumberStyles.Float, CultureInfo.InvariantCulture, out double row)
                        || !double.TryParse(tok[2], NumberStyles.Float, CultureInfo.InvariantCulture, out double dac))
                    {
                        continue;   // header or malformed line
                    }
                    pairs.Add((dac, row));
                }
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                if (!quiet) MsgBoxUtil.ShowWarning($"'{path}' was not read: {ex.Message}");
                return;
            }
            if (pairs.Count < 2)
            {
                if (!quiet) MsgBoxUtil.ShowWarning($"'{path}' holds no 'angle, row, DAC' rows.");
                return;
            }
            pairs.Sort((a, b) => a.dac.CompareTo(b.dac));
            _calDacAsc = pairs.Select(p => p.dac).ToArray();
            _calRowByDac = pairs.Select(p => p.row).ToArray();
            s_lastCalPath = path;
            myTbSimCal.Text = $"mapping: {Path.GetFileName(path)} ({pairs.Count} pts, DAC {_calDacAsc[0]:0}..{_calDacAsc[^1]:0})";
            myTbSimCal.Foreground = Brushes.DarkGreen;
        }

        // Sensor row index (0 = first exposed row) the mirror illuminates at this DAC.
        private double rowOfDac_(double dac)
        {
            if (_calDacAsc == null || _calRowByDac == null)
            {
                double span = _scan.GalvoEndPos - _scan.GalvoStartPos;
                return span == 0 ? 0 : (dac - _scan.GalvoStartPos) / span * _sensorRows;
            }
            double[] x = _calDacAsc, y = _calRowByDac;
            double rel;
            if (dac <= x[0])
            {
                rel = y[0] + (dac - x[0]) * (y[1] - y[0]) / (x[1] - x[0]);
            }
            else if (dac >= x[^1])
            {
                rel = y[^1] + (dac - x[^1]) * (y[^1] - y[^2]) / (x[^1] - x[^2]);
            }
            else
            {
                int hi = Array.BinarySearch(x, dac);
                if (hi < 0) hi = ~hi;
                int lo = hi - 1;
                double t = (dac - x[lo]) / (x[hi] - x[lo]);
                rel = y[lo] + (y[hi] - y[lo]) * t;
            }
            // The table's row is relative to the sensor centre; the strip wants an index.
            return rel + _sensorRows / 2.0;
        }

        private double dacAt_(short[] table, double sampleIndex)
        {
            if (table.Length == 0) return 0;
            double x = Math.Clamp(sampleIndex, 0, table.Length - 1);
            int i0 = (int)Math.Floor(x);
            int i1 = Math.Min(i0 + 1, table.Length - 1);
            return table[i0] + (table[i1] - table[i0]) * (x - i0);
        }

        private void drawSimulation_()
        {
            if (_cursorPos == null || mySensorCanvas == null)
            {
                return;
            }
            short[] table = Current;
            mySensorCanvas.Children.Clear();
            if (table.Length == 0)
            {
                myTbSimReadout.Text = string.Empty;
                return;
            }

            double stepUs = Math.Max(1, _scan.TimeStepUs);
            double t = _simTimeUs;
            double iCmd = t / stepUs;
            double iPhys = (t - _scan.GalvoDelayUs) / stepUs;
            double dacCmd = dacAt_(table, iCmd);
            double dacPhys = dacAt_(table, iPhys);
            double slitRow = rowOfDac_(dacPhys);
            int slitRows = Math.Max(1, _scan.SlitHeightRows);
            bool ledOn = iCmd >= _scan.LedOnPosIndex && iCmd < _scan.LedOffPosIndex;

            double lineTime = _scan.LineTimeUs;
            // TRIG_FIRST: row 0 opens at the Row 0 offset time (BlankingTimeUs), the same value
            // the calculator adds to G.Delay for the lead-in (DevKit "Row 0 Offset").
            double tRow0 = _scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst
                ? _scan.PreMoveDelayUs + _cameraTrigDelayUs + _scan.BlankingTimeUs
                : _scan.BlankingTimeUs;
            // rows whose exposure window contains t
            double expLo = lineTime > 0 ? (t - _exposureUs - tRow0) / lineTime : double.NaN;
            double expHi = lineTime > 0 ? (t - tRow0) / lineTime : double.NaN;

            // cursors on the plots (command solid; the physical lag is visible in the strip)
            double xs = XScale;
            _cursorPos.X = iCmd * xs;
            _cursorVel.X = iCmd * xs;
            _cursorShutter.X = iCmd * xs;
            plotPos.InvalidatePlot(false);
            plotVel.InvalidatePlot(false);
            plotShutter.InvalidatePlot(false);

            // sensor strip
            double w = mySensorCanvas.ActualWidth, h = mySensorCanvas.ActualHeight;
            if (w > 4 && h > 4)
            {
                double py(double row) => Math.Clamp(row / _sensorRows, -0.05, 1.05) * h;
                if (!double.IsNaN(expLo))
                {
                    double a = py(Math.Max(0, expLo)), b = py(Math.Min(_sensorRows, expHi));
                    if (b > a)
                    {
                        addRect_(0, a, w, b - a, new SolidColorBrush(Color.FromArgb(110, 56, 189, 248)));
                    }
                }
                {
                    double a = py(slitRow - slitRows / 2.0), b = py(slitRow + slitRows / 2.0);
                    if (b - a < 2) b = a + 2;
                    addRect_(0, a, w, b - a, ledOn
                        ? new SolidColorBrush(Color.FromArgb(200, 245, 158, 11))
                        : new SolidColorBrush(Color.FromArgb(120, 140, 140, 140)));
                }
                for (int r = 0; r <= _sensorRows; r += Math.Max(1, _sensorRows / 8))
                {
                    addRect_(0, py(r), 6, 1, Brushes.Gray);
                }
            }
            myTbSensorFoot.Text = $"row {_sensorRows}";

            // overlap of the lit slit with the exposing band, in slit rows
            string overlap = "-";
            if (!double.IsNaN(expLo) && ledOn)
            {
                double lo = Math.Max(slitRow - slitRows / 2.0, expLo), hi = Math.Min(slitRow + slitRows / 2.0, expHi);
                overlap = $"{Math.Clamp((hi - lo) / slitRows * 100.0, 0, 100):F0}%";
            }
            string exposing = double.IsNaN(expLo) ? "n/a (line rate 0)"
                : expHi < 0 ? "not yet (blanking)"
                : expLo > _sensorRows ? "done"
                : $"{Math.Max(0, expLo):F0}..{Math.Min(_sensorRows, expHi):F0}";
            myTbSimReadout.Text =
                $"t = {t:F0} us   i = {iCmd:F0} / {table.Length}   DAC cmd {dacCmd:F0}, mirror {dacPhys:F0} (delay {_scan.GalvoDelayUs:F0} us)   "
                + $"slit row {slitRow:F0} (+/-{slitRows / 2})   LED {(ledOn ? "on" : "off")}   exposing rows {exposing}   overlap {overlap}";
        }

        private void addRect_(double x, double y, double w, double h, Brush fill)
        {
            addRect_(mySensorCanvas, x, y, w, h, fill);
        }

        private static void addRect_(Canvas canvas, double x, double y, double w, double h, Brush fill)
        {
            var rect = new System.Windows.Shapes.Rectangle { Width = Math.Max(0, w), Height = Math.Max(0, h), Fill = fill };
            Canvas.SetLeft(rect, x);
            Canvas.SetTop(rect, y);
            canvas.Children.Add(rect);
        }

        #endregion Simulation

        #region Shutter alignment

        // Whole-sweep view of the same model the playback uses, computed once per table
        // change:
        //   * exposure corridor: row r is open from tRow0 + r * lineTime for Exposure;
        //   * slit centre row(t) from the physical mirror through the DAC -> row mapping;
        //   * per-row error = slit-centre crossing time - exposure midpoint, in rows of time;
        //   * predicted brightness per row = time the LIT slit covered the row while its
        //     exposure was open (capped by Exposure itself) - what a real frame's row-mean
        //     profile should look like.
        private void computeShutter_(short[] table)
        {
            _expCorridor.Points.Clear();
            _expCorridor.Points2.Clear();
            _expMid.Points.Clear();
            _slitBand.Points.Clear();
            _slitBand.Points2.Clear();
            _slitCentre.Points.Clear();
            _predBrightUs = new double[_sensorRows];

            int n = table.Length;
            double stepUs = Math.Max(1, _scan.TimeStepUs);
            double lineTime = _scan.LineTimeUs;
            double xs = XScale;
            double toX(double tUs) => tUs / stepUs * xs;

            if (n == 0 || lineTime <= 0)
            {
                myTbShutterMetrics.Text = n == 0 ? string.Empty : "Line Rate is 0 - no shutter model.";
                myTbShutterMetrics.Foreground = Brushes.DarkRed;
                drawBrightnessColumn_();
                return;
            }

            double tRow0 = _scan.SequenceMode == LsoScanSequenceMode.GalvanoFirst
                ? _scan.PreMoveDelayUs + _cameraTrigDelayUs + _scan.BlankingTimeUs
                : _scan.BlankingTimeUs;
            double exp = _exposureUs;
            int rows = _sensorRows;
            int slitRows = Math.Max(1, _scan.SlitHeightRows);
            double half = slitRows / 2.0;

            // corridor + midpoint: straight lines, two points each
            _expCorridor.Points.Add(new DataPoint(toX(tRow0), 0));
            _expCorridor.Points.Add(new DataPoint(toX(tRow0 + (rows - 1) * lineTime), rows - 1));
            _expCorridor.Points2.Add(new DataPoint(toX(tRow0 + exp), 0));
            _expCorridor.Points2.Add(new DataPoint(toX(tRow0 + (rows - 1) * lineTime + exp), rows - 1));
            _expMid.Points.Add(new DataPoint(toX(tRow0 + exp / 2), 0));
            _expMid.Points.Add(new DataPoint(toX(tRow0 + (rows - 1) * lineTime + exp / 2), rows - 1));

            // slit centre row per sample (physical mirror)
            var slitRow = new double[n];
            for (int i = 0; i < n; i++)
            {
                double t = i * stepUs;
                slitRow[i] = rowOfDac_(dacAt_(table, (t - _scan.GalvoDelayUs) / stepUs));
                _slitCentre.Points.Add(new DataPoint(i * xs, slitRow[i]));
            }
            int ledOn = Math.Clamp(_scan.LedOnPosIndex, 0, n), ledOff = Math.Clamp(_scan.LedOffPosIndex, 0, n);
            for (int i = ledOn; i < ledOff; i++)
            {
                _slitBand.Points.Add(new DataPoint(i * xs, slitRow[i] - half));
                _slitBand.Points2.Add(new DataPoint(i * xs, slitRow[i] + half));
            }
            _annLedOn.X = ledOn * xs;
            _annLedOff.X = ledOff * xs;

            // predicted brightness: for each lit sample, the rows under the slit whose exposure is open
            for (int i = ledOn; i < ledOff; i++)
            {
                double t = i * stepUs;
                int lo = (int)Math.Max(0, Math.Floor(slitRow[i] - half));
                int hi = (int)Math.Min(rows - 1, Math.Ceiling(slitRow[i] + half));
                for (int r = lo; r <= hi; r++)
                {
                    double open = tRow0 + r * lineTime;
                    if (t >= open && t <= open + exp)
                    {
                        _predBrightUs[r] += stepUs;
                    }
                }
            }

            // per-row centring error: first crossing of the slit centre through row r
            int covered = 0, within = 0, lit = 0;
            double errFirst = double.NaN, errLast = double.NaN, errAbsMax = 0, errSum = 0;
            double litSum = 0;
            for (int r = 0; r < rows; r++)
            {
                double tCross = crossingTime_(slitRow, r, stepUs);
                if (!double.IsNaN(tCross))
                {
                    double errRows = (tCross - (tRow0 + r * lineTime + exp / 2)) / lineTime;
                    covered++;
                    if (double.IsNaN(errFirst)) errFirst = errRows;
                    errLast = errRows;
                    errAbsMax = Math.Max(errAbsMax, Math.Abs(errRows));
                    errSum += errRows;
                    if (Math.Abs(errRows) <= half) within++;
                }
                if (_predBrightUs[r] > 0) { lit++; litSum += _predBrightUs[r]; }
            }

            string verdict;
            Brush colour;
            if (covered == 0)
            {
                verdict = "slit never crosses the sensor rows - check the DAC -> row mapping and Start/End.";
                colour = Brushes.DarkRed;
            }
            else
            {
                double meanLit = lit > 0 ? litSum / lit : 0;
                verdict = $"slit centre vs exposure midpoint: row 0 {errFirst:+0;-0} rows, last {errLast:+0;-0} rows, |max| {errAbsMax:0} rows, mean {errSum / covered:+0.0;-0.0}   |   "
                    + $"rows within +/-{half:0} (slit/2): {within}/{rows}   |   rows lit: {lit}/{rows}, mean lit {meanLit:0} us of {exp:0} us exposure"
                    + (_calDacAsc == null ? "   |   mapping LINEAR (no calibration)" : string.Empty);
                colour = within == rows && errAbsMax <= 1.5 ? Brushes.DarkGreen
                    : within >= rows * 0.9 && errAbsMax <= 4.0 ? Brushes.DarkGoldenrod
                    : Brushes.DarkRed;
            }
            myTbShutterMetrics.Text = verdict;
            myTbShutterMetrics.Foreground = colour;

            drawBrightnessColumn_();
        }

        // Time at which the slit centre first reaches row r (linear between samples), or NaN.
        private static double crossingTime_(double[] slitRow, int r, double stepUs)
        {
            for (int i = 0; i + 1 < slitRow.Length; i++)
            {
                double a = slitRow[i], b = slitRow[i + 1];
                if ((a <= r && r <= b) || (b <= r && r <= a))
                {
                    double f = b == a ? 0 : (r - a) / (b - a);
                    return (i + f) * stepUs;
                }
            }
            return double.NaN;
        }

        // Grey-scale column of the predicted per-row brightness, one rectangle per canvas
        // pixel row (rows are averaged into it), full white = Exposure worth of light.
        private void drawBrightnessColumn_()
        {
            if (myBrightCanvas == null) return;
            myBrightCanvas.Children.Clear();
            double w = myBrightCanvas.ActualWidth, h = myBrightCanvas.ActualHeight;
            if (w <= 2 || h <= 2 || _predBrightUs.Length == 0) return;
            double full = Math.Max(1.0, _exposureUs);
            int rows = _predBrightUs.Length;
            int px = (int)h;
            for (int y = 0; y < px; y++)
            {
                int r0 = (int)((long)y * rows / px), r1 = Math.Max(r0 + 1, (int)((long)(y + 1) * rows / px));
                double sum = 0;
                for (int r = r0; r < r1 && r < rows; r++) sum += _predBrightUs[r];
                double v = Math.Clamp(sum / (r1 - r0) / full, 0, 1);
                byte g = (byte)Math.Round(v * 255);
                addRect_(myBrightCanvas, 0, y, w, 1, new SolidColorBrush(Color.FromRgb(g, g, g)));
            }
        }

        #endregion Shutter alignment
    }
}
