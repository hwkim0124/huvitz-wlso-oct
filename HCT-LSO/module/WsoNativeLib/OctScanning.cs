using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;



namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoCallback;
    using static WsoNativeLib.WsoOctScan;

    public static class OctScanning
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanWorking();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanPrevewing();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanMeasuring();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanOptimizing();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setupOctScanProtocol(OctProtocolInitParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startOctScanPreview(OctScanPreviewImageCaptured clb_preview, OctScanEnfaceImageCaptured? clb_enface);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool closeOctScanPreview([MarshalAs(UnmanagedType.I1)] bool to_measure, OctScanProtocolCompleted clb_protocol);

        [DllImport(LibraryName)]
        private static extern void cancelOctScanning();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startOctScanAutoDiopterFocus(OctAutoFocusOptimized clb_focus);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startOctScanAutoPolarization(OctAutoPolarOptimized clb_polar);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startOctScanAutoReference(OctAutoReferOptimized clb_refer);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool startOctScanAutoOptimize(OctAutoScanOptimized clb_optimize);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool cancelOctScanAutoOptimizing();

        [DllImport(LibraryName)]
        private static extern void connectOctSpectrumDataCaptured(OctSpectrumDataCaptured clb_spectrum);

        [DllImport(LibraryName)]
        private static extern void connectOctResampleDataCaptured(OctResampleDataCaptured clb_resample);

        [DllImport(LibraryName)]
        private static extern void connectOctIntensityDataCaptured(OctIntensityDataCaptured clb_intensity);

        [DllImport(LibraryName)]
        private static extern void releaseOctSpectrumDataCaptured();

        [DllImport(LibraryName)]
        private static extern void releaseOctResampleDataCaptured();

        [DllImport(LibraryName)]
        private static extern void releaseOctIntensityDataCaptured();


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////   
        public static bool IsWorking()
        {
            return isOctScanWorking();
        }

        public static bool IsPreviewing()
        {
            return isOctScanPrevewing();
        }

        public static bool IsMeasuring()
        {
            return isOctScanMeasuring();
        }

        public static bool IsAutoOptimizing()
        {
            return isOctScanOptimizing();
        }

        public static bool SetupScanProtocol(OctProtocolInitParam param)
        {
            return setupOctScanProtocol(param);
        }

        public static bool StartPreview(OctScanPreviewImageCaptured clb_preview, OctScanEnfaceImageCaptured? clb_enface)
        {
            return startOctScanPreview(clb_preview, clb_enface);
        }

        public static bool ClosePreview(bool to_measure, OctScanProtocolCompleted clb_protocol)
        {
            return closeOctScanPreview(to_measure, clb_protocol);
        }
        public static void CancelScanning()
        {
            cancelOctScanning();
        }

        public static bool StartAutoDiopterFocus(OctAutoFocusOptimized clb_focus)
        {
            return startOctScanAutoDiopterFocus(clb_focus);
        }

        public static bool StartAutoPolarization(OctAutoPolarOptimized clb_polar)
        {
            return startOctScanAutoPolarization(clb_polar);
        }

        public static bool StartAutoReference(OctAutoReferOptimized clb_refer)
        {
            return startOctScanAutoReference(clb_refer);
        }

        public static bool StartAutoOptimize(OctAutoScanOptimized clb_optimize)
        {
            return startOctScanAutoOptimize(clb_optimize);
        }

        public static bool CancelAutoOptimizing()
        {
            return cancelOctScanAutoOptimizing();
        }

        public static void ConnectSpectrumDataCaptured(OctSpectrumDataCaptured clb_spectrum)
        {
            connectOctSpectrumDataCaptured(clb_spectrum);
        }

        public static void ConnectResampleDataCaptured(OctResampleDataCaptured clb_resample)
        {
            connectOctResampleDataCaptured(clb_resample);
        }

        public static void ConnectIntensityDataCaptured(OctIntensityDataCaptured clb_intensity)
        {
            connectOctIntensityDataCaptured(clb_intensity);
        }

        public static void ReleaseSpectrumDataCaptured()
        {
            releaseOctSpectrumDataCaptured();
        }

        public static void ReleaseResampleDataCaptured()
        {
            releaseOctResampleDataCaptured();
        }

        public static void ReleaseIntensityDataCaptured()
        {
            releaseOctIntensityDataCaptured();
        }
    }
}
