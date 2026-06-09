#include "pch.h"
#include "DiscEnfaceMap.h"

using namespace segm_scan;


struct DiscEnfaceMap::DiscEnfaceMapImpl
{
	DiscEnfaceMapImpl()
	{
	}
};


DiscEnfaceMap::DiscEnfaceMap() :
	d_ptr(make_unique<DiscEnfaceMapImpl>())
{
}


segm_scan::DiscEnfaceMap::~DiscEnfaceMap() = default;
segm_scan::DiscEnfaceMap::DiscEnfaceMap(DiscEnfaceMap && rhs) = default;
DiscEnfaceMap & segm_scan::DiscEnfaceMap::operator=(DiscEnfaceMap && rhs) = default;


segm_scan::DiscEnfaceMap::DiscEnfaceMap(const DiscEnfaceMap & rhs)
	: d_ptr(make_unique<DiscEnfaceMapImpl>(*rhs.d_ptr))
{
}


DiscEnfaceMap & segm_scan::DiscEnfaceMap::operator=(const DiscEnfaceMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


DiscEnfaceMap::DiscEnfaceMapImpl & segm_scan::DiscEnfaceMap::getImpl(void) const
{
	return *d_ptr;
}
