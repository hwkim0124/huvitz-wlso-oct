using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Linq;

namespace WsoNativeLib.Presets
{
    public readonly record struct LsoReflectedLightRoi(int X, int Y, int Width, int Height);

    public sealed class LsoReflectedLightConditions
    {
        public byte FundusMode { get; set; } // LsoFundusCaptureRequest.fundusMode
        public uint ExposureUs { get; set; } // LsoColorCameraSettingParam.exposureTime
        public float Gain { get; set; } // LsoColorCameraSettingParam.gain (dB)
        public ushort LedIntensity { get; set; } // LsoFundusCaptureRequest.captureLightIntensity
        public uint Width { get; set; } // LsoColorCameraSettingParam.roiXWidth
        public uint Height { get; set; } // LsoColorCameraSettingParam.roiYHeight
        public uint OffsetX { get; set; } // LsoColorCameraSettingParam.roiXOffset
        public uint OffsetY { get; set; } // LsoColorCameraSettingParam.roiYOffset
        public uint PixelFormat { get; set; } // LsoColorCameraSettingParam.pixelFormat
        public int FlipMode { get; set; } // Frame callback nFlipMode

        public string? FirstMismatch(LsoReflectedLightConditions actual)
        {
            if (FundusMode != actual.FundusMode) return Mismatch(nameof(FundusMode), FundusMode, actual.FundusMode);
            if (ExposureUs != actual.ExposureUs) return Mismatch(nameof(ExposureUs), ExposureUs, actual.ExposureUs);
            if (!float.IsFinite(Gain) || !float.IsFinite(actual.Gain)
                || Math.Abs((double)Gain - actual.Gain) > 0.01)
                return Mismatch(nameof(Gain), Gain, actual.Gain);
            if (LedIntensity != actual.LedIntensity) return Mismatch(nameof(LedIntensity), LedIntensity, actual.LedIntensity);
            if (Width != actual.Width) return Mismatch(nameof(Width), Width, actual.Width);
            if (Height != actual.Height) return Mismatch(nameof(Height), Height, actual.Height);
            if (OffsetX != actual.OffsetX) return Mismatch(nameof(OffsetX), OffsetX, actual.OffsetX);
            if (OffsetY != actual.OffsetY) return Mismatch(nameof(OffsetY), OffsetY, actual.OffsetY);
            if (PixelFormat != actual.PixelFormat) return Mismatch(nameof(PixelFormat), PixelFormat, actual.PixelFormat);
            if (FlipMode != actual.FlipMode) return Mismatch(nameof(FlipMode), FlipMode, actual.FlipMode);
            return null;
        }

        private static string Mismatch(string name, object expected, object actual)
        {
            return string.Format(CultureInfo.InvariantCulture, "{0} {1} (map) != {2} (capture)",
                name, expected, actual);
        }
    }

    public sealed class LsoReflectedLightPoint
    {
        public double Diopter { get; set; }
        public int Position { get; set; }
        public string File { get; set; } = string.Empty;
        public int[] Floor { get; set; } = new int[4];
    }

    public sealed class LsoReflectedLightCorrection
    {
        public int Position { get; }
        public int LowerPosition { get; }
        public int UpperPosition { get; }
        public double Weight { get; }
        public LsoReflectedLightRoi Roi { get; }
        public LsoReflectedLightConditions Conditions { get; }
        private readonly ushort[] _map;

        internal LsoReflectedLightCorrection(int position, int lowerPosition, int upperPosition,
            double weight, LsoReflectedLightRoi roi, LsoReflectedLightConditions conditions, ushort[] map)
        {
            Position = position;
            LowerPosition = lowerPosition;
            UpperPosition = upperPosition;
            Weight = weight;
            Roi = roi;
            Conditions = conditions;
            _map = map;
        }

