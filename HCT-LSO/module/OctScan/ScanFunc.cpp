#include "pch.h"
#include "ScanFunc.h"

#include <iostream>
#include <fstream>

#include "oct_grab.h"
#include "oct_pattern.h"
#include "sig_chain.h"

using namespace oct_scan;
using namespace oct_grab;
using namespace oct_pattern;
using namespace sig_chain;


struct ScanFunc::ScanFuncImpl
{
	MainBoard* board;
	Usb3Grabber* usb3Grab;
	Galvanometer* galvano;

	bool initiated;

	ScanFuncImpl() : board(nullptr), usb3Grab(nullptr), galvano(nullptr), initiated(false)
	{
	}
};


ScanFunc::ScanFunc()
{
}


ScanFunc::~ScanFunc()
{
}


bool oct_scan::ScanFunc::initializeScanFunc(wso_device::MainBoard * board, oct_grab::Usb3Grabber * grabber)
{
	getImpl().board = board;
	getImpl().galvano = board->getGalvanometer();
	getImpl().usb3Grab = grabber;
	getImpl().initiated = true;
	return true;
}


bool oct_scan::ScanFunc::isInitialized(void)
{
	return getImpl().initiated;
}


bool oct_scan::ScanFunc::prepareChainSetup(OctScanPattern pattern, bool measure)
{
	if (!measure) {
		ChainSetup::setScanPattern(pattern);
		ChainSetup::updateSpectrometerParameters();
		ChainSetup::updateDispersionParameters();
		ChainSetup::clearBackgroundSpectrum();
		ChainSetup::resetDynamicDispersionCorrection();

		// if (d_ptr->scanMeasure.getPattern()._name == PatternName::AnteriorLine) {
		ChainSetup::useACALinePatternPreview(true, true);
		// }
	}
	else {
		ChainSetup::useACALinePatternPreview(true, false);
	}
	return true;
}


bool oct_scan::ScanFunc::updateLineTrace(oct_pattern::LineTrace* line, OctScanSpeed speed, bool foreDist)
{
	if (!line) {
		return false;
	}

	auto forePadds = getGalvanometer()->getTriggerForePaddings(speed);
	auto postPadds = getGalvanometer()->getTriggerPostPaddings(speed);
	auto scanPoints = line->getNumberOfScanPoints();
	auto timeStep = getGalvanometer()->getTriggerTimeStep();

	if (foreDist) {
		forePadds = line->getForePaddings();
	}

	line->setPaddings(forePadds, postPadds, scanPoints);
	line->setScanSpeed(speed);
	return true;
}


bool oct_scan::ScanFunc::updatePatternPositions(EyeSide eyeSide, const PatternPlan & pattern, OctScanSpeed speed)
{
	if (!updatePreviewPositions(eyeSide, pattern, speed)) {
		return false;
	}
	if (!updateMeasurePositions(eyeSide, pattern, speed)) {
		return false;
	}
	if (!updateEnfacePositions(eyeSide, pattern, speed)) {
		return false;
	}
	return true;
}


bool oct_scan::ScanFunc::updatePreviewPositions(EyeSide eyeSide, const PatternPlan & pattern, OctScanSpeed speed)
{
	if (!isInitialized()) {
		return false;
	}

	bool cornea = pattern.isCornea();
	PatternFrameVect& frames = pattern.getPreviewScan().getPatternFrames();
	size_t fsize = frames.size();

	LogD() << "---------------------------------------------------------------------";
	LogD() << "Update preview positions, frame size: " << fsize << ", cornea: " << cornea;
	LogD() << "---------------------------------------------------------------------";
	for (int i = 0; i < fsize; i++) {
		LineTraceVect& lines = frames[i].getLineTraceList();
		size_t lsize = lines.size();
		for (int j = 0; j < lsize; j++) {
			LineTrace* pLine = &lines[j];
			updateLineTrace(pLine, speed);
			if (!updateTracePositions(eyeSide, pattern, *pLine, cornea)) {
				LogE() << "Update trace positions in preview failed!, frame: " << i << ", line: " << j;
				return false;
			}
		}
	}
	return true;
}


