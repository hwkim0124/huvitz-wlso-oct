#include "pch.h"
#include "DiscEnfaceImage.h"
#include "OcularBsegm.h"

using namespace segm_scan;


struct DiscEnfaceImage::DiscEnfaceImageImpl
{
	DiscEnfaceImageImpl()
	{
	}
};


DiscEnfaceImage::DiscEnfaceImage() :
	d_ptr(make_unique<DiscEnfaceImageImpl>())
{
}


segm_scan::DiscEnfaceImage::~DiscEnfaceImage() = default;
segm_scan::DiscEnfaceImage::DiscEnfaceImage(DiscEnfaceImage && rhs) = default;
DiscEnfaceImage & segm_scan::DiscEnfaceImage::operator=(DiscEnfaceImage && rhs) = default;


segm_scan::DiscEnfaceImage::DiscEnfaceImage(const DiscEnfaceImage & rhs)
	: d_ptr(make_unique<DiscEnfaceImageImpl>(*rhs.d_ptr))
{
}


DiscEnfaceImage & segm_scan::DiscEnfaceImage::operator=(const DiscEnfaceImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


DiscEnfaceImage::DiscEnfaceImageImpl & segm_scan::DiscEnfaceImage::getImpl(void) const
{
	return *d_ptr;
}
