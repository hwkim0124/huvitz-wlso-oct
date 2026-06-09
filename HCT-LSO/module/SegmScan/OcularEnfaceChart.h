#pragma once

#include "SegmScan.h"

#include <memory>
#include <vector>

namespace ret_param {
	class EnfaceChart;
	class EnfaceTmap;
}


namespace segm_scan
{
	class OcularEnfaceImage;
	class OcularEnfaceMap;

	class SEGMSCAN_DLL_API OcularEnfaceChart
	{
	public:
		OcularEnfaceChart();
		virtual ~OcularEnfaceChart();

		OcularEnfaceChart(OcularEnfaceChart&& rhs);
		OcularEnfaceChart& operator=(OcularEnfaceChart&& rhs);
		OcularEnfaceChart(const OcularEnfaceChart& rhs);
		OcularEnfaceChart& operator=(const OcularEnfaceChart& rhs);

	public:
		EyeSide eyeSide(void) const;
		ret_param::EnfaceChart* enfaceChart(void) const;
		ret_param::EnfaceTmap* enfaceData(void) const;

		void setEyeSide(EyeSide side);
		void setEnfaceChart(ret_param::EnfaceChart* chart);
		void setEnfaceData(ret_param::EnfaceTmap* tmap);
		void setLocation(float xInMM, float yInMM);

		virtual bool updateMetrics(void);
		virtual bool updateMetrics(const std::vector<float>& thicks);
		bool isEmpty(void) const;

	private:
		struct OcularEnfaceChartImpl;
		std::unique_ptr<OcularEnfaceChartImpl> d_ptr;
		OcularEnfaceChartImpl& getImpl(void) const;
	};
}