bool oct_scan::ScanFunc::updateMeasurePositions(EyeSide eyeSide, const PatternPlan & pattern, OctScanSpeed speed)
{
	if (!isInitialized()) {
		return false;
	}

	bool cornea = pattern.isCornea();
	PatternFrameVect& frames = pattern.getMeasureScan().getPatternFrames();
	size_t fsize = frames.size();

	LogD() << "---------------------------------------------------------------------";
	LogD() << "Update measure positions, frame size: " << fsize << ", cornea: " << cornea;
	LogD() << "---------------------------------------------------------------------";
	for (int i = 0; i < fsize; i++) {
		LineTraceVect& lines = frames[i].getLineTraceList();
		size_t lsize = lines.size();
		for (int j = 0; j < lsize; j++) {
			LineTrace* pLine = &lines[j];
			updateLineTrace(pLine, speed, pattern.getMeasureScan().useForeDistance());
			if (!updateTracePositions(eyeSide, pattern, *pLine, cornea)) {
				LogE() << "Update trace positions in measure failed!, frame: " << i << ", line: " << j;
				return false;
			}
		}
	}
	return true;
}


bool oct_scan::ScanFunc::updateEnfacePositions(EyeSide eyeSide, const PatternPlan & pattern, OctScanSpeed speed)
{
	if (!isInitialized()) {
		return false;
	}

	bool cornea = pattern.isCornea();
	PatternFrameVect& frames = pattern.getEnfaceScan().getPatternFrames();
	size_t fsize = frames.size();

	LogD() << "---------------------------------------------------------------------";
	LogD() << "Update enface positions, frame size: " << fsize << ", cornea: " << cornea;
	LogD() << "---------------------------------------------------------------------";
	for (int i = 0; i < fsize; i++) {
		LineTraceVect& lines = frames[i].getLineTraceList();
		size_t lsize = lines.size();
		for (int j = 0; j < lsize; j++) {
			LineTrace* pLine = &lines[j];
			updateLineTrace(pLine, speed);
			if (!updateTracePositions(eyeSide, pattern, *pLine, cornea)) {
				LogE() << "Update trace positions in enface failed!, frame: " << i << ", line: " << j;
				return false;
			}
		}
	}
	return true;
}


bool oct_scan::ScanFunc::updateTracePositions(EyeSide eyeSide, const PatternPlan& pattern, oct_pattern::LineTrace & line, bool cornea)
{
	Galvanometer* galvano = getGalvanometer();

	short forePadds = line.getForePaddings();
	short numPoints = line.getNumberOfScanPoints();
	short postPadds = line.getPostPaddings();

	float startX = line.getStartX();
	float closeX = line.getCloseX();
	float startY = line.getStartY();
	float closeY = line.getCloseY();
	bool circle = line.isCircle();
	auto traceId = line.getTraceId();

	static short buffer[GALVANO_TRACE_POSITION_BUFFER_SIZE] = { 0 };
	short count;

	LogD() << "Trace line, trace-id: " << traceId << ", x1: " << startX << " = > x2: " << closeX << ", y1 : " << startY << " = > y2: " << closeY;

	/*
	// Reverse direction of x, y coordinate
	startX *= -1.0f;
	closeX *= -1.0f;
	startY *= -1.0f;
	closeY *= -1.0f;

	LogD() << "Reversed, x1: " << startX << " => x2: " << closeX << ", y1: " << startY << " => y2: " << closeY;
	*/

	if (!galvano->buildTracePositionsX(eyeSide, forePadds, numPoints, postPadds, startX, closeX, cornea, circle, buffer, &count)) {
		return false;
	}

	auto x1 = buffer[0];
	auto x2 = buffer[count - 1];

	if (line.isHidden()) {
		for (int i = 0; i < count; i++) {
			buffer[i] = GALVANO_STEP_RANGE_MIN;
		}
		LogD() << "Galvano x-coords, pos1: " << x1 << ", pos2: " << x2 << ", count: " << count << ", hidden: true";
	}
	else {
		// Apply scan offset with amount of galvano positions.
		auto offset = (short)pattern.getMeasureScan().getScanMoveX();
		for (int i = 0; i < count; i++) {
			buffer[i] += offset;
		}
		LogD() << "Galvano x-coords, pos1: " << x1 << ", pos2: " << x2 << ", count: " << count << ", shift: " << offset;
	}

	line.setGalvanoPositionsX(buffer, count);

	if (!galvano->buildTracePositionsY(eyeSide, forePadds, numPoints, postPadds, startY, closeY, cornea, circle, buffer, &count)) {
		return false;
	}

	auto y1 = buffer[0];
	auto y2 = buffer[count - 1];

	if (line.isHidden()) {
		for (int i = 0; i < count; i++) {
			buffer[i] = GALVANO_STEP_RANGE_MIN;
		}
		LogD() << "Galvano y-coords, pos1: " << y1 << ", pos2: " << y2 << ", count: " << count << ", hidden: true";
	}
	else {
		// Apply scan offset with amount of galvano positions.
		auto offset = (short)pattern.getMeasureScan().getScanMoveY();
		for (int i = 0; i < count; i++) {
			buffer[i] += offset;
		}
		LogD() << "Galvano y-coords, pos1: " << y1 << ", pos2: " << y2 << ", count: " << count << ", shift: " << offset;
	}

	line.setGalvanoPositionsY(buffer, count);
	return true;
}


