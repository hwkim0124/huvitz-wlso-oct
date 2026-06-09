#include "pch.h"
#include "Angiogram.h"
#include "AngioData.h"
#include "AngioFile.h"
#include "AngioLayout.h"
#include "AngioLayers.h"
#include "AngioDecorr.h"
#include "AngioMotion.h"
#include "AngioPost.h"

#include "OctAngio2.h"

using namespace oct_angio;



struct Angiogram::AngiogramImpl
{
	AngioData data;
	AngioFile file;
	AngioLayout layout;
	AngioLayers layers;
	// AngioLayers layers2;
	// AngioLayers layers3;

	AngioDecorr decorr;
	AngioDecorr decorr2;
	AngioDecorr decorr3;
	AngioMotion motion;
	AngioPost post;

	vector<unsigned char> imageBits;

	bool useAlignAxial = true;
	bool useAlignLateral = true;
	bool useDecorrCircular = false;
	bool useBiasFieldCorrect = true;

	bool useLayersSelected = true;
	bool useVascularLayers = false;
	bool useMotionCorrect = false;
	bool usePostProcessing = true;
	bool useNormProjection = true;
	bool useDecorrOutput = false;
	bool useDifferOutput = false; 

	bool useProjectionRemoval = true;
	bool useReflectCorrect = true;
	bool useProjectionStats = false;

	float upperThreshold = 0.0f;
	float lowerThreshold = 0.0f;
	float decorrUpperThreshold = 0.0f;
	float decorrLowerThreshold = 0.0f;
	float differUpperThreshold = 0.0f;
	float differLowerThreshold = 0.0f;

	float decorrBaseThreshold = 0.0f; // 0.05f;
	float normalizeDropOff = 0.0f;
	float biasFieldSigma = 15.0f;
	float noiseReductionRate = 0.5f;

	int numberOfOverlaps = 0;
	bool usePixelAverage = true;
	int pixelAverageOffset = 3; // 2;

	std::string importPath;

	AngiogramImpl() {
	};
};



Angiogram::Angiogram()
	: d_ptr(make_unique<AngiogramImpl>())
{
}


Angiogram::~Angiogram()
{
	// Destructor should be defined for unique_ptr to delete AngiogramImpl as an incomplete type.
}


oct_angio::Angiogram::Angiogram(Angiogram && rhs) = default;
Angiogram & oct_angio::Angiogram::operator=(Angiogram && rhs) = default;


void oct_angio::Angiogram::setupAngioPattern(const OctScanPattern& pattern)
{
	int points = pattern.numAscan;
	int lines = pattern.numBscan;
	int repeats = pattern.overlaps;
	bool is_vert = pattern.isVerticalScan();

	float rangeX = pattern.getScanRangeX();
	float rangeY = pattern.getScanRangeY();

	resetLayout(lines, points, repeats, is_vert);
	resetScanRange(rangeX, rangeY);
	return;
}


void oct_angio::Angiogram::resetLayout(int lines, int points, int repeats, bool vertical)
{
	Layout().setupLayout(lines, points, repeats, vertical);

	Layers().setupLayerArrays(lines, points, repeats);
	// Layers2().setupLayerArrays(lines, points, repeats);
	// Layers3().setupLayerArrays(lines, points, repeats);

	LogD() << "Angiogram layout reset, lines: " << lines << ", points: " << points << ", repeats: " << repeats << ", vertical: " << vertical;
	return;
}


void oct_angio::Angiogram::resetScanRange(float rangeX, float rangeY, float centerX, float centerY, bool isDisc)
{
	Layout().setupRange(rangeX, rangeY, centerX, centerY, isDisc);

	LogD() << "Angiogram scan range reset, rangeX: " << rangeX << ", rangeY: " << rangeY << ", centerX: " << centerX << ", centerY: " << centerY << ", isDisc: " << isDisc;
	return;
}


void oct_angio::Angiogram::resetSlabRange(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	Layers().setupSlabRange(upper, lower, upperOffset, lowerOffset);

	LogD() << "Angiogram slab range reset, upper: " << int(upper) << ", lower: " << int(lower) << ", upperOffset: " << upperOffset << ", lowerOffset: " << lowerOffset;
	return;
}


std::string oct_angio::Angiogram::importPath(void)
{
	return getImpl().importPath;
}


void oct_angio::Angiogram::setImportPath(std::string path)
{
	getImpl().importPath = path;
	return;
}


