#include "pch.h"
#include "CorneaETDRSChart.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct CorneaETDRSChart::CorneaETDRSChartImpl
{
	ret_param::SectorChart chart;

	CorneaETDRSChartImpl()
	{
	}
};


CorneaETDRSChart::CorneaETDRSChart() :
	d_ptr(make_unique<CorneaETDRSChartImpl>())
{
	setEnfaceChart(&getImpl().chart);
	getImpl().chart.setCorneaThickness(true);
	setSectionSize();
}


segm_scan::CorneaETDRSChart::~CorneaETDRSChart() = default;
segm_scan::CorneaETDRSChart::CorneaETDRSChart(CorneaETDRSChart && rhs) = default;
CorneaETDRSChart & segm_scan::CorneaETDRSChart::operator=(CorneaETDRSChart && rhs) = default;


segm_scan::CorneaETDRSChart::CorneaETDRSChart(const CorneaETDRSChart & rhs)
	: d_ptr(make_unique<CorneaETDRSChartImpl>(*rhs.d_ptr))
{

}


CorneaETDRSChart & segm_scan::CorneaETDRSChart::operator=(const CorneaETDRSChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::CorneaETDRSChart::setSectionSize(float center, float inner, float outer)
{
	d_ptr->chart.setCenterDiameter(center);
	d_ptr->chart.setInnerDiameter(inner);
	d_ptr->chart.setOuterDiameter(outer);
	return;
}


float segm_scan::CorneaETDRSChart::centerThickness(void) const
{
	return d_ptr->chart.meanCenter();
}


float segm_scan::CorneaETDRSChart::innerThickness(int section) const
{
	int index = section;
	if (eyeSide() == EyeSide::OS) {
		index = (section == 1 ? 7 : index);
		index = (section == 2 ? 6 : index);
		index = (section == 3 ? 5 : index);
	}

	return d_ptr->chart.meanInnerSector(section);
}


float segm_scan::CorneaETDRSChart::outerThickness(int section) const
{
	int index = section;
	if (eyeSide() == EyeSide::OS) {
		index = (section == 1 ? 7 : index);
		index = (section == 2 ? 6 : index);
		index = (section == 3 ? 5 : index);
	}

	return d_ptr->chart.meanOuterSector(section);
}


float segm_scan::CorneaETDRSChart::averageThickness(void) const
{
	return d_ptr->chart.meanTotal();
}


float segm_scan::CorneaETDRSChart::superiorThickness(void) const
{
	return d_ptr->chart.meanSuperior();
}


float segm_scan::CorneaETDRSChart::inferiorThickness(void) const
{
	return d_ptr->chart.meanInferior();
}


CorneaETDRSChart::CorneaETDRSChartImpl & segm_scan::CorneaETDRSChart::getImpl(void) const
{
	return *d_ptr;
}
