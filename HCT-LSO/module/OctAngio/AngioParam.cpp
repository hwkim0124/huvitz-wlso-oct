#include "pch.h"
#include "AngioParam.h"

#include "OctAngio2.h"

using namespace oct_angio;


struct AngioParam::AngioParamImpl
{
	bool isVerticalScan = false;
	int overlaps = 3;
	int points = 384;
	int lines = 384;

	float rangeX = 4.5f;
	float rangeY = 4.5f;
	float lineSpace = 0.0f;
	float foveaCx = 0.0f;
	float foveaCy = 0.0f;

	OcularLayerType slabUpperType = OcularLayerType::NFL;
	OcularLayerType slabLowerType = OcularLayerType::OPL;
	float slabUpperOffset = 0.0f;
	float slabLowerOffset = 0.0f;

	bool axialAlign = true;
	bool lateralAlign = false;
	bool pixelAveraging = true;
	int pixelAverageSize = 2;
	bool decorrCircular = false;
	bool differOutput = true;
	bool decorrOutput = false;
	bool reflectCorrect = false;

	float decorrThresh = 0.0f;
	int intensThresh1 = 0;
	int intensThresh2 = 0;

	float decorrNormThresh1 = 0.0f;
	float decorrNormThresh2 = 0.0f;
	float differNormThresh1 = 0.0f;
	float differNormThresh2 = 0.0f;

	bool layersSelected = true;
	bool postProcessing = false;
	bool normProjection = true;
	bool projectArtifactRemoval = false;
	bool motionCorrect = false;
	bool vascularLayers = false;
	bool contrastEnhance = false;
	float contrastClipLimit = 4.0f;

	float biasSigma = 0.0f;
	float normDropOff = 0.0f;
	float noiseReductionRate = 0.25f;

	AngioParamImpl() {

	};
};


AngioParam::AngioParam()
	: d_ptr(make_unique<AngioParamImpl>())
{
}


AngioParam::~AngioParam()
{
	// Destructor should be defined for unique_ptr to delete AngioDecorrImpl as an incomplete type.
}


oct_angio::AngioParam::AngioParam(AngioParam && rhs) = default;
AngioParam & oct_angio::AngioParam::operator=(AngioParam && rhs) = default;


oct_angio::AngioParam::AngioParam(const AngioParam & rhs)
	: d_ptr(make_unique<AngioParamImpl>(*rhs.d_ptr))
{
}


AngioParam & oct_angio::AngioParam::operator=(const AngioParam & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

auto oct_angio::AngioParam::isVerticalScan(void) -> bool &
{
	return getImpl().isVerticalScan;
}

auto oct_angio::AngioParam::numberOfOverlaps(void) -> int &
{
	return getImpl().overlaps;
}

auto oct_angio::AngioParam::numberOfPoints(void) -> int &
{
	return getImpl().points;
}

auto oct_angio::AngioParam::numberOfLines(void) -> int &
{
	return getImpl().lines;
}

auto oct_angio::AngioParam::scanRangeX(void) -> float &
{
	return getImpl().rangeX;
}

auto oct_angio::AngioParam::scanRangeY(void) -> float &
{
	return getImpl().rangeY;
}

auto oct_angio::AngioParam::scanLineSpace(void) -> float &
{
	return getImpl().lineSpace;
}

auto oct_angio::AngioParam::foveaCenterX(void) -> float &
{
	return getImpl().foveaCx;
}

auto oct_angio::AngioParam::foveaCenterY(void) -> float &
{
	return getImpl().foveaCy;
}

auto oct_angio::AngioParam::lowerLayerType(void) -> wso_domain::OcularLayerType &
{
	return getImpl().slabLowerType;
}

auto oct_angio::AngioParam::upperLayerType(void) -> wso_domain::OcularLayerType &
{
	return getImpl().slabUpperType;
}

auto oct_angio::AngioParam::lowerLayerOffset(void) -> float &
{
	return getImpl().slabLowerOffset;
}

auto oct_angio::AngioParam::upperLayerOffset(void) -> float &
{
	return getImpl().slabUpperOffset;
}

auto oct_angio::AngioParam::useAxialAlign(void) -> bool &
{
	return getImpl().axialAlign;
}

auto oct_angio::AngioParam::useLateralAlign(void) -> bool &
{
	return getImpl().lateralAlign;
}

auto oct_angio::AngioParam::usePixelAveraging(void) -> bool &
{
	return getImpl().pixelAveraging;
}

auto oct_angio::AngioParam::pixelAverageSize(void) -> int &
{
	return getImpl().pixelAverageSize;
}

auto oct_angio::AngioParam::useDecorrCircular(void) -> bool &
{
	return getImpl().decorrCircular;
}

auto oct_angio::AngioParam::useDifferOutput(void) -> bool &
{
	return getImpl().differOutput;
}

auto oct_angio::AngioParam::useDecorrOutput(void) -> bool &
{
	return getImpl().decorrOutput;
}

auto oct_angio::AngioParam::useReflectCorrection(void) -> bool &
{
	return getImpl().reflectCorrect;
}

auto oct_angio::AngioParam::decorrLowerThreshold(void) -> float &
{
	return getImpl().decorrThresh;
}

auto oct_angio::AngioParam::intensLowerThreshold(void) -> int &
{
	return getImpl().intensThresh1;
}

auto oct_angio::AngioParam::intensUpperthreshold(void) -> int &
{
	return getImpl().intensThresh2;
}

auto oct_angio::AngioParam::decorrNormLowerThreshold(void) -> float &
{
	return getImpl().decorrNormThresh1;
}

auto oct_angio::AngioParam::decorrNormUpperThreshold(void) -> float &
{
	return getImpl().decorrNormThresh2;
}

auto oct_angio::AngioParam::differNormLowerThreshold(void) -> float &
{
	return getImpl().differNormThresh1;
}

auto oct_angio::AngioParam::differNormUpperThreshold(void) -> float &
{
	return getImpl().differNormThresh2;
}

auto oct_angio::AngioParam::useLayersSelected(void) -> bool &
{
	return getImpl().layersSelected;
}

auto oct_angio::AngioParam::usePostProcessing(void) -> bool &
{
	return getImpl().postProcessing;
}

auto oct_angio::AngioParam::useNormProjection(void) -> bool &
{
	return getImpl().normProjection;
}

auto oct_angio::AngioParam::useProjectArtifactRemoval(void) -> bool &
{
	return getImpl().projectArtifactRemoval;
}

auto oct_angio::AngioParam::useMotionCorrection(void) -> bool &
{
	return getImpl().motionCorrect;
}

auto oct_angio::AngioParam::useVascularLayers(void) -> bool &
{
	return getImpl().vascularLayers;
}

auto oct_angio::AngioParam::useContrastEnhance(void) -> bool &
{
	return getImpl().contrastEnhance;
}


AngioParam::AngioParamImpl & oct_angio::AngioParam::getImpl(void) const
{
	return *d_ptr;
}

auto oct_angio::AngioParam::contrastClipLimit(void) -> float &
{
	return getImpl().contrastClipLimit;
}

auto oct_angio::AngioParam::biasFieldSigma(void) -> float &
{
	return getImpl().biasSigma;
}

auto oct_angio::AngioParam::normalizeDropOff(void) -> float &
{
	return getImpl().normDropOff;
}

auto oct_angio::AngioParam::noiseReductionRate(void) -> float &
{
	return getImpl().noiseReductionRate;
}
