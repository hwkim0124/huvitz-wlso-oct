using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WsoNativeLib
{
    using static WsoNativeLib.WsoDomain;

    public static class WsoOctDefs
    {
        // Constant Definitions 
        ////////////////////////////////////////////////////////////////////////////////////////////
        public const int SEGM_IMAGE_WIDTH_MAX = 2048;
        public const int OCT_SPECTROMETER_COEFFS_SIZE = 4;
        public const int OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE = 3;

        public const int LINE_CAMERA_CCD_PIXELS = 2048;
        public const int LINE_CAMERA_PIXEL_VALUE_MAX = 4095;
        public const int SPECTRO_CALIB_BSCAN_WIDTH = 1024;


        // Enum Definitions
        /////////////////////////////////////////////////////////////////////////////////////////////
        public enum OctPatternCode : int
        {
            UNKNOWN = 0,
            CALIBRATION_POINT = 1,
            MACULAR_POINT = 1001,
            MACULAR_LINE = 1002,
            MACULAR_CROSS = 1003,
            MACULAR_RASTER = 1004,
            MACULAR_RADIAL = 1005,
            MACULAR_CUBE = 1007,
            MACULAR_ANGIO = 1108,
            DISC_POINT = 2001,
            DISC_RASTER = 2004,
            DISC_RADIAL = 2005,
            DISC_CIRCLE = 2006,
            DISC_CUBE = 2007,
            DISC_ANGIO = 2108,
            FUNDUS_LINE = 3002,
            FUNDUS_RASTER = 3004,
            FUNDUS_CUBE = 3007,
            FUNDUS_ANGIO = 3108,
            CORNEA_POINT = 4001,
            CORNEA_LINE = 4002,
            CORNEA_RADIAL = 4005,
            CORNEA_CUBE = 4007,
        };

        public enum OctPatternType : int
        {
            UNKNOWN = 0,
            POINT = 1,
            LINE = 2,
            CROSS = 3,
            RASTER = 4,
            RADIAL = 5,
            CIRCLE = 6,
            CUBE = 7,
            ANGIO = 8,
            MULTI_LINE = 9,
            VERT_LINE = 12,
            VERT_RASTER = 14,
            VERT_CUBE = 17,
            VERT_ANGIO = 18, 
            VERT_MULTI_LINE = 19,
        };
        public enum OctScanDirection : int
        {
            X_TO_Y = 0,
            Y_TO_X = 1
        };

        public enum OctScanSpeed : int
        {
            UNKNOWN = 0,
            SLOW = 1,
            NORMAL = 2,
            FAST = 3,
            CUSTOM = 4
        }
    }
}