bool oct_angio::Angiogram::setupAngioData(std::vector<std::vector<CvImage>>&& ampls)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();

	if (!Data().setupAmplitudes(lines, points, repeats, std::move(ampls))) {
		LogD() << "Angiogram data setup failed!";
		return false;
	}
	return true;
}

bool oct_angio::Angiogram::setupAngioLayers(LayerMapArrays&& layers)
{
	if (!Layers().assignLayerMapArrays(std::move(layers))) {
		LogD() << "Angiogram layers setup failed!";
		return false;
	}
	return true;
}

bool oct_angio::Angiogram::loadDataBuffer(void)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();
	/*
	int lines = AngioSetup::numberOfAngioLines();
	int points = AngioSetup::numberOfAngioPoints();
	int repeats = AngioSetup::numberOfAngioRepeats();
	bool vertical = AngioSetup::isVerticalScan();
	resetLayout(lines, points, repeats, vertical);
	*/

	if (Data().fetchAmplitudesFromBuffer(lines, points, repeats)) {
		return true;
	}
	return false;
}


bool oct_angio::Angiogram::loadDataFiles(const std::string dirPath, const std::string fileName)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();
	auto vertical = Layout().isVerticalScan();

	/*
	std::wstring path = atow(dirPath);
	if (!SystemUtil::isDirectory(path)) {
		LogE() << "Import directory to load angio data not found!, path: " << dirPath;
		return false;
	}
	*/

	cpp_util::StopWatch::start();
	LogD() << "Loading angiogram data, lines: " << lines << ", points: " << points << ", repeats: " << repeats << ", vertical: " << vertical;

	if (!Data().importAmplitudesFromDataFiles(lines, points, repeats, dirPath, fileName)) {
		return false;
	}

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram data loaded, elapsed: " << msec;
	return true;
}


bool oct_angio::Angiogram::loadDataFile2(const std::string dirPath, const std::string fileName)
{
	/*
	std::wstring path = atow(dirPath);
	if (!SystemUtil::isDirectory(path)) {
		LogE() << "Import directory to load angio data not found!, path: " << dirPath;
		return false;
	}
	*/

	cpp_util::StopWatch::start();
	LogD() << "Loading angio data file2...";

	if (!AngioFile::loadAngioDataFile(dirPath, fileName, Layout(), Decorr())) {
		return false;
	}
	else {
		auto lines = Layout().numberOfLines();
		auto points = Layout().numberOfPoints();
		auto repeats = Layout().numberOfRepeats();
		auto vertical = Layout().isVerticalScan();
		resetLayout(lines, points, repeats, vertical);
		LogD() << "lines: " << lines << ", points: " << points << ", repeats: " << repeats << ", vertical: " << vertical;
	}

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram data loaded, elapsed: " << msec;
	return true;
}



bool oct_angio::Angiogram::loadDataImages(const std::string dirPath)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();

	cpp_util::StopWatch::start();
	LogD() << "Loading angiogram images, lines: " << lines << ", points: " << points << ", repeats: " << repeats;

	if (!Data().importAmplitudesFromImageFiles(lines, points, repeats, dirPath)) {
		return false;
	}

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram images loaded, elapsed: " << msec;
	return true;
}


bool oct_angio::Angiogram::saveDataFiles(const std::string dirPath, const std::string fileName)
{
	int lines = Layout().numberOfLines();
	int repeats = Layout().numberOfRepeats();

	/*
	std::wstring path = atow(dirPath);
	if (!SystemUtil::isDirectory(path)) {
		LogE() << "Export directory not found for angio data!, path: " << dirPath;
		return false;
	}
	*/

	if (Data().exportAmplitudesToDataFiles(lines, repeats, dirPath, fileName)) {
		return true;
	}
	return false;
}


bool oct_angio::Angiogram::saveDataFile2(const std::string dirPath, const std::string fileName)
{
	/*
	std::wstring path = atow(dirPath);
	if (!SystemUtil::isDirectory(path)) {
		LogE() << "Export directory not found for angio data!, path: " << dirPath;
		return false;
	}
	*/

	if (AngioFile::saveAngioDataFile(dirPath, fileName, Layout(), Decorr())) {
		return true;
	}
	
	return false;
}


