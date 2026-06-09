#include "pch.h"
#include "OcularEnfaceImage.h"
#include "OcularBsegm.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct OcularEnfaceImage::OcularEnfaceImageImpl
{
	ret_param::EnfaceShot enfaceShot;

	OcularEnfaceImageImpl()
	{
	}
};


OcularEnfaceImage::OcularEnfaceImage() :
	d_ptr(make_unique<OcularEnfaceImageImpl>())
{
}


segm_scan::OcularEnfaceImage::~OcularEnfaceImage() = default;
segm_scan::OcularEnfaceImage::OcularEnfaceImage(OcularEnfaceImage && rhs) = default;
OcularEnfaceImage & segm_scan::OcularEnfaceImage::operator=(OcularEnfaceImage && rhs) = default;


segm_scan::OcularEnfaceImage::OcularEnfaceImage(const OcularEnfaceImage & rhs)
	: d_ptr(make_unique<OcularEnfaceImageImpl>(*rhs.d_ptr))
{
}


OcularEnfaceImage & segm_scan::OcularEnfaceImage::operator=(const OcularEnfaceImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool segm_scan::OcularEnfaceImage::setupEnfaceImage(const OctScanPattern & pattern, OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset, const std::vector<OcularBsegm*>& bsegms)
{
	auto lines = vector<vector<uint8_t>>();
	auto flags = vector<bool>();
	bool output;

	for (const auto& b : bsegms) {
		if (b != nullptr) {
			auto pixels = vector<uint8_t>();
			if (pattern.isAngioPattern() || true)
			{
				output = b->getLateralLine(upper, lower, pixels, upperOffset, lowerOffset);
			}
			else {
				output = b->getMaxValueLine(upper, lower, pixels);
			}
			lines.push_back(std::move(pixels));
			flags.push_back(output);
		}
		else {
			return false;
		}
	}

	bool result = getImpl().enfaceShot.setupData(pattern, upper, lower, upperOffset, lowerOffset, lines, flags);
	return result;
}


bool segm_scan::OcularEnfaceImage::isIdentical(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset) const
{
	return getImpl().enfaceShot.isIdentical(upper, lower, upperOffset, lowerOffset);
}


bool segm_scan::OcularEnfaceImage::isEmpty(void) const
{
	return getImpl().enfaceShot.isEmpty();
}


bool segm_scan::OcularEnfaceImage::fetchEnfaceImageDescript(OctEnfaceImageDescript& desc) const
{
	if (!isEmpty()) {
		if (auto image = getImpl().enfaceShot.getDataImage(); image) {
			auto p = image->getBitsData();
			auto w = image->getWidth();
			auto h = image->getHeight();
			desc.setup(p, w, h);
			return true;
		}
	}
	return false;
}


cpp_util::CvImage segm_scan::OcularEnfaceImage::buildImage(int width, int height) const
{
	auto image = cpp_util::CvImage();
	getImpl().enfaceShot.copyToImage(&image, width, height);
	return image;
}



OcularEnfaceImage::OcularEnfaceImageImpl & segm_scan::OcularEnfaceImage::getImpl(void) const
{
	return *d_ptr;
}
