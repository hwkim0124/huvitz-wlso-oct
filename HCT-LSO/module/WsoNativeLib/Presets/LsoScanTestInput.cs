using System.Globalization;

namespace WsoNativeLib.Presets
{
    // Text-box validation for the anterior camera Gain / Exposure fields of the scan test window.
    // Ranges follow the camera driver: Gain 1..31 (integer), Exposure 1..300 ms.
    public static class LsoScanTestInput
    {
        public const int GainMin = 1;
        public const int GainMax = 31;
        public const float ExposureMinMs = 1.0f;
        public const float ExposureMaxMs = 300.0f;

        public static bool TryParseAnterior(string gainText, string exposureText, out int gain, out float exposureMs, out string error)
        {
            gain = 0;
            exposureMs = 0.0f;

            string g = (gainText ?? string.Empty).Trim();
            if (!int.TryParse(g, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedGain)
                || parsedGain < GainMin || parsedGain > GainMax)
            {
                error = $"Gain must be an integer from {GainMin} to {GainMax}.";
                return false;
            }

            string e = (exposureText ?? string.Empty).Trim();
            if (!float.TryParse(e, NumberStyles.Float, CultureInfo.InvariantCulture, out float parsedExposure)
                || !(parsedExposure >= ExposureMinMs && parsedExposure <= ExposureMaxMs))
            {
                error = "Exposure must be a number from 1.0 to 300.0 ms.";
                return false;
            }

            gain = parsedGain;
            exposureMs = parsedExposure;
            error = string.Empty;
            return true;
        }
    }
}
