using Microsoft.Windows.Themes;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WsoToolkit.utils
{
    internal static class NumberUtil
    {
        public static int ToInt(string? str)
        {
            if (str == null)
            {
                return 0;
            }
            if (int.TryParse(str, out int result))
            {
                return result;
            }
            if (float.TryParse(str, out float result2))
            {
                // To handle "8.0"
                return (int)result2;
            }
            return 0;
        }

        public static double ToDouble(string str)
        {
            if (double.TryParse(str, out double result))
            {
                return result;
            }
            return 0.0;
        }

        public static float ToFloat(string? str)
        {
            if (str == null)
            {
                return 0.0f;
            }
            if (float.TryParse(str, out float result))
            {
                return result;
            }
            return 0.0f;
        }

        public static ushort ToUshort(string str)
        {
            if (ushort.TryParse(str, out ushort result))
            {
                return result;
            }
            return 0;
        }

        public static short ToShort(string str)
        {
            if (short.TryParse(str, out short result))
            {
                return result;
            }
            return 0;
        }
    }
}
