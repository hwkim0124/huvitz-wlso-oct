#include "pch.h"
#include "CorneaEnfaceImage.h"
#include "OcularBsegm.h"


using namespace segm_scan;


struct CorneaEnfaceImage::CorneaEnfaceImageImpl
{
	CorneaEnfaceImageImpl()
	{
	}
};


CorneaEnfaceImage::CorneaEnfaceImage() :
	d_ptr(make_unique<CorneaEnfaceImageImpl>())
{
}


segm_scan::CorneaEnfaceImage::~CorneaEnfaceImage() = default;
segm_scan::CorneaEnfaceImage::CorneaEnfaceImage(CorneaEnfaceImage && rhs) = default;
CorneaEnfaceImage & segm_scan::CorneaEnfaceImage::operator=(CorneaEnfaceImage && rhs) = default;


segm_scan::CorneaEnfaceImage::CorneaEnfaceImage(const CorneaEnfaceImage & rhs)
	: d_ptr(make_unique<CorneaEnfaceImageImpl>(*rhs.d_ptr))
{
}


CorneaEnfaceImage & segm_scan::CorneaEnfaceImage::operator=(const CorneaEnfaceImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


CorneaEnfaceImage::CorneaEnfaceImageImpl & segm_scan::CorneaEnfaceImage::getImpl(void) const
{
	return *d_ptr;
}
