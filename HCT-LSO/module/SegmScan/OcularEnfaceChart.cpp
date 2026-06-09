#include "pch.h"
#include "OcularEnfaceChart.h"
#include "OcularEnfaceMap.h"
#include "OcularBsegm.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct OcularEnfaceChart::OcularEnfaceChartImpl
{
	EyeSide side;
	ret_param::EnfaceChart* chart;
	ret_param::EnfaceTmap* tmap;

	OcularEnfaceChartImpl() : side(EyeSide::OD), tmap(nullptr), chart(nullptr)
	{
	}
};


OcularEnfaceChart::OcularEnfaceChart() :
	d_ptr(make_unique<OcularEnfaceChartImpl>())
{
}


segm_scan::OcularEnfaceChart::~OcularEnfaceChart() = default;
segm_scan::OcularEnfaceChart::OcularEnfaceChart(OcularEnfaceChart && rhs) = default;
OcularEnfaceChart & segm_scan::OcularEnfaceChart::operator=(OcularEnfaceChart && rhs) = default;


segm_scan::OcularEnfaceChart::OcularEnfaceChart(const OcularEnfaceChart & rhs)
	: d_ptr(make_unique<OcularEnfaceChartImpl>(*rhs.d_ptr))
{
}


OcularEnfaceChart & segm_scan::OcularEnfaceChart::operator=(const OcularEnfaceChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


EyeSide segm_scan::OcularEnfaceChart::eyeSide(void) const
{
	return d_ptr->side;
}


ret_param::EnfaceChart * segm_scan::OcularEnfaceChart::enfaceChart(void) const
{
	return getImpl().chart;
}


void segm_scan::OcularEnfaceChart::setEyeSide(EyeSide side)
{
	d_ptr->side = side;
	enfaceChart()->setEyeSide(eyeSide());
	return;
}


void segm_scan::OcularEnfaceChart::setEnfaceChart(ret_param::EnfaceChart * chart)
{
	getImpl().chart = chart;
	return;
}


void segm_scan::OcularEnfaceChart::setEnfaceData(ret_param::EnfaceTmap * tmap)
{
	d_ptr->tmap = tmap;
	return;
}


void segm_scan::OcularEnfaceChart::setLocation(float xInMM, float yInMM)
{
	if (enfaceChart()) {
		enfaceChart()->setChartPosition(xInMM, yInMM);
	}
	return;
}


bool segm_scan::OcularEnfaceChart::updateMetrics(void)
{
	if (enfaceData() && enfaceChart()) {
		enfaceChart()->setEnfaceData(enfaceData());
		return enfaceChart()->updateContent();
	}
	return false;
}


bool segm_scan::OcularEnfaceChart::updateMetrics(const std::vector<float>& thicks)
{
	if (enfaceChart()) {
		return enfaceChart()->updateContent(thicks);
	}
	return false;
}


bool segm_scan::OcularEnfaceChart::isEmpty(void) const
{
	// return (!enfaceData() || enfaceData()->isEmpty() || !enfaceChart() || enfaceChart()->isEmpty
	return (!enfaceChart() || enfaceChart()->isEmpty());
}


ret_param::EnfaceTmap* segm_scan::OcularEnfaceChart::enfaceData(void) const
{
	return d_ptr->tmap;
}


OcularEnfaceChart::OcularEnfaceChartImpl & segm_scan::OcularEnfaceChart::getImpl(void) const
{
	return *d_ptr;
}