bool oct_angio::Angiogram::isAmplitduesValid(void)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();

	if (Data().checkIfAmplitudesLoaded(lines, points, repeats)) {
		return true;
	}
	return false;
}


bool oct_angio::Angiogram::isDecorrelationsValid(void)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();

	if (Decorr().checkIfDecorrelationsLoaded(lines, points, repeats)) {
		return true;
	}
	return false;
}


bool oct_angio::Angiogram::isAngioImage(void)
{
	return imageBits() != nullptr;
}


bool oct_angio::Angiogram::isFoveaAvascularZone(void)
{
	if (Layout().isDiscScan()) {
		return false;
	}

	if (Layout().scanRangeX() > 6.0f || Layout().scanRangeY() > 6.0f) {
		return false;
	}

	if (Layers().isOuterRetinaFlows()) {
		return false;
	}

	return true;
}

/*
bool oct_angio::Angiogram::loadLayerSegments(const std::string dirPath)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();
	bool vascular = useVascularLayers();

	auto path = dirPath;
	if (path.empty()) {
		path = getImpl().importPath;
	}

	if (!Layers().loadLayerSegmentFiles(path)) {
		return false;
	}

	return true;
}



bool oct_angio::Angiogram::loadLayerSegments(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset, const std::string dirPath)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();
	bool vascular = useVascularLayers();

	auto path = dirPath;
	if (path.empty()) {
		path = getImpl().importPath;
	}

	if (!Layers().importLayersFromSegmentFiles(upper, lower, upperOffset, lowerOffset, path, vascular)) {
		return false;
	}

	if (!Layers2().importLayersFromSegmentFiles(OcularLayerType::NFL, OcularLayerType::IPL, 0.0f, 15.0f, path)) {
		return false;
	}

	if (!Layers3().importLayersFromSegmentFiles(OcularLayerType::IOS, OcularLayerType::BRM, 0.0f, 0.0f, path)) {
		return false;
	}
	return true;
}


void oct_angio::Angiogram::setLayerSegments(AngioLayers&& layers)
{
	getImpl().layers = move(layers);
	return;
}


void oct_angio::Angiogram::setLayerSegments(OcularLayerType upper, OcularLayerType lower, 
											LayerArrays& upperLayers, LayerArrays& lowerLayers,
											LayerArrays& upperLayers2, LayerArrays& lowerLayers2,
											LayerArrays& upperLayers3, LayerArrays& lowerLayers3)
{
	Layers().setUpperLayers(upperLayers);
	Layers().setLowerLayers(lowerLayers);
	Layers().upperLayerType() = upper;
	Layers().lowerLayerType() = lower;

	Layers2().setUpperLayers(upperLayers2);
	Layers2().setLowerLayers(lowerLayers2);
	Layers2().upperLayerType() = OcularLayerType::NFL;
	Layers2().lowerLayerType() = OcularLayerType::IPL;

	Layers3().setUpperLayers(upperLayers3);
	Layers3().setLowerLayers(lowerLayers3);
	Layers3().upperLayerType() = OcularLayerType::IOS;
	Layers3().lowerLayerType() = OcularLayerType::BRM;
	return;
}
*/

bool oct_angio::Angiogram::prepareData(bool align)
{
	int lines = Layout().numberOfLines();
	int repeats = Layout().numberOfRepeats();
	int points = Layout().numberOfPoints();

	cpp_util::StopWatch::start();
	LogD() << "Preparing angio data, lines: " << lines << ", points: " << points << ", repeats: " << repeats;

	if (!produceFlowSignals(align)) {
		return false;
	}

	if (!buildProjectionMasks()) {
		return false;
	}

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram data prepared, elapsed: " << msec;
	return true;
}


bool oct_angio::Angiogram::prepareData2(void)
{
	int lines = Layout().numberOfLines();
	int repeats = Layout().numberOfRepeats();
	int points = Layout().numberOfPoints();

	cpp_util::StopWatch::start();
	LogD() << "Preparing angio data2, lines: " << lines << ", points: " << points << ", repeats: " << repeats;

	if (!buildProjectionMasks()) {
		return false;
	}

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram data2 prepared, elapsed: " << msec;
	return true;
}


