using System;
using System.IO;

namespace WsoNativeLib.Presets
{
    // Flat-field correction of a raw Bayer capture, in place, before demosaicing:
    //   out = raw x gain,   gain = (centre level of the pixel's Bayer plane) / flat.
    // Per pixel, so each colour plane keeps its own level (colour balance is untouched) and
    // column/pixel fixed pattern is corrected along with the illumination fall-off.
    //
    // The flat is a raw Bayer PGM of a uniform target shot with the preset's own geometry
    // (lsotune capture writes exactly that). It must come from a uniform target: a flat taken
    // from a real scene divides that scene's own shading out of every later image.
    //
    // WPF-free and dependency-free, so the Toolkit and the Agent apply the same arithmetic.
    public sealed class LsoFlatField
    {
        // Beyond this the correction only amplifies noise (measured 2026-09-24: 1% of the
        // field needs more, all of it at the optical edge).
        public const double MaxGain = 4.0;

        // A flat pixel below this share of its plane's centre level is outside the optical
        // field; it is set to 0 instead of being boosted into noise.
        public const double FieldThreshold = 0.05;

        private const int GainShift = 12;           // Q12 fixed point: 4096 = x1.0
        private const int GainRound = 1 << (GainShift - 1);

        private readonly ushort[] _gain;

        private LsoFlatField(string path, int width, int height, ushort[] gain)
        {
            Path = path;
            Width = width;
            Height = height;
            _gain = gain;
        }

        public string Path { get; }

        public int Width { get; }

        public int Height { get; }

        // Disabled is success with no flat: the caller then leaves frames as they are.
        // Enabled with a missing, unreadable or wrong-sized file is a failure - an acquisition
        // must not silently run uncorrected when the preset says it is corrected.
        public static bool TryLoadFor(LsoModePreset preset, int width, int height,
            out LsoFlatField? flat, out string error)
        {
            flat = null;
            error = string.Empty;
            if (!preset.PostProcess.FlatFieldEnabled)
            {
                return true;
            }

            string file = preset.PostProcess.FlatFieldFile;
            if (string.IsNullOrWhiteSpace(file))
            {
                error = $"'{preset.Name}': flat-field is on but no flat file is set.";
                return false;
            }

            return TryLoad(file, width, height, out flat, out error);
        }

        public static bool TryLoad(string path, int width, int height, out LsoFlatField? flat, out string error)
        {
            flat = null;
            if (!TryReadPgm(path, out int fileWidth, out int fileHeight, out ushort[] values, out error))
            {
                return false;
            }

            if (fileWidth != width || fileHeight != height)
            {
                error = $"Flat '{System.IO.Path.GetFileName(path)}' is {fileWidth}x{fileHeight}, "
                    + $"the capture is {width}x{height}. Shoot the flat with this preset.";
                return false;
            }

            // Centre level per Bayer plane: median of the central half of the frame, from a
            // histogram (values are integers, so this is exact and O(n)).
            var levels = new double[4];
            for (int plane = 0; plane < 4; plane++)
            {
                levels[plane] = CentreMedian(values, width, height, plane >> 1, plane & 1);
                if (levels[plane] <= 0)
                {
                    error = $"Flat '{System.IO.Path.GetFileName(path)}' is black in the centre "
                        + $"(Bayer plane {plane}) - it was not shot on a lit target.";
                    return false;
                }
            }

            var gain = new ushort[width * height];
            int maxQ = (int)Math.Round(MaxGain * (1 << GainShift));
            for (int y = 0; y < height; y++)
            {
                int row = y * width;
                int planeRow = (y & 1) << 1;
                for (int x = 0; x < width; x++)
                {
                    double level = levels[planeRow | (x & 1)];
                    int v = values[row + x];
                    if (v < FieldThreshold * level)
                    {
                        continue; // outside the optical field: gain 0
                    }

                    int q = (int)Math.Round(level / v * (1 << GainShift));
                    gain[row + x] = (ushort)Math.Min(q, maxQ);
                }
            }

            flat = new LsoFlatField(path, width, height, gain);
            error = string.Empty;
            return true;
        }

        // In place. bytesPerPixel 1 = 8-bit, 2 = 16-bit little-endian (the camera's Bayer
        // formats). False, and the frame untouched, when it is not the size the flat was made for.
        // ponytail: no dark frame is subtracted (measured dark ~0.5 DN at 8 bit); add a dark
        // PGM when a 16-bit or high-gain preset shows a real offset.
        public bool Apply(byte[] data, int width, int height, int bytesPerPixel)
        {
            int pixels = width * height;
            if (width != Width || height != Height || data.Length < pixels * bytesPerPixel)
            {
                return false;
            }

            if (bytesPerPixel == 1)
            {
                for (int i = 0; i < pixels; i++)
                {
                    int v = (data[i] * _gain[i] + GainRound) >> GainShift;
                    data[i] = (byte)(v > 255 ? 255 : v);
                }
                return true;
            }

            if (bytesPerPixel == 2)
            {
                for (int i = 0; i < pixels; i++)
                {
                    int raw = data[2 * i] | (data[2 * i + 1] << 8);
                    long v = ((long)raw * _gain[i] + GainRound) >> GainShift;
                    int clamped = v > 65535 ? 65535 : (int)v;
                    data[2 * i] = (byte)clamped;
                    data[2 * i + 1] = (byte)(clamped >> 8);
                }
                return true;
            }

            return false;
        }