        public bool Apply(byte[] data, int width, int height, int bytesPerPixel)
        {
            if (bytesPerPixel != 2 || width <= 0 || height <= 0
                || width != Conditions.Width || height != Conditions.Height
                || data.Length < (long)width * height * 2) return false;
            for (int y = 0; y < Roi.Height; y++)
            {
                for (int x = 0; x < Roi.Width; x++)
                {
                    int offset = ((y + Roi.Y) * width + x + Roi.X) * 2;
                    int raw = data[offset] | (data[offset + 1] << 8);
                    int value = Math.Max(0, raw - _map[y * Roi.Width + x]);
                    data[offset] = (byte)value;
                    data[offset + 1] = (byte)(value >> 8);
                }
            }
            return true;
        }
    }

    public sealed class LsoReflectedLightSet
    {
        public const string ManifestFileName = "ReflectedLight.xml";
        public string Directory { get; }
        public DateTime Created { get; }
        public LsoReflectedLightConditions Conditions { get; }
        public LsoReflectedLightRoi Roi { get; }
        public int FramesPerPoint { get; }
        public IReadOnlyList<LsoReflectedLightPoint> Points { get; }

        private LsoReflectedLightSet(string directory, DateTime created,
            LsoReflectedLightConditions conditions, LsoReflectedLightRoi roi, int framesPerPoint,
            IEnumerable<LsoReflectedLightPoint> points)
        {
            Directory = directory;
            Created = created;
            Conditions = conditions;
            Roi = roi;
            FramesPerPoint = framesPerPoint;
            Points = Array.AsReadOnly(points.OrderBy(p => p.Position).ToArray());
        }

        public static bool TryLoad(string directory, out LsoReflectedLightSet? set, out string error)
        {
            set = null;
            try
            {
                XElement root = XElement.Load(Path.Combine(directory, ManifestFileName));
                Require(root.Name == "ReflectedLight" && Attribute(root, "Version") == "1",
                    "ReflectedLight Version must be 1.");
                DateTime created = DateTime.ParseExact(Attribute(root, "Created"), "O",
                    CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind);
                XElement c = Element(root, "Conditions");
                var conditions = new LsoReflectedLightConditions
                {
                    FundusMode = byte.Parse(Attribute(c, "FundusMode"), CultureInfo.InvariantCulture),
                    ExposureUs = uint.Parse(Attribute(c, "ExposureUs"), CultureInfo.InvariantCulture),
                    Gain = float.Parse(Attribute(c, "Gain"), CultureInfo.InvariantCulture),
                    LedIntensity = ushort.Parse(Attribute(c, "LedIntensity"), CultureInfo.InvariantCulture),
                    Width = uint.Parse(Attribute(c, "Width"), CultureInfo.InvariantCulture),
                    Height = uint.Parse(Attribute(c, "Height"), CultureInfo.InvariantCulture),
                    OffsetX = uint.Parse(Attribute(c, "OffsetX"), CultureInfo.InvariantCulture),
                    OffsetY = uint.Parse(Attribute(c, "OffsetY"), CultureInfo.InvariantCulture),
                    PixelFormat = uint.Parse(Attribute(c, "PixelFormat"), CultureInfo.InvariantCulture),
                    FlipMode = Int(c, "FlipMode"),
                };
                XElement r = Element(root, "Roi");
                var roi = new LsoReflectedLightRoi(Int(r, "X"), Int(r, "Y"), Int(r, "Width"), Int(r, "Height"));
                int frames = int.Parse(Element(root, "FramesPerPoint").Value, CultureInfo.InvariantCulture);
                ValidateGeometry(conditions, roi, frames);
                var points = new List<LsoReflectedLightPoint>();
                var positions = new HashSet<int>();
                foreach (XElement p in root.Elements("Point"))
                {
                    var point = new LsoReflectedLightPoint
                    {
                        Diopter = double.Parse(Attribute(p, "Diopter"), CultureInfo.InvariantCulture),
                        Position = Int(p, "Position"),
                        File = Attribute(p, "File"),
                        Floor = Attribute(p, "Floor").Split(',')
                            .Select(v => int.Parse(v, CultureInfo.InvariantCulture)).ToArray(),
                    };
                    ValidatePoint(point);
                    Require(positions.Add(point.Position), $"Duplicate Position {point.Position}.");
                    points.Add(point);
                }
                Require(points.Count > 0, "No Point elements.");
                var loaded = new LsoReflectedLightSet(Path.GetFullPath(directory), created, conditions, roi, frames, points);
                foreach (LsoReflectedLightPoint point in loaded.Points)
                {
                    if (!LsoFlatField.TryReadPgmHeader(Path.Combine(loaded.Directory, point.File),
                        out int width, out int height, out error, 4095)) return false;
                    Require(width == roi.Width && height == roi.Height,
                        $"Point '{point.File}' size {width}x{height} != Roi {roi.Width}x{roi.Height}.");
                }
                set = loaded;
                error = string.Empty;
                return true;
            }
            catch (Exception e) when (IsStoreError(e))
            {
                error = $"Reflected-light manifest: {e.Message}";
                return false;
            }
        }

