#include "pch.h"
#include "CorneaEnfaceMap.h"
#include "CorneaBsegm.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct CorneaEnfaceMap::CorneaEnfaceMapImpl
{
	float radiusCentHorz;
	float radiusCentVert;
	float averageCenter;

	CorneaEnfaceMapImpl() : radiusCentHorz(0.0f), radiusCentVert(0.0f), averageCenter(0.0f)
	{
	}
};


CorneaEnfaceMap::CorneaEnfaceMap() :
	d_ptr(make_unique<CorneaEnfaceMapImpl>())
{
}


segm_scan::CorneaEnfaceMap::~CorneaEnfaceMap() = default;
segm_scan::CorneaEnfaceMap::CorneaEnfaceMap(CorneaEnfaceMap && rhs) = default;
CorneaEnfaceMap & segm_scan::CorneaEnfaceMap::operator=(CorneaEnfaceMap && rhs) = default;


segm_scan::CorneaEnfaceMap::CorneaEnfaceMap(const CorneaEnfaceMap & rhs)
	: d_ptr(make_unique<CorneaEnfaceMapImpl>(*rhs.d_ptr))
{
}


CorneaEnfaceMap & segm_scan::CorneaEnfaceMap::operator=(const CorneaEnfaceMap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool segm_scan::CorneaEnfaceMap::setupThicknessMap(const OctScanPattern & pattern, OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset, const std::vector<OcularBsegm*>& bsegms)
{
	if (!OcularEnfaceMap::setupThicknessMap(pattern, upper, lower, upperOffset, lowerOffset, bsegms)) {
		return false;
	}

	getImpl().averageCenter = getThicknessMap()->getAverageOnCenter(3.0f);
	return true;
}


bool segm_scan::CorneaEnfaceMap::setupRadiusMap(const OctScanPattern & pattern, const std::vector<OcularBsegm*>& bsegms)
{
	auto lines = vector<vector<float>>();
	auto flags = vector<bool>();
	auto radius = vector<float>();
	bool result;

	for (const auto& b : bsegms) {
		if (b != nullptr) {
			result = ((CorneaBsegm*)b)->getCurvatureRadius(radius);
			lines.push_back(std::move(radius));
			flags.push_back(result);
		}
		else {
			return false;
		}
	}

	OcularLayerType upper = OcularLayerType::EPI;
	OcularLayerType lower = OcularLayerType::END;
	result = getThicknessMap()->setupData(pattern, upper, lower, 0.0f, 0.0f, lines, flags);
	if (result) {
		getImpl().radiusCentHorz = getThicknessMap()->getAverageOnHorzLine(3.0f);
		getImpl().radiusCentVert = getThicknessMap()->getAverageOnVertLine(3.0f);
	}
	return result;

}


float segm_scan::CorneaEnfaceMap::getCurvatureRadiusOnCenterHorz(void) const
{
	return getImpl().radiusCentHorz;
}


float segm_scan::CorneaEnfaceMap::getCurvatureRadiusOnCenterVert(void) const
{
	return getImpl().radiusCentVert;
}


float segm_scan::CorneaEnfaceMap::getAverageThicknessOnCenter(void) const
{
	return getImpl().averageCenter;
}


CorneaEnfaceMap::CorneaEnfaceMapImpl & segm_scan::CorneaEnfaceMap::getImpl(void) const
{
	return *d_ptr;
}
