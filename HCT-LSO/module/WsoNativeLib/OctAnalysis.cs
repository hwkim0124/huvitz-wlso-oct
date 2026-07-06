using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.ComponentModel;

namespace WsoNativeLib
{
    using static LibraryConfig;
    using static WsoDomain;
    using static WsoOctScan;
    using static WsoOctAngio;
    using static WsoOctSegm;

    public static class OctAnalysis
    {
        const string LibraryName = WsoSystemDllPath;
        static private int _sourceId = -1;
        static private int _reportId = -1;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanProtocolResultReady();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool clearAllOctScanDataSources();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool clearAllOctScanDataReports();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanDataSourceValid(int sourceId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctScanDataReportValid(int reportId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool processOctScanProtocolResult([MarshalAs(UnmanagedType.I1)] bool segment, [MarshalAs(UnmanagedType.I1)] bool clearOlds, ref int sourceId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool analyzeOctScanProtocolData(int sourceId, [MarshalAs(UnmanagedType.I1)] bool segment, ref int reportId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool segmentOctScanProtocolData(int sourceId);

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool importOctScanProtocolResult(string dir_path, [MarshalAs(UnmanagedType.I1)] bool clearOlds, ref int sourceId);

        [DllImport(LibraryName, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool exportOctScanProtocolResult(string dir_path, StringBuilder out_path, int sourceId);

        [DllImport(LibraryName)]
        private static extern int getOctPatternContentCount(int sourceId);

        [DllImport(LibraryName)]
        private static extern int getOctPreviewContentCount(int sourceId);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctPatternContentImage(int sourceId, int imageIdx, ref OctScanImageDescript image);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctPreviewContentImage(int sourceId, int imageIdx, ref OctScanImageDescript image);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctRetinaBsegmDescriptor(int sourceId, int imageIdx, ref OctRetinaBsegmDescriptor desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctCorneaBsegmDescriptor(int sourceId, int imageIdx, ref OctCorneaBsegmDescriptor desc);
        
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctBsegmLayerPoints(int sourceId, int imageIdx, OcularLayerType type, ref OctBsegmLayerPoints layer);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctRetinaBsegmTraits(int sourceId, int imageIdx, ref OctRetinaBsegmTraits traits);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctCorneaBsegmTraits(int sourceId, int imageIdx, ref OctCorneaBsegmTraits traits);


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportEnfaceImage(int reportId, OctStratumParam param, ref OctEnfaceImageDescript desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportAngioImage(int reportId, OctStratumParam param, ref OctAngioImageDescript desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportThicknessMap(int reportId, OctStratumParam param, ref OctThicknessMapDescript desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportETDRSChart(int reportId, OctStratumParam param, ref OctETDRSChartDescript desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportGCCThickChart(int reportId, OctStratumParam param, ref OctGCCThickChartDescript desc);


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportClockChart(int reportId, OctStratumParam param, ref OctClockChartDescript desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportAngioChart(int reportId, OctStratumParam param, ref OctAngioChartDescript desc);


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportMacularSummary(int reportId, ref OctMacularSummaryDescriptor desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportOpticDiscSummary(int reportId, ref OctOpticDiscSummaryDescriptor desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportProtocolDescript(int reportId, ref OctProtocolDescript desc);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctReportProtocolInitParam(int reportId, ref OctProtocolInitParam param);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool IsProtocolResultReady()
        {
            return isOctScanProtocolResultReady();
        }
        public static bool ClearAllDataSources()
        {
            _sourceId = -1;
            return clearAllOctScanDataSources();
        }
        public static bool ClearAllDataReports()
        {
            _reportId = -1;
            return clearAllOctScanDataReports();
        }

        public static bool IsOctScanDataSourceValid(int sourceId)
        {
            return isOctScanDataSourceValid(sourceId);
        }
        public static bool IsOctScanDataReportValid(int reportId)
        {
            return isOctScanDataReportValid(reportId);
        }

        public static int GetCurrentSourceId()
        {
            return _sourceId;
        }
        public static int GetCurrentReportId()
        {
            return _reportId;
        }
                
        public static bool ProcessProtocolResult(bool segment, bool clearOlds, ref int sourceId)
        {
            bool result = processOctScanProtocolResult(segment, clearOlds, ref sourceId);
            _sourceId = result ? sourceId : -1;
            return result;
        }

        public static bool AnalyzeProtocolData(int sourceId, bool segment, ref int reportId)
        {
            bool result = analyzeOctScanProtocolData(sourceId, segment, ref reportId);
            _reportId = result ? _reportId : -1;
            return result;
        }

        public static bool SegmentProtocolData(int sourceId)
        {
            return segmentOctScanProtocolData(sourceId);
        }

        public static bool ImportProtocolResult(string dir_path, bool clearOlds, ref int sourceId)
        {
            bool result = importOctScanProtocolResult(dir_path, clearOlds, ref sourceId);
            _sourceId = result ? sourceId : -1;
            return result;
        }

        public static bool ExportProtocolResult(int sourceId, string root_path, out string export_path)
        {
            var builder = new StringBuilder(512);
            if (exportOctScanProtocolResult(root_path, builder, sourceId))
            {
                export_path = builder.ToString();
                return true;
            }
            export_path = "";
            return false;
        }

        public static int GetPatternContentCount(int sourceId)
        {
            return getOctPatternContentCount(sourceId);
        }

        public static int GetPreviewContentCount(int sourceId)
        {
            return getOctPreviewContentCount(sourceId);
        }

        public static bool GetPatternImage(int sourceId, int image_idx, ref OctScanImageDescript image)
        {
            return getOctPatternContentImage(sourceId, image_idx, ref image);
        }
        public static bool GetPreviewImage(int sourceId, int image_idx, ref OctScanImageDescript image)
        {
            return getOctPreviewContentImage(sourceId, image_idx, ref image);
        }

        public static bool GetRetinaBsegmDescriptor(int sourceId, int image_idx, out OctRetinaBsegmDescriptor desc)
        {
            desc = new OctRetinaBsegmDescriptor();
            desc.InitializeObject();
            if (getOctRetinaBsegmDescriptor(sourceId, image_idx, ref desc))
            {
                return true;
            }
            return false;
        }

        public static bool GetCorneaBsegmDescriptor(int sourceId, int image_idx, out OctCorneaBsegmDescriptor desc)
        {
            desc = new OctCorneaBsegmDescriptor();
            desc.InitializeObject();
            if (getOctCorneaBsegmDescriptor(sourceId, image_idx, ref desc))
            {
                return true;
            }
            return false;
        }

        public static bool GetBsegmLayerPoints(int sourceId, int image_idx, OcularLayerType type, out OctBsegmLayerPoints layer)
        {
            layer = new OctBsegmLayerPoints();
            layer.InitializeObject();
            if (getOctBsegmLayerPoints(sourceId, image_idx, type, ref layer))
            {
                return true;
            }
            return false;
        }

        public static bool GetRetinaBsegmTraits(int sourceId, int image_idx, out OctRetinaBsegmTraits traits)
        {
            traits = new OctRetinaBsegmTraits();
            traits.InitializeObject();
            if (getOctRetinaBsegmTraits(sourceId, image_idx, ref traits))
            {
                return true;
            }
            return false;
        }

        public static bool GetCorneaBsegmTraits(int sourceId, int image_idx, out OctCorneaBsegmTraits traits)
        {
            traits = new OctCorneaBsegmTraits();
            traits.InitializeObject();
            if (getOctCorneaBsegmTraits(sourceId, image_idx, ref traits))
            {
                return true;
            }
            return false;
        }
        
        public static bool GetEnfaceImage(int reportId, OctStratumParam param, ref OctEnfaceImageDescript desc)
        {
            return getOctReportEnfaceImage(reportId, param, ref desc);
        }

        public static bool GetAngioImage(int reportId, OctStratumParam param, ref OctAngioImageDescript desc)
        {
            return getOctReportAngioImage(reportId, param, ref desc);
        }

        public static bool GetThicknessMap(int reportId, OctStratumParam param, ref OctThicknessMapDescript desc)
        {
            return getOctReportThicknessMap(reportId, param, ref desc);
        }

        public static bool GetETDRSChart(int reportId, OctStratumParam param, ref OctETDRSChartDescript desc)
        {
            return getOctReportETDRSChart(reportId, param, ref desc);
        }

        public static bool GetGCCThickChart(int reportId, OctStratumParam param, ref OctGCCThickChartDescript desc)
        {
            return getOctReportGCCThickChart(reportId, param, ref desc);
        }

        public static bool GetClockChart(int reportId, OctStratumParam param, ref OctClockChartDescript desc)
        {
            return getOctReportClockChart(reportId, param, ref desc);
        }

        public static bool GetAngioChart(int reportId, OctStratumParam param, ref OctAngioChartDescript desc)
        {
            return getOctReportAngioChart(reportId, param, ref desc);
        }

        public static bool GetMacularSummary(int reportId, ref OctMacularSummaryDescriptor desc)
        {
            return getOctReportMacularSummary(reportId, ref desc);
        }

        public static bool GetOpticDiscSummary(int reportId, ref OctOpticDiscSummaryDescriptor desc)
        {
            return getOctReportOpticDiscSummary(reportId, ref desc);
        }

        public static bool GetProtocolDescript(int reportId, ref OctProtocolDescript desc)
        {
            return getOctReportProtocolDescript(reportId, ref desc);
        }

        public static bool GetProtocolInitParam(int reportId, ref OctProtocolInitParam param)
        {
            return getOctReportProtocolInitParam(reportId, ref param);
        }
    }
}
