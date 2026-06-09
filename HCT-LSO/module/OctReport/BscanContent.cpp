#include "pch.h"
#include "BscanContent.h"
#include "OctReport2.h"

#define _SILENCE_CXX20_CISO646_REMOVED_WARNING

#include <json\json.hpp>
#include <fstream>
#include <iostream>

using namespace oct_report;


struct BscanContent::BscanDataImpl
{
	BscanImage* image;
	std::unique_ptr<OcularBsegm> bsegm;
	int idxOverlap = 0;
	int idxSection = 0;

	BscanDataImpl() : image(nullptr)
	{
	}
};


BscanContent::BscanContent() :
	d_ptr(make_unique<BscanDataImpl>())
{
}


oct_report::BscanContent::BscanContent(oct_result::BscanImage * image) :
	d_ptr(make_unique<BscanDataImpl>())
{
	d_ptr->image = image;
}


oct_report::BscanContent::~BscanContent() = default;
oct_report::BscanContent::BscanContent(BscanContent && rhs) = default;
BscanContent & oct_report::BscanContent::operator=(BscanContent && rhs) = default;

/*
oct_report::BscanData::BscanData(const BscanData & rhs)
	: d_ptr(make_unique<BscanDataImpl>(*rhs.d_ptr))
{
}


BscanData & oct_report::BscanData::operator=(const BscanData & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}
*/


const OctScanImageDescript* oct_report::BscanContent::getImageDescript(void) const
{
	if (!isEmpty()) {
		return &d_ptr->image->getDescript();
	}
	return nullptr;
}


oct_result::BscanImage * oct_report::BscanContent::getImageObject(void) const
{
	if (!isEmpty()) {
		return d_ptr->image;
	}
	return nullptr;
}


std::uint8_t * oct_report::BscanContent::getImageBuffer(void) const
{
	if (!isEmpty()) {
		return d_ptr->image->getBuffer();
	}
	return nullptr;
}


std::wstring oct_report::BscanContent::getImageName(bool path) const
{
	std::wstring name;
	if (!isEmpty()) {
		name = d_ptr->image->getFileName(path);
	}
	return name;
}


int oct_report::BscanContent::getImageWidth(void) const
{
	if (!isEmpty()) {
		return d_ptr->image->getWidth();
	}
	return 0;
}


int oct_report::BscanContent::getImageHeight(void) const
{
	if (!isEmpty()) {
		return d_ptr->image->getHeight();
	}
	return 0;
}

int oct_report::BscanContent::getSectionIndex(void) const
{
	return d_ptr->idxSection;
}

int oct_report::BscanContent::getOverlapIndex(void) const
{
	return d_ptr->idxOverlap;
}


bool oct_report::BscanContent::isBsegmResult(void) const
{
	return (d_ptr->bsegm != nullptr /* && d_ptr->bsegm->isResult() */);
}


bool oct_report::BscanContent::isBsegmEmpty(void) const
{
	return (d_ptr->bsegm != nullptr && d_ptr->bsegm->isEmpty());
}


void oct_report::BscanContent::setSectionIndex(int index)
{
	d_ptr->idxSection = index;
}

void oct_report::BscanContent::setOverlapIndex(int index)
{
	d_ptr->idxOverlap = index;
}

void oct_report::BscanContent::setOcularBsegm(std::unique_ptr<OcularBsegm> bsegm)
{
	d_ptr->bsegm = std::move(bsegm);
	return;
}


bool oct_report::BscanContent::employBsegmImage(void)
{
	auto segm = getOcularBsegm();
	if (segm && segm->isResult()) {
		auto image = segm->getImageObject();
		if (d_ptr->image->setImage(image)) {
			return true;
		}
	}
	return false;
}


segm_scan::OcularBsegm * oct_report::BscanContent::getOcularBsegm(void) const
{
	if (d_ptr->bsegm) {
		return d_ptr->bsegm.get();
	}
	return nullptr;
}


ret_segm::SegmImage * oct_report::BscanContent::getBsegmSource(void)
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getImageObject(OcularBsegm::OcularImageType::SOURCE);
	}
	return nullptr;
}


ret_segm::SegmImage * oct_report::BscanContent::getBsegmSample(void)
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getImageObject(OcularBsegm::OcularImageType::SAMPLE);
	}
	return nullptr;
}


ret_segm::SegmImage * oct_report::BscanContent::getBsegmDenoised(void)
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getImageObject(OcularBsegm::OcularImageType::DENOISED);
	}
	return nullptr;
}


