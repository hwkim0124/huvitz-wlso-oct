using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using OxyPlot;
using OxyPlot.Annotations;
using OxyPlot.Series;
using WsoNativeLib.Analysis;
using Analysis = WsoNativeLib.Analysis.LsoImageAnalysis;
using Point = System.Windows.Point;

namespace WsoToolkit
{
    public partial class LsoImageAnalysisWindow
    {
        private readonly double[] _edgeOffsets = new double[6];
        private bool _resolutionDragging, _resolutionStart, _thresholdDragging;
        private float _resolutionMin, _resolutionMax;
        private LsoResolutionAnalysis.UsafDetectionResult? _resolution;

        private void RefreshResolution(int thickness, bool rec709, float gain, int mode)
        {
            var profile = Analysis.Extract(_image!, Analysis.Direction.Diagonal, 0, 0, thickness,
                (float)_start.X, (float)_start.Y, (float)_end.X, (float)_end.Y, rec709, gain, mode,
                fafMode: AnalysisFafMode);
            float[] values = profile.Channel(Channel.SelectedIndex);
            var stats = Analysis.GetStatistics(values);
            _resolutionMin = stats.Min; _resolutionMax = stats.Max;
            float percent = (float)UsafThreshold.Value;
            float threshold = stats.Min + percent / 100f * (stats.Max - stats.Min);
            _resolution = LsoResolutionAnalysis.Detect(values, profile.Distance, percent, AutoUsaf.IsChecked == true, _edgeOffsets);
            var model = new PlotModel { Title = "Resolution profile", Subtitle = $"Threshold {threshold:F0} ({percent:F1}%) - drag to adjust" };
            void Add(float[] channel, string name, OxyColor color)
            {
                var line = new LineSeries { Title = name, Color = color };
                for (int i = 0; i < channel.Length; ++i)
                    line.Points.Add(new DataPoint(i * profile.Distance / (channel.Length - 1), channel[i]));
                model.Series.Add(line);
            }
            Add(values, "Active", OxyColors.Black);
            if (FafMode.SelectedIndex > 0)
            {
                Add(profile.R, "R", OxyColors.Red); Add(profile.G, "G", OxyColors.Green); Add(profile.B, "B", OxyColors.Blue);
            }
            model.Annotations.Add(new LineAnnotation { Type = LineAnnotationType.Horizontal, Y = threshold, Color = OxyColors.Crimson, LineStyle = LineStyle.Dash });
            if (_resolution.Success)
            {
                double[] edges = _resolution.EdgeDistances;
                for (int i = 0; i < edges.Length; ++i)
                    model.Annotations.Add(new PointAnnotation { X = edges[i], Y = threshold, Text = $"E{i + 1}", Fill = OxyColors.Green, Shape = MarkerType.Circle });
                ResolutionLabel.Text = $"Bars: {_resolution.DetectedBarCount} | Width: {_resolution.MeanBarWidth:F2} px | Space: {_resolution.MeanSpaceWidth:F2} px | Pitch: {_resolution.Pitch:F2} px/lp ({_resolution.SpatialFrequency:F2} lp/px) | Duty: {_resolution.DutyCycle:F1}% | Mod: {_resolution.TripletContrast:F3} ({_resolution.TripletContrast * 100:F1}%)";
            }
            else
            {
                float modulation = stats.Max + stats.Min > 0.001f ? (stats.Max - stats.Min) / (stats.Max + stats.Min) : 0;
                ResolutionLabel.Text = $"No detection | Modulation (Full Range): {modulation:F3}";
            }
            LsoResolutionAnalysis.Calculate10To90EdgeRise(values, profile.Distance, out double width, out string edgeType);
            ResolutionLabel.Text += $" | 10-90: {width:F2} px ({edgeType})";
            if (FafMode.SelectedIndex > 0)
            {
                var rejection = LsoResolutionAnalysis.FafRejection(profile, FafMode.SelectedIndex + 1);
                ResolutionLabel.Text += $"\nBarrier Rej: {rejection.Db:F1} dB ({rejection.Status}{(rejection.Status == "FAIL" ? " / LEAK" : "")}) | Peak: {rejection.Peak:F0}";
            }
            ResolutionPlot.Model = model;
            DrawResolution();
        }

