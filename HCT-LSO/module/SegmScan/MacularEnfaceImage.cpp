#include "pch.h"
#include "MacularEnfaceImage.h"
#include "OcularBsegm.h"

using namespace segm_scan;


struct MacularEnfaceImage::MacularEnfaceImageImpl
{
	MacularEnfaceImageImpl()
	{
	}
};


MacularEnfaceImage::MacularEnfaceImage() :
	d_ptr(make_unique<MacularEnfaceImageImpl>())
{
}


segm_scan::MacularEnfaceImage::~MacularEnfaceImage() = default;
segm_scan::MacularEnfaceImage::MacularEnfaceImage(MacularEnfaceImage && rhs) = default;
MacularEnfaceImage & segm_scan::MacularEnfaceImage::operator=(MacularEnfaceImage && rhs) = default;


segm_scan::MacularEnfaceImage::MacularEnfaceImage(const MacularEnfaceImage & rhs)
	: d_ptr(make_unique<MacularEnfaceImageImpl>(*rhs.d_ptr))
{
}


MacularEnfaceImage & segm_scan::MacularEnfaceImage::operator=(const MacularEnfaceImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


MacularEnfaceImage::MacularEnfaceImageImpl & segm_scan::MacularEnfaceImage::getImpl(void) const
{
	return *d_ptr;
}
