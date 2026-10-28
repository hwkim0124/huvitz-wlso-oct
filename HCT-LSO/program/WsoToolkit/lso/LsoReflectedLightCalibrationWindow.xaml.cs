using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using WsoNativeLib.Presets;

namespace WsoToolkit
{
    public partial class LsoReflectedLightCalibrationWindow : Window
    {
        private readonly LsoScanTestWindow _scan;
        private CancellationTokenSource? _cancel;
        private bool _closeWhenFinished;

        public LsoReflectedLightCalibrationWindow(LsoScanTestWindow scan)
        {
            _scan = scan;
            InitializeComponent();
            myFolder.Text = LsoConfigLocation.ReflectedLightDirectory();
            refreshSummary_();
            Range_TextChanged(this, new TextChangedEventArgs(TextBox.TextChangedEvent, UndoAction.None));
        }

        private void refreshSummary_()
        {
            mySummary.Text = LsoReflectedLightSet.TryLoad(LsoConfigLocation.ReflectedLightDirectory(),
                out var set, out _) && set != null
                ? $"{set.Points.Count} points; ROI {set.Roi.X}/{set.Roi.Y}/{set.Roi.Width}/{set.Roi.Height}; created {set.Created:yyyy-MM-dd HH:mm:ss}"
                : "(not calibrated)";
        }

        private void Range_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (myWarning == null || myFrom == null || myTo == null) return;
            myWarning.Text = double.TryParse(myFrom.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double from)
                && double.TryParse(myTo.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double to)
                && from == -15 && to == 15 ? string.Empty
                : "partial run - the whole map set is replaced by these points";
        }

        private static int integer_(TextBox box) =>
            int.TryParse(box.Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out int value)
                ? value : throw new ArgumentException("ROI and frames per point must be integers.");

        private static double number_(TextBox box) =>
            double.TryParse(box.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double value)
                && double.IsFinite(value) ? value
                : throw new ArgumentException("Start, end and step must be finite numbers (use a decimal point).");

        private async void Start_Click(object sender, RoutedEventArgs e)
        {
            if (_cancel != null) return;
            var elapsed = new Stopwatch();
            try
            {
                var roi = new LsoReflectedLightRoi(integer_(myX), integer_(myY), integer_(myWidth), integer_(myHeight));
                int frames = integer_(myFrames);
                double from = number_(myFrom), to = number_(myTo), step = number_(myStep);
                _scan.ValidateReflectedLightCalibration(roi, frames, from, to, step);
                if (MessageBox.Show(this,
                    "Is the objective covered with the black cloth, and is nobody's eye in front of the instrument?",
                    Title, MessageBoxButton.YesNo, MessageBoxImage.Question, MessageBoxResult.No) != MessageBoxResult.Yes)
                    return;

                _cancel = new CancellationTokenSource();
                myInputs.IsEnabled = false;
                myStart.IsEnabled = false;
                myCancel.Content = "Cancel";
                myProgress.Value = 0;
                myStatus.Text = "Preparing calibration and probe capture...";
                elapsed.Start();
                var progress = new Progress<LsoReflectedLightProgress>(p =>
                {
                    myProgress.Maximum = p.PointCount * p.FramesPerPoint;
                    myProgress.Value = (p.Point - 1) * p.FramesPerPoint + p.Frame;
                    myStatus.Text = $"{p.Text}; point {p.Point}/{p.PointCount}, frame {p.Frame}/{p.FramesPerPoint}";
                });
                var result = await _scan.RunReflectedLightCalibrationAsync(roi, frames, from, to, step, progress, _cancel.Token);
                elapsed.Stop();
                string outcome = result.Succeeded ? "Succeeded" : result.Cancelled ? "Cancelled" : "Error";
                myStatus.Text = $"{outcome}; points written: {result.PointsWritten}; restored position: {result.RestoredPosition}; elapsed: {elapsed.Elapsed.TotalSeconds:F1} s."
                    + (string.IsNullOrEmpty(result.Error) ? string.Empty : "\n" + result.Error);
                MessageBox.Show(this, myStatus.Text, Title, MessageBoxButton.OK,
                    result.Succeeded ? MessageBoxImage.Information : MessageBoxImage.Warning);
            }
            catch (Exception ex)
            {
                myStatus.Text = $"Calibration was not completed: {ex.Message}";
                MessageBox.Show(this, myStatus.Text, Title, MessageBoxButton.OK, MessageBoxImage.Warning);
            }
            finally
            {
                _cancel?.Dispose();
                _cancel = null;
                myInputs.IsEnabled = true;
                myStart.IsEnabled = true;
                myCancel.Content = "Close";
                myCancel.IsEnabled = true;
                _scan.AfterReflectedLightCalibration();
                refreshSummary_();
                if (_closeWhenFinished) Close();
            }
        }

        private void cancel_()
        {
            if (_cancel == null) return;
            _cancel.Cancel();
            myCancel.IsEnabled = false;
            myStatus.Text = "Cancelling; waiting for the current capture and focus restoration...";
        }

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            if (_cancel == null) Close();
            else cancel_();
        }

        private void Window_Closing(object? sender, CancelEventArgs e)
        {
            if (_cancel == null) return;
            e.Cancel = true;
            _closeWhenFinished = true;
            cancel_();
        }
    }
}
