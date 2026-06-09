#include "pch.h"
#include "DiscClockChart.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct DiscClockChart::DiscClockChartImpl
{
	ret_param::ClockChart chart;

	DiscClockChartImpl()
	{
	}
};


DiscClockChart::DiscClockChart() :
	d_ptr(make_unique<DiscClockChartImpl>())
{
	setEnfaceChart(&getImpl().chart);
}


segm_scan::DiscClockChart::~DiscClockChart() = default;
segm_scan::DiscClockChart::DiscClockChart(DiscClockChart && rhs) = default;
DiscClockChart & segm_scan::DiscClockChart::operator=(DiscClockChart && rhs) = default;


segm_scan::DiscClockChart::DiscClockChart(const DiscClockChart & rhs)
	: d_ptr(make_unique<DiscClockChartImpl>(*rhs.d_ptr))
{

}


DiscClockChart & segm_scan::DiscClockChart::operator=(const DiscClockChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::DiscClockChart::setCircleSize(float diam)
{
	d_ptr->chart.setOuterDiameter(diam);
	return;
}


float segm_scan::DiscClockChart::clockThickness(int section) const
{
	return d_ptr->chart.meanClock(section);
}


float segm_scan::DiscClockChart::quadrantThickness(int section) const
{
	return d_ptr->chart.meanQuadrant(section);
}


float segm_scan::DiscClockChart::clockThicknessPercentile(int section) const
{
	float value = clockThickness(section);

	switch (section) {
	case 11:
	case 0:
	case 1:
		if (value <= 66.78f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 83.41f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 166.85f) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	case 2:
	case 3:
	case 4:
		if (value <= (eyeSide() == EyeSide::OD ? 34.53f : 35.42f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 47.29f : 43.96f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 115.21f : 93.98f)) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	case 5:
	case 6:
	case 7:
		if (value <= 65.47f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 82.34f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 166.67f) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	case 8:
	case 9:
	case 10:
		if (value <= (eyeSide() == EyeSide::OD ? 35.42f : 34.53f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 43.96f : 47.29f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 93.98f : 115.21f)) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	}
	return 0.0f;
}


float segm_scan::DiscClockChart::quadrantThicknessPercentile(int section) const
{
	float value = quadrantThickness(section);

	switch (section) {
	case 0:
		if (value <= 66.78f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 83.41f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 166.85f) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	case 1:
		if (value <= (eyeSide() == EyeSide::OD ? 34.53f : 35.42f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 47.29f : 43.96f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 115.21f : 93.98f)) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	case 2:
		if (value <= 65.47f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 82.34f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 166.67f) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	case 3:
		if (value <= (eyeSide() == EyeSide::OD ? 35.42f : 34.53f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 43.96f : 47.29f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 93.98f : 115.21f)) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	}
	return 0.0f;
}


std::vector<float> segm_scan::DiscClockChart::getTSNITGraph(int size, int filter) const
{
	auto raws = d_ptr->chart.getGraphTSNIT(size);
	auto data = raws;
	auto dlen = (int) data.size();

	int half = filter / 2;
	int tcnt;
	float tsum;

	for (int i = 0; i < dlen; i++) {
		tsum = 0.0f; tcnt = 0;
		for (int j = i - half, k = 0; j <= i + half; j++) {
			if (j < 0) {
				k = dlen + j;
			}
			else if (j >= dlen) {
				k = j - dlen;
			}
			else {
				k = j;
			}
			tsum += raws[k];
			tcnt += 1;
		}

		if (tcnt > 0) {
			data[i] = tsum / tcnt;
		}
	}

	return data;
}


std::vector<float> segm_scan::DiscClockChart::getNormalLine(int size) const
{
	return norm_data::DiscTsnitGraph::getNormalLine(size);
}


std::vector<float> segm_scan::DiscClockChart::getBorderLine(int size) const
{
	return norm_data::DiscTsnitGraph::getBorderLine(size);
}


std::vector<float> segm_scan::DiscClockChart::getOutsideLine(int size) const
{
	return norm_data::DiscTsnitGraph::getOutsideLine(size);
}


bool segm_scan::DiscClockChart::fetchClockChartDescript(OctClockChartDescript& desc) const
{
	for (int i = 0; i < OCT_RNFL_CHART_SECTION_SIZE; i++) {
		desc.clockThicks[i] = clockThickness(i);
		desc.clockPercents[i] = clockThicknessPercentile(i);
	}

	for (int i = 0; i < OCT_RNFL_CHART_QUAD_SIZE; i++) {
		desc.quadThicks[i] = quadrantThickness(i);
		desc.quadPercents[i] = quadrantThicknessPercentile(i);
	}

	auto tsnit = getTSNITGraph(OCT_RNFL_TSNIT_GRAPH_SIZE);
	auto normal = getNormalLine(OCT_RNFL_TSNIT_GRAPH_SIZE);
	auto border = getBorderLine(OCT_RNFL_TSNIT_GRAPH_SIZE);
	auto outside = getOutsideLine(OCT_RNFL_TSNIT_GRAPH_SIZE);

	if (tsnit.size() == OCT_RNFL_TSNIT_GRAPH_SIZE) {
		for (int i = 0; i < OCT_RNFL_TSNIT_GRAPH_SIZE; i++) {
			desc.graphTSNIT[i] = tsnit[i];
			desc.graphNormal[i] = normal[i];
			desc.graphBorder[i] = border[i];
			desc.graphOutside[i] = outside[i];
		}
	}

	desc.eyeSide = getImpl().chart.eyeSide();
	desc.centerX = getImpl().chart.positionX();
	desc.centerY = getImpl().chart.positionY();

	LogD() << "Disc Clock chart descript fetched";
	LogD() << "Clock Thickness: " << desc.clockThicks[0] << " " << desc.clockThicks[1] << " " << desc.clockThicks[2] << " " << desc.clockThicks[3] << " " << desc.clockThicks[4] << " " << desc.clockThicks[5] << " " << desc.clockThicks[6] << " " << desc.clockThicks[7] << " " << desc.clockThicks[8] << " " << desc.clockThicks[9] << " " << desc.clockThicks[10] << " " << desc.clockThicks[11];
	LogD() << "Clock Percentile: " << desc.clockPercents[0] << " " << desc.clockPercents[1] << " " << desc.clockPercents[2] << " " << desc.clockPercents[3] << " " << desc.clockPercents[4] << " " << desc.clockPercents[5] << " " << desc.clockPercents[6] << " " << desc.clockPercents[7] << " " << desc.clockPercents[8] << " " << desc.clockPercents[9] << " " << desc.clockPercents[10] << " " << desc.clockPercents[11];
	LogD() << "Quadrant Thickness: " << desc.quadThicks[0] << " " << desc.quadThicks[1] << " " << desc.quadThicks[2] << " " << desc.quadThicks[3];
	LogD() << "Quadrant Percentile: " << desc.quadPercents[0] << " " << desc.quadPercents[1] << " " << desc.quadPercents[2] << " " << desc.quadPercents[3];
	LogD() << "TSNIT graph size: " << tsnit.size();
	LogD() << "Eye side: " << (desc.eyeSide == EyeSide::OD ? "OD" : "OS");
	LogD() << "Location: " << desc.centerX << " Y: " << desc.centerY;
	return true;
}


DiscClockChart::DiscClockChartImpl & segm_scan::DiscClockChart::getImpl(void) const
{
	return *d_ptr;
}
