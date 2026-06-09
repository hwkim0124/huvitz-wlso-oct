#pragma once


#include "WsoSystem2.h"

namespace oct_result {
	class ProtocolResult;
}

namespace oct_report {
	class ProtocolSource;
	class LineDataSource;
	class CrossDataSource;
	class CircleDataSource;
	class RadialDataSource;
	class RasterDataSource;
	class CubeDataSource;
	class AngioDataSource;
}


namespace wso_system
{
	class WSOSYSTEM_DLL_API OctAnalysis
	{
	public:
		OctAnalysis();
		virtual ~OctAnalysis();

		OctAnalysis(const OctAnalysis& rhs) = delete;
		OctAnalysis& operator=(const OctAnalysis& rhs) = delete;

		static OctAnalysis* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeOctAnalysis(void);
		void releaseOctAnalysis(void);
		bool isInitialized(void) const;

		bool processScanningResult(bool segment, bool clear_olds, int& sourceId);
		bool analyzeProtocolData(int sourceId, bool segment, int& reportId);
		bool segmentProtocolData(int sourceId);
		bool importProtocolResult(const std::wstring& dir_path, bool clear_olds, int& sourceId);
		bool exportProtocolResult(const std::wstring& dir_path, std::wstring& out_path, int sourceId);

		bool performSegmentation(oct_report::ProtocolSource* data);

		bool produceEnfaceImage(int reportId, OctLayerStratumParam param, OctEnfaceImageDescript& image);
		bool produceAngioImage(int reportId, OctLayerStratumParam param, OctAngioImageDescript& image);
		bool getThicknessMap(int reportId, OctLayerStratumParam param, OctThicknessMapDescript& desc);
		bool getETDRSChart(int reportId, OctLayerStratumParam param, OctETDRSChartDescript& desc);
		bool getGCCThickChart(int reportId, OctLayerStratumParam param, OctGCCThickChartDescript& desc);
		bool getClockChart(int reportId, OctLayerStratumParam param, OctClockChartDescript& desc);
		bool getAngioChart(int reportId, OctLayerStratumParam param, OctAngioChartDescript& desc);

		bool getMacularSummary(int reportId, OctMacularSummaryDescript& desc);
		bool getOpticDiscSummary(int reportId, OctOpticDiscSummaryDescript& desc);
		bool getReportProtocolDescript(int reportId, OctProtocolDescript& desc);

		oct_report::ProtocolSource* emplaceProtocolSource(const std::wstring& dir_path, bool clear_olds);
		oct_report::ProtocolSource* emplaceProtocolSource(unique_ptr<oct_result::ProtocolResult>&& result, bool clear_olds);
		oct_report::ProtocolSource* getProtocolSource(int sourceId = 0) const;
		
		bool checkIfDataSourceValid(int sourceId = 0) const;
		bool checkIfScanReportValid(int reportId = 0) const;
		void clearAllDataSources(void);
		void clearAllScanReports(void);

		oct_report::ProtocolReport* emplaceProtocolReport(oct_report::ProtocolSource* source, bool clear);
		oct_report::ProtocolReport* getProtocolReport(int reportId = 0) const;

	protected:
		oct_report::LineDataSource* obtainLineDataSource(int sourceId = 0);
		oct_report::CrossDataSource* obtainCrossDataSource(int sourceId = 0);
		oct_report::CircleDataSource* obtainCircleDataSource(int sourceId = 0);
		oct_report::RadialDataSource* obtainRadialDataSource(int sourceId = 0);
		oct_report::RasterDataSource* obtainRasterDataSource(int sourceId = 0);
		oct_report::CubeDataSource* obtainCubeDataSource(int sourceId = 0);
		oct_report::AngioDataSource* obtainAngioDataSource(int sourceId = 0);

		oct_report::MacularScanReport* obtainMacularScanReport(int reportId = 0);
		oct_report::OpticDiscScanReport* obtainOpticDiscScanReport(int reportId = 0);
		oct_report::CorneaScanReport* obtainCorneaScanReport(int reportId = 0);
		oct_report::MacularAngioReport* obtainMacularAngioReport(int reportId = 0);
		oct_report::OpticDiscAngioReport* obtainOpticDiscAngioReport(int reportId = 0);

	private:
		struct OctAnalysisImpl;
		std::unique_ptr<OctAnalysisImpl> d_ptr;
		OctAnalysisImpl& impl(void) const;
	};
}