ret_segm::SegmImage * oct_report::BscanContent::getBsegmGradients(void)
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getImageObject(OcularBsegm::OcularImageType::GRADIENTS);
	}
	return nullptr;
}


ret_segm::SegmImage * oct_report::BscanContent::getBsegmAscends(void)
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getImageObject(OcularBsegm::OcularImageType::ASCENDS);
	}
	return nullptr;
}


ret_segm::SegmImage * oct_report::BscanContent::getBsegmDescends(void)
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getImageObject(OcularBsegm::OcularImageType::DESCENDS);
	}
	return nullptr;
}


std::vector<int> oct_report::BscanContent::getLayerPoints(OcularLayerType layer, int width, int height) const
{
	if (isBsegmResult()) {
		auto layers = d_ptr->bsegm->getRetinaLayers();
		return layers->getLayerPoints(layer, width, height);
	}
	return std::vector<int>();
}


std::vector<float> oct_report::BscanContent::getLayerThickness(OcularLayerType upper, OcularLayerType lower, int width, int height) const
{
	if (isBsegmResult()) {
		std::vector<float> thicks;
		if (d_ptr->bsegm->getThicknessLine(upper, lower, thicks, width, height)) {
			return thicks;
		}
	}
	return std::vector<float>();
}


std::vector<float> oct_report::BscanContent::getLayerDistance(OcularLayerType upper, OcularLayerType lower, int width, int height) const
{
	if (isBsegmResult()) {
		std::vector<float> thicks;
		if (d_ptr->bsegm->getDistanceLine(upper, lower, thicks, width, height)) {
			return thicks;
		}
	}
	return std::vector<float>();
}


std::vector<float> oct_report::BscanContent::getCurvatureRadius(void) const
{
	std::vector<float> curvs;
	if (isBsegmResult()) {
		d_ptr->bsegm->getCurvatureRadius(curvs);
	}
	return curvs;
}

std::vector<float> oct_report::BscanContent::getCorneaCurvature(int mapType) const
{
	std::vector<float> curvs;
	if (isBsegmResult()) {
		d_ptr->bsegm->getCorneaCurvature(curvs, mapType);
	}
	return curvs;
}

bool oct_report::BscanContent::isLayerPoints(OcularLayerType layer) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getRetinaLayers()->isLayerPoints(layer);
	}
	return false;
}

void oct_report::BscanContent::setLayerPoints(OcularLayerType layer, std::vector<int> points) const
{
	if (isBsegmResult()) {
		d_ptr->bsegm->getRetinaLayers()->setLayerPoints(layer, points);
	}
}

void oct_report::BscanContent::setCurvatureRadius(const std::vector<float>& rads)
{
	if (isBsegmResult()) {
		d_ptr->bsegm->setCurvatureRadius(rads);
	}
}


bool oct_report::BscanContent::isOpticNerveDisc(void) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->isOpticNerveDisc();
	}
	return false;
}


bool oct_report::BscanContent::getOpticNerveDiscRange(int & x1, int & x2) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getOpticNerveDiscRange(x1, x2);
	}
	return false;
}


bool oct_report::BscanContent::getOpticNerveDiscPixels(int & pixels) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getOpticNerveDiscPixels(pixels);
	}
	return false;
}


bool oct_report::BscanContent::isOpticNerveCup(void) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->isOpticNerveCup();
	}
	return false;
}


bool oct_report::BscanContent::getOpticNerveCupRange(int & x1, int & x2) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getOpticNerveCupRange(x1, x2);
	}
	return false;
}


bool oct_report::BscanContent::getOpticNerveCupPixels(int & pixels) const
{
	if (isBsegmResult()) {
		return d_ptr->bsegm->getOpticNerveCupPixels(pixels);
	}
	return false;
}


void oct_report::BscanContent::setOpticNerveDiscRange(int x1, int x2)
{
	if (isBsegmResult()) {
		d_ptr->bsegm->setOpticNerveDiscRange(x1, x2);
	}
	return;
}


void oct_report::BscanContent::setOpticNerveDiscPixels(int pixels)
{
	if (isBsegmResult()) {
		d_ptr->bsegm->setOpticNerveDiscPixels(pixels);
	}
	return;
}


void oct_report::BscanContent::setOpticNerveCupRange(int x1, int x2)
{
	if (isBsegmResult()) {
		d_ptr->bsegm->setOpticNerveCupRange(x1, x2);
	}
	return;
}


void oct_report::BscanContent::setOpticNerveCupPixels(int pixels)
{
	if (isBsegmResult()) {
		d_ptr->bsegm->setOpticNerveCupPixels(pixels);
	}
	return;
}


