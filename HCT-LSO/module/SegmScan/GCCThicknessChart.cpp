#include "pch.h"
#include "GCCThicknessChart.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct GCCThicknessChart::GCCThicknessChartImpl
{
	ret_param::DonutChart chart;

	GCCThicknessChartImpl() 
	{
	}
};


GCCThicknessChart::GCCThicknessChart() :
	d_ptr(make_unique<GCCThicknessChartImpl>())
{
	setEnfaceChart(&getImpl().chart);
}


segm_scan::GCCThicknessChart::~GCCThicknessChart() = default;
segm_scan::GCCThicknessChart::GCCThicknessChart(GCCThicknessChart && rhs) = default;
GCCThicknessChart & segm_scan::GCCThicknessChart::operator=(GCCThicknessChart && rhs) = default;


segm_scan::GCCThicknessChart::GCCThicknessChart(const GCCThicknessChart & rhs)
	: d_ptr(make_unique<GCCThicknessChartImpl>(*rhs.d_ptr))
{
}


GCCThicknessChart & segm_scan::GCCThicknessChart::operator=(const GCCThicknessChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::GCCThicknessChart::setSectionSize(float inner, float outer)
{
	d_ptr->chart.setInnerDiameter(inner);
	d_ptr->chart.setOuterDiameter(outer);
	return;
}


float segm_scan::GCCThicknessChart::sectionThickness(int section) const
{
	int index = section;
	if (eyeSide() == EyeSide::OS) {
		index = (section == 1 ? 5 : index);
		index = (section == 2 ? 4 : index);
		index = (section == 4 ? 2 : index);
		index = (section == 5 ? 1 : index);
	}
	auto value = d_ptr->chart.meanSection(index);
	return value;
}


float segm_scan::GCCThicknessChart::averageThickness(void) const
{
	auto value = d_ptr->chart.meanTotal();
	return value;
}


float segm_scan::GCCThicknessChart::superiorThickness(void) const
{
	auto value = d_ptr->chart.meanSuperior();
	return value;
}


float segm_scan::GCCThicknessChart::inferiorThickness(void) const
{
	auto value = d_ptr->chart.meanInferior();
	return value;
}


bool segm_scan::GCCThicknessChart::fetchGCCThickChartDescript(OctGCCThickChartDescript& desc) const
{
	for (int i = 0; i < OCT_GCC_CHART_SECTION_SIZE; i++) {
		desc.sectionThicks[i] = sectionThickness(i);
	}

	desc.superiorThick = superiorThickness();
	desc.inferiorThick = inferiorThickness();
	desc.averageThick = averageThickness();

	desc.innerDiam = d_ptr->chart.getInnerDiameter();
	desc.outerDiam = d_ptr->chart.getOuterDiameter();

	desc.eyeSide = eyeSide();
	desc.centerX = getImpl().chart.positionX();
	desc.centerY = getImpl().chart.positionY();

	LogD() << "GCC Thickness chart descript fetched";
	LogD() << "Sections mean: " << desc.sectionThicks[0] << ", " << desc.sectionThicks[1] << ", " << desc.sectionThicks[2] << ", " << desc.sectionThicks[3] << ", " << desc.sectionThicks[4] << ", " << desc.sectionThicks[5];	
	LogD() << "Superior mean: " << desc.superiorThick;
	LogD() << "Inferior mean: " << desc.inferiorThick;
	LogD() << "Total mean: " << desc.averageThick;
	LogD() << "Section range: " << desc.innerDiam << " - " << desc.outerDiam;	
	LogD() << "Eye side: " << (desc.eyeSide == EyeSide::OD ? "OD" : "OS");
	LogD() << "Location: " << desc.centerX << ", " << desc.centerY;
	return true;
}


GCCThicknessChart::GCCThicknessChartImpl & segm_scan::GCCThicknessChart::getImpl(void) const
{
	return *d_ptr;
}
