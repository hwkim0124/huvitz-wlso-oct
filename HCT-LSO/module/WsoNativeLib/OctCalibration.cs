using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Threading;


namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoOctDefs;
    using static WsoOctCalib;

    public static class OctCalibration
    {
        const string LibraryName = WsoSystemDllPath;

        static private double[] _doubleBuff = new double[BufferSize];
        private const int BufferSize = 128;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        private static extern int getOctFbgClassListSize();

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctFbgClassName(int index, StringBuilder name);

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        private static extern int getOctFbgClassValues(int index, [Out] double[] data, int buff_size);

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        private static extern int findOctSpectrumDataPeaks([In] ushort[] data, int data_size, int threshold1, int threshold2, [Out] int[] values, [Out] int[] indice);

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        private static extern int computeOctSpectrometerCoefficients([In] int[] indice, [In] double[] values, int data_size, int poly_order, [Out] double[] coeffs);

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool applyOctSpectrometerCoefficients([In] double[] values, int data_size);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static int GetFbgClassListSize()
        {
            return getOctFbgClassListSize();
        }

        public static bool GetFbgClassName(int index, out string name)
        {
            StringBuilder sb = new StringBuilder(256);
            if (getOctFbgClassName(index, sb))
            {
                name = sb.ToString();
                return true;
            }
            name = "";
            return false;
        }

        public static int GetFbgClassValues(int index, out double[] values)
        {
            int count = getOctFbgClassValues(index, _doubleBuff, BufferSize);
            values = new double[count];
            Array.Copy(_doubleBuff, values, count);
            return count;
        }
        
        public static int FindSpectrumDataPeaks(ushort[] data, int threshold1, int threshold2, out List<int> values, out List<int> indice)
        {
            var varr = new int[BufferSize];
            var iarr = new int[BufferSize];
            int count = findOctSpectrumDataPeaks(data, data.Length, threshold1, threshold2, varr, iarr);
            values = varr[0..count].ToList();
            indice = iarr[0..count].ToList();
            return count;
        }

        public static int ComputeSpectrometerCoefficients(int[] indice, double[] values, int poly_order, out List<double> coeffs)
        {
            var buffs = new double[BufferSize];
            int count = computeOctSpectrometerCoefficients(indice, values, indice.Length, poly_order, buffs);
            coeffs = buffs[0..count].ToList();
            return count;
        }

        public static bool ApplySpectrometerCoefficients(double[] values)
        {
            return applyOctSpectrometerCoefficients(values, values.Length);
        }
    }
}
