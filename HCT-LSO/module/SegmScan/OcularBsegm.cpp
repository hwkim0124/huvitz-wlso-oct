#include "pch.h"
#include "OcularBsegm.h"
#include "OcularLayers.h"
#include "SegmScan2.h"

using namespace segm_scan;


struct OcularBsegm::OcularBsegmImpl
{
	OcularImage imgSource;
	OcularImage imgSample;
	OcularImage imgResized;
	OcularImage imgSmoothed;
	OcularImage imgGradient;
	OcularImage imgAscends;
	OcularImage imgDescends;
	OcularImage imgCosts;

	OctScanPattern patternDesc;

	std::unique_ptr<OcularLayers> layers;
	int srcWidth;
	int srcHeight;
	int imageIdx;

	bool result;

	OcularBsegmImpl() : result(false), layers(nullptr), srcWidth(0), srcHeight(0), imageIdx(0)
	{
	}
};


OcularBsegm::OcularBsegm() :
	d_ptr(make_unique<OcularBsegmImpl>())
{
}


segm_scan::OcularBsegm::~OcularBsegm() = default;
segm_scan::OcularBsegm::OcularBsegm(OcularBsegm && rhs) = default;
OcularBsegm & segm_scan::OcularBsegm::operator=(OcularBsegm && rhs) = default;

/*
segm_scan::OcularBsegm::OcularBsegm(const OcularBsegm & rhs)
	: d_ptr(make_unique<OcularBsegmImpl>(*rhs.d_ptr))
{
}


OcularBsegm & segm_scan::OcularBsegm::operator=(const OcularBsegm & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}
*/


bool segm_scan::OcularBsegm::loadSource(const unsigned char * bits, int width, int height, int padding)
{
	if (source()->fromBitsData(bits, width, height, padding)) {
		getImpl().srcWidth = source()->getWidth();
		getImpl().srcHeight = source()->getHeight();

		// getOcularLayers()->setRegionSize(getSourceWidth(), getSourceHeight());
		initializeLayers();
		return true;
	}
	return false;
}


bool segm_scan::OcularBsegm::isEmpty(void) const
{
	return source()->isEmpty();
}


bool segm_scan::OcularBsegm::isResult(void) const
{
	return d_ptr->result;
}

void segm_scan::OcularBsegm::setResult(bool flag)
{
	d_ptr->result = flag;
	return;
}

void segm_scan::OcularBsegm::setImageIndex(int index)
{
	d_ptr->imageIdx = index;
	return;
}

int segm_scan::OcularBsegm::getImageIndex(void) const
{
	return d_ptr->imageIdx;
}


int segm_scan::OcularBsegm::getSourceWidth(void) const
{
	return getImpl().srcWidth;
}


int segm_scan::OcularBsegm::getSourceHeight(void) const
{
	return getImpl().srcHeight;
}


void segm_scan::OcularBsegm::setPatternDescript(OctScanPattern desc)
{
	d_ptr->patternDesc = desc;
	return;
}


const OctScanPattern & segm_scan::OcularBsegm::getPatternDescript(void) const
{
	return d_ptr->patternDesc;
}


float segm_scan::OcularBsegm::getResolutionX(bool inMM) const
{
	float dist;
	if (false) { // d_ptr->patternDesc.getPatternName() == PatternName::Topography) {
		dist = d_ptr->patternDesc.getScanDistance();
	}
	else {
		// dist = d_ptr->patternDesc.getScanDistance() * 0.80f;
		dist = d_ptr->patternDesc.getScanDistance();
	}
	float resX = dist / (d_ptr->patternDesc.getNumberOfScanPoints() - 1);

	if (!inMM) {
		resX *= 1000.0f;
	}
	return resX;
}


float segm_scan::OcularBsegm::getResolutionY(bool inAir) const
{
	float yRes = 0.0f;
	if (d_ptr->patternDesc.isCorneaScan()) {
		if (inAir) {
			yRes = (float)OctDataSetup::getInAirScanAxialResolution();
		}
		else {
			yRes = (float)OctDataSetup::getCorneaScanAxialResolution();
		}
	}
	else {
		yRes = (float)OctDataSetup::getRetinaScanAxialResolution();
	}
	return yRes;
}


float segm_scan::OcularBsegm::getSampleWidthRatio(void) const
{
	if (!source()->isEmpty()) {
		float ratio = (float)sample()->getWidth() / (float)source()->getWidth();
		return ratio;
	}
	return 0.0f;
}


float segm_scan::OcularBsegm::getSampleHeightRatio(void) const
{
	if (!source()->isEmpty()) {
		float ratio = (float)sample()->getHeight() / (float)source()->getHeight();
		return ratio;
	}
	return 0.0f;
}


