using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Security.RightsManagement;

namespace WsoNativeLib
{
    using static WsoNativeLib.WsoDomain;

    public static class WsoCallback
    {
        // System Log message
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public delegate void WsoLogMsgCallback(string msg, int type);

        // External Keys 
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void JoystickButtonPressed();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OptimizeButtonPressed();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void AdapterLensAttached(int type);


        // Cornea Camera
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void CorneaCameraFrameCaptured(IntPtr data, int width, int height);

        
        // OCT Scanning 
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctScanPreviewImageCaptured(IntPtr data, int width, int height, float quality, float snr_ratio, int ref_point, int index_image);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctScanEnfaceImageCaptured(IntPtr data, int width, int height);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctAutoScanOptimized([MarshalAs(UnmanagedType.I1)] bool result);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctAutoFocusOptimized([MarshalAs(UnmanagedType.I1)] bool result, float quality, float diopter);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctAutoPolarOptimized([MarshalAs(UnmanagedType.I1)] bool result, float quality, float degree);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctAutoReferOptimized([MarshalAs(UnmanagedType.I1)] bool result, float quality, int ref_point, int position);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctSpectrumDataCaptured(IntPtr data, int width, int height);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctResampleDataCaptured(IntPtr data, int width, int height);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctIntensityDataCaptured(IntPtr data, int width, int height);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctScanPatternAcquired([MarshalAs(UnmanagedType.I1)] bool result);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OctScanProtocolCompleted([MarshalAs(UnmanagedType.I1)] bool result);
    }
}
