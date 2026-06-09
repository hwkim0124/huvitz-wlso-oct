#include "pch.h"
#include "CorneaThicknessMap.h"

using namespace segm_scan;


struct CorneaThicknessMap::CorneaThicknessMapImpl
{
	CorneaThicknessMapImpl()
	{
	}
};



CorneaThicknessMap::CorneaThicknessMap() :
	d_ptr(make_unique<CorneaThicknessMapImpl>())
{
}


segm_scan::CorneaThicknessMap::~CorneaThicknessMap() = default;
segm_scan::CorneaThicknessMap::CorneaThicknessMap(CorneaThicknessMap && rhs) = default;
CorneaThicknessMap & segm_scan::CorneaThicknessMap::operator=(CorneaThicknessMap && rhs) = default;


segm_scan::CorneaThicknessMap::CorneaThicknessMap(const CorneaThicknessMap & rhs)
	: d_ptr(make_unique<CorneaThicknessMapImpl>(*rhs.d_ptr))
{
}


CorneaThicknessMap & segm_scan::CorneaThicknessMap::operator=(const CorneaThicknessMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


CorneaThicknessMap::CorneaThicknessMapImpl & segm_scan::CorneaThicknessMap::getImpl(void) const
{
	return *d_ptr;
}