bool segm_scan::OcularBsegm::performAnalysis(bool meye)
{
	if (source()->isEmpty()) {
		d_ptr->result = false;
		return false;
	}
	d_ptr->result = true;
	return true;
}


bool segm_scan::OcularBsegm::getThicknessLine(OcularLayerType upper, OcularLayerType lower, std::vector<std::float_t>& thicks, int width, int height, float upperOffset, float lowerOffset) const
{
	auto image_w = (width == 0 ? getSourceWidth() : width);
	auto image_h = (height == 0 ? getSourceHeight() : height);

	if (isEmpty() || image_w <= 0 || image_h <= 0) {
		return false;
	}

	thicks = std::vector<float>(image_w, 0.0f);

	float axialResol = (float)OctDataSetup::getRetinaScanAxialResolution();
	if (getPatternDescript().isCorneaScan()) {
		axialResol = (float)OctDataSetup::getCorneaScanAxialResolution();
		// axialResol = (float)GlobalSettings::getInAirScanAxialResolution();
	}

	auto upps = getOcularLayers()->getLayerPoints(upper, image_w, image_h);
	auto lows = getOcularLayers()->getLayerPoints(lower, image_w, image_h);

	auto upp_size = upps.size();
	auto low_size = lows.size();
	if (upp_size == image_w && low_size == image_w) {
		if (std::all_of(upps.begin(), upps.end(), [](int i) { return i < 0; }) &&
			std::all_of(lows.begin(), lows.end(), [](int i) { return i < 0; })) {
			return false;
		}

		if (upperOffset != 0.0f) {
			int offset = (int)(upperOffset / axialResol);
			transform(upps.begin(), upps.end(), upps.begin(), [&](int x) { return min(max(x + offset, 0), image_h - 1); });
		}
		if (lowerOffset != 0.0f) {
			int offset = (int)(lowerOffset / axialResol);
			transform(lows.begin(), lows.end(), lows.begin(), [&](int x) { return min(max(x + offset, 0), image_h - 1); });
		}
		// if (upperOffset != 0.0f || lowerOffset != 0.0f) {
		transform(begin(lows), end(lows), begin(upps), begin(lows), [](int e1, int e2) { return max(e1, e2); });
		// }

		for (int i = 0; i < upp_size; i++) {
			if (lows[i] >= 0 && upps[i] >= 0 && lows[i] >= upps[i]) {
				thicks[i] = ((lows[i] - upps[i]) * axialResol);
			}
		}

		/*
		if (isOpticNerveDisc()) {
			int x1, x2;
			if (getOpticNerveDiscRange(x1, x2)) {
				for (int k = x1; k <= x2; k++) {
					thicks[k] = ONH_DISC_MASK_VALUE;
				}
			}
		}

		if (isOpticNerveCup()) {
			int x1, x2;
			if (getOpticNerveCupRange(x1, x2)) {
				for (int k = x1; k <= x2; k++) {
					thicks[k] = ONH_CUP_MASK_VALUE;
				}
			}
		}
		*/
		return true;
	}

	return false;
}


bool segm_scan::OcularBsegm::getDistanceLine(OcularLayerType upper, OcularLayerType lower, std::vector<std::float_t>& thicks, int width, int height) const
{
	auto image_w = (width == 0 ? getSourceWidth() : width);
	auto image_h = (height == 0 ? getSourceHeight() : height);

	if (isEmpty() || image_w <= 0 || image_h <= 0) {
		return false;
	}

	thicks = std::vector<float>(image_w, 0.0f);


	float axialResol = (float)OctDataSetup::getRetinaScanAxialResolution();
	auto upps = getOcularLayers()->getLayerPoints(upper, image_w, image_h);
	auto lows = getOcularLayers()->getLayerPoints(lower, image_w, image_h);

	auto upp_size = upps.size();
	auto low_size = lows.size();
	if (upp_size == image_w && low_size == image_w) {
		if (std::all_of(upps.begin(), upps.end(), [](int i) { return i < 0; }) &&
			std::all_of(lows.begin(), lows.end(), [](int i) { return i < 0; })) {
			return false;
		}

		for (int i = 0; i < upp_size; i++) {
			if (lows[i] >= 0 && upps[i] >= 0 && lows[i] >= upps[i])
			{
				float dist, dx, dy;
				float minDist = 999999.0f;
				for (int j = i, k = 0; j >= 0; j--, k++) {
					dx = (float)k;
					dy = (float)(lows[j] - upps[i]);
					dist = sqrt(dx * dx + dy * dy);
					if (dist < minDist) {
						minDist = dist;
					}
					else {
						break;
					}
				}

				for (int j = i + 1, k = 1; j < image_w; j++, k++) {
					dx = (float)k;
					dy = (float)(lows[j] - upps[i]);
					dist = sqrt(dx * dx + dy * dy);
					if (dist < minDist) {
						minDist = dist;
					}
					else {
						break;
					}
				}

				thicks[i] = minDist * axialResol;
			}
		}
		return true;
	}

	return false;
}