bool oct_scan::ScanFunc::exportPatternPositions(const oct_pattern::PatternPlan & pattern, std::string filename)
{
	ofstream file(filename);
	if (file.is_open()) {
		file << "[Preview Pattern]\n";
		auto frame = pattern.getPreviewScan().getPatternFrame(0);
		if (frame != nullptr) {
			auto line = frame->getLineTrace(0);
			if (line != nullptr) {
				auto xpos = line->getGalvanoPositionsX();
				auto xcnt = line->getCountOfPositionsX();
				file << "Galvano Position Xs, size: " << xcnt << "\n";
				for (int i = 0; i < xcnt; i++) {
					file << i << " : " << xpos[i] << "\n";
				}
				auto ypos = line->getGalvanoPositionsY();
				auto ycnt = line->getCountOfPositionsY();
				file << "Galvano Position Ys, size: " << xcnt << "\n";
				for (int i = 0; i < ycnt; i++) {
					file << i << " : " << ypos[i] << "\n";
				}
			}
		}
		file.close();
	}
	else {
		return false;
	}
	return true;
}


bool oct_scan::ScanFunc::uploadPatternProfiles(const PatternPlan & pattern, bool measure)
{
	if (!measure) {
		// Upload the first lines of preview frame with trace profile index into mainboard.
		if (!uploadPreviewProfiles(pattern)) {
			return false;
		}

		if (!uploadEnfaceProfiles(pattern)) {
			return false;
		}
	}
	else {
		// Upload the first lines of measure frame with trace profile index into mainboard. 
		// The following lines within frame are operated through control command parameters
		// of scan ACQ with number of scan lines and x, y offsets. 

		// Note that in case the following line trajectory within frame can't be covered 
		// via x, y offsets from the previous line's, then it should be added as next frame. 
		if (!uploadMeasureProfiles(pattern)) {
			return false;
		}
	}
	return true;
}


bool oct_scan::ScanFunc::uploadPreviewProfiles(const PatternPlan & pattern)
{
	if (!isInitialized()) {
		return false;
	}

	PatternFrameVect& frames = pattern.getPreviewScan().getPatternFrames();
	size_t fsize = frames.size();

	LogD() << "---------------------------------------------------------------------";
	LogD() << "Upload preview profiles, frame-size: " << fsize;
	LogD() << "---------------------------------------------------------------------";
	for (int i = 0; i < fsize; i++) {
		LineTrace* pLine = frames[i].getLineTraceFirst();
		if (pLine != nullptr) {
			if (uploadTraceProfile(*pLine)) {
				LogD() << "Trace profile uploaded for preview, frame: " << i << ", traceId: " << pLine->getTraceId();
			}
			else {
				LogE() << "Upload trace profile for preview failed!, frame: " << i << ", traceId: " << pLine->getTraceId();
				return false;
			}
		}
		else {
			LogE() << "Invalid trace profile to preview!, frame: " << i ;
			return false;
		}
	}
	return true;
}


