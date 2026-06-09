#include "pch.h"
#include "DiscThicknessMap.h"

using namespace segm_scan;


struct DiscThicknessMap::DiscThicknessMapImpl
{
	DiscThicknessMapImpl()
	{
	}
};


DiscThicknessMap::DiscThicknessMap() :
	d_ptr(make_unique<DiscThicknessMapImpl>())
{
}


segm_scan::DiscThicknessMap::~DiscThicknessMap() = default;
segm_scan::DiscThicknessMap::DiscThicknessMap(DiscThicknessMap && rhs) = default;
DiscThicknessMap & segm_scan::DiscThicknessMap::operator=(DiscThicknessMap && rhs) = default;


segm_scan::DiscThicknessMap::DiscThicknessMap(const DiscThicknessMap & rhs)
	: d_ptr(make_unique<DiscThicknessMapImpl>(*rhs.d_ptr))
{
}


DiscThicknessMap & segm_scan::DiscThicknessMap::operator=(const DiscThicknessMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


DiscThicknessMap::DiscThicknessMapImpl & segm_scan::DiscThicknessMap::getImpl(void) const
{
	return *d_ptr;
}
