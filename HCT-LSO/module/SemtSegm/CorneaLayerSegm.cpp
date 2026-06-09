#include "pch.h"
#include "CorneaLayerSegm.h"

using namespace semt_segm;
using namespace std;


struct CorneaLayerSegm::CorneaLayerSegmImpl
{
	CorneaLayerSegmImpl()
	{
	}
};


CorneaLayerSegm::CorneaLayerSegm(LayeredBscan* bscan) :
	d_ptr(make_unique<CorneaLayerSegmImpl>()), BscanSegmentator(bscan)
{
}


semt_segm::CorneaLayerSegm::~CorneaLayerSegm() = default;
semt_segm::CorneaLayerSegm::CorneaLayerSegm(CorneaLayerSegm && rhs) = default;
CorneaLayerSegm & semt_segm::CorneaLayerSegm::operator=(CorneaLayerSegm && rhs) = default;


CorneaLayerSegm::CorneaLayerSegmImpl & semt_segm::CorneaLayerSegm::getImpl(void) const
{
	return *d_ptr;
}

