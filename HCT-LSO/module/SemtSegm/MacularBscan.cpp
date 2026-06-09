#include "pch.h"
#include "MacularBscan.h"

using namespace semt_segm;
using namespace std;


struct MacularBscan::MacularBscanImpl
{
	MacularBscanImpl()
	{
	}
};


MacularBscan::MacularBscan() :
	d_ptr(make_unique<MacularBscanImpl>())
{
}


semt_segm::MacularBscan::~MacularBscan() = default;
semt_segm::MacularBscan::MacularBscan(MacularBscan && rhs) = default;
MacularBscan & semt_segm::MacularBscan::operator=(MacularBscan && rhs) = default;


MacularBscan::MacularBscanImpl & semt_segm::MacularBscan::getImpl(void) const
{
	return *d_ptr;
}

