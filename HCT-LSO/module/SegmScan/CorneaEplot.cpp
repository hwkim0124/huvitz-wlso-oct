#include "pch.h"
#include "CorneaEplot.h"
#include "CorneaBsegm.h"

using namespace segm_scan;


struct CorneaEplot::CorneaEplotImpl
{
	CorneaEplotImpl()
	{
	}
};



CorneaEplot::CorneaEplot() :
	d_ptr(make_unique<CorneaEplotImpl>())
{
}


segm_scan::CorneaEplot::~CorneaEplot() = default;
segm_scan::CorneaEplot::CorneaEplot(CorneaEplot && rhs) = default;
CorneaEplot & segm_scan::CorneaEplot::operator=(CorneaEplot && rhs) = default;


segm_scan::CorneaEplot::CorneaEplot(const CorneaEplot & rhs)
	: d_ptr(make_unique<CorneaEplotImpl>(*rhs.d_ptr))
{
}


CorneaEplot & segm_scan::CorneaEplot::operator=(const CorneaEplot & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


CorneaEplot::CorneaEplotImpl & segm_scan::CorneaEplot::getImpl(void) const
{
	return *d_ptr;
}
