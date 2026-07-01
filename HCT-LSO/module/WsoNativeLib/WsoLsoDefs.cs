using System;
using System.Collections.Generic;
using System.Text;

namespace WsoNativeLib
{
    using static WsoNativeLib.WsoDomain;

    public static class WsoLsoDefs
    {
        // Enum Definitions
        /////////////////////////////////////////////////////////////////////////////////////////////
        public enum LsoScannerPatternId : int
        {
            COLOR = 0, 
            IR = 1,
        }

        public enum LsoScannerTriggerMode : int
        {
            RollingShutter = 0,
            GlobalShutter = 1,
        }

        public enum ColorPixelFormat : int
        {
            Mono8 = 0,      // 8 bit
            Mono16 = 1,     // 16 bit
            RGB8Packed = 2, // 24 bit
            BayerRG8 = 4,   // 8 bit
            BayerRG16 = 8,  // 16 bit
            BGR8 = 27,       // 24 bit
            UNKNOWN = -1
        }
        public enum ColorAdcBitDepth : int
        {
            Bit10 = 1,      // 10 bit
            Bit12 = 2,      // 12 bit
            UNKNOWN = -1
        }
    }
}
