#include "pch.h"
#include "MacularEnfaceMap.h"

using namespace segm_scan;


struct MacularEnfaceMap::MacularEnfaceMapImpl
{
	MacularEnfaceMapImpl()
	{
	}
};


MacularEnfaceMap::MacularEnfaceMap() :
	d_ptr(make_unique<MacularEnfaceMapImpl>())
{
}


segm_scan::MacularEnfaceMap::~MacularEnfaceMap() = default;
segm_scan::MacularEnfaceMap::MacularEnfaceMap(MacularEnfaceMap && rhs) = default;
MacularEnfaceMap & segm_scan::MacularEnfaceMap::operator=(MacularEnfaceMap && rhs) = default;


segm_scan::MacularEnfaceMap::MacularEnfaceMap(const MacularEnfaceMap & rhs)
	: d_ptr(make_unique<MacularEnfaceMapImpl>(*rhs.d_ptr))
{
}


MacularEnfaceMap & segm_scan::MacularEnfaceMap::operator=(const MacularEnfaceMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


MacularEnfaceMap::MacularEnfaceMapImpl & segm_scan::MacularEnfaceMap::getImpl(void) const
{
	return *d_ptr;
}
