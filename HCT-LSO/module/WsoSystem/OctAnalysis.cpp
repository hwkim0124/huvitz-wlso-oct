#include "pch.h"
#include "OctAnalysis.h"
#include "OctArchive.h"
#include "WsoSystem2.h"

using namespace wso_system;
using namespace std;


std::mutex OctAnalysis::singleMutex_;


struct OctAnalysis::OctAnalysisImpl
{
	bool initialized;
	ProtocolSourceSet sourceSet;
	ProtocolReportSet reportSet;

	OctAnalysisImpl() {
		initialized = false;
	}
};


wso_system::OctAnalysis::OctAnalysis() :
	d_ptr(std::make_unique<OctAnalysisImpl>())
{
}


wso_system::OctAnalysis::~OctAnalysis()
{
}


OctAnalysis* wso_system::OctAnalysis::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static OctAnalysis instance;
	return &instance;
}

bool wso_system::OctAnalysis::initializeOctAnalysis(void)
{
	impl().initialized = true;
	return isInitialized();
}


void wso_system::OctAnalysis::releaseOctAnalysis(void)
{
	clearAllScanReports();
	clearAllDataSources();
	impl().initialized = false;
	return;
}


bool wso_system::OctAnalysis::isInitialized(void) const
{
	return impl().initialized;
}

bool wso_system::OctAnalysis::processScanningResult(bool segment, bool clear_olds, int& sourceId)
{
	sourceId = -1;
	if (auto* p = OctArchive::getInstance(); p) {
		if (auto result = p->fetchProtocolResultLast(); result.get()) {
			if (auto data = emplaceProtocolSource(std::move(result), clear_olds); data) {
				if (segment) {
					auto flag = performSegmentation(data);
				}
				sourceId = data->getSourceId();
				return true;
			}
		}
	}
	LogD() << "Failed to process scanning result!";
	return false;
}