        private static double CentreMedian(ushort[] values, int width, int height, int rowPhase, int colPhase)
        {
            var histogram = new int[65536];
            long count = 0;
            for (int y = height / 4 + ((height / 4 + rowPhase) & 1); y < height * 3 / 4; y += 2)
            {
                int row = y * width;
                for (int x = width / 4 + ((width / 4 + colPhase) & 1); x < width * 3 / 4; x += 2)
                {
                    histogram[values[row + x]]++;
                    count++;
                }
            }

            long half = (count + 1) / 2;
            long seen = 0;
            for (int v = 0; v < histogram.Length; v++)
            {
                seen += histogram[v];
                if (seen >= half)
                {
                    return v;
                }
            }
            return 0;
        }

        // Binary PGM (P5), 8-bit or 16-bit (big-endian, per the format); '#' comments allowed.
        internal static bool TryReadPgm(string path, out int width, out int height, out ushort[] values,
            out string error, int? expectedMaxValue = null)
        {
            width = height = 0;
            values = Array.Empty<ushort>();
            byte[] data;
            try
            {
                data = File.ReadAllBytes(path);
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException
                || e is NotSupportedException)
            {
                error = $"Flat '{path}' could not be read: {e.Message}";
                return false;
            }

            int pos = 0;
            var header = new int[3];
            string? magic = NextToken(data, ref pos);
            if (magic != "P5")
            {
                error = $"Flat '{System.IO.Path.GetFileName(path)}' is not a binary PGM (P5).";
                return false;
            }

            for (int i = 0; i < 3; i++)
            {
                string? token = NextToken(data, ref pos);
                if (token == null || !int.TryParse(token, out header[i]) || header[i] <= 0)
                {
                    error = $"Flat '{System.IO.Path.GetFileName(path)}' has a broken PGM header.";
                    return false;
                }
            }

            width = header[0];
            height = header[1];
            int maxValue = header[2];
            if (expectedMaxValue.HasValue && maxValue != expectedMaxValue.Value)
            {
                error = $"PGM '{System.IO.Path.GetFileName(path)}' maxval {maxValue} != {expectedMaxValue.Value}.";
                return false;
            }
            int bytesPerValue = maxValue < 256 ? 1 : 2;
            pos++; // the single whitespace after maxval
            long needed = (long)width * height * bytesPerValue;
            if (maxValue > 65535 || data.Length - pos < needed)
            {
                error = $"Flat '{System.IO.Path.GetFileName(path)}' is truncated or not 8/16-bit.";
                return false;
            }

            values = new ushort[width * height];
            for (int i = 0; i < values.Length; i++)
            {
                values[i] = bytesPerValue == 1
                    ? data[pos + i]
                    : (ushort)((data[pos + 2 * i] << 8) | data[pos + 2 * i + 1]);
            }

            error = string.Empty;
            return true;
        }

        // Do not buffer past the header: set discovery must not read every point's pixels.
        internal static bool TryReadPgmHeader(string path, out int width, out int height,
            out string error, int expectedMaxValue)
        {
            width = height = 0;
            try
            {
                using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1);
                string? NextHeaderToken()
                {
                    int value;
                    while ((value = stream.ReadByte()) >= 0)
                    {
                        if (value == '#')
                        {
                            while ((value = stream.ReadByte()) >= 0 && value != '\n') { }
                        }
                        else if (!char.IsWhiteSpace((char)value)) break;
                    }
                    var token = new System.Text.StringBuilder();
                    while (value >= 0 && !char.IsWhiteSpace((char)value))
                    {
                        token.Append((char)value);
                        value = stream.ReadByte();
                    }
                    return value >= 0 && token.Length > 0 ? token.ToString() : null;
                }

                string name = System.IO.Path.GetFileName(path);
                if (NextHeaderToken() != "P5")
                {
                    error = $"PGM '{name}' is not a binary PGM (P5).";
                    return false;
                }
                var header = new int[3];
                for (int i = 0; i < header.Length; i++)
                {
                    if (!int.TryParse(NextHeaderToken(), out header[i]) || header[i] <= 0)
                    {
                        error = $"PGM '{name}' has a broken PGM header.";
                        return false;
                    }
                }
                width = header[0];
                height = header[1];
                if (header[2] != expectedMaxValue)
                {
                    error = $"PGM '{name}' maxval {header[2]} != {expectedMaxValue}.";
                    return false;
                }
                if (stream.Length - stream.Position != (long)width * height * 2)
                {
                    error = $"PGM '{name}' is truncated or has an unexpected PGM payload length.";
                    return false;
                }
                error = string.Empty;
                return true;
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is ArgumentException
                || e is NotSupportedException)
            {
                error = $"PGM '{path}' could not be read: {e.Message}";
                return false;
            }
        }

        private static string? NextToken(byte[] data, ref int pos)
        {
            while (pos < data.Length)
            {
                if (data[pos] == (byte)'#')
                {
                    while (pos < data.Length && data[pos] != (byte)'\n')
                    {
                        pos++;
                    }
                }
                else if (char.IsWhiteSpace((char)data[pos]))
                {
                    pos++;
                }
                else
                {
                    break;
                }
            }

            int start = pos;
            while (pos < data.Length && !char.IsWhiteSpace((char)data[pos]))
            {
                pos++;
            }
            return pos > start ? System.Text.Encoding.ASCII.GetString(data, start, pos - start) : null;
        }
    }
}
