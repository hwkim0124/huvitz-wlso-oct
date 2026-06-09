#include "pch.h"
#include "SegmScan2.h"
#include "CorneaLayers.h"

using namespace segm_scan;


struct CorneaLayers::CorneaLayersImpl
{
	CorneaLayersImpl()
	{
	}
};


CorneaLayers::CorneaLayers() :
	d_ptr(make_unique<CorneaLayersImpl>())
{
}


segm_scan::CorneaLayers::~CorneaLayers() = default;
segm_scan::CorneaLayers::CorneaLayers(CorneaLayers && rhs) = default;
CorneaLayers & segm_scan::CorneaLayers::operator=(CorneaLayers && rhs) = default;


segm_scan::CorneaLayers::CorneaLayers(const CorneaLayers & rhs)
: d_ptr(make_unique<CorneaLayersImpl>(*rhs.d_ptr))
{
}


CorneaLayers & segm_scan::CorneaLayers::operator=(const CorneaLayers & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::CorneaLayers::initialize(unsigned int width, unsigned int height)
{
	OcularLayers::initializeAsCornea(width, height);
	return;
}


std::vector<int> segm_scan::CorneaLayers::getLayerPoints(OcularLayerType layer, int width, int height) const
{
	auto type = static_cast<unsigned short>(layer);
	return OcularLayers::getLayerPoints(type, width, height);
}


void segm_scan::CorneaLayers::setLayerPoints(OcularLayerType layer, const std::vector<int>& points)
{
	auto type = static_cast<unsigned short>(layer);
	OcularLayers::setLayerPoints(type, points);
	return;
}


bool segm_scan::CorneaLayers::isLayerPoints(OcularLayerType layer) const
{
	auto type = static_cast<unsigned short>(layer);
	auto data = OcularLayers::getLayerPoints(type);
	bool none = std::all_of(data.cbegin(), data.cend(), [](int v) { return v < 0; });
	return !none;
	// return (data.size() > 0);
}


OcularLayer * segm_scan::CorneaLayers::getEPI(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::EPI);
	return getLayer(type);
}


OcularLayer * segm_scan::CorneaLayers::getBOW(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::BOW);
	return getLayer(type);
}


OcularLayer * segm_scan::CorneaLayers::getEND(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::END);
	return getLayer(type);
}


OcularLayer * segm_scan::CorneaLayers::getInner(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::INNER);
	return getLayer(type);
}


OcularLayer * segm_scan::CorneaLayers::getOuter(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::OUTER);
	return getLayer(type);
}


CorneaLayers::CorneaLayersImpl & segm_scan::CorneaLayers::getImpl(void) const
{
	return *d_ptr;
}
