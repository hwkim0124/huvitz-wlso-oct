using System;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;

namespace WsoToolkit.utils
{
    /// <summary>
    /// UNICODE 기반 INI 파일 읽기/쓰기 유틸리티 클래스.
    /// C++ <c>cpp_util::IniFile</c>과 동일한 Windows API를 사용합니다.
    /// </summary>
    [SupportedOSPlatform("windows")]
    internal sealed class IniFileUtil
    {
        private const int DefaultStringBufferSize = 1024;

        private readonly string _filePath;

        public IniFileUtil(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
            {
                throw new ArgumentException("INI file path is required.", nameof(filePath));
            }

            _filePath = filePath;

            if (!File.Exists(_filePath))
            {
                using FileStream _ = File.Create(_filePath);
            }
        }

        public string FilePath => _filePath;

        public string ReadString(string section, string key, string defaultValue = "")
        {
            var buffer = new StringBuilder(DefaultStringBufferSize);
            GetPrivateProfileStringW(
                section,
                key,
                defaultValue,
                buffer,
                buffer.Capacity,
                _filePath);
            return buffer.ToString();
        }

        public int ReadInt(string section, string key, int defaultValue = 0)
        {
            return GetPrivateProfileIntW(section, key, defaultValue, _filePath);
        }

        public uint ReadUInt(string section, string key, uint defaultValue = 0)
        {
            return (uint)GetPrivateProfileIntW(section, key, (int)defaultValue, _filePath);
        }

        public float ReadFloat(string section, string key, float defaultValue = 0.0f)
        {
            string value = ReadString(section, key, string.Empty);
            if (float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out float result))
            {
                return result;
            }

            return defaultValue;
        }

        public double ReadDouble(string section, string key, double defaultValue = 0.0)
        {
            string value = ReadString(section, key, string.Empty);
            if (double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double result))
            {
                return result;
            }

            return defaultValue;
        }

        public bool WriteString(string section, string key, string value)
        {
            return WritePrivateProfileStringW(section, key, value, _filePath);
        }

        public bool WriteInt(string section, string key, int value)
        {
            return WriteString(section, key, value.ToString(CultureInfo.InvariantCulture));
        }

        public bool WriteFloat(string section, string key, float value)
        {
            return WriteString(section, key, value.ToString(CultureInfo.InvariantCulture));
        }

        public bool WriteDouble(string section, string key, double value)
        {
            return WriteString(section, key, value.ToString(CultureInfo.InvariantCulture));
        }

        public bool DeleteKey(string section, string key)
        {
            return WritePrivateProfileStringW(section, key, null, _filePath);
        }

        public bool DeleteSection(string section)
        {
            return WritePrivateProfileStringW(section, null, null, _filePath);
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern int GetPrivateProfileStringW(
            string section,
            string key,
            string defaultValue,
            StringBuilder returnValue,
            int size,
            string filePath);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern int GetPrivateProfileIntW(
            string section,
            string key,
            int defaultValue,
            string filePath);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool WritePrivateProfileStringW(
            string section,
            string? key,
            string? value,
            string filePath);
    }
}
