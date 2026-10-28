using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Win32;
using OxyPlot;
using OxyPlot.Axes;
using OxyPlot.Legends;
using OxyPlot.Series;
using WsoNativeLib.Presets;

namespace WsoToolkit
{
    public partial class LsoGalvoCalibrationWindow : Window
    {
        private readonly LsoScanTestWindow _host;
        private readonly ColorCameraPresetMode _targetMode;
        private readonly LsoModePreset _targetPreset;
        private readonly int _sensorRows;
        private readonly ObservableCollection<EditablePoint> _points = new ObservableCollection<EditablePoint>();
        private LsoGalvoSlitCalibration _calibration = new LsoGalvoSlitCalibration();
        private string _filePath;
        private bool _updating;
        private bool _dirty;
        private bool _fitValid;
        private bool _closed;
        private CancellationTokenSource? _measurementCancellation;

        public LsoGalvoCalibrationWindow(LsoScanTestWindow host, string file, int start, int end)
        {
            _host = host;
            _targetMode = host.CalibrationTargetMode;
            _targetPreset = host.CalibrationTargetPreset;
            _sensorRows = host.CalibrationSensorRows;
            _filePath = file;
            InitializeComponent();
            Closed += (_, _) => { _closed = true; CancelMeasurement(); };
            myLbTarget.Content = host.CalibrationTargetName;
            myGridPoints.ItemsSource = _points;
            _updating = true;
            myCbOrder.ItemsSource = Enumerable.Range(1, LsoGalvoSlitCalibration.MaxPolynomialOrder).ToArray();
            myCbOrder.SelectedItem = _calibration.ModelType;
            _updating = false;
            AddPoint(new LsoGalvoSlitCalibration.Point(start, null));
            if (end != start)
            {
                AddPoint(new LsoGalvoSlitCalibration.Point(end, null));
            }
            if (File.Exists(file))
            {
                LoadTable(file);
            }
            else
            {
                myTbStatus.Text = "Enter points or record the parked galvo while Alignment is on.";
            }
            RefreshFit();
            if (_points.Count > 0) myGridPoints.SelectedIndex = 0;
        }

