using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using Microsoft.Win32;
using OpenCvSharp;
using OxyPlot;
using OxyPlot.Axes;
using OxyPlot.Series;
using WsoNativeLib;
using WsoNativeLib.Analysis;
using Analysis = WsoNativeLib.Analysis.LsoImageAnalysis;
using Frame = WsoNativeLib.Analysis.LsoImageAnalysis.Frame;
using Point = System.Windows.Point;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit
{
    public partial class LsoImageAnalysisWindow : System.Windows.Window
    {
        private static (byte[] Data, int Width, int Height, int Format, int Bytes)? _latest;
        public static (byte[] Data, int Width, int Height, int Format, int Bytes)? Latest => _latest;
        public static void RememberLatest(byte[] data, int width, int height, int format, int bytes)
            => _latest = ((byte[])data.Clone(), width, height, format, bytes);

        private Frame? _image;
        private Point _start, _end;
        private bool _dragging, _dragStart;
        private bool _ready;
        private int _viewGeneration;
        private int AnalysisFafMode => FafMode.SelectedIndex > 0 ? FafMode.SelectedIndex + 1 : 0;

        public LsoImageAnalysisWindow()
        {
            InitializeComponent();
            _ready = true;
        }

        // A full-sensor frame is ~20 M pixels: decoding it on the UI thread left the window
        // white for many seconds, so it is built on a worker and handed back.
        private void Window_Loaded(object sender, RoutedEventArgs e) => LoadLatest();
        private void Latest_Click(object sender, RoutedEventArgs e) => LoadLatest();
        private async void LoadLatest()
        {
            if (_latest is not { } latest) { SourceLabel.Text = "No captured frame yet. Open PNG / TIFF."; return; }
            SourceLabel.Text = $"Loading latest capture {latest.Width} x {latest.Height} ...";
            Frame frame = await Task.Run(() => BuildFrame(latest));
            SetFrame(frame, "Latest capture");
        }

        private static Frame BuildFrame((byte[] Data, int Width, int Height, int Format, int Bytes) latest)
        {
            bool mono = latest.Format is (int)ColorPixelFormat.Mono8 or (int)ColorPixelFormat.Mono16;
            if (latest.Bytes == 2)
            {
                var high = new CameraHighDepthData(latest.Data, latest.Width, latest.Height,
                    latest.Format == (int)ColorPixelFormat.BayerRG12Packed ? 12 : 16, mono);
                return Analysis.FromSensor(high.Raw16, latest.Width, latest.Height, high.BitDepth, mono);
            }
            else
            {
                using var raw = new Mat(latest.Height, latest.Width, latest.Bytes == 3 ? MatType.CV_8UC3 : MatType.CV_8UC1);
                Marshal.Copy(latest.Data, 0, raw.Data, latest.Data.Length);
                using var rgbMat = new Mat();
                if (mono) Cv2.CvtColor(raw, rgbMat, ColorConversionCodes.GRAY2RGB);
                else if (latest.Format == (int)ColorPixelFormat.BayerRG8) Cv2.CvtColor(raw, rgbMat, ColorConversionCodes.BayerBG2RGB);
                else if (latest.Format == (int)ColorPixelFormat.BGR8) Cv2.CvtColor(raw, rgbMat, ColorConversionCodes.BGR2RGB);
                else raw.CopyTo(rgbMat);
                var bytes = new byte[latest.Width * latest.Height * 3];
                Marshal.Copy(rgbMat.Data, bytes, 0, bytes.Length);
                return new Frame(latest.Width, latest.Height, 8, mono, Array.ConvertAll(bytes, b => (float)b));
            }
        }

        private void Open_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog { Filter = "Image|*.png;*.tif;*.tiff;*.bmp;*.jpg|All files|*.*" };
            if (dialog.ShowDialog(this) != true) return;
            try { LoadImage(dialog.FileName); }
            catch (Exception ex) { MessageBox.Show(this, ex.Message, "Open image"); }
        }

        public void LoadImage(string path)
        {
            using var stream = File.OpenRead(path);
            BitmapSource source = BitmapDecoder.Create(stream, BitmapCreateOptions.PreservePixelFormat, BitmapCacheOption.OnLoad).Frames[0];
            bool high = source.Format == PixelFormats.Gray16 || source.Format == PixelFormats.Rgb48;
            bool mono = source.Format == PixelFormats.Gray16 || source.Format == PixelFormats.Gray8;
            int width = source.PixelWidth, height = source.PixelHeight;
            var rgb = new float[width * height * 3];
            if (high)
            {
                var pixels = new ushort[width * height * (mono ? 1 : 3)];
                source.CopyPixels(pixels, width * (mono ? 2 : 6), 0);
                for (int i = 0; i < rgb.Length; ++i) rgb[i] = pixels[mono ? i / 3 : i];
            }
            else
            {
                source = new FormatConvertedBitmap(source, PixelFormats.Rgb24, null, 0);
                var pixels = new byte[rgb.Length];
                source.CopyPixels(pixels, width * 3, 0);
                for (int i = 0; i < rgb.Length; ++i) rgb[i] = pixels[i];
            }
            SetFrame(new Frame(width, height, high ? 16 : 8, mono, rgb), path);
        }

        private void SetFrame(Frame image, string label)
        {
            _image = image;
            _start = new Point(image.Width / 2, image.Height / 2);
            _end = new Point(image.Width - 1, image.Height - 1);
            SourceLabel.Text = $"{label} | {image.Width} x {image.Height} | {image.BitDepth}-bit";
            Array.Clear(_edgeOffsets);
            RefreshAnalysis();
        }

        private void Changed(object sender, RoutedEventArgs e) { if (_ready) RefreshAnalysis(); }
        private void Gain_Changed(object sender, RoutedPropertyChangedEventArgs<double> e) { if (_ready) RefreshAnalysis(); }
        private void Auto_Click(object sender, RoutedEventArgs e) { if (_image != null) Gain.Value = Analysis.AutoGain(_image); }
        private void Peak_Click(object sender, RoutedEventArgs e)
        {
            if (_image == null) return;
            bool horizontal = Direction.SelectedIndex != 1;
            int target = Analysis.SnapToPeak(_image, horizontal);
            _start = horizontal ? new Point(_start.X, target) : new Point(target, _start.Y);
            RefreshAnalysis();
        }

        private void RefreshAnalysis(bool full = true)
        {
            if (_image == null) return;
            Frame image = _image;
            int thickness = int.TryParse(Thickness.Text, out int value) ? Math.Clamp(value, 1, 999) : 1;
            int mode = Curve.SelectedIndex;
            float gain = (float)Gain.Value, analysisGain = ApplyAnalysis.IsChecked == true ? gain : 1;
            bool rec709 = Rec709.IsChecked == true;
            var direction = (Analysis.Direction)Direction.SelectedIndex;
            var profile = Analysis.Extract(image, direction, (int)_start.Y, (int)_start.X, thickness,
                (float)_start.X, (float)_start.Y, (float)_end.X, (float)_end.Y, rec709, analysisGain, mode,
                fafMode: AnalysisFafMode);
            var model = new PlotModel { Title = "Line profile" };
            void Add(float[] values, string name, OxyColor color, double distance)
            {
                var series = new LineSeries { Title = name, Color = color };
                for (int i = 0; i < values.Length; ++i) series.Points.Add(new DataPoint(i * distance / Math.Max(1, values.Length - 1), values[i]));
                model.Series.Add(series);
            }
            Add(profile.Y, "Y", OxyColors.Black, profile.Distance);
            if (!image.IsMono)
            {
                Add(profile.R, "R", OxyColors.Red, profile.Distance); Add(profile.G, "G", OxyColors.Green, profile.Distance); Add(profile.B, "B", OxyColors.Blue, profile.Distance);
            }
            if (direction == Analysis.Direction.Cross)
            {
                var vertical = Analysis.Extract(image, Analysis.Direction.Vertical, (int)_start.Y, (int)_start.X, thickness,
                    rec709: rec709, gain: analysisGain, mode: mode, fafMode: AnalysisFafMode);
                Add(vertical.Channel(Channel.SelectedIndex), "Vertical", OxyColors.Purple, vertical.Distance);
            }
            ProfilePlot.Model = model;
            var stats = Analysis.GetStatistics(profile.Channel(Channel.SelectedIndex));
            StatisticsLabel.Text = $"Min {stats.Min:F2} | Max {stats.Max:F2} | Mean {stats.Mean:F2} | Std {stats.Std:F2}\nLine Uniformity {stats.Uniformity:F1}% | Gain {gain:F1}x | Row {(int)_start.Y}, Column {(int)_start.X}";
            RefreshResolution(thickness, rec709, analysisGain, mode);
            if (!full) { DrawLines(); return; }
            var curveModel = new PlotModel { Title = "Brightness curve" };
            var curve = new LineSeries();
            for (int i = 0; i <= 255; ++i)
            {
                float x = i * image.Maximum / 255f;
                curve.Points.Add(new DataPoint(x, Analysis.ApplyBoost(x, gain, mode, image.Maximum)));
            }
            curveModel.Series.Add(curve); CurvePlot.Model = curveModel;
            DrawLines();
            int block = Block.SelectedIndex switch { 1 => 32, 2 => 64, _ => 16 };
            RefreshImageViews(image, block, Metric.SelectedIndex, rec709, analysisGain, gain, mode);
        }

        // The whole-image passes (uniformity map, boosted display) run on a worker; a newer
        // refresh supersedes an older one still running.
        private async void RefreshImageViews(Frame image, int block, int metric, bool rec709, float analysisGain, float gain, int mode)
        {
            int generation = ++_viewGeneration;
            var (map, display) = await Task.Run(() =>
            {
                float[,] uniformity = Analysis.UniformityMap(image, block, metric, rec709, analysisGain, mode);
                var bytes = new byte[image.Rgb.Length];
                int rowLength = image.Width * 3;
                Parallel.For(0, image.Height, y =>
                {
                    for (int i = y * rowLength; i < (y + 1) * rowLength; ++i)
                        bytes[i] = (byte)Math.Clamp((int)Math.Round(Analysis.ApplyBoost(image.Rgb[i], gain, mode, image.Maximum) * 255f / image.Maximum), 0, 255);
                });
                return (uniformity, bytes);
            });
            if (generation != _viewGeneration || image != _image) return;
            var mapModel = new PlotModel { Title = "2D uniformity" };
            if (map.GetLength(0) > 0 && map.GetLength(1) > 0)
            {
                var data = new double[map.GetLength(0), map.GetLength(1)];
                for (int y = 0; y < map.GetLength(1); ++y)
                    for (int x = 0; x < map.GetLength(0); ++x) data[x, y] = map[x, y];
                mapModel.Axes.Add(new LinearColorAxis { Position = AxisPosition.Right, Palette = OxyPalettes.Jet(256) });
                mapModel.Axes.Add(new LinearAxis { Position = AxisPosition.Left, StartPosition = 1, EndPosition = 0 });
                mapModel.Series.Add(new HeatMapSeries { X0 = 0, X1 = map.GetLength(0) - 1, Y0 = 0, Y1 = map.GetLength(1) - 1, Data = data, Interpolate = false });
            }
            else mapModel.Subtitle = "Image is smaller than one block.";
            MapPlot.Model = mapModel;
            Preview.Source = BitmapSource.Create(image.Width, image.Height, 96, 96, PixelFormats.Rgb24, null, display, image.Width * 3);
            ResolutionPreview.Source = Preview.Source;
            DrawLines();
        }

        private (double Scale, double X, double Y) ImageTransform()
        {
            double scale = Math.Min(ImageHost.ActualWidth / _image!.Width, ImageHost.ActualHeight / _image.Height);
            return (scale, (ImageHost.ActualWidth - _image.Width * scale) / 2, (ImageHost.ActualHeight - _image.Height * scale) / 2);
        }
        private void DrawLines()
        {
            Overlay.Children.Clear();
            if (_image == null) return;
            var t = ImageTransform();
            void Line(double x1, double y1, double x2, double y2) => Overlay.Children.Add(new Line
            {
                X1 = t.X + x1 * t.Scale, Y1 = t.Y + y1 * t.Scale, X2 = t.X + x2 * t.Scale, Y2 = t.Y + y2 * t.Scale,
                Stroke = Brushes.Lime, StrokeThickness = 2
            });
            if (Direction.SelectedIndex == 3) Line(_start.X, _start.Y, _end.X, _end.Y);
            else
            {
                if (Direction.SelectedIndex != 1) Line(0, _start.Y, _image.Width - 1, _start.Y);
                if (Direction.SelectedIndex is 1 or 2) Line(_start.X, 0, _start.X, _image.Height - 1);
            }
        }
        private Point ImagePoint(MouseEventArgs e)
        {
            var t = ImageTransform(); Point p = e.GetPosition(ImageHost);
            return new Point(Math.Clamp((p.X - t.X) / t.Scale, 0, _image!.Width - 1), Math.Clamp((p.Y - t.Y) / t.Scale, 0, _image.Height - 1));
        }
        private void Image_Down(object sender, MouseButtonEventArgs e)
        {
            if (_image == null) return;
            Point p = ImagePoint(e);
            _dragStart = (p - _start).Length <= (p - _end).Length;
            _dragging = true; ImageHost.CaptureMouse(); MoveLine(p);
        }
        private void MoveLine(Point p)
        {
            if (Direction.SelectedIndex != 3 || _dragStart) _start = p; else _end = p;
            RefreshAnalysis(false);
        }
        private void Image_Move(object sender, MouseEventArgs e) { if (_dragging) MoveLine(ImagePoint(e)); }
        private void Image_Up(object sender, MouseButtonEventArgs e) { _dragging = false; ImageHost.ReleaseMouseCapture(); }
        private void Image_SizeChanged(object sender, SizeChangedEventArgs e) { if (_ready) DrawLines(); }
    }
}