bool oct_scan::ScanFunc::uploadMeasureProfiles(const PatternPlan & pattern, int startIdx, int maxLines)
{
	if (!isInitialized()) {
		return false;
	}

	PatternFrameVect& frames = pattern.getMeasureScan().getPatternFrames();
	size_t fsize = frames.size();

	LogD() << "---------------------------------------------------------------------";
	LogD() << "Upload measure profiles, frame-size: " << fsize;
	LogD() << "---------------------------------------------------------------------";

	int count = 0;
	for (int i = startIdx; i < fsize; i++) {
		LineTrace* pLine = frames[i].getLineTraceFirst();
		if (pLine != nullptr) {
			if (uploadTraceProfile(*pLine)) {
				LogD() << "Trace profile uploaded for measure, frame: " << i << ", traceId: " << pLine->getTraceId();
			}
			else {
				LogE() << "Upload trace profile for measure failed!, frame: " << i << ", traceId: " << pLine->getTraceId();
				return false;
			}
		}
		else {
			LogE() << "Invalid trace profile for measure!, frame: " << i ;
			return false;
		}
		if (++count >= maxLines) {
			break;
		}
	}
	return true;
}


bool oct_scan::ScanFunc::uploadEnfaceProfiles(const PatternPlan & pattern)
{
	if (!isInitialized()) {
		return false;
	}

	PatternFrameVect& frames = pattern.getEnfaceScan().getPatternFrames();
	size_t fsize = frames.size();

	LogD() << "---------------------------------------------------------------------";
	LogD() << "Upload enface profiles, frame-size: " << fsize;
	LogD() << "---------------------------------------------------------------------";

	for (int i = 0; i < fsize; i++) {
		LineTrace* pLine = frames[i].getLineTraceFirst();
		if (pLine != nullptr) {
			if (uploadTraceProfile(*pLine)) {
				LogD() << "Trace profile uploaded for enface, frame: " << i << ", traceId: " << pLine->getTraceId();
			}
			else {
				LogE() << "Upload trace profile for enface failed!, frame: " << i << ", traceId: " << pLine->getTraceId();
				return false;
			}
		}
		else {
			LogE() << "Invalid trace profile for enface!, frame: " << i ;
			return false;
		}
	}
	return true;
}


bool oct_scan::ScanFunc::uploadTraceProfile(oct_pattern::LineTrace & line)
{
	Galvanometer* galvano = getGalvanometer();

	int traceId = line.getTraceId();
	short forePadds = line.getForePaddings();
	short numPoints = line.getNumberOfScanPoints();
	short postPadds = line.getPostPaddings();
	short numRepeats = line.getNumberOfRepeats();

	float startX = line.getStartX();
	float closeX = line.getCloseX();
	float startY = line.getStartY();
	float closeY = line.getCloseY();

	short* bufferX = nullptr;
	short* bufferY = nullptr;
	int countX = 0;
	int countY = 0;

	float timeStep = galvano->getTriggerTimeStep();
	uint32_t timeDelay = galvano->getTriggerTimeDelay();

	// Transmit a trace profile with id into mainboard. 
	if (!galvano->setTraceProfile(traceId, forePadds, numPoints, postPadds, numRepeats, timeStep, timeDelay)) {
		return false;
	}

	// Transmit x coordinates along the line. 
	bufferX = line.getGalvanoPositionsX();
	countX = line.getCountOfPositionsX();
	if (!galvano->setTracePositionsX(traceId, bufferX, countX)) {
		return false;
	}

	// Transmit y coordinates along the line. 
	bufferY = line.getGalvanoPositionsY();
	countY = line.getCountOfPositionsY();
	if (!galvano->setTracePositionsY(traceId, bufferY, countY)) {
		return false;
	}

	auto x1 = line.getPositionFirstX();
	auto x2 = line.getPositionLastX();
	auto y1 = line.getPositionFirstY();
	auto y2 = line.getPositionLastY();

	LogD() << "Upload profile, trace-id: " << traceId << ", fore-padd: " << forePadds << ", n-points: " << numPoints << ", post-padd: " << postPadds << ", n-repeats: " << numRepeats;
	LogD() << "Galvano x-coords, pos1: " << x1 << ", pos2: " << x2 << ", x1: " << startX << " = > x2: " << closeX;
	LogD() << "Galvano y-coords, pos1: " << y1 << ", pos2: " << y2 << ", y1: " << startY << " = > y2: " << closeY;
	LogD() << "Galvano time-step: " << timeStep << ", time-delay: " << timeDelay;
	LogD() << "RasterFastX: " << line.isRasterFastX() << ", RasterFastY: " << line.isRasterFastY() << ", RasterX: " << line.isRasterX() << ", RasterY: " << line.isRasterY();

	if (line.isRasterFastX() || line.isRasterFastY() || line.isRasterX() || line.isRasterY())
	{
		// Transmit a trace profile with id into mainboard. 
		if (!galvano->setTraceProfile(traceId + 1, forePadds, numPoints, postPadds, numRepeats, timeStep, timeDelay)) {
			return false;
		}

		// Transmit x coordinates along the line in reversed direction.
		bufferX = line.getGalvanoPositionsX(true);
		countX = line.getCountOfPositionsX();
		if (!galvano->setTracePositionsX(traceId + 1, bufferX, countX)) {
			return false;
		}

		// Transmit y coordinates along the line in reversed direction.
		bufferY = line.getGalvanoPositionsY(true);
		countY = line.getCountOfPositionsY();
		if (!galvano->setTracePositionsY(traceId + 1, bufferY, countY)) {
			return false;
		}
	}
	return true;
}


