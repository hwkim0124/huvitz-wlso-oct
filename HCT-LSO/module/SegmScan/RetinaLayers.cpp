#include "pch.h"
#include "SegmScan2.h"
#include "RetinaLayers.h"

using namespace segm_scan;


struct RetinaLayers::RetinaLayersImpl
{
	RetinaLayersImpl()
	{
	}
};


RetinaLayers::RetinaLayers() :
	d_ptr(make_unique<RetinaLayersImpl>())
{
}


segm_scan::RetinaLayers::~RetinaLayers() = default;
segm_scan::RetinaLayers::RetinaLayers(RetinaLayers && rhs) = default;
RetinaLayers & segm_scan::RetinaLayers::operator=(RetinaLayers && rhs) = default;


segm_scan::RetinaLayers::RetinaLayers(const RetinaLayers & rhs)
	: d_ptr(make_unique<RetinaLayersImpl>(*rhs.d_ptr))
{
}


RetinaLayers & segm_scan::RetinaLayers::operator=(const RetinaLayers & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::RetinaLayers::initialize(unsigned int width, unsigned int height)
{
	OcularLayers::initializeAsRetina(width, height);
	return;
}


std::vector<int> segm_scan::RetinaLayers::getLayerPoints(OcularLayerType layer, int width, int height) const
{
	auto type = static_cast<unsigned short>(layer);
	return OcularLayers::getLayerPoints(type, width, height);
}


void segm_scan::RetinaLayers::setLayerPoints(OcularLayerType layer, const std::vector<int>& points)
{
	auto type = static_cast<unsigned short>(layer);
	OcularLayers::setLayerPoints(type, points);
	return;
}


bool segm_scan::RetinaLayers::isLayerPoints(OcularLayerType layer) const
{
	auto type = static_cast<unsigned short>(layer);
	auto data = OcularLayers::getLayerPoints(type);
	bool none = std::all_of(data.cbegin(), data.cend(), [](int v) { return v < 0; });
	return !none;
	// return (data.size() > 0);
}


OcularLayer * segm_scan::RetinaLayers::getILM(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::ILM);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getNFL(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::NFL);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getIPL(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::IPL);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getOPL(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::OPL);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getIOS(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::IOS);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getRPE(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::RPE);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getBRM(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::BRM);
	return getLayer(type);
}

OcularLayer * segm_scan::RetinaLayers::getBASE(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::BMO);
	return getLayer(type);
}

OcularLayer * segm_scan::RetinaLayers::getInner(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::INNER);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getOuter(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::OUTER);
	return getLayer(type);
}


OcularLayer * segm_scan::RetinaLayers::getOPR(void) const
{
	auto type = static_cast<unsigned short>(OcularLayerType::OPR);
	return getLayer(type);
}


RetinaLayers::RetinaLayersImpl & segm_scan::RetinaLayers::getImpl(void) const
{
	return *d_ptr;
}