void oct_angio::Angiogram::alignDataToBase(void)
{
	int lines = Layout().numberOfLines();
	int repeats = Layout().numberOfRepeats();

	/*
	if (useDecorrCircular()) {
		Data().appendAmplitudesToCircular(lines, repeats);
	}
	*/

	if (useAlignAxial() || useAlignLateral()) {
		Data().alignAmplitudes(lines, repeats, useAlignAxial(), useAlignLateral(),
			Layers().upperLayers(), Layers().lowerLayers());
	}
	return;
}

bool oct_angio::Angiogram::produceFlowSignals(bool align)
{
	if (!isAmplitduesValid()) {
		return false;
	}

	if (align) {
		alignDataToBase();
	}

	if (!Decorr().estimateThresholds(Layout(), Data(), Layers())) {
		return false;
	}

	/*
	int overlaps = numberOfOverlaps();
	float thresh1 = intensityUpperThreshold();
	float thresh2 = intensityLowerThreshold();
	bool pixelAvg = usePixelAverage();
	int avgOffset = pixelAverageOffset();
	bool circular = useDecorrCircular();
	bool differ = useDifferOutput();
	float decorrThresh = decorrBaseThreshold();
	bool reflectCorrect = useReflectionCorrection();

	overlaps = (overlaps == 0 ? repeats : overlaps);

	if (!Decorr().calculateDecorrelations(data, reals, imags, Layers(),
		overlaps, thresh1, thresh2, pixelAvg, avgOffset,
		differ, circular, decorrThresh, reflectCorrect)) {
		return false;
	}
	*/
	bool circular = useDecorrCircular();
	bool pixelAvg = usePixelAverage();
	int avgOffset = pixelAverageOffset();

	int overlaps = numberOfOverlaps();
	int repeats = Layout().numberOfRepeats();

	overlaps = (overlaps == 0 ? repeats : overlaps);

	if (!Decorr().calculateSignals(Layout(), Data(), Layers(), overlaps, pixelAvg, avgOffset, circular)) {
		return false;
	}

	return true;
}


bool oct_angio::Angiogram::buildProjectionMasks(void)
{
	if (true) { // useMotionCorrection()) {
		// Projection mask for vascular structure. (NFL~IPL)
		auto uppersV = Layers().getUpperLayersOfVasculature();
		auto lowersV = Layers().getLowerLayersOfVasculature();

		if (!Decorr().updateProjectionMasks(Layout(), uppersV, lowersV)) {
			return false;
		}

		Decorr2().decorrAngiogram() = Decorr().decorrAngiogram();
		Decorr2().differAngiogram() = Decorr().differAngiogram();

		Post().createProjectionMask(Layout(), Decorr2().decorrAngiogram(), Decorr2().decorrProjectionMask(), false);
		Post().createProjectionMask(Layout(), Decorr2().differAngiogram(), Decorr2().differProjectionMask(), false);

		// Projection mask to measure the decorrelation variance. (IOS~BRM)
		auto uppersD = Layers().getUpperLayersOfVariance2();
		auto lowersD = Layers().getLowerLayersOfVariance2();
		auto outerFlow = true;

		if (!Decorr().updateProjectionMasks(Layout(), uppersD, lowersD, outerFlow)) {
			return false;
		}

		Decorr3().decorrAngiogram() = Decorr().decorrAngiogram();
		Decorr3().differAngiogram() = Decorr().differAngiogram();

		Post().createProjectionMask(Layout(), Decorr3().decorrAngiogram(), Decorr3().decorrProjectionMask(), false);
		Post().createProjectionMask(Layout(), Decorr3().differAngiogram(), Decorr3().differProjectionMask(), false);

		auto& maskV = Decorr2().decorrAngiogram();
		auto& maskD = Decorr3().decorrAngiogram();

		bool blank = false;
		blank |= all_of(maskV.begin(), maskV.end(), [](float i) { return i <= 0; });
		blank |= all_of(maskD.begin(), maskD.end(), [](float i) { return i <= 0; });

		if (!blank) {
			Motion().rotateVerticalScan(Layout(), true, maskV);
			Motion().rotateVerticalScan(Layout(), true, maskD);
			Motion().maskMotionAffectedLines2(Layout(), maskV, maskD);

			int w = Layout().getWidth();
			int h = Layout().getHeight();
			Post().performVesselProcessing(w, h, maskV);
		}
		else {
			LogD() << "Projection masks of angiogram invalid!";
		}
	}
	
	return true;
}

