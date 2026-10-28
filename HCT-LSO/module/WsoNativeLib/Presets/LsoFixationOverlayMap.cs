using System;
using System.IO;
using WsoNativeLib.Utils;

namespace WsoNativeLib.Presets
{
    // Maps the fixation LCD grid (row 0-319 = Y, column 0-239 = X) onto a W x H preview image:
    //   x = W/2 + sx*(col-CenterCol)*ScaleX*W/240,  y = H/2 + sy*(row-CenterRow)*ScaleY*H/320
    // with sx/sy = -1 when FlipX/FlipY. ToLcd is the exact inverse, rounded and clamped to the grid.
    //
    // Values come from the [FIXATION_OVERLAY] section of the device INI. Load only reads that
    // file (never creates or writes it) and keeps no cache, so every call sees the current file.
    public sealed record LsoFixationOverlayMap(int CenterRow, int CenterCol, double ScaleX, double ScaleY, bool FlipX, bool FlipY)
    {
        public const int LcdRows = 320;
        public const int LcdCols = 240;
        public const string SectionName = "FIXATION_OVERLAY";

        public static LsoFixationOverlayMap Default { get; } = new LsoFixationOverlayMap(160, 120, 1.0, 1.0, false, false);

        public static LsoFixationOverlayMap Load(string iniPath)
        {
            if (string.IsNullOrWhiteSpace(iniPath) || !File.Exists(iniPath))
            {
                return Default;
            }

            var ini = new IniFileUtil(iniPath, createIfMissing: false);
            LsoFixationOverlayMap d = Default;
            double scaleX = ini.ReadDouble(SectionName, "ScaleX", d.ScaleX);
            double scaleY = ini.ReadDouble(SectionName, "ScaleY", d.ScaleY);
            return new LsoFixationOverlayMap(
                ini.ReadInt(SectionName, "CenterRow", d.CenterRow),
                ini.ReadInt(SectionName, "CenterCol", d.CenterCol),
                PositiveOrOne(scaleX),
                PositiveOrOne(scaleY),
                ini.ReadInt(SectionName, "FlipX", d.FlipX ? 1 : 0) != 0,
                ini.ReadInt(SectionName, "FlipY", d.FlipY ? 1 : 0) != 0);
        }

        public (double X, double Y) ToImage(int row, int col, double width, double height)
        {
            double x = width / 2.0 + SignX * (col - CenterCol) * PositiveOrOne(ScaleX) * width / LcdCols;
            double y = height / 2.0 + SignY * (row - CenterRow) * PositiveOrOne(ScaleY) * height / LcdRows;
            return (x, y);
        }

        public (int Row, int Col) ToLcd(double x, double y, double width, double height)
        {
            double col = CenterCol + (x - width / 2.0) / (SignX * PositiveOrOne(ScaleX) * width / LcdCols);
            double row = CenterRow + (y - height / 2.0) / (SignY * PositiveOrOne(ScaleY) * height / LcdRows);
            return (RoundClamp(row, LcdRows - 1), RoundClamp(col, LcdCols - 1));
        }

        private double SignX => FlipX ? -1.0 : 1.0;

        private double SignY => FlipY ? -1.0 : 1.0;

        private static double PositiveOrOne(double scale) => scale > 0.0 && !double.IsInfinity(scale) ? scale : 1.0;

        private static int RoundClamp(double value, int max)
        {
            if (double.IsNaN(value))
            {
                return 0;
            }

            double rounded = Math.Round(value, MidpointRounding.AwayFromZero);
            return (int)Math.Clamp(rounded, 0.0, max);
        }
    }
}