bool wso_system::OctAnalysis::analyzeProtocolData(int sourceId, bool segment, int& reportId)
{
	reportId = -1;
	if (auto data = getProtocolSource(sourceId); data) {
		if (segment) {
			auto flag = performSegmentation(data);
		}

		if (auto report = emplaceProtocolReport(data, false); report) {
			LogD() << "Protocol report emplaced, sourceId: " << sourceId << ", reportId: " << report->getReportId();
			reportId = report->getReportId();
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::segmentProtocolData(int sourceId)
{
	if (auto data = getProtocolSource(sourceId); data) {
		auto flag = performSegmentation(data);
		return flag;
	}
	return false;
}

bool wso_system::OctAnalysis::importProtocolResult(const std::wstring& dir_path, bool clear_olds, int& sourceId)
{
	if (auto data = emplaceProtocolSource(dir_path, clear_olds); data) {
		if (data->importProtocolContent(dir_path)) {
			sourceId = data->getSourceId();
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::exportProtocolResult(const std::wstring& dir_path, std::wstring& out_path, int sourceId)
{
	if (auto data = getProtocolSource(sourceId); data) {
		auto flag = data->exportProtocolContent(dir_path, out_path);
		return flag;
	}
	return false;
}

bool wso_system::OctAnalysis::performSegmentation(oct_report::ProtocolSource* data)
{
	if (data != nullptr) {
		StopWatch::start();
		auto desc = data->getScanPattern();
		if (desc.isCorneaScan()) {
			CorneaSession session;
			if (session.loadScanData(data)) {
				auto flag = session.processScanData();
				LogD() << "Cornea scan data segmented, elapsed: " << StopWatch::getElapsedMillis();
				return flag;
			}
		}
		if (desc.isMarcularScan()) {
			MacularSession session;
			if (session.loadScanData(data)) {
				auto flag = session.processScanData();
				LogD() << "Macular scan data segmented, elapsed: " << StopWatch::getElapsedMillis();
				return flag;
			}
		}
		if (desc.isOpticDiscScan()) {
			OpticDiscSession session;
			if (session.loadScanData(data)) {
				auto flag = session.processScanData();
				LogD() << "Optic disc scan data segmented, elapsed: " << StopWatch::getElapsedMillis();
				return flag;
			}
		}
	}
	LogD() << "Failed to perform scan data segmentation!";
	return false;
}

bool wso_system::OctAnalysis::produceEnfaceImage(int reportId, OctLayerStratumParam param, OctEnfaceImageDescript& image)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->isMacularReport() || report->isOpticDiscReport()) {
			if (report->getEnfaceImageDescript(param, image)) {
				return true;
			}
		}
	}
	return false;
}

bool wso_system::OctAnalysis::produceAngioImage(int reportId, OctLayerStratumParam param, OctAngioImageDescript& image)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->isAngioReport()) {
			if (report->getAngioImageDescript(param, image)) {
				return true;
			}
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getThicknessMap(int reportId, OctLayerStratumParam param, OctThicknessMapDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getThicknessMapDescript(param, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getETDRSChart(int reportId, OctLayerStratumParam param, OctETDRSChartDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getETDRSChartDescript(param, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getGCCThickChart(int reportId, OctLayerStratumParam param, OctGCCThickChartDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getGCCThickChartDescript(param, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getClockChart(int reportId, OctLayerStratumParam param, OctClockChartDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getClockChartDescript(param, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getAngioChart(int reportId, OctLayerStratumParam param, OctAngioChartDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getAngioChartDescript(param, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getMacularSummary(int reportId, OctMacularSummaryDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getMacularSummaryDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getOpticDiscSummary(int reportId, OctOpticDiscSummaryDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getOpticDiscSummaryDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctAnalysis::getReportProtocolDescript(int reportId, OctProtocolDescript& desc)
{
	if (auto report = getProtocolReport(reportId); report) {
		if (report->getProtocolDescript(desc)) {
			return true;
		}
	}
	return false;
}

oct_report::ProtocolSource* wso_system::OctAnalysis::emplaceProtocolSource(const std::wstring& dir_path, bool clear_olds)
{
	auto result = std::make_unique<ProtocolResult>();
	if (result->importScanProtocol(dir_path)) {
		auto source = emplaceProtocolSource(std::move(result), clear_olds);
		return source;
	}
	return nullptr;
}

oct_report::ProtocolSource* wso_system::OctAnalysis::emplaceProtocolSource(unique_ptr<oct_result::ProtocolResult>&& result, bool clear_olds)
{
	if (clear_olds) {
		clearAllDataSources();
	}

	auto& desc = result->getScanProtocol();
	ProtocolSource* data = nullptr;
	int sourceId = impl().sourceSet.generateSourceId();

	if (desc.isLineScan()) {
		data = obtainLineDataSource(sourceId);
	}
	else if (desc.isCrossScan()) {
		data = obtainCrossDataSource(sourceId);
	}
	else if (desc.isCircleScan()) {
		data = obtainCircleDataSource(sourceId);
	}
	else if (desc.isRadialScan()) {
		data = obtainRadialDataSource(sourceId);
	}
	else if (desc.isRasterScan()) {
		data = obtainRasterDataSource(sourceId);
	}
	else if (desc.isCubeScan()) {
		data = obtainCubeDataSource(sourceId);
	}
	else if (desc.isAngioScan()) {
		data = obtainAngioDataSource(sourceId);
	}
	else {
		LogD() << "Failed to emplace data source, unknown pattern type: " << (int)desc.getMeasure().patternType;
		return nullptr;
	}

	data->setupProtocolResult(std::move(result));
	return data;
}

oct_report::ProtocolSource* wso_system::OctAnalysis::getProtocolSource(int sourceId) const
{
	return impl().sourceSet.getProtocolSource(sourceId);
}

oct_report::LineDataSource* wso_system::OctAnalysis::obtainLineDataSource(int index)
{
	return impl().sourceSet.getOrCreateLineDataSource(index);
}

oct_report::CrossDataSource* wso_system::OctAnalysis::obtainCrossDataSource(int index)
{
	return impl().sourceSet.getOrCreateCrossDataSource(index);
}

oct_report::CircleDataSource* wso_system::OctAnalysis::obtainCircleDataSource(int index)
{
	return impl().sourceSet.getOrCreateCircleDataSource(index);
}

oct_report::RadialDataSource* wso_system::OctAnalysis::obtainRadialDataSource(int index)
{
	return impl().sourceSet.getOrCreateRadialDataSource(index);
}

oct_report::RasterDataSource* wso_system::OctAnalysis::obtainRasterDataSource(int index)
{
	return impl().sourceSet.getOrCreateRasterDataSource(index);
}

oct_report::CubeDataSource* wso_system::OctAnalysis::obtainCubeDataSource(int index)
{
	return impl().sourceSet.getOrCreateCubeDataSource(index);
}

oct_report::AngioDataSource* wso_system::OctAnalysis::obtainAngioDataSource(int index)
{
	return impl().sourceSet.getOrCreateAngioDataSource(index);
}

bool wso_system::OctAnalysis::checkIfDataSourceValid(int sourceId) const
{
	if (auto data = getProtocolSource(sourceId); data) {
		auto flag = !data->isContentEmpty();
		return flag;
	}
	return false;
}

bool wso_system::OctAnalysis::checkIfScanReportValid(int reportId) const
{
	bool flag = getProtocolReport(reportId) != nullptr;
	return flag;
}

void wso_system::OctAnalysis::clearAllDataSources(void)
{
	impl().sourceSet.clearAllSources();
}

void wso_system::OctAnalysis::clearAllScanReports(void)
{
	impl().reportSet.clearAllReports();
}

oct_report::ProtocolReport* wso_system::OctAnalysis::getProtocolReport(int reportId) const
{
	return impl().reportSet.getProtocolReport(reportId);
}

oct_report::ProtocolReport* wso_system::OctAnalysis::emplaceProtocolReport(oct_report::ProtocolSource* source, bool clear)
{
	if (clear) {
		clearAllScanReports();
	}

	if (source != nullptr) {
		auto desc = source->getScanProtocol();
		ProtocolReport* report = nullptr;
		int reportId = impl().reportSet.generateReportId();

		if (desc.isMacularScan()) {
			if (desc.isAngioScan()) {
				report = obtainMacularAngioReport(reportId);
			}
			else {
				report = obtainMacularScanReport(reportId);
			}
		}
		else if (desc.isOpticDiscScan()) {
			if (desc.isAngioScan()) {
				report = obtainOpticDiscAngioReport(reportId);
			}
			else {
				report = obtainOpticDiscScanReport(reportId);
			}
		}
		else if (desc.isCorneaScan()) {
			report = obtainCorneaScanReport(reportId);
		}

		if (report) {
			report->setProtocolSource(source, true);
			return report;
		}
	}

	LogD() << "Failed to emplace scan report, invalid protocol source!";
	return nullptr;
}

oct_report::MacularScanReport* wso_system::OctAnalysis::obtainMacularScanReport(int reportId)
{
	return impl().reportSet.getOrCreateMacularScanReport(reportId);
}

oct_report::OpticDiscScanReport* wso_system::OctAnalysis::obtainOpticDiscScanReport(int reportId)
{
	return impl().reportSet.getOrCreateOpticDiscScanReport(reportId);
}

oct_report::CorneaScanReport* wso_system::OctAnalysis::obtainCorneaScanReport(int reportId)
{
	return impl().reportSet.getOrCreateCorneaScanReport(reportId);
}

oct_report::MacularAngioReport* wso_system::OctAnalysis::obtainMacularAngioReport(int reportId)
{
	return impl().reportSet.getOrCreateMacularAngioReport(reportId);
}

oct_report::OpticDiscAngioReport* wso_system::OctAnalysis::obtainOpticDiscAngioReport(int reportId)
{
	return impl().reportSet.getOrCreateOpticDiscAngioReport(reportId);
}


OctAnalysis::OctAnalysisImpl& wso_system::OctAnalysis::impl(void) const
{
	return *d_ptr;
}