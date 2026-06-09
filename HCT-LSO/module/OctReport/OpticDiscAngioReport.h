#pragma once

#include "OctReport.h"
#include "OpticDiscScanReport.h"


namespace oct_angio {
	class AngioEtdrsChart;
	class AngioGridChart;
	class AngioDonutChart;
}

namespace oct_report
{
	class OCTREPORT_DLL_API OpticDiscAngioReport : public OpticDiscScanReport
	{
	public:
		OpticDiscAngioReport();
		virtual ~OpticDiscAngioReport();

		OpticDiscAngioReport(OpticDiscAngioReport&& rhs);
		OpticDiscAngioReport& operator=(OpticDiscAngioReport&& rhs);
		OpticDiscAngioReport(const OpticDiscAngioReport& rhs) = delete;
		OpticDiscAngioReport& operator=(const OpticDiscAngioReport& rhs) = delete;

	public:
		bool updateContents(void) override;
		void clearContents(void) override;

		bool getAngioImageDescript(OctLayerStratumParam param, OctAngioImageDescript& desc) override;
		bool getAngioChartDescript(OctLayerStratumParam param, OctAngioChartDescript& desc) override;

		bool composeAngioImage(OctLayerStratumParam param) const;
		bool composeAngioETDRSChart(OctLayerStratumParam param, oct_angio::AngioEtdrsChart& chart) const;

	private:
		struct OpticDiscAngioReportImpl;
		std::unique_ptr<OpticDiscAngioReportImpl> d_ptr;
		OpticDiscAngioReportImpl& impl(void) const;
	};
}

