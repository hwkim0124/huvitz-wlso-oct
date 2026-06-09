#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		bool WSOSYSTEM_DLL_API _stdcall isOctScanProtocolResultReady(void);
		void WSOSYSTEM_DLL_API _stdcall clearAllOctScanDataSources(void);
		void WSOSYSTEM_DLL_API _stdcall clearAllOctScanDataReports(void);
		bool WSOSYSTEM_DLL_API _stdcall isOctScanDataSourceValid(int sourceId);
		bool WSOSYSTEM_DLL_API _stdcall isOctScanDataReportValid(int reportId);

		bool WSOSYSTEM_DLL_API _stdcall processOctScanProtocolResult(bool segment, bool clear_olds, int* sourceId);
		bool WSOSYSTEM_DLL_API _stdcall analyzeOctScanProtocolData(int sourceId, bool segment, int* reportId);
		bool WSOSYSTEM_DLL_API _stdcall segmentOctScanProtocolData(int sourceId);
		bool WSOSYSTEM_DLL_API _stdcall importOctScanProtocolResult(LPCTSTR dir_path, bool clear_olds, int* sourceId);
		bool WSOSYSTEM_DLL_API _stdcall exportOctScanProtocolResult(LPCTSTR dir_path, LPTSTR out_path, int sourceId);

		int WSOSYSTEM_DLL_API _stdcall getOctPatternContentCount(int sourceId);
		int WSOSYSTEM_DLL_API _stdcall getOctPreviewContentCount(int sourceId);
		bool WSOSYSTEM_DLL_API _stdcall getOctPatternContentImage(int sourceId, int imageIdx, OctScanImageDescript* image);
		bool WSOSYSTEM_DLL_API _stdcall getOctPreviewContentImage(int sourceId, int imageIdx, OctScanImageDescript* image);

		bool WSOSYSTEM_DLL_API _stdcall getOctRetinaBsegmDescriptor(int sourceId, int imageIdx, OctRetinaBsegmDescriptor* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctCorneaBsegmDescriptor(int sourceId, int imageIdx, OctCorneaBsegmDescriptor* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctRetinaBsegmTraits(int sourceId, int imageIdx, OctRetinaBsegmTraits* traits);
		bool WSOSYSTEM_DLL_API _stdcall getOctCorneaBsegmTraits(int sourceId, int imageIdx, OctCorneaBsegmTraits* traits);
		bool WSOSYSTEM_DLL_API _stdcall getOctBsegmLayerPoints(int sourceId, int imageIdx, OcularLayerType type, OctBsegmLayerPoints* layer);

		bool WSOSYSTEM_DLL_API _stdcall getOctReportEnfaceImage(int reportId, OctLayerStratumParam param, OctEnfaceImageDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportAngioImage(int reportId, OctLayerStratumParam param, OctAngioImageDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportThicknessMap(int reportId, OctLayerStratumParam param, OctThicknessMapDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportETDRSChart(int reportId, OctLayerStratumParam param, OctETDRSChartDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportGCCThickChart(int reportId, OctLayerStratumParam param, OctGCCThickChartDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportClockChart(int reportId, OctLayerStratumParam param, OctClockChartDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportAngioChart(int reportId, OctLayerStratumParam param, OctAngioChartDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportMacularSummary(int reportId, OctMacularSummaryDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportOpticDiscSummary(int reportId, OctOpticDiscSummaryDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportProtocolDescript(int reportId, OctProtocolDescript* desc);

		bool WSOSYSTEM_DLL_API _stdcall getOctResultProtocolInitParam(int sourceId, OctProtocolInitParam* param);
		bool WSOSYSTEM_DLL_API _stdcall getOctReportProtocolInitParam(int reportId, OctProtocolInitParam* param);
	}
}