bool oct_angio::Angiogram::buildProjectionImages(bool calcStats)
{
	const auto& maskDecorr = Decorr2().decorrProjectionMask();
	const auto& maskDiffer = Decorr2().differProjectionMask();
	bool usePAR = useProjectionArtifactRemoval();

	if (!Decorr().updateProjectionProfiles(Layout(), Layers(), calcStats, usePAR, maskDecorr, maskDiffer)) {
		return false;
	}

	/*
	if (false) {
		auto uppers = Layers().getUpperLayersOfSlab();
		auto lowers = Layers().getLowerLayersOfSlab();

		if (useProjectionArtifactRemoval()) {
			const auto& maskDecorr = Decorr2().decorrProjectionMask();
			const auto& maskDiffer = Decorr2().differProjectionMask();

			if (!Decorr().updateProjectionProfiles(Layout(), uppers, lowers, calcStats, false, reflectCorrect,
				Layers(), maskDecorr, maskDiffer)) {
				return false;
			}
		}
		else {
			if (!Decorr().updateProjectionProfiles(Layout(), uppers, lowers, calcStats, false, reflectCorrect, Layers())) {
				return false;
			}
		}
	}
	*/

	if (calcStats) {
		if (!Decorr().calculateBscanImageStats(Layout())) {
			return false;
		}

		if (!Decorr().calculateProjectionStats()) {
			return false;
		}
	}
	return true;
}

bool oct_angio::Angiogram::processProjectionImages(void)
{
	int width = Data().dataWidth();
	int height = Data().dataHeight();

	int angioWidth = Layout().getWidth();
	int angioHeight = Layout().getHeight();
	bool outerflows = Layers().isOuterRetinaFlows();

	float noiseRate = getImpl().noiseReductionRate;
	Post().applyNoiseReduction2(Layout(), Decorr().differAngiogram(), noiseRate);
	Post().applyNoiseReduction2(Layout(), Decorr().decorrAngiogram(), noiseRate);

	if (usePostProcessing()) {
		if (!outerflows) {
			if (useBiasFieldCorrection()) {
				Motion().correctBiasField(Layout(), Decorr().differAngiogram(), biasFieldSigma(), isFoveaAvascularZone());
				Motion().correctBiasField(Layout(), Decorr().decorrAngiogram(), biasFieldSigma(), isFoveaAvascularZone());
			}
		}
	}

	if (usePostProcessing()) { // useMotionCorrection()) {
		auto& dcs = Decorr().decorrAngiogram();
		auto& dfs = Decorr().differAngiogram();
		auto dvs = Decorr2().decorrAngiogram();

		bool blank = false;
		blank |= all_of(dcs.begin(), dcs.end(), [](float i) { return i <= 0; });
		blank |= all_of(dfs.begin(), dfs.end(), [](float i) { return i <= 0; });
		blank |= all_of(dvs.begin(), dvs.end(), [](float i) { return i <= 0; });

		if (!blank) {
			Motion().rotateVerticalScan(Layout(), true, dcs);
			Motion().rotateVerticalScan(Layout(), true, dfs);

			bool postproc = true;// (Layers().isOuterRetinaFlows() ? false : true);
			bool regist = (useMotionCorrection() && postproc);

			Motion().correctMotionArtifacts2(Layout(), dcs, dfs, dvs, regist);
			Motion().rotateVerticalScan(Layout(), false, dcs);
			Motion().rotateVerticalScan(Layout(), false, dfs);

			if (postproc) {
				std::vector<std::thread> workers;
				for (int t = 0; t < 2; t++) {
					workers.push_back(std::thread([t, angioWidth, angioHeight, &dcs, &dfs, outerflows, this]() {
						if (t == 0) {
							Post().performPostProcessing(angioWidth, angioHeight, dcs);
						}
						else {
							Post().performPostProcessing(angioWidth, angioHeight, dfs, outerflows);
						}
					}));
				}
				std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
					t.join();
				});
			}
		}
	}

	return true;
}