bool segm_scan::OcularBsegm::getLateralLine(OcularLayerType upper, OcularLayerType lower, std::vector<std::uint8_t>& pixels, float upperOffset, float lowerOffset)
{
	auto bits = source()->getBitsData();
	auto image_w = getSourceWidth();
	auto image_h = getSourceHeight();
	pixels = vector<uint8_t>(image_w, 0);

	if (isEmpty() || image_w <= 0 || image_h <= 0) {
		return false;
	}

	auto upps = getOcularLayers()->getLayerPoints(upper, image_w, image_h);
	auto lows = getOcularLayers()->getLayerPoints(lower, image_w, image_h);

	if (upps.size() != image_w || lows.size() != image_w) {
		upps = vector<int>(image_w, 0);
		lows = vector<int>(image_w, image_h - 1);
	}
	else {
		replace(upps.begin(), upps.end(), -1, 0);
		replace(lows.begin(), lows.end(), -1, image_h - 1);

		auto resol = OctDataSetup::getRetinaScanAxialResolution();
		if (upperOffset != 0.0f) {
			int offset = (int)(upperOffset / resol);
			transform(upps.begin(), upps.end(), upps.begin(), [&](int x) { return min(max(x + offset, 0), image_h - 1); });
		}
		if (lowerOffset != 0.0f) {
			int offset = (int)(lowerOffset / resol);
			transform(lows.begin(), lows.end(), lows.begin(), [&](int x) { return min(max(x + offset, 0), image_h - 1); });
		}
		// if (upperOffset != 0.0f || lowerOffset != 0.0f) {
			transform(begin(lows), end(lows), begin(upps), begin(lows), [](int e1, int e2) { return max(e1, e2); });
		// }

		/*
		for (int i = 0; i < image_w; i++) {
			if (lows[i] < upps[i]) {
				lows[i] = upps[i];
			}
		}
		*/
	}

	for (int i = 0; i < image_w; i++) {
		int gsum = 0;
		int gcnt = 0;
		for (int j = upps[i]; j <= lows[i]; j++) {
			gsum += bits[j * image_w + i];
			gcnt++;
		}
		if (gcnt > 0) {
			pixels[i] = (std::uint8_t)(gsum / gcnt);
		}
	}
	return true;
}

bool segm_scan::OcularBsegm::getMaxValueLine(OcularLayerType upper, OcularLayerType lower, std::vector<std::uint8_t>& pixels)
{
	auto bits = source()->getBitsData();
	auto image_w = getSourceWidth();
	auto image_h = getSourceHeight();
	pixels = vector<uint8_t>(image_w, 0);

	if (isEmpty() || image_w <= 0 || image_h <= 0) {
		return false;
	}

	auto upps = getOcularLayers()->getLayerPoints(upper, image_w, image_h);
	auto lows = getOcularLayers()->getLayerPoints(lower, image_w, image_h);
	if (lower == OcularLayerType::OUTER) {
		for (int i = 0 ; i < lows.size();i++)
			lows[i] = image_h-2;
	}

	if (upps.size() != image_w || lows.size() != image_w) {
		upps = vector<int>(image_w, 0);
		lows = vector<int>(image_w, image_h - 1);
	}
	else {
		replace(upps.begin(), upps.end(), -1, 0);
		replace(lows.begin(), lows.end(), -1, image_h - 1);

		transform(begin(lows), end(lows), begin(upps), begin(lows), [](int e1, int e2) { return max(e1, e2); });
	}

	for (int i = 0; i < image_w; i++) {
		auto size = lows[i] - upps[i];
		
		if (size > 2) {
			vector<int> layerPixels;

			for (int j = upps[i]; j <= lows[i]; j++) {
				if (upps[i] > 0 && lows[i] > 0 && upps[i] < image_h - 1 && lows[i] < image_h - 1)
				{
					layerPixels.push_back(bits[j*image_w + i]);
				}
				else
				{
					// layerPixels.push_back(255);
					layerPixels.push_back(0);
				}
			}

			pixels[i] = *max_element(&layerPixels[0], &layerPixels[size - 1]);
		}
	}
	return true;
}

OcularLayer * segm_scan::OcularBsegm::layerInn(void) const
{	
	return nullptr;
}


OcularLayer * segm_scan::OcularBsegm::layerOut(void) const
{
	return nullptr;
}


