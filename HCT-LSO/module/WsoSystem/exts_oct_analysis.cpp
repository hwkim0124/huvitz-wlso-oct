#include "pch.h"
#include "exts_oct_analysis.h"
#include "OctAnalysis.h"
#include "OctArchive.h"



bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanProtocolResultReady(void)
{
	if (auto* p = OctArchive::getInstance(); p) {
		if (p->isProtocolResultValid()) {
			return true;
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::clearAllOctScanDataSources(void)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		p->clearAllDataSources();
	}
}

void WSOSYSTEM_DLL_API _stdcall wso_system::clearAllOctScanDataReports(void)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		p->clearAllScanReports();
	}
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanDataSourceValid(int sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		auto result = p->checkIfDataSourceValid(sourceId);
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanDataReportValid(int reportId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		auto result = p->checkIfScanReportValid(reportId);
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::processOctScanProtocolResult(bool segment, bool clear_olds, int* sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		return p->processScanningResult(segment, clear_olds, *sourceId);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::analyzeOctScanProtocolData(int sourceId, bool segment, int* reportId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		return p->analyzeProtocolData(sourceId, segment, *reportId);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::segmentOctScanProtocolData(int sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		return p->segmentProtocolData(sourceId);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::importOctScanProtocolResult(LPCTSTR dir_path, bool clear_olds, int* sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		auto path = std::wstring(dir_path);
		auto flag = p->importProtocolResult(path, clear_olds, *sourceId);
		return flag;
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::exportOctScanProtocolResult(LPCTSTR dir_path, LPTSTR out_path, int sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		auto dir_str = std::wstring(dir_path);
		auto out_str = std::wstring();
		auto flag = p->exportProtocolResult(dir_str, out_str, sourceId);
		if (flag) {
			wcsncpy_s((wchar_t*)out_path, 512, out_str.c_str(), out_str.length());
		}
		return flag;
	}
	return false;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctPatternContentCount(int sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			return data->getPatternContentCount();
		}
	}
	return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctPreviewContentCount(int sourceId)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			return data->getPreviewContentCount();
		}
	}
	return 0;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctPatternContentImage(int sourceId, int imageIdx, OctScanImageDescript* image)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			if (auto* desc = data->getPatternImageDescript(imageIdx); desc) {
				memcpy(image, desc, sizeof(OctScanImageDescript));
				return true;
			}
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctPreviewContentImage(int sourceId, int imageIdx, OctScanImageDescript* image)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			if (auto* desc = data->getPreviewImageDescript(imageIdx); desc) {
				memcpy(image, desc, sizeof(OctScanImageDescript));
				return true;
			}
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctRetinaBsegmDescriptor(int sourceId, int imageIdx, OctRetinaBsegmDescriptor* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			int sectIdx = imageIdx;
			if (data->getRetinaBsegmDescriptor(sectIdx, 0, *desc)) {
				return true;
			}
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctCorneaBsegmDescriptor(int sourceId, int imageIdx, OctCorneaBsegmDescriptor* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			int sectIdx = imageIdx;
			if (data->getCorneaBsegmDescriptor(sectIdx, 0, *desc)) {
				return true;
			}
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctRetinaBsegmTraits(int sourceId, int imageIdx, OctRetinaBsegmTraits* traits)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			int sectIdx = imageIdx;
			if (data->getRetinaBsegmTraits(sectIdx, 0, *traits)) {
				return true;
			}
		}
	}
	return false;
}


bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctCorneaBsegmTraits(int sourceId, int imageIdx, OctCorneaBsegmTraits* traits)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			int sectIdx = imageIdx;
			if (data->getCorneaBsegmTraits(sectIdx, 0, *traits)) {
				return true;
			}
		}
	}
	return false;
}


bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctBsegmLayerPoints(int sourceId, int imageIdx, OcularLayerType type, OctBsegmLayerPoints* layer)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			int sectIdx = imageIdx;
			if (data->getBsegmLayerPoints(sectIdx, 0, type, *layer)) {
				return true;
			}
		}
	}
	return false;
}


bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportEnfaceImage(int reportId, OctLayerStratumParam param, OctEnfaceImageDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->produceEnfaceImage(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportAngioImage(int reportId, OctLayerStratumParam param, OctAngioImageDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->produceAngioImage(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportThicknessMap(int reportId, OctLayerStratumParam param, OctThicknessMapDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getThicknessMap(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportETDRSChart(int reportId, OctLayerStratumParam param, OctETDRSChartDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getETDRSChart(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportGCCThickChart(int reportId, OctLayerStratumParam param, OctGCCThickChartDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getGCCThickChart(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportClockChart(int reportId, OctLayerStratumParam param, OctClockChartDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getClockChart(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportAngioChart(int reportId, OctLayerStratumParam param, OctAngioChartDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getAngioChart(reportId, param, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportMacularSummary(int reportId, OctMacularSummaryDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getMacularSummary(reportId, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportOpticDiscSummary(int reportId, OctOpticDiscSummaryDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getOpticDiscSummary(reportId, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportProtocolDescript(int reportId, OctProtocolDescript* desc)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (p->getReportProtocolDescript(reportId, *desc)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultProtocolInitParam(int sourceId, OctProtocolInitParam* param)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* data = p->getProtocolSource(sourceId); data) {
			data->getScanProtocol().fetchToInitParam(*param);
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctReportProtocolInitParam(int reportId, OctProtocolInitParam* param)
{
	if (auto* p = OctAnalysis::getInstance(); p) {
		if (auto* report = p->getProtocolReport(reportId); report) {
			if (auto* proto = report->getProtocolSource(); proto) {
				proto->getScanProtocol().fetchToInitParam(*param);
				return true;
			}
		}
	}
	return false;
}