bool oct_angio::Angiogram::normalizeProjectionImages(void)
{
	int width = Data().dataWidth();
	int height = Data().dataHeight();

	int angioWidth = Layout().getWidth();
	int angioHeight = Layout().getHeight();

	float decorrMin = decorrLowerThreshold();
	float decorrMax = decorrUpperThreshold();
	float differMin = differLowerThreshold();
	float differMax = differUpperThreshold();

	if (useNormProjection()) {
		Decorr().normalizeProjectionProfiles(Layout(), Layers(), decorrMin, decorrMax, differMin, differMax, normalizeDropOff());
	}
	else {
		Decorr().denoiseProjectionProfiles(angioWidth, angioHeight, decorrMin, decorrMax);
	}

	bool decorrout = useDecorrOutput(); 
	if (WsoSettings::getInstance()->isUserModeOn()) {
		decorrout = false;
		if (Layers().isOuterRetinaFlows()) {
			// if (!Layers().isChoroidalFlows()) {
				decorrout = true;
			// }
		}
	}

	buildImageBitmap(decorrout);
	return true;
}


bool oct_angio::Angiogram::processData(bool calcStats)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();
	auto& data = Data().getAmplitudes();	
	bool reflectCorrect = useReflectionCorrection();

	cpp_util::StopWatch::start();
	LogD() << "Processing angio data, lines: " << lines << ", points: " << points;

	auto setts = WsoSettings::getInstance();
	if (setts->isUserModeOn()) {
		biasFieldSigma() = OctAngioSetup::getBiasFieldSigma();
		Post().garborFilerOrients() = OctAngioSetup::getFilterOrients();
		Post().garborFilterDivider() = OctAngioSetup::getFilterDivider();
		Post().garborFilterSigma() = OctAngioSetup::getFilterSigma();
		Post().garborFilterWeight() = OctAngioSetup::getFilterWeight();
		calcStats = false;
	}
	// calcStats = false;

	if (!buildProjectionImages(calcStats)) {
		return false;
	}

	if (!processProjectionImages()) {
		return false;
	}

	if (!normalizeProjectionImages()) {
		return false;
	}

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram updated, elapsed: " << msec;
	return true;
}


bool oct_angio::Angiogram::processData2(void)
{
	int lines = Layout().numberOfLines();
	int points = Layout().numberOfPoints();
	int repeats = Layout().numberOfRepeats();
	auto& data = Data().getAmplitudes();
	bool reflectCorrect = useReflectionCorrection();

	cpp_util::StopWatch::start();
	LogD() << "Processing angiogram, lines: " << lines << ", points: " << points;

	auto setts = WsoSettings::getInstance();
	if (setts->isUserModeOn()) {
		biasFieldSigma() = OctAngioSetup::getBiasFieldSigma();
		Post().garborFilerOrients() = OctAngioSetup::getFilterOrients();
		Post().garborFilterDivider() = OctAngioSetup::getFilterDivider();
		Post().garborFilterSigma() = OctAngioSetup::getFilterSigma();
		Post().garborFilterWeight() = OctAngioSetup::getFilterWeight();
	}

	if (!buildProjectionImages(false)) {
		return false;
	}
	if (!processProjectionImages()) {
		return false;
	}
	if (!normalizeProjectionImages()) {
		return false;
	}
	
	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angiogram updated, elapsed: " << msec;
	return true;
}


bool oct_angio::Angiogram::buildImageBitmap(bool decorr)
{
	int width = imageWidth();
	int height = imageHeight();
	int size = width * height;
	vector<float> output;

	if (decorr) {
		output = Decorr().decorrAngiogram();
	}
	else {
		output = Decorr().differAngiogram();
	}

	auto grays = vector<unsigned char>(size);
	int index;
	float dcval = 0.0f;
	for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++) {
			index = r * width + c;
			dcval = output[index];
			grays[index] = (unsigned char)min(max((int)(dcval * 255.0f), 0), 255);
		}
	}

	getImpl().imageBits = grays;
	return true;
}


const unsigned char * oct_angio::Angiogram::imageBits(void)
{
	if (getImpl().imageBits.size() <= 0) {
		return nullptr;
	}
	return &(getImpl().imageBits[0]);
}


int oct_angio::Angiogram::imageWidth(void) 
{
	return (Layout().isVerticalScan() ? Layout().numberOfLines() : Layout().numberOfPoints());
}


int oct_angio::Angiogram::imageHeight(void) 
{
	return (Layout().isVerticalScan() ? Layout().numberOfPoints() : Layout().numberOfLines());
}


cpp_util::CvImage oct_angio::Angiogram::createAnigoImage(bool enhance)
{
	int width = imageWidth();
	int height = imageHeight();
	int size = width * height;

	auto bits = imageBits();
	if (!bits) {
		return cpp_util::CvImage();
	}

	CvImage image;
	image.fromBitsData(&bits[0], width, height);
	if (enhance) {
		image.equalizeHistogram(4.0f);
	}
	return image;
}


