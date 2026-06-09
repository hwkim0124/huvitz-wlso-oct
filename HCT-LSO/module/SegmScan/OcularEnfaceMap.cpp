#include "pch.h"
#include "OcularEnfaceMap.h"
#include "OcularBsegm.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct OcularEnfaceMap::OcularEnfaceMapImpl
{
	ret_param::EnfaceTmap enfaceTmap;

	OcularEnfaceMapImpl()
	{
	}
};


OcularEnfaceMap::OcularEnfaceMap() :
	d_ptr(make_unique<OcularEnfaceMapImpl>())
{
}


segm_scan::OcularEnfaceMap::~OcularEnfaceMap() = default;
segm_scan::OcularEnfaceMap::OcularEnfaceMap(OcularEnfaceMap && rhs) = default;
OcularEnfaceMap & segm_scan::OcularEnfaceMap::operator=(OcularEnfaceMap && rhs) = default;


segm_scan::OcularEnfaceMap::OcularEnfaceMap(const OcularEnfaceMap & rhs)
	: d_ptr(make_unique<OcularEnfaceMapImpl>(*rhs.d_ptr))
{
}


OcularEnfaceMap & segm_scan::OcularEnfaceMap::operator=(const OcularEnfaceMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool segm_scan::OcularEnfaceMap::setupThicknessMap(const OctScanPattern & pattern, OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset, const std::vector<OcularBsegm*>& bsegms)
{
	auto lines = vector<vector<float>>();
	auto flags = vector<bool>();
	auto thick = vector<float>();
	bool result;

	for (const auto& b : bsegms) {
		if (b != nullptr) {
			result = b->getThicknessLine(upper, lower, thick, 0, 0, upperOffset, lowerOffset);
			lines.push_back(std::move(thick));
			flags.push_back(result);
		}
		else {
			return false;
		}
	}

	result = getImpl().enfaceTmap.setupData(pattern, upper, lower, upperOffset, lowerOffset, lines, flags);
	return result;
}


bool segm_scan::OcularEnfaceMap::isIdentical(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset) const
{
	return getImpl().enfaceTmap.isIdentical(upper, lower, upperOffset, lowerOffset);
}


bool segm_scan::OcularEnfaceMap::isEmpty(void) const
{
	return getImpl().enfaceTmap.isEmpty();
}


bool segm_scan::OcularEnfaceMap::fetchThicknessMapDescript(OctThicknessMapDescript& desc) const
{
	if (!isEmpty()) {
		if (auto image = getImpl().enfaceTmap.getDataImage(); image) {
			auto p = image->getFloatData();
			auto w = image->getWidth();
			auto h = image->getHeight();
			desc.setup(p, w, h);
			return true;
		}
	}
	return false;
}


cpp_util::CvImage segm_scan::OcularEnfaceMap::buildImage(int width, int height) const
{
	auto image = cpp_util::CvImage();
	getImpl().enfaceTmap.copyToImage(&image, width, height);
	return image;
}


ret_param::EnfaceTmap * segm_scan::OcularEnfaceMap::getThicknessMap(void) const
{
	return &getImpl().enfaceTmap;
}


OcularEnfaceMap::OcularEnfaceMapImpl & segm_scan::OcularEnfaceMap::getImpl(void) const
{
	return *d_ptr;
}
