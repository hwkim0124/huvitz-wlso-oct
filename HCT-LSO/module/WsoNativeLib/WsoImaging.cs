using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WsoNativeLib
{
    public static class WsoImaging
    {
        ///////////////////////////////////////////////////////////////////////////////////
        /// Enum Definitions
        /// 
        public enum ImagingMode : uint 
        {
            SLO_IR = 1,
            SLO_ICGA = 2,
            SLO_IR_OCT = 3,
            SLO_FA = 4,
            SLO_FA_ICGA = 5,
            SLO_BLUE_FAF = 6,
            SLO_GREEN_FAF = 7,
            SLO_RGB = 8,
            NUM_IMAGING_MODES = 9,
        }

        public enum ImagingSourceLevel : uint
        {
            NORMAL = 0,
            LOW = 1,
        }
    }
}