cpp_util::CvImage oct_angio::Angiogram::createOffsetImage(void)
{
	int width = imageWidth();
	int height = imageHeight();
	int size = width * height;

	auto& offsets = Decorr().decorrAxialOffsets();
	if (offsets.size() != size) {
		return cpp_util::CvImage();
	}

	CvImage image;
	image.fromFloat32((const unsigned char*)&offsets[0], width, height);
	return image;
}


cpp_util::CvImage oct_angio::Angiogram::createDecorrImage(int lineIdx)
{
	return Decorr().getDecorrelationImage(lineIdx);
}


cpp_util::CvImage oct_angio::Angiogram::createScanImage(int lineIdx)
{
	int width = scanImageWidth();
	int height = scanImageHeight();
	int size = width * height;

	auto bits = getScanImageBits(lineIdx, 0);
	if (!bits) {
		return cpp_util::CvImage();
	}

	CvImage image;
	image.fromBitsData(&bits[0], width, height);
	return image;
}


std::vector<float> oct_angio::Angiogram::getDecorrelationsOnHorzLines(bool axialMax)
{
	int width = imageWidth();
	int height = imageHeight();

	auto& data = (axialMax ? Decorr().decorrProjectionMax() : Decorr().decorrAngiogram());
	if (data.empty() || data.size() != width * height) {
		return std::vector<float>();
	}

	auto decors = vector<float>(height);
	for (int r = 0; r < height; r++) {
		decors[r] = std::accumulate(begin(data) + r*width, data.begin() + (r + 1)*width, 0.0f) / width;
	}
	return decors;
}


std::vector<float> oct_angio::Angiogram::getDecorrelationsOnVertLines(bool axialMax)
{
	int width = imageWidth();
	int height = imageHeight();

	auto image = Decorr().decorrAngiogramImage();
	image.rotate90();

	const auto& data = image.copyDataInFloats();
	if (data.empty() || data.size() != width * height) {
		return std::vector<float>();
	}

	auto decors = vector<float>(width);
	for (int r = 0; r < width; r++) {
		decors[r] = std::accumulate(begin(data) + r*height, data.begin() + (r + 1)*height, 0.0f) / height;
	}
	return decors;
}


std::vector<int> oct_angio::Angiogram::getUpperLayerPoints(int imageIdx)
{
	if (!isAngioImage()) {
		return std::vector<int>();
	}

	auto layers = Layers().getUpperLayersOfSlab();
	if (layers.size() > imageIdx) {
		return layers[imageIdx];
	}
	return std::vector<int>();
}


std::vector<int> oct_angio::Angiogram::getLowerLayerPoints(int imageIdx)
{
	if (!isAngioImage()) {
		return std::vector<int>();
	}

	auto layers = Layers().getLowerLayersOfSlab();
	if (layers.size() > imageIdx) {
		return layers[imageIdx];
	}
	return std::vector<int>();
}


int oct_angio::Angiogram::numberOfDecorrImages(void)
{
	return (int)Data().getAmplitudes().size();
}


int oct_angio::Angiogram::numberOfOverlapImages(void)
{
	if (numberOfDecorrImages() > 0) {
		return (int)Data().getAmplitudes()[0].size();
	}
	return 0;
}


float * oct_angio::Angiogram::getDecorrImageBits(int lineIdx)
{
	return Decorr().getDecorrelationData(lineIdx);
}


unsigned char* oct_angio::Angiogram::getScanImageBits(int lineIdx, int repeatIdx)
{
	return Data().getGrayscaledDataBits(lineIdx, repeatIdx);
}

int oct_angio::Angiogram::scanImageWidth(void)
{
	return Data().dataWidth();
}

int oct_angio::Angiogram::scanImageHeight(void)
{
	return Data().dataHeight();
}

float oct_angio::Angiogram::scanRangeX(void)
{
	return Layout().scanRangeX();
}

float oct_angio::Angiogram::scanRangeY(void)
{
	return Layout().scanRangeY();
}


bool & oct_angio::Angiogram::useAlignAxial(void)
{
	return getImpl().useAlignAxial;
}

bool & oct_angio::Angiogram::useAlignLateral(void)
{
	return getImpl().useAlignLateral;
}

