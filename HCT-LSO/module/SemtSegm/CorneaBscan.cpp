#include "pch.h"
#include "CorneaBscan.h"

using namespace semt_segm;
using namespace std;


struct CorneaBscan::CorneaBscanImpl
{
	CorneaBscanImpl()
	{
	}
};


CorneaBscan::CorneaBscan() :
	d_ptr(make_unique<CorneaBscanImpl>())
{
}


semt_segm::CorneaBscan::~CorneaBscan() = default;
semt_segm::CorneaBscan::CorneaBscan(CorneaBscan && rhs) = default;
CorneaBscan & semt_segm::CorneaBscan::operator=(CorneaBscan && rhs) = default;


CorneaBscan::CorneaBscanImpl & semt_segm::CorneaBscan::getImpl(void) const
{
	return *d_ptr;
}

