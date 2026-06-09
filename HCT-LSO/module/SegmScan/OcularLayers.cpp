#include "pch.h"
#include "OcularLayers.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct OcularLayers::OcularLayersImpl
{
	std::map<unsigned short, OcularLayer> layers;
	int width;
	int height;

	OcularLayersImpl() : width(0), height(0)
	{
	}
};


OcularLayers::OcularLayers() :
	d_ptr(make_unique<OcularLayersImpl>())
{
}


segm_scan::OcularLayers::~OcularLayers() = default;
segm_scan::OcularLayers::OcularLayers(OcularLayers && rhs) = default;
OcularLayers & segm_scan::OcularLayers::operator=(OcularLayers && rhs) = default;


segm_scan::OcularLayers::OcularLayers(const OcularLayers & rhs)
	: d_ptr(make_unique<OcularLayersImpl>(*rhs.d_ptr))
{
}


OcularLayers & segm_scan::OcularLayers::operator=(const OcularLayers & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void segm_scan::OcularLayers::initialize(unsigned int width, unsigned int height)
{
	return;
}


std::vector<int> segm_scan::OcularLayers::getLayerPoints(OcularLayerType layer, int width, int height) const
{
	return std::vector<int>(width, -1);
}

void segm_scan::OcularLayers::setLayerPoints(OcularLayerType layer, const std::vector<int>& points)
{
	return;
}


bool segm_scan::OcularLayers::isLayerPoints(OcularLayerType layer) const
{
	return false;
}


OcularLayer * segm_scan::OcularLayers::getILM(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getNFL(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getIPL(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getOPL(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getIOS(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularLayers::getRPE(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getBRM(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getBASE(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getInner(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularLayers::getOuter(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularLayers::getOPR(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularLayers::getEPI(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularLayers::getBOW(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularLayers::getEND(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularLayers::getLayer(unsigned short type) const
{
	auto iter = d_ptr->layers.find(type);
	if (iter != end(d_ptr->layers)) {
		return &iter->second;
	}
	return nullptr;
}


std::vector<int> segm_scan::OcularLayers::getLayerPoints(unsigned short type, int width, int height) const
{
	if (d_ptr->width == 0 || d_ptr->height == 0) {
		return vector<int>();
	}

	auto layer = OcularLayers::getLayer(type);
	int reg_w = (width == 0 ? d_ptr->width : width);
	int reg_h = (height == 0 ? d_ptr->height : height);

	auto data = std::vector<int>(reg_w, -1);

	if (layer != nullptr) {
		if (reg_w == d_ptr->width && reg_h == d_ptr->height) {
			data = layer->getYs();
		}
		else {
			data = layer->getYsResized(reg_w, reg_h);
		}
	}

	return data;
}


void segm_scan::OcularLayers::setLayerPoints(unsigned short type, const std::vector<int>& points)
{
	auto layer = OcularLayers::getLayer(type);
	layer->initialize(points, getRegionWidth(), getRegionHeight());
	return;
}


void segm_scan::OcularLayers::initializeAsRetina(unsigned int width, unsigned int height)
{
	d_ptr->width = width;
	d_ptr->height = height;

	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::ILM)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::NFL)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::IPL)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::OPL)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::IOS)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::RPE)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::BRM)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::BMO)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::INNER)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::OUTER)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::OPR)] = OcularLayer(width, height);
	return;
}


void segm_scan::OcularLayers::initializeAsCornea(unsigned int width, unsigned int height)
{
	d_ptr->width = width;
	d_ptr->height = height;

	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::INNER)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::OUTER)] = OcularLayer(width, height);

	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::EPI)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::BOW)] = OcularLayer(width, height);
	d_ptr->layers[static_cast<unsigned short>(OcularLayerType::END)] = OcularLayer(width, height);

	return;
}


void segm_scan::OcularLayers::applyRegionSize(unsigned int width, unsigned int height)
{
	for (auto iter = d_ptr->layers.begin(); iter != d_ptr->layers.end(); iter++) {
		auto layer = iter->second;
		// if (layer != nullptr) {
			layer.resize(width, height);
		// }
	}

	setRegionSize(width, height);
	return;
}


void segm_scan::OcularLayers::setRegionSize(unsigned int width, unsigned int height)
{
	d_ptr->width = width;
	d_ptr->height = height;
	return;
}


int segm_scan::OcularLayers::getNumberOfLayers(void)
{
	return (int)d_ptr->layers.size();
}


int segm_scan::OcularLayers::getRegionWidth(void)
{
	return d_ptr->width;
}


int segm_scan::OcularLayers::getRegionHeight(void)
{
	return d_ptr->height;
}


void segm_scan::OcularLayers::clear(void)
{
	d_ptr->layers.clear();
	return;
}


OcularLayers::OcularLayersImpl & segm_scan::OcularLayers::getImpl(void) const
{
	return *d_ptr;
}