        private (double Scale, double X, double Y) ResolutionTransform()
        {
            double scale = Math.Min(ResolutionHost.ActualWidth / _image!.Width, ResolutionHost.ActualHeight / _image.Height);
            return (scale, (ResolutionHost.ActualWidth - _image.Width * scale) / 2, (ResolutionHost.ActualHeight - _image.Height * scale) / 2);
        }
        private void DrawResolution()
        {
            ResolutionOverlay.Children.Clear();
            if (_image == null) return;
            var t = ResolutionTransform();
            ResolutionOverlay.Children.Add(new Line { X1 = t.X + _start.X * t.Scale, Y1 = t.Y + _start.Y * t.Scale,
                X2 = t.X + _end.X * t.Scale, Y2 = t.Y + _end.Y * t.Scale, Stroke = Brushes.Lime, StrokeThickness = 2 });
            foreach (Point p in new[] { _start, _end })
            {
                var dot = new Ellipse { Width = 10, Height = 10, Fill = Brushes.Gold };
                Canvas.SetLeft(dot, t.X + p.X * t.Scale - 5); Canvas.SetTop(dot, t.Y + p.Y * t.Scale - 5);
                ResolutionOverlay.Children.Add(dot);
            }
        }
        private Point ResolutionPoint(MouseEventArgs e)
        {
            var t = ResolutionTransform(); Point p = e.GetPosition(ResolutionHost);
            return new Point(Math.Clamp(Math.Round((p.X - t.X) / t.Scale), 0, _image!.Width - 1),
                Math.Clamp(Math.Round((p.Y - t.Y) / t.Scale), 0, _image.Height - 1));
        }
        private void Resolution_Down(object sender, MouseButtonEventArgs e)
        {
            if (_image == null) return;
            Point p = ResolutionPoint(e);
            _resolutionStart = (p - _start).Length <= (p - _end).Length;
            _resolutionDragging = true; ResolutionHost.CaptureMouse();
            Array.Clear(_edgeOffsets); Resolution_Move(sender, e);
        }
        private void Resolution_Move(object sender, MouseEventArgs e)
        {
            if (!_resolutionDragging) return;
            if (_resolutionStart) _start = ResolutionPoint(e); else _end = ResolutionPoint(e);
            RefreshAnalysis(false);
        }
        private void Resolution_Up(object sender, MouseButtonEventArgs e)
        {
            _resolutionDragging = false; ResolutionHost.ReleaseMouseCapture();
        }
        private void Resolution_SizeChanged(object sender, SizeChangedEventArgs e) { if (_ready) DrawResolution(); }
        private void Nudge_Click(object sender, RoutedEventArgs e)
        {
            if (_resolution?.Success != true) return;
            double delta = double.Parse((string)((Button)sender).Tag, CultureInfo.InvariantCulture);
            for (int i = 0; i < 6; ++i)
                if (UsafEdge.SelectedIndex == i || UsafEdge.SelectedIndex == 6) _edgeOffsets[i] += delta;
            RefreshAnalysis(false);
        }
        private void ResetEdges_Click(object sender, RoutedEventArgs e) { Array.Clear(_edgeOffsets); RefreshAnalysis(false); }
        private void Threshold_Down(object sender, MouseButtonEventArgs e)
        {
            if (_image == null || ResolutionPlot.Model?.DefaultYAxis == null) return;
            double threshold = _resolutionMin + UsafThreshold.Value / 100 * (_resolutionMax - _resolutionMin);
            var axis = ResolutionPlot.Model.DefaultYAxis;
            double range = axis.ActualMaximum - axis.ActualMinimum;
            if (range <= 0 || Math.Abs(axis.InverseTransform(e.GetPosition(ResolutionPlot).Y) - threshold) / range >= 0.04) return;
            _thresholdDragging = true; ResolutionPlot.CaptureMouse(); e.Handled = true;
        }
        private void Threshold_Move(object sender, MouseEventArgs e)
        {
            if (!_thresholdDragging || _resolutionMax - _resolutionMin <= 1) return;
            double value = ResolutionPlot.Model.DefaultYAxis.InverseTransform(e.GetPosition(ResolutionPlot).Y);
            float percent = (float)Math.Clamp((value - _resolutionMin) / (_resolutionMax - _resolutionMin) * 100, 1, 99);
            UsafThreshold.Value = Math.Round(percent, 1);
            e.Handled = true;
        }
        private void Threshold_Up(object sender, MouseButtonEventArgs e)
        {
            if (!_thresholdDragging) return;
            _thresholdDragging = false; ResolutionPlot.ReleaseMouseCapture(); e.Handled = true;
        }
    }
}