bool oct_report::BscanContent::isEmpty(void) const
{
	if (d_ptr->image == nullptr || d_ptr->image->isEmpty()) {
		return true;
	}
	return false;
}

bool oct_report::BscanContent::fetchRetinaBsegmResult(const OctScanPattern& pattern, OctRetinaBsegmDescriptor& desc) const
{
	if (!pattern.isRetinaScan()) {
		return false;
	}

	desc.width = getImageWidth();
	desc.height = getImageHeight();
	desc.range = pattern.getScanDistance();

	if (const auto& points = getLayerPoints(OcularLayerType::ILM); points.empty() == false) {
		copy(points.begin(), points.end(), desc.ilms);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::NFL); points.empty() == false) {
		copy(points.begin(), points.end(), desc.nfls);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::IPL); points.empty() == false) {
		copy(points.begin(), points.end(), desc.ipls);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::OPL); points.empty() == false) {
		copy(points.begin(), points.end(), desc.opls);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::IOS); points.empty() == false) {
		copy(points.begin(), points.end(), desc.ioss);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::RPE); points.empty() == false) {
		copy(points.begin(), points.end(), desc.rpes);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::OPR); points.empty() == false) {
		copy(points.begin(), points.end(), desc.oprs);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::BRM); points.empty() == false) {
		copy(points.begin(), points.end(), desc.brms);
	}


	if (const auto& points = getLayerPoints(OcularLayerType::BMO); points.empty() == false) {
		copy(points.begin(), points.end(), desc.datums);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::INNER); points.empty() == false) {
		copy(points.begin(), points.end(), desc.inners);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::OUTER); points.empty() == false) {
		copy(points.begin(), points.end(), desc.outers);
	}

	if (pattern.isOpticDiscScan()) {
		int discX1, discX2, discPixels;
		int cupX1, cupX2, cupPixels;

		getOpticNerveDiscRange(discX1, discX2);
		getOpticNerveDiscPixels(discPixels);
		getOpticNerveCupRange(cupX1, cupX2);
		getOpticNerveCupPixels(cupPixels);

		desc.opticDiscX1 = discX1;
		desc.opticDiscX2 = discX2;
		desc.opticCupX1 = cupX1;
		desc.opticCupX2 = cupX2;
		desc.opticCupPixelSize = cupPixels;
		desc.opticDiscPixelSize = discPixels;

		desc.isOpticCupValid = isOpticNerveCup();
		desc.isOpticDiscValid = isOpticNerveDisc();
	}
	return true;
}

bool oct_report::BscanContent::fetchCorneaBsegmResult(const OctScanPattern& pattern, OctCorneaBsegmDescriptor& desc) const
{
	if (!pattern.isCorneaScan()) {
		return false;
	}

	desc.width = getImageWidth();
	desc.height = getImageHeight();
	desc.range = pattern.getScanDistance();

	if (const auto& points = getLayerPoints(OcularLayerType::EPI); points.empty() == false) {
		copy(points.begin(), points.end(), desc.epis);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::BOW); points.empty() == false) {
		copy(points.begin(), points.end(), desc.bows);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::END); points.empty() == false) {
		copy(points.begin(), points.end(), desc.ends);
	}

	if (const auto& points = getLayerPoints(OcularLayerType::INNER); points.empty() == false) {
		copy(points.begin(), points.end(), desc.inners);
	}
	if (const auto& points = getLayerPoints(OcularLayerType::OUTER); points.empty() == false) {
		copy(points.begin(), points.end(), desc.outers);
	}

	if (const auto& points = getCurvatureRadius(); points.empty() == false) {
		copy(points.begin(), points.end(), desc.curveRadius);
	}
	return true;
}

bool oct_report::BscanContent::fetchRetinaBsegmTraits(const OctScanPattern& pattern, OctRetinaBsegmTraits& traits) const
{
	if (!pattern.isRetinaScan()) {
		return false;
	}

	traits.width = getImageWidth();
	traits.height = getImageHeight();
	traits.range = pattern.getScanDistance();

	if (pattern.isOpticDiscScan()) {
		int discX1, discX2, discPixels;
		int cupX1, cupX2, cupPixels;

		getOpticNerveDiscRange(discX1, discX2);
		getOpticNerveDiscPixels(discPixels);
		getOpticNerveCupRange(cupX1, cupX2);
		getOpticNerveCupPixels(cupPixels);

		traits.opticDiscX1 = discX1;
		traits.opticDiscX2 = discX2;
		traits.opticCupX1 = cupX1;
		traits.opticCupX2 = cupX2;
		traits.opticCupPixelSize = cupPixels;
		traits.opticDiscPixelSize = discPixels;

		traits.isOpticCupValid = isOpticNerveCup();
		traits.isOpticDiscValid = isOpticNerveDisc();
	}
	return true;
}

