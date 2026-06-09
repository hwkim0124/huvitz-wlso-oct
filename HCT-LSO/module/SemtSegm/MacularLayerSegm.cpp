#include "pch.h"
#include "MacularLayerSegm.h"

using namespace semt_segm;
using namespace std;


struct MacularLayerSegm::MacularLayerSegmImpl
{
	MacularLayerSegmImpl()
	{
	}
};


MacularLayerSegm::MacularLayerSegm(LayeredBscan* bscan) :
	d_ptr(make_unique<MacularLayerSegmImpl>()), BscanSegmentator(bscan)
{
}


semt_segm::MacularLayerSegm::~MacularLayerSegm() = default;
semt_segm::MacularLayerSegm::MacularLayerSegm(MacularLayerSegm && rhs) = default;
MacularLayerSegm & semt_segm::MacularLayerSegm::operator=(MacularLayerSegm && rhs) = default;


MacularLayerSegm::MacularLayerSegmImpl & semt_segm::MacularLayerSegm::getImpl(void) const
{
	return *d_ptr;
}