        public bool TryReadPointMap(LsoReflectedLightPoint point, out ushort[] map, out string error)
        {
            map = Array.Empty<ushort>();
            try
            {
                ValidatePoint(point);
                if (!LsoFlatField.TryReadPgm(Path.Combine(Directory, point.File),
                    out int width, out int height, out ushort[] values, out error, 4095)) return false;
                Require(width == Roi.Width && height == Roi.Height,
                    $"Point '{point.File}' size {width}x{height} != Roi {Roi.Width}x{Roi.Height}.");
                Require(values.All(v => v <= 4095), $"Point '{point.File}' has values above 4095.");
                map = values;
                error = string.Empty;
                return true;
            }
            catch (Exception e) when (IsStoreError(e))
            {
                error = $"Reflected-light point '{point.File}': {e.Message}";
                return false;
            }
        }

        public bool TryForFocus(int position, out LsoReflectedLightCorrection? correction, out string error)
        {
            correction = null;
            int upper = 0;
            while (upper < Points.Count - 1 && Points[upper].Position < position) upper++;
            int lower = upper;
            if (Points[upper].Position > position && upper > 0) lower--;
            if (!TryReadPointMap(Points[lower], out ushort[] map, out error)) return false;
            double weight = 0;
            if (lower != upper)
            {
                if (!TryReadPointMap(Points[upper], out ushort[] high, out error)) return false;
                weight = ((double)position - Points[lower].Position)
                    / ((double)Points[upper].Position - Points[lower].Position);
                for (int i = 0; i < map.Length; i++)
                    map[i] = (ushort)Math.Round((1 - weight) * map[i] + weight * high[i],
                        MidpointRounding.AwayFromZero);
            }
            correction = new LsoReflectedLightCorrection(position, Points[lower].Position,
                Points[upper].Position, weight, Roi, Conditions, map);
            error = string.Empty;
            return true;
        }

