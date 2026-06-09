#pragma once

#include "OctReport.h"
#include "MacularScanReport.h"

namespace oct_angio {
	class AngioEtdrsChart;
	class AngioGridChart;
	class AngioDonutChart;
}

namespace oct_report
{
	class OCTREPORT_DLL_API MacularAngioReport : public MacularScanReport
	{
	public:
		MacularAngioReport();
		virtual ~MacularAngioReport();

		MacularAngioReport(MacularAngioReport&& rhs);
		MacularAngioReport& operator=(MacularAngioReport&& rhs);
		MacularAngioReport(const MacularAngioReport& rhs) = delete;
		MacularAngioReport& operator=(const MacularAngioReport& rhs) = delete;

	public:
		bool updateContents(void) override;
		void clearContents(void) override;

		bool getAngioImageDescript(OctLayerStratumParam param, OctAngioImageDescript& desc) override;
		bool getAngioChartDescript(OctLayerStratumParam param, OctAngioChartDescript& desc) override;

		bool composeAngioImage(OctLayerStratumParam param) const;
		bool composeAngioETDRSChart(OctLayerStratumParam param, oct_angio::AngioEtdrsChart& chart) const;

	private:
		struct MacularAngioReportImpl;
		std::unique_ptr<MacularAngioReportImpl> d_ptr;
		MacularAngioReportImpl& impl(void) const;
	};
}