bool & oct_angio::Angiogram::useDecorrCircular(void)
{
	return getImpl().useDecorrCircular;
}

bool & oct_angio::Angiogram::useLayersSelected(void)
{
	return getImpl().useLayersSelected;
}

bool & oct_angio::Angiogram::useVascularLayers(void)
{
	return getImpl().useVascularLayers;
}

bool & oct_angio::Angiogram::useMotionCorrection(void)
{
	return getImpl().useMotionCorrect;
}

bool & oct_angio::Angiogram::useBiasFieldCorrection(void)
{
	return getImpl().useBiasFieldCorrect;
}

bool & oct_angio::Angiogram::usePostProcessing(void)
{
	return getImpl().usePostProcessing;
}

bool & oct_angio::Angiogram::useNormProjection(void)
{
	return getImpl().useNormProjection;
}

bool & oct_angio::Angiogram::useDecorrOutput(void)
{
	return getImpl().useDecorrOutput;
}

bool & oct_angio::Angiogram::useDifferOutput(void)
{
	return getImpl().useDifferOutput;
}

bool & oct_angio::Angiogram::useProjectionArtifactRemoval(void)
{
	return getImpl().useProjectionRemoval;
}

bool & oct_angio::Angiogram::useReflectionCorrection(void)
{
	return getImpl().useReflectCorrect;
}

bool & oct_angio::Angiogram::useProjectionStatistics(void)
{
	return getImpl().useProjectionStats;
}

float & oct_angio::Angiogram::intensityUpperThreshold(void)
{
	return getImpl().upperThreshold;
}

float & oct_angio::Angiogram::intensityLowerThreshold(void)
{
	return getImpl().lowerThreshold;
}

float & oct_angio::Angiogram::decorrLowerThreshold(void)
{
	return getImpl().decorrLowerThreshold;
}

float & oct_angio::Angiogram::decorrUpperThreshold(void)
{
	return getImpl().decorrUpperThreshold;
}

float & oct_angio::Angiogram::differLowerThreshold(void)
{
	return getImpl().differLowerThreshold;
}

float & oct_angio::Angiogram::differUpperThreshold(void)
{
	return getImpl().differUpperThreshold;
}

float & oct_angio::Angiogram::decorrBaseThreshold(void)
{
	return getImpl().decorrBaseThreshold;
}

float & oct_angio::Angiogram::biasFieldSigma(void)
{
	return getImpl().biasFieldSigma;
}

float & oct_angio::Angiogram::noiseReductionRate(void)
{
	return getImpl().noiseReductionRate;
}

float & oct_angio::Angiogram::normalizeDropOff(void)
{
	return getImpl().normalizeDropOff;
}

int & oct_angio::Angiogram::numberOfOverlaps(void)
{
	return getImpl().numberOfOverlaps;
}

bool & oct_angio::Angiogram::usePixelAverage(void)
{
	return getImpl().usePixelAverage;
}

int & oct_angio::Angiogram::pixelAverageOffset(void)
{
	return getImpl().pixelAverageOffset;
}

oct_angio::AngioLayout & oct_angio::Angiogram::Layout(void)
{
	return getImpl().layout;
}


oct_angio::AngioData & oct_angio::Angiogram::Data(void)
{
	return getImpl().data;
}

oct_angio::AngioLayers & oct_angio::Angiogram::Layers(void)
{
	return getImpl().layers;
}

/*
oct_angio::AngioLayers & oct_angio::Angiogram::Layers2(void)
{
	return getImpl().layers2;
}

oct_angio::AngioLayers & oct_angio::Angiogram::Layers3(void)
{
	return getImpl().layers3;
}
*/

oct_angio::AngioDecorr & oct_angio::Angiogram::Decorr(void)
{
	return getImpl().decorr;
}

oct_angio::AngioDecorr & oct_angio::Angiogram::Decorr2(void)
{
	return getImpl().decorr2;
}

oct_angio::AngioDecorr & oct_angio::Angiogram::Decorr3(void)
{
	return getImpl().decorr3;
}

oct_angio::AngioMotion & oct_angio::Angiogram::Motion(void)
{
	return getImpl().motion;
}


oct_angio::AngioPost & oct_angio::Angiogram::Post(void)
{
	return getImpl().post;
}

Angiogram::AngiogramImpl & oct_angio::Angiogram::getImpl(void) const
{
	return *d_ptr;
}
