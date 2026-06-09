#include "pch.h"
#include "CorneaRadiusMap.h"

using namespace segm_scan;


struct CorneaRadiusMap::CorneaRadiusMapImpl
{
	CorneaRadiusMapImpl()
	{
	}
};



CorneaRadiusMap::CorneaRadiusMap() :
	d_ptr(make_unique<CorneaRadiusMapImpl>())
{
}


segm_scan::CorneaRadiusMap::~CorneaRadiusMap() = default;
segm_scan::CorneaRadiusMap::CorneaRadiusMap(CorneaRadiusMap && rhs) = default;
CorneaRadiusMap & segm_scan::CorneaRadiusMap::operator=(CorneaRadiusMap && rhs) = default;


segm_scan::CorneaRadiusMap::CorneaRadiusMap(const CorneaRadiusMap & rhs)
	: d_ptr(make_unique<CorneaRadiusMapImpl>(*rhs.d_ptr))
{
}


CorneaRadiusMap & segm_scan::CorneaRadiusMap::operator=(const CorneaRadiusMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


CorneaRadiusMap::CorneaRadiusMapImpl & segm_scan::CorneaRadiusMap::getImpl(void) const
{
	return *d_ptr;
}
