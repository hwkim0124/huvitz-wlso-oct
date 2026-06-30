using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using WsoNativeLib;

namespace WsoToolkit.controls
{
    public partial class OctReportSectionChart : UserControl
    {
        public WsoOctScan.OctETDRSChartDescript ETDRSDescript;
        public WsoOctScan.OctClockChartDescript ClockDescript;

        private bool _isETDRSChart = false;
        private bool _isClockChart = false;
        public bool IsETDRSChart { get => _isETDRSChart; set { _isETDRSChart = value; _isClockChart = !value; } }
        public bool IsClockChart { get => _isClockChart; set { _isClockChart = value; _isETDRSChart = !value; } }

        public OctReportSectionChart()
        {
            InitializeComponent();

            IsETDRSChart = true;
            ClearChart();
        }

        public void ClearChart()
        {
            ETDRSDescript = new WsoOctScan.OctETDRSChartDescript(WsoDomain.EyeSide.OD);
            ClockDescript = new WsoOctScan.OctClockChartDescript(WsoDomain.EyeSide.OD);
        }

        private FormattedText GetETDRSThicknessText(int index)
        {
            var value = "";
            if (ETDRSDescript.eyeSide == WsoDomain.EyeSide.OD)
            {
                value = index switch
                {
                    0 => ETDRSDescript.centerThick == 0.0 ? "N/A" : ((int)ETDRSDescript.centerThick).ToString(),
                    1 => ETDRSDescript.innerThicks[0] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[0]).ToString(),
                    2 => ETDRSDescript.innerThicks[1] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[1]).ToString(),
                    3 => ETDRSDescript.innerThicks[2] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[2]).ToString(),
                    4 => ETDRSDescript.innerThicks[3] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[3]).ToString(),
                    5 => ETDRSDescript.outerThicks[0] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[0]).ToString(),
                    6 => ETDRSDescript.outerThicks[1] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[1]).ToString(),
                    7 => ETDRSDescript.outerThicks[2] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[2]).ToString(),
                    8 => ETDRSDescript.outerThicks[3] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[3]).ToString(),
                    _ => "",
                };
            }
            else
            {
                value = index switch
                {
                    0 => ETDRSDescript.centerThick == 0.0 ? "N/A" : ((int)ETDRSDescript.centerThick).ToString(),
                    1 => ETDRSDescript.innerThicks[0] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[0]).ToString(),
                    2 => ETDRSDescript.innerThicks[3] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[3]).ToString(),
                    3 => ETDRSDescript.innerThicks[2] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[2]).ToString(),
                    4 => ETDRSDescript.innerThicks[1] == 0.0 ? "N/A" : ((int)ETDRSDescript.innerThicks[1]).ToString(),
                    5 => ETDRSDescript.outerThicks[0] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[0]).ToString(),
                    6 => ETDRSDescript.outerThicks[3] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[3]).ToString(),
                    7 => ETDRSDescript.outerThicks[2] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[2]).ToString(),
                    8 => ETDRSDescript.outerThicks[1] == 0.0 ? "N/A" : ((int)ETDRSDescript.outerThicks[1]).ToString(),
                    _ => "",
                };
            }

            var text = new FormattedText(value, CultureInfo.CurrentCulture, FlowDirection.LeftToRight, new Typeface("Arial"), 14.0, Brushes.Black, VisualTreeHelper.GetDpi(this).PixelsPerDip);
            text.TextAlignment = TextAlignment.Center;
            return text;
        }

        private FormattedText GetClockThicknessText(int index)
        {
            var value = index switch
            {
                >= 0 and <= 11 => ClockDescript.clockThicks[index] == 0.0 ? "" : ((int)ClockDescript.clockThicks[index]).ToString(),
                _ => "",
            };

            var text = new FormattedText(value, CultureInfo.CurrentCulture, FlowDirection.LeftToRight, new Typeface("Arial"), 14.0, Brushes.Black, VisualTreeHelper.GetDpi(this).PixelsPerDip);
            text.TextAlignment = TextAlignment.Center;
            return text;
        }

        public void Refresh()
        {
            RenderChart();
            UpdateChartStatus();
        }

        private void UpdateChartStatus()
        {
            if (IsETDRSChart)
            {
                if (ETDRSDescript.centerThick <= 0.0f)
                {
                    return;
                }
                string side = ETDRSDescript.eyeSide == WsoDomain.EyeSide.OS ? "OS" : "OD";
                string s = string.Format("{0}, Center: {1:F2}, {2:F2}", side, ETDRSDescript.centerX, ETDRSDescript.centerY);
                lblStatus.Content = s;
            }
            else if (IsClockChart)
            {
                if (ClockDescript.clockThicks[0] <= 0.0f)
                {
                    return;
                }
                string side = ClockDescript.eyeSide == WsoDomain.EyeSide.OS ? "OS" : "OD";
                string s = string.Format("{0}, Center: {1:F2}, {2:F2}", side, ClockDescript.centerX, ClockDescript.centerY);
                lblStatus.Content = s;
            }
        }

        private void RenderETDRSChart()
        {
            int width = (int)canvasChart.ActualWidth;
            int height = (int)canvasChart.ActualHeight;

            canvasChart.Children.Clear();

            var bg_color = Color.FromRgb(0xFF, 0xFF, 0xFF);
            canvasChart.Background = new SolidColorBrush(bg_color);

            var line_color = Color.FromRgb(0x00, 0x00, 0x00);
            var pen = new Pen(new SolidColorBrush(line_color), 1.0);

            DrawingVisual visual = new();
            DrawingContext context = visual.RenderOpen();

            var mm_size = width / 6;
            var center = new Point(width / 2, height / 2);
            var radius3 = mm_size * 3.0;
            var radius2 = mm_size * 1.5;
            var radius1 = mm_size * 0.5;

            context.DrawEllipse(null, pen, new Point(width / 2, height / 2), radius1, radius1);
            context.DrawEllipse(null, pen, new Point(width / 2, height / 2), radius2, radius2);
            context.DrawEllipse(null, pen, new Point(width / 2, height / 2), radius3, radius3);

            var pi = Math.PI;
            var point1 = new Point(center.X + radius1 * Math.Cos(pi / 4), center.Y + radius1 * Math.Sin(pi / 4));
            var point2 = new Point(center.X + radius3 * Math.Cos(pi / 4), center.Y + radius3 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            point1 = new Point(center.X + radius1 * Math.Cos(pi / 4), center.Y - radius1 * Math.Sin(pi / 4));
            point2 = new Point(center.X + radius3 * Math.Cos(pi / 4), center.Y - radius3 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            point1 = new Point(center.X - radius1 * Math.Cos(pi / 4), center.Y + radius1 * Math.Sin(pi / 4));
            point2 = new Point(center.X - radius3 * Math.Cos(pi / 4), center.Y + radius3 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            point1 = new Point(center.X - radius1 * Math.Cos(pi / 4), center.Y - radius1 * Math.Sin(pi / 4));
            point2 = new Point(center.X - radius3 * Math.Cos(pi / 4), center.Y - radius3 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            var y_offs = 8.0;
            var textpos = new Point(center.X, center.Y - y_offs);
            context.DrawText(GetETDRSThicknessText(0), new Point(textpos.X, textpos.Y));

            var delta1 = radius1 + (radius2 - radius1) / 2;
            var delta2 = radius2 + (radius3 - radius2) / 2;
            context.DrawText(GetETDRSThicknessText(1), new Point(textpos.X, textpos.Y - delta1));
            context.DrawText(GetETDRSThicknessText(2), new Point(textpos.X + delta1, textpos.Y));
            context.DrawText(GetETDRSThicknessText(3), new Point(textpos.X, textpos.Y + delta1));
            context.DrawText(GetETDRSThicknessText(4), new Point(textpos.X - delta1, textpos.Y));

            context.DrawText(GetETDRSThicknessText(5), new Point(textpos.X, textpos.Y - delta2));
            context.DrawText(GetETDRSThicknessText(6), new Point(textpos.X + delta2, textpos.Y));
            context.DrawText(GetETDRSThicknessText(7), new Point(textpos.X, textpos.Y + delta2));
            context.DrawText(GetETDRSThicknessText(8), new Point(textpos.X - delta2, textpos.Y));
            context.Close();


            var bitmap = new RenderTargetBitmap(width, height, 0, 0, PixelFormats.Pbgra32);
            bitmap.Render(visual);

            Image image = new();
            image.Source = bitmap;
            canvasChart.Children.Add(image);
        }

        private void RenderClockChart()
        {
            int width = (int)canvasChart.ActualWidth;
            int height = (int)canvasChart.ActualHeight;

            canvasChart.Children.Clear();

            var bg_color = Color.FromRgb(0xFF, 0xFF, 0xFF);
            canvasChart.Background = new SolidColorBrush(bg_color);

            var line_color = Color.FromRgb(0x00, 0x00, 0x00);
            var pen = new Pen(new SolidColorBrush(line_color), 1.0);

            DrawingVisual visual = new();
            DrawingContext context = visual.RenderOpen();

            var center = new Point(width / 2, height / 2);
            var radius = (Math.Min(width, height) / 2) * 0.6;
            var radius2 = radius * 1.3;
            var offset = radius2 - radius;

            context.DrawEllipse(null, pen, center, radius, radius);

            var pi = Math.PI;
            for (int i = 0; i < 12; i++)
            {
                var sectd = ((pi * 2) / 12);
                var angle = sectd * i + sectd / 2;
                var x = center.X + radius * Math.Sin(angle);
                var y = center.Y - radius * Math.Cos(angle);
                context.DrawLine(pen, center, new Point(x, y));

                var angle2 = sectd * i;
                var x2 = center.X + radius2 * Math.Sin(angle2);
                var y2 = center.Y - radius2 * Math.Cos(angle2) - 8;
                context.DrawText(GetClockThicknessText(i), new Point(x2, y2));
            }

            context.Close();

            var bitmap = new RenderTargetBitmap(width, height, 0, 0, PixelFormats.Pbgra32);
            bitmap.Render(visual);

            Image image = new();
            image.Source = bitmap;
            canvasChart.Children.Add(image);
        }

        private void RenderChart()
        {
            if (IsETDRSChart)
            {
                RenderETDRSChart();
            }
            else if (IsClockChart)
            {
                RenderClockChart();
            }
        }

        private void CanvasChart_Loaded(object sender, RoutedEventArgs e)
        {
            RenderChart();
        }
    }
}
