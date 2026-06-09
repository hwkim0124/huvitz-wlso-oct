#include "pch.h"
#include "MacularThicknessMap.h"

using namespace segm_scan;


struct MacularThicknessMap::MacularThicknessMapImpl
{
	MacularThicknessMapImpl()
	{
	}
};


MacularThicknessMap::MacularThicknessMap() :
	d_ptr(make_unique<MacularThicknessMapImpl>())
{
}


segm_scan::MacularThicknessMap::~MacularThicknessMap() = default;
segm_scan::MacularThicknessMap::MacularThicknessMap(MacularThicknessMap && rhs) = default;
MacularThicknessMap & segm_scan::MacularThicknessMap::operator=(MacularThicknessMap && rhs) = default;


segm_scan::MacularThicknessMap::MacularThicknessMap(const MacularThicknessMap & rhs)
	: d_ptr(make_unique<MacularThicknessMapImpl>(*rhs.d_ptr))
{
}


MacularThicknessMap & segm_scan::MacularThicknessMap::operator=(const MacularThicknessMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


MacularThicknessMap::MacularThicknessMapImpl & segm_scan::MacularThicknessMap::getImpl(void) const
{
	return *d_ptr;
}
