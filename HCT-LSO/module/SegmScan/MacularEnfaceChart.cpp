#include "pch.h"
#include "MacularEnfaceChart.h"
#include "MacularEnfaceMap.h"
#include "OcularBsegm.h"

using namespace segm_scan;


struct MacularEnfaceChart::MacularEnfaceChartImpl
{
	MacularEnfaceChartImpl() 
	{
	}
};


MacularEnfaceChart::MacularEnfaceChart() :
	d_ptr(make_unique<MacularEnfaceChartImpl>())
{
}


segm_scan::MacularEnfaceChart::~MacularEnfaceChart() = default;
segm_scan::MacularEnfaceChart::MacularEnfaceChart(MacularEnfaceChart && rhs) = default;
MacularEnfaceChart & segm_scan::MacularEnfaceChart::operator=(MacularEnfaceChart && rhs) = default;


segm_scan::MacularEnfaceChart::MacularEnfaceChart(const MacularEnfaceChart & rhs)
	: d_ptr(make_unique<MacularEnfaceChartImpl>(*rhs.d_ptr))
{
}


MacularEnfaceChart & segm_scan::MacularEnfaceChart::operator=(const MacularEnfaceChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


MacularEnfaceChart::MacularEnfaceChartImpl & segm_scan::MacularEnfaceChart::getImpl(void) const
{
	return *d_ptr;
}