bool oct_scan::ScanFunc::updatePatternBuffers(const PatternPlan & pattern, bool measure, bool reset)
{
	if (reset) {
		releaseGrabberBuffers();
	}

	if (!measure) {
		if (!updatePreviewBuffers(pattern)) {
			return false;
		}
		if (!updateEnfaceBuffers(pattern)) {
			return false;
		}
	}
	else {
		if (!updateMeasureBuffers(pattern)) {
			return false;
		}
	}
	return true;
}


bool oct_scan::ScanFunc::updatePreviewBuffers(const PatternPlan & pattern)
{
	if (!isInitialized()) {
		return false;
	}

	// vector<int> sizes = pattern.getPreviewScan().getLateralSizeListOfFrame(0, false);
	
	PatternFrameVect& frames = pattern.getPreviewScan().getPatternFrames();
	vector<int> sizes;

	for (PatternFrame& frame : frames) {
		auto line = frame.getLineTraceFirst();
		if (!line->isHidden()) {
			vector<int> fsize = frame.getLateralSizeList(true);
			sizes.insert(end(sizes), begin(fsize), end(fsize));
		}
	}

	if (!getUsb3Grabber()->prepareBuffersToPreview(sizes)) {
		getUsb3Grabber()->releaseBuffersToPreview();
		return false;
	}
	return true;
}


bool oct_scan::ScanFunc::updateMeasureBuffers(const PatternPlan & pattern)
{
	if (!isInitialized()) {
		return false;
	}

	PatternFrameVect& frames = pattern.getMeasureScan().getPatternFrames();
	vector<int> sizes;

	int count = 0;
	int depth = 0;
	for (PatternFrame& frame : frames) {
		vector<int> fsize = frame.getLateralSizeList(true);
		if (depth == 0 && pattern.getMeasureScan().isScan3D()) {
			depth = (int)fsize.size();
			LogD() << "Pattern depth: " << depth;
		}
		sizes.insert(end(sizes), begin(fsize), end(fsize));
	}

	if (!getUsb3Grabber()->prepareBuffersToMeasure(sizes, depth)) {
		getUsb3Grabber()->releaseBuffersToMeasure();
		return false;
	}

	return true;
}


bool oct_scan::ScanFunc::updateEnfaceBuffers(const PatternPlan & pattern)
{
	if (!isInitialized()) {
		return false;
	}

	vector<int> sizes = pattern.getEnfaceScan().getLateralSizeListOfFrame(0, false);

	if (!getUsb3Grabber()->prepareBuffersToEnface(sizes)) {
		getUsb3Grabber()->releaseBuffersToEnface();
		return false;
	}

	return true;
}


void oct_scan::ScanFunc::releaseGrabberBuffers(void)
{
	getUsb3Grabber()->releaseBuffersToPattern();
	return;
}


oct_grab::Usb3Grabber * oct_scan::ScanFunc::getUsb3Grabber(void)
{
	return getImpl().usb3Grab;
}


wso_device::Galvanometer * oct_scan::ScanFunc::getGalvanometer(void)
{
	return getImpl().galvano;
}


// Direct initialization of static smart pointer.
std::unique_ptr<ScanFunc::ScanFuncImpl> ScanFunc::d_ptr(new ScanFuncImpl());


ScanFunc::ScanFuncImpl & oct_scan::ScanFunc::getImpl(void)
{
	return *d_ptr;
}