bool oct_report::BscanContent::fetchCorneaBsegmTraits(const OctScanPattern& pattern, OctCorneaBsegmTraits& traits) const
{
	if (!pattern.isCorneaScan()) {
		return false;
	}

	traits.width = getImageWidth();
	traits.height = getImageHeight();
	traits.range = pattern.getScanDistance();

	if (const auto& points = getCurvatureRadius(); points.empty() == false) {
		copy(points.begin(), points.end(), traits.curveRadius);
	}
	return true;
}

bool oct_report::BscanContent::fetchBsegmLayerPoints(OcularLayerType type, const OctScanPattern& pattern, OctBsegmLayerPoints& layer) const
{
	layer.width = getImageWidth();
	layer.height = getImageHeight();
	layer.range = pattern.getScanDistance();

	if (const auto& points = getLayerPoints(type); points.size() == layer.width) {
		copy(points.begin(), points.end(), layer.data);
		return true;
	}
	return false;
}


bool oct_report::BscanContent::exportBsegmResult(std::wstring path, const OctScanPattern desc)
{
	if (!isBsegmResult()) {
		return false;
	}

	using json = nlohmann::json;
	json j;
	// j["name"] = wtoa(getImageName());
	j["width"] = getImageWidth();
	j["height"] = getImageHeight();
	
	std::vector<int> layer;
	if (!desc.isCorneaScan()) {
		if (isLayerPoints(OcularLayerType::ILM) || true) {
			layer = getLayerPoints(OcularLayerType::ILM);
			j["layers"]["ILM"] = layer;
		}
		if (isLayerPoints(OcularLayerType::NFL) || true) {
			layer = getLayerPoints(OcularLayerType::NFL);
			j["layers"]["NFL"] = layer;
		}
		if (isLayerPoints(OcularLayerType::IPL) || true) {
			layer = getLayerPoints(OcularLayerType::IPL);
			j["layers"]["IPL"] = layer;
		}
		if (isLayerPoints(OcularLayerType::OPL) || true) {
			layer = getLayerPoints(OcularLayerType::OPL);
			j["layers"]["OPL"] = layer;
		}
		if (isLayerPoints(OcularLayerType::IOS) || true) {
			layer = getLayerPoints(OcularLayerType::IOS);
			j["layers"]["IOS"] = layer;
		}
		if (isLayerPoints(OcularLayerType::RPE) || true) {
			layer = getLayerPoints(OcularLayerType::RPE);
			j["layers"]["RPE"] = layer;
		}
		if (isLayerPoints(OcularLayerType::BRM) || true) {
			layer = getLayerPoints(OcularLayerType::BRM);
			j["layers"]["BRM"] = layer;
		}
		if (isLayerPoints(OcularLayerType::OPR) || true) {
			layer = getLayerPoints(OcularLayerType::OPR);
			j["layers"]["OPR"] = layer;
		}

		if (desc.isOpticDiscScan()) {
			int discX1, discX2, discPixels;
			int cupX1, cupX2, cupPixels;

			getOpticNerveDiscRange(discX1, discX2);
			getOpticNerveDiscPixels(discPixels);
			getOpticNerveCupRange(cupX1, cupX2);
			getOpticNerveCupPixels(cupPixels);

			j["opticDisc"]["state"] = isOpticNerveDisc();
			j["opticDisc"]["x1"] = discX1;
			j["opticDisc"]["x2"] = discX2;
			j["opticDisc"]["pixels"] = discPixels;

			j["opticCup"]["state"] = isOpticNerveCup();
			j["opticCup"]["x1"] = cupX1;
			j["opticCup"]["x2"] = cupX2;
			j["opticCup"]["pixels"] = cupPixels;
		}
	}
	else {
		if (isLayerPoints(OcularLayerType::EPI) || true) {
			layer = getLayerPoints(OcularLayerType::EPI);
			j["layers"]["EPI"] = layer;
		}
		if (isLayerPoints(OcularLayerType::BOW) || true) {
			layer = getLayerPoints(OcularLayerType::BOW);
			j["layers"]["BOW"] = layer;
		}
		if (isLayerPoints(OcularLayerType::END) || true) {
			layer = getLayerPoints(OcularLayerType::END);
			j["layers"]["END"] = layer;
		}

		std::vector<float> curvs;
		curvs = getCurvatureRadius();
		j["curvature"] = curvs;
	}

	try {
		std::ofstream f(path);
		f << j.dump();
		f.close();
		return true;
	}
	catch (...) {
		return false;
	}
}


