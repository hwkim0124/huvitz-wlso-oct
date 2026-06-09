#include "pch.h"
#include "MacularETDRSChart.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct MacularETDRSChart::MacularETDRSChartImpl
{
	ret_param::SectorChart chart;

	MacularETDRSChartImpl()
	{
	}
};


MacularETDRSChart::MacularETDRSChart() :
	d_ptr(make_unique<MacularETDRSChartImpl>())
{
	setEnfaceChart(&getImpl().chart);
	getImpl().chart.setCorneaThickness(false);
	setSectionSize();
}


segm_scan::MacularETDRSChart::~MacularETDRSChart() = default;
segm_scan::MacularETDRSChart::MacularETDRSChart(MacularETDRSChart && rhs) = default;
MacularETDRSChart & segm_scan::MacularETDRSChart::operator=(MacularETDRSChart && rhs) = default;


segm_scan::MacularETDRSChart::MacularETDRSChart(const MacularETDRSChart & rhs)
	: d_ptr(make_unique<MacularETDRSChartImpl>(*rhs.d_ptr))
{
}


MacularETDRSChart & segm_scan::MacularETDRSChart::operator=(const MacularETDRSChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::MacularETDRSChart::setSectionSize(float center, float inner, float outer)
{
	d_ptr->chart.setCenterDiameter(center);
	d_ptr->chart.setInnerDiameter(inner);
	d_ptr->chart.setOuterDiameter(outer);
	return;
}


float segm_scan::MacularETDRSChart::centerThickness(void) const
{
	return d_ptr->chart.meanCenter();
}


float segm_scan::MacularETDRSChart::innerThickness(int section) const
{
	int index = section;
	if (eyeSide() == EyeSide::OS) {
		index = (section == 1 ? 3 : index);
		index = (section == 3 ? 1 : index);
	}

	return d_ptr->chart.meanInnerSector(index);
}


float segm_scan::MacularETDRSChart::outerThickness(int section) const
{
	int index = section;
	if (eyeSide() == EyeSide::OS) {
		index = (section == 1 ? 3 : index);
		index = (section == 3 ? 1 : index);
	}

	return d_ptr->chart.meanOuterSector(index);
}


float segm_scan::MacularETDRSChart::hexagonalThickness(int section) const
{
	int index = section;
	if (eyeSide() == EyeSide::OS) {
		index = (section == 1 ? 5 : index);
		index = (section == 2 ? 4 : index);
		index = (section == 4 ? 2 : index);
		index = (section == 5 ? 1 : index);
	}

	return d_ptr->chart.meanHexagonalSector(index);
}


float segm_scan::MacularETDRSChart::centerThicknessPercentile(void) const
{
	float value = centerThickness();

	if (value <= 210.0f) {
		return CHART_PERCENTILE_1;
	}
	else if (value <= 220.0f) {
		return CHART_PERCENTILE_5;
	}
	else if (value <= 300.0f) {
		return CHART_PERCENTILE_95;
	}
	else if (value <= 315.0f) {
		return CHART_PERCENTILE_99;
	}
	else {
		return CHART_PERCENTILE_100;
	}
	return 0.0f;
}


float segm_scan::MacularETDRSChart::innerThicknessPercentile(int section) const
{
	float value = innerThickness(section);

	switch (section) {
	case 0:
		if (value <= 270.0f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 285.0f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 300.0f) {
			return CHART_PERCENTILE_95;
		}
		if (value <= 350.0f) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;

	case 1:
		if (value <= (eyeSide() == EyeSide::OD ? 260.0f : 265.0f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 280.0f : 285.0f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 300.0f : 305.0f)) {
			return CHART_PERCENTILE_95;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 345.0f : 350.0f)) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;

	case 2:
		if (value <= 275.0f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 290.0f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 310.0f) {
			return CHART_PERCENTILE_95;
		}
		if (value <= 360.0f) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;

	case 3:
		if (value <= (eyeSide() == EyeSide::OD ? 265.0f : 260.0f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 285.0f : 280.0f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 305.0f : 300.0f)) {
			return CHART_PERCENTILE_95;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 350.0f : 345.0f)) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;
	}

	return 0.0f;
}


