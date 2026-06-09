#pragma once

#include "OctReport.h"
#include "ProtocolReport.h"

namespace oct_result {
	class EnfaceImage;
}


namespace segm_scan {
	class DiscEshot;
	class DiscEplot;
	class DiscChart;

	class DiscThicknessMap;
	class DiscEnfaceImage;
	class DiscClockChart;
}


namespace oct_report
{
	class OCTREPORT_DLL_API OpticDiscScanReport : public ProtocolReport
	{
	public:
		OpticDiscScanReport();
		virtual ~OpticDiscScanReport();

		OpticDiscScanReport(OpticDiscScanReport&& rhs);
		OpticDiscScanReport& operator=(OpticDiscScanReport&& rhs);
		OpticDiscScanReport(const OpticDiscScanReport& rhs) = delete;
		OpticDiscScanReport& operator=(const OpticDiscScanReport& rhs) = delete;

	public:
		bool isNerveHeadCup(void) const;
		bool isNerveHeadDisc(void) const;
		bool isNerveHeadCenter(void) const;

		int getNerveHeadCenterLineIndex(void) const;
		int getNerveHeadCenterLateralPos(void) const;
		float getNerveHeadCenterX(void) const;
		float getNerveHeadCenterY(void) const;

		float getDiscArea(void) const;
		float getDiscVolume(void) const;
		float getCupArea(void) const;
		float getCupVolume(void) const;
		float getRimArea(void) const;

		float getCupDiscHorzRatio(void) const;
		float getCupDiscVertRatio(void) const;
		float getCupDiscAreaRatio(void) const;

		bool updateContents(void) override;
		void clearContents(void) override;

		bool getEnfaceImageDescript(OctLayerStratumParam param, OctEnfaceImageDescript& desc) override;
		bool getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc) override;
		bool getClockChartDescript(OctLayerStratumParam param, OctClockChartDescript& desc) override;
		bool getOpticDiscSummaryDescript(OctOpticDiscSummaryDescript& desc) override;

		bool composeDiscClockChart(OctLayerStratumParam param, segm_scan::DiscClockChart& chart, float circleSize = ONH_DISC_RNFL_345_SIZE);
		bool composeEnfaceImage(OctLayerStratumParam param, segm_scan::DiscEnfaceImage** image);
		bool composeThicknessMap(OctLayerStratumParam param, segm_scan::DiscThicknessMap** thick);

	protected:
		void locateCupDiscCenter(void);
		void calculateCupDiscArea(void);
		void calculateCupDiscVolume(void);
		void calculateCupDiscRatio(void);

		segm_scan::DiscEnfaceImage* getEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		segm_scan::DiscThicknessMap* getThicknessMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);

		std::unique_ptr<segm_scan::OcularEnfaceImage> createEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f) override;
		std::unique_ptr<segm_scan::OcularEnfaceMap> createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f) override;

	private:
		struct OpticDiscScanReportImpl;
		std::unique_ptr<OpticDiscScanReportImpl> d_ptr;
		OpticDiscScanReportImpl& getImpl(void) const;
	};
}