bool oct_report::BscanContent::importBsegmResult(std::wstring path, const OctScanPattern desc)
{
	if (isEmpty()) {
		return false;
	}

	using json = nlohmann::json;

	json j;

	try {
		std::ifstream f(path);
		f >> j;
		f.close();

		std::unique_ptr<OcularBsegm> bsegm;
		std::vector<int> points;
		if (desc.isCorneaScan()) {
			bsegm = make_unique<CorneaBsegm>();
		}
		else {
			bsegm = (desc.isOpticDiscScan() ? make_unique<DiscBsegm>() : make_unique<MacularBsegm>());
		}

		bsegm->loadSource(getImageBuffer(), getImageWidth(), getImageHeight());
		bsegm->setPatternDescript(desc);

		if (desc.isCorneaScan()) {
			auto layers = bsegm->getCorneaLayers();
			points = j.at("layers").at("EPI").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::EPI, points);
			points = j.at("layers").at("BOW").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::BOW, points);
			points = j.at("layers").at("END").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::END, points);

			std::vector<float> curvs;
			curvs = j.at("curvature").get<std::vector<float>>();
			bsegm->setCurvatureRadius(curvs);

			setOcularBsegm(std::move(bsegm));
		}
		else {
			auto layers = bsegm->getRetinaLayers();
			points = j.at("layers").at("ILM").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::ILM, points);
			points = j.at("layers").at("NFL").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::NFL, points);
			points = j.at("layers").at("IPL").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::IPL, points);
			points = j.at("layers").at("OPL").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::OPL, points);
			points = j.at("layers").at("IOS").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::IOS, points);
			points = j.at("layers").at("RPE").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::RPE, points);
			points = j.at("layers").at("BRM").get<std::vector<int>>();
			layers->setLayerPoints(OcularLayerType::BRM, points);

			try {
				points = j.at("layers").at("OPR").get<std::vector<int>>();
			}
			catch (...) {
				points = std::vector<int>(desc.getNumberOfScanPoints(), -1);
			}
			layers->setLayerPoints(OcularLayerType::OPR, points);
			/*
			layers->getILM()->setRegionSize(256, 384);
			layers->getNFL()->setRegionSize(256, 384);
			layers->getIPL()->setRegionSize(256, 384);
			layers->getOPL()->setRegionSize(256, 384);
			layers->getIOS()->setRegionSize(256, 384);
			layers->getRPE()->setRegionSize(256, 384);
			layers->getBRM()->setRegionSize(256, 384);
			layers->getOPR()->setRegionSize(256, 384);

			layers->getILM()->resize(250, 300);
			layers->getNFL()->resize(250, 300);
			layers->getIPL()->resize(250, 300);
			layers->getOPL()->resize(250, 300);
			layers->getIOS()->resize(250, 300);
			layers->getRPE()->resize(250, 300);
			layers->getBRM()->resize(250, 300);
			layers->getOPR()->resize(250, 300);
			*/
			setOcularBsegm(std::move(bsegm));
			
			if (desc.isOpticDiscScan()) {
				int discX1, discX2, discPixels;
				int cupX1, cupX2, cupPixels;

				discX1 = j["opticDisc"]["x1"];
				discX2 = j["opticDisc"]["x2"];
				discPixels = j["opticDisc"]["pixels"];

				if (!j["opticDisc"]["state"]) {
					discX1 = discX2 = -1;
					discPixels = 0;
				}

				cupX1 = j["opticCup"]["x1"];
				cupX2 = j["opticCup"]["x2"];
				cupPixels = j["opticCup"]["pixels"];

				if (!j["opticCup"]["state"]) {
					cupX1 = cupX2 = -1;
					cupPixels = 0;
				}

				setOpticNerveDiscRange(discX1, discX2);
				setOpticNerveDiscPixels(discPixels);
				setOpticNerveCupRange(cupX1, cupX2);
				setOpticNerveCupPixels(cupPixels);
			}
		}
	}
	catch (...) {
		return false;
	}

	return true;
}


BscanContent::BscanDataImpl & oct_report::BscanContent::getImpl(void) const
{
	return *d_ptr;
}