        // Keep typed text until it is valid; an incomplete edit must never silently become 0.
        private sealed class EditablePoint : INotifyPropertyChanged
        {
            private string _dac;
            private string _row;
            private string _residual = string.Empty;
            public EditablePoint(LsoGalvoSlitCalibration.Point point)
            {
                _dac = point.GalvoDac.ToString("G17", CultureInfo.CurrentCulture);
                _row = point.SensorRow?.ToString("G17", CultureInfo.CurrentCulture) ?? string.Empty;
                MechanicalAngleDeg = point.MechanicalAngleDeg;
            }
            public double? MechanicalAngleDeg { get; }
            public string GalvoDac { get => _dac; set { _dac = value; PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(GalvoDac))); } }
            public string SensorRow { get => _row; set { _row = value; PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(SensorRow))); } }
            public string Residual { get => _residual; set { _residual = value; PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Residual))); } }
            public event PropertyChangedEventHandler? PropertyChanged;
        }

        private void AddPoint(LsoGalvoSlitCalibration.Point point)
        {
            var item = new EditablePoint(point);
            item.PropertyChanged += PointChanged;
            _points.Add(item);
        }

        private void PointChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (!_updating && e.PropertyName != nameof(EditablePoint.Residual))
            {
                _dirty = true;
                RefreshFit();
            }
        }

        private static bool TryNumber(string text, out double value)
        {
            return (double.TryParse(text, NumberStyles.Float, CultureInfo.CurrentCulture, out value)
                || double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out value))
                && double.IsFinite(value);
        }

        private bool ReadPoints(out string error)
        {
            error = string.Empty;
            if (myCbOrder.SelectedItem is not int order)
            {
                error = "Choose a polynomial order from 1 to 5 (cubic spline is not supported).";
                return false;
            }
            var points = new System.Collections.Generic.List<LsoGalvoSlitCalibration.Point>();
            for (int i = 0; i < _points.Count; i++)
            {
                EditablePoint item = _points[i];
                if (!TryNumber(item.GalvoDac, out double dac) || dac < short.MinValue || dac > short.MaxValue)
                {
                    error = $"Point {i + 1}: Galvo DAC must be a finite number from -32768 to 32767.";
                    return false;
                }
                double? row = null;
                if (!string.IsNullOrWhiteSpace(item.SensorRow))
                {
                    if (!TryNumber(item.SensorRow, out double measuredRow))
                    {
                        error = $"Point {i + 1}: SensorRow must be a finite number or blank.";
                        return false;
                    }
                    row = measuredRow;
                }
                points.Add(new LsoGalvoSlitCalibration.Point(dac, row, item.MechanicalAngleDeg));
            }
            _calibration.ModelType = order;
            _calibration.Points.Clear();
            _calibration.Points.AddRange(points);
            return true;
        }

        private void RefreshFit()
        {
            _fitValid = false;
            foreach (EditablePoint item in _points)
            {
                item.Residual = string.Empty;
            }
            bool validInput = ReadPoints(out string error);
            if (validInput)
            {
                _fitValid = _calibration.TryFit(out error);
                if (_fitValid && (!double.IsFinite(_calibration.R2) || !double.IsFinite(_calibration.Rmse)))
                {
                    _fitValid = false;
                    error = "The fit is not finite. Check the point coordinates or choose a lower order.";
                }
                for (int i = 0; i < _points.Count; i++)
                {
                    double residual = _calibration.Points[i].Residual;
                    _points[i].Residual = double.IsFinite(residual) ? residual.ToString("F2", CultureInfo.CurrentCulture) : string.Empty;
                }
            }
            myTbFit.Text = _fitValid ? _calibration.Describe() : error;
            myTbFile.Text = _filePath + (_dirty ? " (unsaved changes)" : string.Empty);
            DrawPlot(validInput);
            RefreshTargetState();
        }

        private void DrawPlot(bool validInput)
        {
            System.Windows.Media.Color text = SystemColors.WindowTextColor;
            System.Windows.Media.Color accent = SystemColors.HighlightColor;
            System.Windows.Media.Color curve = SystemColors.HotTrackColor;
            var model = new PlotModel
            {
                DefaultFont = FontFamily.Source,
                DefaultFontSize = FontSize,
                TextColor = OxyColor.FromArgb(text.A, text.R, text.G, text.B),
                PlotAreaBorderColor = OxyColor.FromArgb(text.A, text.R, text.G, text.B),
            };
            model.Axes.Add(new LinearAxis { Position = AxisPosition.Bottom, Title = "Sensor row (absolute)" });
            model.Axes.Add(new LinearAxis { Position = AxisPosition.Left, Title = "Galvo DAC" });
            model.Legends.Add(new Legend { LegendPosition = LegendPosition.TopLeft });
            var measured = new ScatterSeries
            {
                Title = "Measured points", MarkerType = MarkerType.Circle,
                MarkerFill = OxyColor.FromArgb(accent.A, accent.R, accent.G, accent.B),
            };
            if (validInput)
            {
                foreach (LsoGalvoSlitCalibration.Point point in _calibration.Points.Where(p => p.SensorRow.HasValue))
                {
                    measured.Points.Add(new ScatterPoint(point.SensorRow!.Value, point.GalvoDac));
                }
            }
            model.Series.Add(measured);
            if (_fitValid)
            {
                var fit = new LineSeries { Title = $"Poly{_calibration.FittedOrder}", Color = OxyColor.FromArgb(curve.A, curve.R, curve.G, curve.B) };
                double first = Math.Min(0, measured.Points.Min(p => p.X));
                double last = Math.Max(Math.Max(1, _sensorRows - 1), measured.Points.Max(p => p.X));
                const int plotSamples = 256;
                for (int i = 0; i <= plotSamples; i++)
                {
                    double row = first + ((last - first) * i / plotSamples);
                    fit.Points.Add(new DataPoint(row, _calibration.EvaluateDac(row)));
                }
                model.Series.Add(fit);
            }
            myPlot.Model = model;
        }

        internal void RefreshTargetState()
        {
            if (_closed)
            {
                return;
            }
            bool current = _host.IsCalibrationTargetCurrent(_targetMode, _targetPreset);
            bool busy = _measurementCancellation != null;
            myTbTargetState.Text = current
                ? "Bound to this preset. Move / Prev / Next move the scanner in Alignment. Settings saves the applied preset."
                : "The selected mode or preset changed. Record and Use are disabled; close and reopen calibration for the new target.";
            myBtUse.IsEnabled = current && _fitValid && !busy;
            myBtRecord.IsEnabled = current && _host.CalibrationMeasurementAvailable && !busy;
            myBtTwoClick.IsEnabled = myBtLive.IsEnabled = myBtRecord.IsEnabled;
            myBtMove.IsEnabled = myBtPrev.IsEnabled = myBtNext.IsEnabled =
                current && _host.CalibrationMeasurementAvailable && !busy;
            myBtCancel.IsEnabled = busy;
            myGridPoints.IsReadOnly = busy;
            myPanelPointActions.IsEnabled = !busy;
            myCbOrder.IsEnabled = !busy;
            myBtFit.IsEnabled = !busy;
            myBtLoad.IsEnabled = !busy;
            myBtSave.IsEnabled = !busy;
            if (!current)
            {
                CancelMeasurement();
            }
        }

        internal void CancelMeasurement() => _measurementCancellation?.Cancel();

        private void myBtAdd_Click(object sender, RoutedEventArgs e)
        {
            AddPoint(new LsoGalvoSlitCalibration.Point(_host.CalibrationGalvoPosition, null));
            _dirty = true;
            myGridPoints.SelectedItem = _points.Last();
            RefreshFit();
        }

        private void myBtDelete_Click(object sender, RoutedEventArgs e)
        {
            foreach (EditablePoint item in myGridPoints.SelectedItems.Cast<EditablePoint>().ToArray())
            {
                item.PropertyChanged -= PointChanged;
                _points.Remove(item);
                _dirty = true;
            }
            RefreshFit();
        }

        private void myCbOrder_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_updating)
            {
                _dirty = true;
                RefreshFit();
            }
        }

        private void myBtFit_Click(object sender, RoutedEventArgs e) => RefreshFit();

        private void myBtReset_Click(object sender, RoutedEventArgs e)
        {
            if (!ConfirmReplace()) return;
            _updating = true;
            foreach (EditablePoint point in _points) point.PropertyChanged -= PointChanged;
            _points.Clear();
            _calibration.ResetNominal(_sensorRows > 0 ? _sensorRows : 3648);
            foreach (var point in _calibration.Points) AddPoint(point);
            myCbOrder.SelectedItem = 3;
            myGridPoints.SelectedIndex = 0;
            _updating = false;
            _dirty = true;
            RefreshFit();
            myTbStatus.Text = "Nine nominal candidates restored; measure each row before applying.";
        }

        private void myBtMove_Click(object sender, RoutedEventArgs e)
        {
            if (_measurementCancellation != null || !_host.IsCalibrationTargetCurrent(_targetMode, _targetPreset)) return;
            if (myGridPoints.SelectedItem is not EditablePoint point || !TryNumber(point.GalvoDac, out double dac))
            {
                myTbStatus.Text = "Select a valid target DAC first.";
                return;
            }
            int target = (int)Math.Clamp(Math.Round(dac), short.MinValue, short.MaxValue);
            myTbStatus.Text = _host.MoveGalvoForCalibration(target, out string error)
                ? $"Moved scanner to DAC {target}." : error;
        }

        private void SelectAdjacent(int direction)
        {
            int current = myGridPoints.SelectedIndex;
            int next = LsoGalvoSlitCalibration.SelectAdjacentPoint(current, _points.Count, direction);
            if (next < 0 || next == Math.Max(0, current)) return;
            myGridPoints.SelectedIndex = next;
            myGridPoints.ScrollIntoView(_points[next]);
            myBtMove_Click(this, new RoutedEventArgs());
        }

        private void myBtPrev_Click(object sender, RoutedEventArgs e) => SelectAdjacent(-1);
        private void myBtNext_Click(object sender, RoutedEventArgs e) => SelectAdjacent(1);
        private async void myBtRecord_Click(object sender, RoutedEventArgs e) => await Measure(false, false);
        private async void myBtTwoClick_Click(object sender, RoutedEventArgs e) => await Measure(true, false);
        private async void myBtLive_Click(object sender, RoutedEventArgs e) => await Measure(false, true);

        private async System.Threading.Tasks.Task Measure(bool twoClick, bool live)
        {
            if (_measurementCancellation != null || !_host.IsCalibrationTargetCurrent(_targetMode, _targetPreset))
            {
                RefreshTargetState();
                return;
            }
            if (!ReadPoints(out string error))
            {
                myTbStatus.Text = error;
                return;
            }
            int dac = _host.CalibrationGalvoPosition;
            EditablePoint? selected = myGridPoints.SelectedItem as EditablePoint;
            if (!live && (selected == null || !TryNumber(selected.GalvoDac, out double selectedDac)
                || Math.Round(selectedDac) != dac))
            {
                myTbStatus.Text = "Select a target point and Move to its DAC before measuring.";
                return;
            }
            bool advance = false;
            var cancellation = new CancellationTokenSource();
            _measurementCancellation = cancellation;
            RefreshTargetState();
            myTbStatus.Text = $"Waiting for a fresh slit frame at DAC {dac}...";
            try
            {
                var result = twoClick
                    ? await _host.MeasureSlitEdgesForCalibrationAsync(_targetMode, _targetPreset, cancellation.Token)
                    : await _host.MeasureSlitRowForCalibrationAsync(_targetMode, _targetPreset, cancellation.Token);
                cancellation.Token.ThrowIfCancellationRequested();
                if (_closed)
                {
                    return;
                }
                if (!result.Row.HasValue)
                {
                    myTbStatus.Text = result.Error;
                    return;
                }
                if (live)
                {
                    _calibration.OpticalCenterRow = Math.Round(result.Row.Value, 1);
                    _dirty = true;
                    RefreshFit();
                    myTbStatus.Text = $"Live optical-center row locked: {_calibration.OpticalCenterRow:F1}.";
                    return;
                }
                EditablePoint item = selected!;
                item.SensorRow = result.Row.Value.ToString("G17", CultureInfo.CurrentCulture);
                _dirty = true;
                myGridPoints.SelectedItem = item;
                RefreshFit();
                myTbStatus.Text = $"Recorded DAC {dac} at sensor row {result.Row.Value:F2}.";
                advance = myCheckAutoNext.IsChecked == true;
            }
            catch (OperationCanceledException)
            {
                if (!_closed) myTbStatus.Text = "Measurement cancelled; no point was changed.";
            }
            catch (Exception ex)
            {
                if (!_closed) myTbStatus.Text = "Measurement failed: " + ex.Message;
            }
            finally
            {
                _measurementCancellation = null;
                cancellation.Dispose();
                RefreshTargetState();
            }
            if (advance) SelectAdjacent(1);
        }

        private void myBtCancel_Click(object sender, RoutedEventArgs e) => CancelMeasurement();

        private bool LoadTable(string path)
        {
            if (!LsoGalvoSlitCalibration.TryLoad(path, out LsoGalvoSlitCalibration? loaded, out string error) || loaded == null)
            {
                myTbStatus.Text = "Load failed: " + error;
                return false;
            }
            _updating = true;
            foreach (EditablePoint item in _points) item.PropertyChanged -= PointChanged;
            _points.Clear();
            _calibration = loaded;
            foreach (LsoGalvoSlitCalibration.Point point in loaded.Points) AddPoint(point);
            myCbOrder.SelectedItem = loaded.ModelType;
            _filePath = path;
            _dirty = false;
            _updating = false;
            myTbStatus.Text = "Loaded calibration JSON; unknown file fields are preserved when saved.";
            RefreshFit();
            return true;
        }

        private void myBtLoad_Click(object sender, RoutedEventArgs e)
        {
            if (!ConfirmReplace()) return;
            var dialog = new OpenFileDialog { Filter = "Calibration JSON (*.json)|*.json", CheckFileExists = true };
            if (dialog.ShowDialog(this) == true) LoadTable(dialog.FileName);
        }

        private bool SaveTable()
        {
            myGridPoints.CommitEdit(DataGridEditingUnit.Cell, true);
            myGridPoints.CommitEdit(DataGridEditingUnit.Row, true);
            if (!ReadPoints(out string error))
            {
                myTbStatus.Text = "Save refused: " + error;
                return false;
            }
            _calibration.TryFit(out _); // Incomplete but valid tables can be saved before fitting.
            var dialog = new SaveFileDialog
            {
                Filter = "Calibration JSON (*.json)|*.json", DefaultExt = ".json", AddExtension = true,
                FileName = Path.GetFileName(_filePath), InitialDirectory = Path.GetDirectoryName(_filePath), OverwritePrompt = true,
            };
            if (dialog.ShowDialog(this) != true) return false;
            try
            {
                _calibration.Save(dialog.FileName);
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException || ex is ArgumentException || ex is InvalidOperationException)
            {
                myTbStatus.Text = "Save failed: " + ex.Message;
                return false;
            }
            _filePath = dialog.FileName;
            _dirty = false;
            RefreshFit();
            myTbStatus.Text = "Saved " + _filePath;
            return true;
        }

        private void myBtSave_Click(object sender, RoutedEventArgs e) => SaveTable();

        private void myBtUse_Click(object sender, RoutedEventArgs e)
        {
            RefreshFit();
            if (!_fitValid || !_host.IsCalibrationTargetCurrent(_targetMode, _targetPreset)) return;
            try
            {
                string directory = _host.CalibrationDirectory;
                string defaultPath = Path.Combine(directory, LsoGalvoSlitCalibration.DefaultFileName);
                string cameraPath = Path.Combine(directory, _calibration.CameraFileName);
                _calibration.Save(defaultPath);
                if (!string.Equals(defaultPath, cameraPath, StringComparison.OrdinalIgnoreCase))
                    _calibration.Save(cameraPath);
                _filePath = cameraPath;
                _dirty = false;
                if (_host.UseCalibrationInPreset(_targetMode, _targetPreset, cameraPath, quiet: false))
                {
                    RefreshFit();
                    myTbStatus.Text = $"Saved {defaultPath} and {cameraPath}; applied to the preset in memory. Save Settings to persist the preset.";
                }
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException || ex is InvalidOperationException)
            {
                myTbStatus.Text = "Save & Apply failed: " + ex.Message;
            }
        }

        private bool ConfirmReplace()
        {
            if (!_dirty) return true;
            MessageBoxResult answer = MessageBox.Show(this, "Save calibration changes before closing or loading another table?",
                Title, MessageBoxButton.YesNoCancel, MessageBoxImage.Question);
            return answer == MessageBoxResult.No || (answer == MessageBoxResult.Yes && SaveTable());
        }

        private void Window_Closing(object? sender, CancelEventArgs e)
        {
            if (_host.IsLoaded && !ConfirmReplace())
            {
                e.Cancel = true;
                return;
            }
            _closed = true;
            CancelMeasurement();
        }

        private void myBtClose_Click(object sender, RoutedEventArgs e) => Close();
    }
}
