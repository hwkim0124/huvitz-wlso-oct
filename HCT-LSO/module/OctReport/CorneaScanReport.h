#pragma once

#include "OctReport.h"
#include "ProtocolReport.h"


namespace oct_result {
	class EnfaceImage;
}


namespace segm_scan {
	class CorneaEplot;
	class CorneaChart;

	class CorneaThicknessMap;
	class CorneaRadiusMap;
	class CorneaETDRSChart;
}


namespace oct_report
{
	class OCTREPORT_DLL_API CorneaScanReport : public ProtocolReport
	{
	public:
		CorneaScanReport();
		virtual ~CorneaScanReport();

		CorneaScanReport(CorneaScanReport&& rhs);
		CorneaScanReport& operator=(CorneaScanReport&& rhs);
		CorneaScanReport(const CorneaScanReport& rhs) = delete;
		CorneaScanReport& operator=(const CorneaScanReport& rhs) = delete;

	public:
		bool updateContents(void) override;
		void clearContents(void) override;

		bool getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc) override;

		bool composeCorneaETDRSChart(OctLayerStratumParam param, segm_scan::CorneaETDRSChart& chart);
		bool composeCurvatureRadiusMap(OctLayerStratumParam param, segm_scan::CorneaRadiusMap** rmap);
		bool composeThicknessMap(OctLayerStratumParam param, segm_scan::CorneaThicknessMap** thick);
	
	protected:
		segm_scan::CorneaThicknessMap* getThicknessMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		segm_scan::CorneaRadiusMap* getCurvatureRadiusMap(void);

		// These interfaces had been deprecated. 
		segm_scan::CorneaEplot* getCorneaEplot(OcularLayerType upper, OcularLayerType lower);

		std::unique_ptr<segm_scan::OcularEnfaceMap> createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f) override;
	
	private:
		struct CorneaScanReportImpl;
		std::unique_ptr<CorneaScanReportImpl> d_ptr;
		CorneaScanReportImpl& getImpl(void) const;
	};
}



