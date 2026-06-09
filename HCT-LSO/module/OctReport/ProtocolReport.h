#pragma once

#include "OctReport.h"

namespace segm_scan {
	class OcularEnfaceImage;
	class OcularEnfaceMap;
}

namespace oct_report
{
	class ProtocolSource;
	class LineScanData;
	class CubeScanData;

	class OCTREPORT_DLL_API ProtocolReport
	{
	public:
		ProtocolReport();
		virtual ~ProtocolReport();

		ProtocolReport(ProtocolReport&& rhs);
		ProtocolReport& operator=(ProtocolReport&& rhs);
		ProtocolReport(const ProtocolReport& rhs) = delete;
		ProtocolReport& operator=(const ProtocolReport& rhs) = delete;

	public:
		void setProtocolSource(oct_report::ProtocolSource* data, bool update = true);
		oct_report::ProtocolSource* getProtocolSource(void) const;
		OctScanProtocol* getScanProtocol(void) const;
		OctScanPattern* getScanPattern(void) const;

		int getReportId(void) const;
		void setReportId(int index);
		bool isSourceEmpty(void) const;

		void setEyeSide(EyeSide side);
		EyeSide getEyeSide(void) const;
		bool isEyeOD(void) const;

		bool isMacularReport(void) const;
		bool isOpticDiscReport(void) const;
		bool isCorneaReport(void) const;
		bool isAngioReport(void) const;

		virtual bool updateContents(void);
		virtual void clearContents(void);

		virtual bool getEnfaceImageDescript(OctLayerStratumParam param, OctEnfaceImageDescript& desc);
		virtual bool getAngioImageDescript(OctLayerStratumParam param, OctAngioImageDescript& desc);
		virtual bool getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc);
		virtual bool getETDRSChartDescript(OctLayerStratumParam param, OctETDRSChartDescript& desc);
		virtual bool getGCCThickChartDescript(OctLayerStratumParam param, OctGCCThickChartDescript& desc);
		virtual bool getClockChartDescript(OctLayerStratumParam param, OctClockChartDescript& desc);
		virtual bool getAngioChartDescript(OctLayerStratumParam param, OctAngioChartDescript& desc);

		virtual bool getMacularSummaryDescript(OctMacularSummaryDescript& desc);
		virtual bool getOpticDiscSummaryDescript(OctOpticDiscSummaryDescript& desc);
		virtual bool getProtocolDescript(OctProtocolDescript& desc);

	protected:
		segm_scan::OcularEnfaceImage* prepareEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		segm_scan::OcularEnfaceImage* findEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		bool registEnfaceImage(std::unique_ptr<segm_scan::OcularEnfaceImage> enface);

		segm_scan::OcularEnfaceMap* prepareEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		segm_scan::OcularEnfaceMap* findEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		bool registEnfaceMap(std::unique_ptr<segm_scan::OcularEnfaceMap> tmap);

		virtual std::unique_ptr<segm_scan::OcularEnfaceImage> createEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);
		virtual std::unique_ptr<segm_scan::OcularEnfaceMap> createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset = 0.0f, float lowerOffset = 0.0f);


	private:
		struct ProtocolReportImpl;
		std::unique_ptr<ProtocolReportImpl> d_ptr;
		ProtocolReportImpl& getImpl(void) const;
	};
}

