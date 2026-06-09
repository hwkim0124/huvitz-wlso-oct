#pragma once

#include "SegmScan.h"
#include "DiscEnfaceChart.h"

#include <memory>
#include <vector>


namespace segm_scan
{
	class DiscEplot;

	class SEGMSCAN_DLL_API DiscClockChart : public DiscEnfaceChart
	{
	public:
		DiscClockChart();
		virtual ~DiscClockChart();

		DiscClockChart(DiscClockChart&& rhs);
		DiscClockChart& operator=(DiscClockChart&& rhs);
		DiscClockChart(const DiscClockChart& rhs);
		DiscClockChart& operator=(const DiscClockChart& rhs);

	public:
		void setCircleSize(float diam = ONH_DISC_RNFL_345_SIZE);

		float clockThickness(int section) const;
		float quadrantThickness(int section) const;
		float clockThicknessPercentile(int section) const;
		float quadrantThicknessPercentile(int section) const;

		std::vector<float> getTSNITGraph(int size, int filter = 21) const;
		std::vector<float> getNormalLine(int size) const;
		std::vector<float> getBorderLine(int size) const;
		std::vector<float> getOutsideLine(int size) const;

		bool fetchClockChartDescript(OctClockChartDescript& desc) const;

	private:
		struct DiscClockChartImpl;
		std::unique_ptr<DiscClockChartImpl> d_ptr;
		DiscClockChartImpl& getImpl(void) const;
	};
}
