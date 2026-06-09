#include "pch.h"
#include "DiscEnfaceChart.h"
#include "OcularBsegm.h"

using namespace segm_scan;


struct DiscEnfaceChart::DiscEnfaceChartImpl
{
	DiscEnfaceChartImpl()
	{
	}
};


DiscEnfaceChart::DiscEnfaceChart() :
	d_ptr(make_unique<DiscEnfaceChartImpl>())
{
}


segm_scan::DiscEnfaceChart::~DiscEnfaceChart() = default;
segm_scan::DiscEnfaceChart::DiscEnfaceChart(DiscEnfaceChart && rhs) = default;
DiscEnfaceChart & segm_scan::DiscEnfaceChart::operator=(DiscEnfaceChart && rhs) = default;


segm_scan::DiscEnfaceChart::DiscEnfaceChart(const DiscEnfaceChart & rhs)
	: d_ptr(make_unique<DiscEnfaceChartImpl>(*rhs.d_ptr))
{
}


DiscEnfaceChart & segm_scan::DiscEnfaceChart::operator=(const DiscEnfaceChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


DiscEnfaceChart::DiscEnfaceChartImpl & segm_scan::DiscEnfaceChart::getImpl(void) const
{
	return *d_ptr;
}