OcularLayer * segm_scan::OcularBsegm::layerILM(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerNFL(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerIPL(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerOPL(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerOPR(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerIOS(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerRPE(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerBRM(void) const
{
	return nullptr;
}

OcularLayer * segm_scan::OcularBsegm::layerBASE(void) const
{
	return nullptr;
}

OcularImage * segm_scan::OcularBsegm::getImageObject(OcularImageType type)
{
	switch (type) {
	case OcularImageType::SOURCE:
		return source();
	case OcularImageType::SAMPLE:
		return sample();
	case OcularImageType::DENOISED:
		return smoothed();
	case OcularImageType::GRADIENTS:
		return gradients();
	case OcularImageType::ASCENDS:
		return ascends();
	case OcularImageType::DESCENDS:
		return descends();
	}
	return source();
}


OcularLayers * segm_scan::OcularBsegm::getOcularLayers(void) const
{
	return d_ptr->layers.get();
}


void segm_scan::OcularBsegm::setOcularLayers(std::unique_ptr<OcularLayers>&& layers)
{
	d_ptr->layers = std::move(layers);
	return;
}

bool segm_scan::OcularBsegm::isOpticNerveDisc(void) const
{
	return false;
}

bool segm_scan::OcularBsegm::getOpticNerveDiscRange(int & x1, int & x2) const
{
	return false;
}

bool segm_scan::OcularBsegm::getOpticNerveDiscPixels(int & pixels) const
{
	return false;
}

void segm_scan::OcularBsegm::setOpticNerveDiscRange(int x1, int x2)
{
}

void segm_scan::OcularBsegm::setOpticNerveDiscPixels(int pixels)
{
}

bool segm_scan::OcularBsegm::elaborateParams(const OcularLayer* layerILM, const OcularLayer* layerRPE)
{
	return false;
}

bool segm_scan::OcularBsegm::isOpticNerveCup(void) const
{
	return false;
}

bool segm_scan::OcularBsegm::getOpticNerveCupRange(int & x1, int & x2) const
{
	return false;
}

bool segm_scan::OcularBsegm::getOpticNerveCupPixels(int & pixels) const
{
	return false;
}


void segm_scan::OcularBsegm::setOpticNerveCupRange(int x1, int x2)
{
}

void segm_scan::OcularBsegm::setOpticNerveCupPixels(int pixels)
{
}

void segm_scan::OcularBsegm::clearOpticNerveRange(void)
{
	return;
}

bool segm_scan::OcularBsegm::getCurvatureRadius(std::vector<float>& radius) const
{
	return false;
}

bool segm_scan::OcularBsegm::setCurvatureRadius(const std::vector<float>& rads)
{
	return false;
}

bool segm_scan::OcularBsegm::getCorneaCurvature(std::vector<float>& output, int mapType) const
{
	return false;
}

void segm_scan::OcularBsegm::initializeLayers(int width, int height)
{
	return;
}


RetinaLayers * segm_scan::OcularBsegm::getRetinaLayers(void) const
{
	return (RetinaLayers*)getOcularLayers();
}


CorneaLayers * segm_scan::OcularBsegm::getCorneaLayers(void) const
{
	return (CorneaLayers*)getOcularLayers();
}


OcularImage * segm_scan::OcularBsegm::resized(void) const
{
	return &(d_ptr->imgResized);
}


OcularImage * segm_scan::OcularBsegm::sample(void) const
{
	return &(d_ptr->imgSample);
}


OcularImage * segm_scan::OcularBsegm::smoothed(void) const
{
	return &(d_ptr->imgSmoothed);
}


OcularImage * segm_scan::OcularBsegm::source(void) const
{
	return &(d_ptr->imgSource);
}


OcularImage * segm_scan::OcularBsegm::gradients(void) const
{
	return &(d_ptr->imgGradient);
}


OcularImage * segm_scan::OcularBsegm::ascends(void) const
{
	return &(d_ptr->imgAscends);
}


OcularImage * segm_scan::OcularBsegm::descends(void) const
{
	return &(d_ptr->imgDescends);
}


OcularImage * segm_scan::OcularBsegm::costs(void) const
{
	return &(d_ptr->imgCosts);
}

void segm_scan::OcularBsegm::setSample(OcularImage && image)
{
	getImpl().imgSample = move(image);
}

void segm_scan::OcularBsegm::setAscends(OcularImage && image)
{
	getImpl().imgAscends = move(image);
}

void segm_scan::OcularBsegm::setDescends(OcularImage && image)
{
	getImpl().imgDescends = move(image);
}



OcularBsegm::OcularBsegmImpl & segm_scan::OcularBsegm::getImpl(void) const
{
	return *d_ptr;
}