float segm_scan::MacularETDRSChart::outerThicknessPercentile(int section) const
{
	float value = outerThickness(section);

	switch (section) {
	case 0:
		if (value <= 250.0f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 270.0f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 290.0f) {
			return CHART_PERCENTILE_95;
		}
		if (value <= 300.0f) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;

	case 1:
		if (value <= (eyeSide() == EyeSide::OD ? 230.0f : 240.0f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 245.0f : 255.0f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 300.0f : 315.0f)) {
			return CHART_PERCENTILE_95;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 315.0f : 330.0f)) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;

	case 2:
		if (value <= 235.0f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 245.0f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 295.0f) {
			return CHART_PERCENTILE_95;
		}
		if (value <= 305.0f) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;

	case 3:
		if (value <= (eyeSide() == EyeSide::OD ? 240.0f : 230.0f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 255.0f : 245.0f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 315.0f : 300.0f)) {
			return CHART_PERCENTILE_95;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 330.0f : 315.0f)) {
			return CHART_PERCENTILE_99;
		}
		return CHART_PERCENTILE_100;
	}

	return 0.0f;
}


float segm_scan::MacularETDRSChart::hexagonalThicknessPercentile(int section) const
{
	float value = hexagonalThickness(section);
	float range = 0.0f;

	switch (section) {
	case 0:
		if (value <= 70.0f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 75.0f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 90.0f) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;

	case 1:
	case 2:
		if (value <= (eyeSide() == EyeSide::OD ? 70.0f : 75.0f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 75.0f : 80.0f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 90.0f : 95.0f)) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;

	case 3:
		if (value <= 70.0f) {
			return CHART_PERCENTILE_1;
		}
		if (value <= 75.0f) {
			return CHART_PERCENTILE_5;
		}
		if (value <= 95.0f) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;

	case 4:
	case 5:
		if (value <= (eyeSide() == EyeSide::OD ? 75.0f : 70.0f)) {
			return CHART_PERCENTILE_1;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 80.0f : 75.0f)) {
			return CHART_PERCENTILE_5;
		}
		if (value <= (eyeSide() == EyeSide::OD ? 95.0f : 90.0f)) {
			return CHART_PERCENTILE_95;
		}
		return CHART_PERCENTILE_100;
	}

	return 0.0f;
}


float segm_scan::MacularETDRSChart::averageThickness(void) const
{
	return d_ptr->chart.meanTotal();
}


float segm_scan::MacularETDRSChart::superiorThickness(void) const
{
	return d_ptr->chart.meanSuperior();
}


float segm_scan::MacularETDRSChart::inferiorThickness(void) const
{
	return d_ptr->chart.meanInferior();
}

bool segm_scan::MacularETDRSChart::fetchETDRSChartDescript(OctETDRSChartDescript& desc) const
{
	desc.centerThick = centerThickness();
	desc.centerPercent = centerThicknessPercentile();

	for (int i = 0; i < OCT_ETDRS_CHART_SIDE_SIZE; i++) {
		desc.innerThicks[i] = innerThickness(i);
		desc.outerThicks[i] = outerThickness(i);
		desc.innerPercents[i] = innerThicknessPercentile(i);
		desc.outerPercents[i] = outerThicknessPercentile(i);
	}

	desc.superiorThick = superiorThickness();
	desc.inferiorThick = inferiorThickness();
	desc.averageThick = averageThickness();

	desc.eyeSide = eyeSide();
	desc.centerX = getImpl().chart.positionX();
	desc.centerY = getImpl().chart.positionY();

	LogD() << "ETDRS chart descript fetched";
	LogD() << "Center: " << desc.centerThick << " (" << desc.centerPercent << "%)";
	LogD() << "Inner: " << desc.innerThicks[0] << ", (" << desc.innerPercents[0] << "%), " << desc.innerThicks[1] << ", (" << desc.innerPercents[1] << "%), " << desc.innerThicks[2] << ", (" << desc.innerPercents[2] << "%), " << desc.innerThicks[3] << ", (" << desc.innerPercents[3] << "%)";
	LogD() << "Outer: " << desc.outerThicks[0] << ", (" << desc.outerPercents[0] << "%), " << desc.outerThicks[1] << ", (" << desc.outerPercents[1] << "%), " << desc.outerThicks[2] << ", (" << desc.outerPercents[2] << "%), " << desc.outerThicks[3] << ", (" << desc.outerPercents[3] << "%)";
	LogD() << "Superior: " << desc.superiorThick ;
	LogD() << "Inferior: " << desc.inferiorThick ;
	LogD() << "Average: " << desc.averageThick ;
	LogD() << "Eye side: " << (desc.eyeSide == EyeSide::OD ? "OD" : "OS");
	LogD() << "Location: " << desc.centerX << ", " << desc.centerY;
	return true;
}


MacularETDRSChart::MacularETDRSChartImpl & segm_scan::MacularETDRSChart::getImpl(void) const
{
	return *d_ptr;
}
