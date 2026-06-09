#include "pch.h"
#include "CorneaEnfaceChart.h"
#include "MacularEnfaceMap.h"
#include "OcularBsegm.h"


using namespace segm_scan;


struct CorneaEnfaceChart::CorneaEnfaceChartImpl
{
	CorneaEnfaceChartImpl()
	{
	}
};


CorneaEnfaceChart::CorneaEnfaceChart() :
	d_ptr(make_unique<CorneaEnfaceChartImpl>())
{
}


segm_scan::CorneaEnfaceChart::~CorneaEnfaceChart() = default;
segm_scan::CorneaEnfaceChart::CorneaEnfaceChart(CorneaEnfaceChart && rhs) = default;
CorneaEnfaceChart & segm_scan::CorneaEnfaceChart::operator=(CorneaEnfaceChart && rhs) = default;


segm_scan::CorneaEnfaceChart::CorneaEnfaceChart(const CorneaEnfaceChart & rhs)
	: d_ptr(make_unique<CorneaEnfaceChartImpl>(*rhs.d_ptr))
{
}


CorneaEnfaceChart & segm_scan::CorneaEnfaceChart::operator=(const CorneaEnfaceChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


CorneaEnfaceChart::CorneaEnfaceChartImpl & segm_scan::CorneaEnfaceChart::getImpl(void) const
{
	return *d_ptr;
}