        public static bool TryBuildPoint(IReadOnlyList<byte[]> frames, int width, int height,
            LsoReflectedLightRoi roi, out ushort[] map, out int[] floor, out string error)
        {
            map = Array.Empty<ushort>();
            floor = Array.Empty<int>();
            try
            {
                Require(width > 0 && height > 0 && (long)width * height * 2 <= int.MaxValue,
                    "Frame size must be positive and fit a 16-bit buffer.");
                ValidateGeometry(new LsoReflectedLightConditions
                    { ExposureUs = 1, Width = (uint)width, Height = (uint)height }, roi, frames.Count);
                int pixels = width * height;
                Require(frames.All(f => f != null && f.Length == pixels * 2),
                    "All frames must have exactly Width * Height * 2 bytes.");
                var average = new ushort[pixels];
                var histogram = new int[4, 4096];
                var counts = new int[4];
                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        int i = y * width + x;
                        long sum = 0;
                        foreach (byte[] frame in frames)
                        {
                            int value = frame[2 * i] | (frame[2 * i + 1] << 8);
                            Require(value <= 4095, "Frame has values above 4095.");
                            sum += value;
                        }
                        int avg = (int)((sum + frames.Count / 2) / frames.Count);
                        average[i] = (ushort)avg;
                        int plane = ((y & 1) << 1) | (x & 1);
                        histogram[plane, avg]++;
                        counts[plane]++;
                    }
                }
                var levels = new int[4];
                for (int plane = 0; plane < 4; plane++)
                {
                    int cumulative = 0;
                    // The lower middle keeps even-sized planes deterministic.
                    int middle = (counts[plane] - 1) / 2;
                    for (int value = 0; value <= 4095; value++)
                    {
                        cumulative += histogram[plane, value];
                        if (cumulative > middle) { levels[plane] = value; break; }
                    }
                }
                var result = new ushort[roi.Width * roi.Height];
                for (int y = roi.Y; y < roi.Y + roi.Height; y++)
                    for (int x = roi.X; x < roi.X + roi.Width; x++)
                        result[(y - roi.Y) * roi.Width + x - roi.X] = (ushort)Math.Max(0,
                            average[y * width + x] - levels[((y & 1) << 1) | (x & 1)]);
                map = result;
                floor = levels;
                error = string.Empty;
                return true;
            }
            catch (Exception e) when (IsStoreError(e))
            {
                error = $"Reflected-light build: {e.Message}";
                return false;
            }
        }

        public static bool TryWrite(string directory, LsoReflectedLightConditions conditions,
            LsoReflectedLightRoi roi, int framesPerPoint,
            IReadOnlyList<(LsoReflectedLightPoint Point, ushort[] Map)> points, out string error)
        {
            string? temporary = null;
            string? previous = null;
            string? target = null;
            bool movedPrevious = false;
            try
            {
                ValidateGeometry(conditions, roi, framesPerPoint);
                Require(points.Count > 0, "No Point elements.");
                target = Path.TrimEndingDirectorySeparator(Path.GetFullPath(directory));
                previous = target + ".old";
                temporary = target + ".tmp-" + Guid.NewGuid().ToString("N");
                System.IO.Directory.CreateDirectory(temporary);
                var root = new XElement("ReflectedLight", new XAttribute("Version", "1"),
                    new XAttribute("Created", DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture)),
                    new XElement("Conditions",
                        new XAttribute("FundusMode", conditions.FundusMode),
                        new XAttribute("ExposureUs", conditions.ExposureUs),
                        new XAttribute("Gain", conditions.Gain),
                        new XAttribute("LedIntensity", conditions.LedIntensity),
                        new XAttribute("Width", conditions.Width), new XAttribute("Height", conditions.Height),
                        new XAttribute("OffsetX", conditions.OffsetX), new XAttribute("OffsetY", conditions.OffsetY),
                        new XAttribute("PixelFormat", conditions.PixelFormat), new XAttribute("FlipMode", conditions.FlipMode)),
                    new XElement("Roi", new XAttribute("X", roi.X), new XAttribute("Y", roi.Y),
                        new XAttribute("Width", roi.Width), new XAttribute("Height", roi.Height)),
                    new XElement("FramesPerPoint", framesPerPoint));
                foreach (var entry in points)
                {
                    // Check here, not in a pre-pass: a later bad frame must also clean up earlier writes.
                    ValidatePoint(entry.Point);
                    Require(entry.Map.Length == checked(roi.Width * roi.Height),
                        $"Point {entry.Point.Position} map length does not match Roi size.");
                    Require(entry.Map.All(v => v <= 4095), $"Point {entry.Point.Position} has values above 4095.");
                    string file = PointFile(entry.Point.Position);
                    using (var stream = System.IO.File.Create(Path.Combine(temporary, file)))
                    {
                        byte[] header = Encoding.ASCII.GetBytes(
                            FormattableString.Invariant($"P5\n{roi.Width} {roi.Height}\n4095\n"));
                        stream.Write(header);
                        foreach (ushort value in entry.Map)
                        {
                            stream.WriteByte((byte)(value >> 8));
                            stream.WriteByte((byte)value);
                        }
                    }
                    root.Add(new XElement("Point", new XAttribute("Diopter", entry.Point.Diopter),
                        new XAttribute("Position", entry.Point.Position), new XAttribute("File", file),
                        new XAttribute("Floor", string.Join(",", entry.Point.Floor.Select(
                            v => v.ToString(CultureInfo.InvariantCulture))))));
                }
                root.Save(Path.Combine(temporary, ManifestFileName));
                Require(TryLoad(temporary, out _, out string readError), readError);
                if (System.IO.Directory.Exists(previous)) System.IO.Directory.Delete(previous, true);
                if (System.IO.Directory.Exists(target))
                {
                    System.IO.Directory.Move(target, previous);
                    movedPrevious = true;
                }
                System.IO.Directory.Move(temporary, target);
            }
            catch (Exception e) when (IsStoreError(e))
            {
                if (movedPrevious) System.IO.Directory.Move(previous!, target!);
                error = $"Reflected-light write: {e.Message}";
                return false;
            }
            finally
            {
                if (temporary != null && System.IO.Directory.Exists(temporary))
                    System.IO.Directory.Delete(temporary, true);
            }
            // The new set is committed. A locked backup is harmless and can be removed on the next write.
            if (movedPrevious)
            {
                try { System.IO.Directory.Delete(previous!, true); }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
            }
            error = string.Empty;
            return true;
        }

        private static void ValidateGeometry(LsoReflectedLightConditions conditions, LsoReflectedLightRoi roi, int frames)
        {
            Require(conditions.ExposureUs > 0 && conditions.Width > 0 && conditions.Height > 0
                && float.IsFinite(conditions.Gain), "Conditions require positive ExposureUs, Width, Height and finite Gain.");
            Require(frames > 0, "FramesPerPoint must be positive.");
            Require(roi.X >= 0 && roi.Y >= 0 && roi.Width > 0 && roi.Height > 0
                && ((roi.X | roi.Y | roi.Width | roi.Height) & 1) == 0, "Roi must be positive and even aligned.");
            Require((long)roi.X + roi.Width <= conditions.Width && (long)roi.Y + roi.Height <= conditions.Height,
                "Roi is outside Conditions Width/Height.");
            Require((long)roi.Width * roi.Height <= int.MaxValue, "Roi size is too large.");
        }

        private static void ValidatePoint(LsoReflectedLightPoint point)
        {
            Require(double.IsFinite(point.Diopter), "Point Diopter must be finite.");
            Require(point.File == PointFile(point.Position), $"Point File must be '{PointFile(point.Position)}'.");
            Require(point.Floor.Length == 4 && point.Floor.All(v => v >= 0 && v <= 4095),
                "Point Floor must contain four Bayer values in 0..4095.");
        }

        private static string PointFile(int position) => "p_" + position.ToString(CultureInfo.InvariantCulture) + ".pgm";
        private static string Attribute(XElement element, string name) =>
            (string?)element.Attribute(name) ?? throw new FormatException($"Missing {element.Name}/{name} attribute.");
        private static XElement Element(XElement root, string name)
        {
            Require(root.Elements(name).Count() == 1, $"Missing or duplicate {name} element.");
            return root.Element(name)!;
        }
        private static int Int(XElement element, string name) => int.Parse(Attribute(element, name), CultureInfo.InvariantCulture);
        private static void Require(bool valid, string error)
        {
            if (!valid) throw new FormatException(error);
        }
        private static bool IsStoreError(Exception e) => e is IOException || e is UnauthorizedAccessException
            || e is ArgumentException || e is FormatException || e is OverflowException
            || e is System.Xml.XmlException || e is NotSupportedException;
    }
}
