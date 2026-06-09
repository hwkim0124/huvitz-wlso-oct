#pragma once

#include "OctReport.h"
#include "ProtocolReport.h"


namespace oct_result {
	class EnfaceImage;
}

namespace segm_scan {
	class GCCThicknessChart;
	class MacularETDRSChart;
	class MacularEnfaceImage;
	class MacularThicknessMap;
}


namespace oct_report
{
	class OCTREPORT_DLL_API MacularScanReport : public ProtocolReport
	{
	public:
		MacularScanReport();
		virtual ~MacularScanReport();

		MacularScanReport(MacularScanReport&& rhs);
		MacularScanReport& operator=(MacularScanReport&& rhs);
		MacularScanReport(const MacularScanReport& rhs) = delete;
		MacularScanReport& operator=(const MacularScanReport& rhs) = delete;

	public:
		bool isFoveaCenter(void) const;
		float getFoveaCenterX(void) const;
		float getFoveaCenterY(void) const;
		float getFoveaCenterThickness(void) const;

		int getFoveaCenterLineIndex(void) const;
		int getFoveaCenterLateralPos(void) const;

		bool updateContents(void) override;
		void clearContents(void) override;

		bool getEnfaceImageDescript(OctLayerStratumParam param, OctEnfaceImageDescript& desc) override;
		bool getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc) override;
		bool getETDRSChartDescript(OctLayerStratumParam param, OctETDRSChartDescript& desc) override;
		bool getGCCThickChartDescript(OctLayerStratumParam param, OctGCCThickChartDescript& desc) override;
		bool getMacularSummaryDescript(OctMacularSummaryDescript& desc) override;

		bool composeGCCThicknessChart(OctLayerStratumParam param, segm_scan::GCCThicknessChart& chart);
		bool composeMacularETDRSChart(OctLayerStratumParam param, segm_scan::MacularETDRSChart& chart);
		bool composeEnfaceImage(OctLayerStratumParam param, segm_scan::MacularEnfaceImage** image);
		bool composeThicknessMap(OctLayerStratumParam param, segm_scan::MacularThicknessMap** thick);
	
	protected:
		void locateFoveaCenter(void);

		segm_scan::MacularEnfaceImage* getEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		segm_scan::MacularThicknessMap* getThicknessMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		std::unique_ptr<segm_scan::OcularEnfaceImage> createEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f) override;
		std::unique_ptr<segm_scan::OcularEnfaceMap> createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f) override;

	private:
		struct MacularScanReportImpl;
		std::unique_ptr<MacularScanReportImpl> d_ptr;
		MacularScanReportImpl& getImpl(void) const;
	};
}

