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
    public partial class OctReportAngioChart : UserControl
    {
        public WsoOctAngio.OctAngioChartDescript ChartDescript;

        public OctReportAngioChart()
        {
            InitializeComponent();
            ClearChart();
        }

        public void ClearChart()
        {
            ChartDescript = new WsoOctAngio.OctAngioChartDescript(WsoDomain.EyeSide.OD);
        }

        private FormattedText GetFlowsText(int index)
        {
            var value = index switch
            {
                0 => ChartDescript.centerFlows == 0.0f ? "N/A" : ChartDescript.centerFlows.ToString("0.00"),
                1 => ChartDescript.innerFlows[0] == 0.0f ? "N/A" : ChartDescript.innerFlows[0].ToString("0.00"),
                2 => ChartDescript.innerFlows[1] == 0.0f ? "N/A" : ChartDescript.innerFlows[1].ToString("0.00"),
                3 => ChartDescript.innerFlows[2] == 0.0f ? "N/A" : ChartDescript.innerFlows[2].ToString("0.00"),
                4 => ChartDescript.innerFlows[3] == 0.0f ? "N/A" : ChartDescript.innerFlows[3].ToString("0.00"),
                _ => "N/A"
            };

            value = "(" + value + ")";
            var text = new FormattedText(value, CultureInfo.CurrentCulture, FlowDirection.LeftToRight, new Typeface("Arial"), 14.0, Brushes.Black, VisualTreeHelper.GetDpi(this).PixelsPerDip);
            text.TextAlignment = TextAlignment.Center;
            return text;
        }

        private FormattedText GetDensityText(int index)
        {
            var value = index switch
            {
                0 => ChartDescript.centerDensity == 0.0f ? "N/A" : ChartDescript.centerDensity.ToString("0.00"),
                1 => ChartDescript.innerDensity[0] == 0.0f ? "N/A" : ChartDescript.innerDensity[0].ToString("0.00"),
                2 => ChartDescript.innerDensity[1] == 0.0f ? "N/A" : ChartDescript.innerDensity[1].ToString("0.00"),
                3 => ChartDescript.innerDensity[2] == 0.0f ? "N/A" : ChartDescript.innerDensity[2].ToString("0.00"),
                4 => ChartDescript.innerDensity[3] == 0.0f ? "N/A" : ChartDescript.innerDensity[3].ToString("0.00"),
                _ => "N/A"
            };

            value += " %";
            var text = new FormattedText(value, CultureInfo.CurrentCulture, FlowDirection.LeftToRight, new Typeface("Arial"), 14.0, Brushes.Black, VisualTreeHelper.GetDpi(this).PixelsPerDip);
            text.TextAlignment = TextAlignment.Center;
            return text;
        }

        public void Refresh()
        {
            RenderChart();
            UpdateChartStatus();
        }

        private void RenderChart()
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
            // var radius3 = mm_size * 3.0;
            var radius2 = mm_size * 3.0;
            var radius1 = mm_size * 1.0;

            context.DrawEllipse(null, pen, new Point(width / 2, height / 2), radius1, radius1);
            context.DrawEllipse(null, pen, new Point(width / 2, height / 2), radius2, radius2);
            // context.DrawEllipse(null, pen, new Point(width / 2, height / 2), radius3, radius3);

            var pi = Math.PI;
            var point1 = new Point(center.X + radius1 * Math.Cos(pi / 4), center.Y + radius1 * Math.Sin(pi / 4));
            var point2 = new Point(center.X + radius2 * Math.Cos(pi / 4), center.Y + radius2 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            point1 = new Point(center.X + radius1 * Math.Cos(pi / 4), center.Y - radius1 * Math.Sin(pi / 4));
            point2 = new Point(center.X + radius2 * Math.Cos(pi / 4), center.Y - radius2 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            point1 = new Point(center.X - radius1 * Math.Cos(pi / 4), center.Y + radius1 * Math.Sin(pi / 4));
            point2 = new Point(center.X - radius2 * Math.Cos(pi / 4), center.Y + radius2 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            point1 = new Point(center.X - radius1 * Math.Cos(pi / 4), center.Y - radius1 * Math.Sin(pi / 4));
            point2 = new Point(center.X - radius2 * Math.Cos(pi / 4), center.Y - radius2 * Math.Sin(pi / 4));
            context.DrawLine(pen, point1, point2);

            var y_offs = 8.0;
            var y_offs2 = 8.0;

            var textpos = new Point(center.X, center.Y - y_offs);
            context.DrawText(GetDensityText(0), new Point(textpos.X, textpos.Y - y_offs));
            context.DrawText(GetFlowsText(0), new Point(textpos.X, textpos.Y + y_offs2));

            var delta1 = radius1 + (radius2 - radius1) / 2;
            //var delta2 = radius2 + (radius3 - radius2) / 2;
            context.DrawText(GetDensityText(1), new Point(textpos.X, textpos.Y - delta1 - y_offs));
            context.DrawText(GetDensityText(2), new Point(textpos.X + delta1, textpos.Y - y_offs));
            context.DrawText(GetDensityText(3), new Point(textpos.X, textpos.Y + delta1 - y_offs));
            context.DrawText(GetDensityText(4), new Point(textpos.X - delta1, textpos.Y - y_offs));


            context.DrawText(GetFlowsText(1), new Point(textpos.X, textpos.Y - delta1 + y_offs2));
            context.DrawText(GetFlowsText(2), new Point(textpos.X + delta1, textpos.Y + y_offs2));
            context.DrawText(GetFlowsText(3), new Point(textpos.X, textpos.Y + delta1 + y_offs2));
            context.DrawText(GetFlowsText(4), new Point(textpos.X - delta1, textpos.Y + y_offs2));
            context.Close();

            var bitmap = new RenderTargetBitmap(width, height, 0, 0, PixelFormats.Pbgra32);
            bitmap.Render(visual);

            Image image = new();
            image.Source = bitmap;
            canvasChart.Children.Add(image);
        }

        private void UpdateChartStatus()
        {
            if (ChartDescript.centerFlows <= 0.0f)
            {
                return;
            }

            string side = ChartDescript.eyeSide == WsoDomain.EyeSide.OS ? "OS" : "OD";
            string s = string.Format("{0}, Center: {1:F2}, {2:F2}", side, ChartDescript.centerX, ChartDescript.centerY);
            lblStatus.Content = s;
        }

        private void CanvasChart_Loaded(object sender, RoutedEventArgs e)
        {
            RenderChart();
        }
    }
}
