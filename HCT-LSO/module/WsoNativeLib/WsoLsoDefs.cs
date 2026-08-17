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

        // Mono aiming-preview drive mode for the color camera one-call preview API.
        public enum ColorMonoPreviewMode : int
        {
            SensorMono8 = 0,  // camera Mono8 + binning 2x2 (half pixel resolution)
            BayerSwGray = 1,  // camera BayerRG8 + binning 1x1 (full resolution), grayscale at display side
        }

        // Result/stage codes for the color capture one-call (mirror of native ColorCaptureStage).
        // Start return: Ok/Busy/Param/Scanner (no completion callback follows a non-Ok start).
        // Completion callback: Ok/Camera/Cancelled/Fault (how the acquisition ended).
        public enum ColorCaptureStage : int
        {
            Ok = 0,
            Busy = 1,
            Param = 2,
            Scanner = 3,
            Camera = 4,
            Cancelled = 5,
            Fault = 9,
        }
    }
}
