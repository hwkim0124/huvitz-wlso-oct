#include "pch.h"
#include "ScanGrab.h"
#include "ScanFunc.h"
#include "ScanOptimizer.h"

#include <mutex>
#include <atomic>
#include <functional>


#include "oct_grab.h"
#include "oct_pattern.h"
#include "sig_chain.h"

using namespace oct_scan;
using namespace oct_grab;
using namespace oct_pattern;
using namespace sig_chain;


std::mutex ScanGrab::singleMutex_;


struct ScanGrab::ScanGrabImpl
{
	MainBoard* board;
	Usb3Grabber* usb3Grab;
	Galvanometer* galvano;
	PatternPlan* pattern;
	OctProcess* process;

	bool initiated;
	bool isNotGrabbingImage;

	OctScanProtocol* scanProtocol;
	OctScanSpeed scanSpeed;
	float triggerTimeStep;
	uint32_t triggerTimeDelay;

	ULONGLONG tickMsrAcqStart;
	ULONGLONG tickMsrAcqEnd;

	bool repeatOfPreviews;
	int countOfPreviews;
	int countOfMeasures;
	int countOfEnfaces;
	int countOfGrabErrors;

	mutex mutexGrab;
	atomic<bool> previewRun;
	atomic<bool> measureRun;
	atomic<bool> grabberRun;
	atomic<bool> measureCanceled;
	atomic<bool> measureCompleted;
	thread threadGrab;

	ScanGrabImpl() : galvano(nullptr), usb3Grab(nullptr), process(nullptr), pattern(nullptr), initiated(false),
		scanSpeed(OctScanSpeed::UNKNOWN), triggerTimeStep(TRAJECT_TIME_STEP_IN_US), triggerTimeDelay(TRAJECT_TRIGGER_DELAY),
		previewRun(false), measureRun(false), grabberRun(false), isNotGrabbingImage(false), 
		countOfPreviews(0), countOfMeasures(0), countOfEnfaces(0), countOfGrabErrors(0), 
		tickMsrAcqStart(0), tickMsrAcqEnd(0), repeatOfPreviews(false), measureCompleted(false), measureCanceled(false)
	{
	}
};


ScanGrab::ScanGrab() :
	d_ptr(std::make_unique<ScanGrabImpl>())
{
}


ScanGrab::~ScanGrab()
{
	if (impl().threadGrab.joinable()) {
		// impl().threadGrab.join();
		impl().threadGrab.detach();
	}
}


ScanGrab* oct_scan::ScanGrab::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static ScanGrab instance;
	return &instance;
}


void oct_scan::ScanGrab::initiateScanGrab(wso_device::MainBoard * board, oct_grab::Usb3Grabber * grabber)
{
	impl().board = board;
	impl().galvano = board->getGalvanometer();
	impl().usb3Grab = grabber;
	impl().initiated = true;

	if (grabber) {
		auto func1 = std::bind(&ScanGrab::onOctPreviewBufferGrabbed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		auto func2 = std::bind(&ScanGrab::onOctMeasureBufferGrabbed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		auto func3 = std::bind(&ScanGrab::onOctEnfaceBufferGrabbed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		grabber->setGrabPreviewBufferCallback(func1);
		grabber->setGrabMeasureBufferCallback(func2);
		grabber->setGrabEnfaceBufferCallback(func3);
	}

	return;
}

void oct_scan::ScanGrab::releaseScanGrab(void)
{
	/*
	if (auto* p = EventRegistry::getInstance(); p) {
		p->setOctPreviewBufferGrabbed();
		p->setOctMeasureBufferGrabbed();
		p->setOctMeasureBufferGrabbed();
	}
	*/

	closeScanGrab(false);

	if (auto grabber = impl().usb3Grab; grabber) {
		grabber->setGrabPreviewBufferCallback(nullptr);
		grabber->setGrabMeasureBufferCallback(nullptr);
		grabber->setGrabEnfaceBufferCallback(nullptr);
	}

	impl().initiated = false;
	return;
}


bool oct_scan::ScanGrab::grabPreview(oct_pattern::PatternPlan* pattern, sig_chain::OctProcess* process, bool isNotGrabbingImage)
{
	impl().pattern = pattern;
	impl().process = process;
	impl().isNotGrabbingImage = isNotGrabbingImage;

	startScanGrab();
	return true;
}


bool oct_scan::ScanGrab::grabMeasure(bool start, OctScanProtocol* protocol)
{
	if (protocol == NULL) {
		return false;
	}
	impl().scanProtocol = protocol;
	closeScanGrab(start);

	if (start) {
		bool result = isMeasureCompleted();
		if (auto* p = EventRegistry::getInstance(); p) {
			p->runOctProtocolPatternAcquired(result);
		}
	}
	return true;
}


float oct_scan::ScanGrab::getMeasureAcquisitionTime(void)
{
	return (float)(impl().tickMsrAcqEnd - impl().tickMsrAcqStart) / 1000.0f;
}


bool oct_scan::ScanGrab::isInitiated(void)
{
	return impl().initiated;
}


bool oct_scan::ScanGrab::startScanGrab(void)
{
	resetCountOfEnfaceImages();
	resetCountOfMeasureImages();
	resetCountOfPreviewImages(false);
	clearGrabErrorCount();

	impl().previewRun = true;
	impl().measureRun = false;
	impl().measureCanceled = false;
	impl().measureCompleted = false;

	impl().threadGrab = thread{ &ScanGrab::threadGrabFunction, this };
	return true;
}


bool oct_scan::ScanGrab::closeScanGrab(bool measure)
{
	if (isGrabbing()) {
		impl().measureRun = measure; // false;
		impl().previewRun = false;

		// Sleep(4000);
		// RetinaTrack2::cancelTracking();

		if (impl().threadGrab.joinable()) {
			impl().threadGrab.join();
		}
	}

	return true;
}


void oct_scan::ScanGrab::cancelMeasureGrab(void)
{
	impl().measureCanceled = true;
	return;
}


bool oct_scan::ScanGrab::isGrabbing(void)
{
	return impl().grabberRun;
}


bool oct_scan::ScanGrab::isPreviewing(void)
{
	return impl().previewRun;
}


bool oct_scan::ScanGrab::isMeasuring(void)
{
	// return (impl().previewRun == false && impl().measureRun == true);
	return impl().measureRun;
}


bool oct_scan::ScanGrab::isMeasureCanceled(void)
{
	return impl().measureCanceled;
}


bool oct_scan::ScanGrab::isMeasureCompleted(void)
{
	return impl().measureCompleted;
}

void oct_scan::ScanGrab::threadGrabFunction(void)
{
	impl().grabberRun = true;

	bool hidden_init = true;
	bool hidden_done = false;

	LogD() << "Grabbing thread started ...";
	getUsb3Grabber()->startCameraAcquisition(true);

	while (true)
	{
		if (isGrabErrorOverDelay()) {
			this_thread::sleep_for(chrono::milliseconds(SCANNER_GRAB_ERROR_DELAY_TIME));
			// Sleep(SCANNER_GRAB_ERROR_DELAY_TIME);
			if (isGrabErrorOverLimit()) {
				break;
			}
		}
		else {
			if (!processGrabHidden(hidden_init, hidden_done)) {
				increaseGrabErrorCount();
			}
			else {
				clearGrabErrorCount();
			}
		}

		if (hidden_done) {
			// LogD() << "Move to preview step";
			break;
		}
		if (!isPreviewing()) {
			break;
		}
	}


	if (isPreviewing() && !isGrabErrorOverLimit())
	{
		bool preview_init = true;
		bool enface_init = true;

		while (true)
		{
			if (isGrabErrorOverLimit()) {
				// this_thread::sleep_for(chrono::milliseconds(SCANNER_GRAB_ERROR_DELAY_TIME));
				Sleep(SCANNER_GRAB_ERROR_DELAY_TIME);
				if (isGrabErrorOverLimit()) {
					break;
				}
			}
			else {
				if (!processGrabPreview(preview_init)) {
					increaseGrabErrorCount();
				}
				else {
					clearGrabErrorCount();
				}
			}

			if (isGrabErrorOverLimit()) {
				//this_thread::sleep_for(chrono::milliseconds(SCANNER_GRAB_ERROR_DELAY_TIME));
				Sleep(SCANNER_GRAB_ERROR_DELAY_TIME);
				if (isGrabErrorOverLimit()) {
					break;
				}
			}
			else {
				if (!processGrabEnface(enface_init)) {
					increaseGrabErrorCount();
				}
				else {
					clearGrabErrorCount();
				}
			}

			if (!isPreviewing()) {
				break;
			}
		}
	}

	if (waitForPreviewAndEnfaceProcessed() && !isGrabErrorOverLimit())
	{
		if (isMeasuring()) {
			if (auto p = OctScanStatus::getInstance(); p) {
				p->isCapturingToMeasure(true);
				p->isPreviewAveragingEnabled(true);
			}
			resetCountOfPreviewImages(true);
			if (processGrabPreviewHD()) {
				waitForPreviewAndEnfaceProcessed();
			}

			impl().measureCompleted = proceedToMeasure();
		}
	}

	getUsb3Grabber()->cancelCameraAcquisition();
	impl().grabberRun = false;
	LogD() << "Grabbing thread stopped.";
	return;
}


bool oct_scan::ScanGrab::waitForPreviewAndEnfaceProcessed(void)
{
	//CppUtil::ClockTimer::start();

	bool res = false;
	for (int i = 0; i < SCANNER_WAIT_PREVIEW_PROCESSED_COUNT_MAX; i++) {
		// this_thread::sleep_for(chrono::milliseconds(SCANNER_WAIT_PREVIEW_PROCESSED_DELAY));
		Sleep(SCANNER_WAIT_PREVIEW_PROCESSED_DELAY);

		if (!getProcess()->isBusying()) {
			res = true;
			break;
		}
	}

	//double msec = CppUtil::ClockTimer::elapsedMsec();
	//LogD() << "Preview and enface processed elapsed: " << msec;

	if (!res) {
		LogD() << "Preview and enface processed timeout!";
	}
	return res;
}


void oct_scan::ScanGrab::notifyMeasureFrameReceived(void)
{
	getProcess()->notifyMeasureDataReceived();
	return;
}


bool oct_scan::ScanGrab::checkIfScanOptimizing(void)
{
	if (auto* p = ScanOptimizer::getInstance(); p) {
		return p->isOptimizing();
	}	
	return false;
}


bool oct_scan::ScanGrab::processGrabHidden(bool& initiate, bool& completed)
{
	PreviewScan& preview = getPattern()->getPreviewScan();
	int numFrames = preview.getNumberOfPatternFrames();

	/*
	static int count = 0;
	if (count == 0) {
	CppUtil::ClockTimer::start();
	}
	*/

	const int HIDDEN_LINES_FOR_BACKGROUND = 5;
	static int frameIdx = 0;
	static int hiddens = 0;

	if (frameIdx >= numFrames || initiate) {
		frameIdx = 0;
		resetCountOfPreviewImages(false);

		if (initiate) {
			hiddens = 0;
			initiate = false;
		}
		else {
			if (++hiddens > HIDDEN_LINES_FOR_BACKGROUND) {
				// LogD() << "Preview hidden lines completed";
				completed = true;
				return true;
			}
		}
	}

	if (frameIdx >= 0 && frameIdx < numFrames)
	{
		auto line = preview.getPatternFrame(frameIdx)->getLineTraceFirst();

		if (line->isHidden()) {
			if (impl().isNotGrabbingImage) {
				if (!triggerLineTracesToPreview(frameIdx, false)) {
					return false;
				}
			}
			else {
				if (!assignGrabBufferToPreview(frameIdx, 0, false)) {
					return false;
				}

				if (!triggerLineTracesToPreview(frameIdx, false)) {
					return false;
				}

				int numLines = processGrabBufferToPreview(frameIdx, 0, false);
				if (numLines <= 0) {
					return false;
				}
			}
			if (!isPreviewing()) {
				return false;
			}
			// LogD() << "Preview hidden line found";
		}
		frameIdx++;
	}

	/*
	count++;
	if (count > 100) {
	double msec = CppUtil::ClockTimer::elapsedMsec();
	LogD() << "Preview Grab 100 elapsed: " << msec;
	count = 0;
	}
	*/
	return true;
}


bool oct_scan::ScanGrab::processGrabPreview(bool& initiate)
{
	PreviewScan& preview = getPattern()->getPreviewScan();
	int numFrames = preview.getNumberOfPatternFrames();

	/*
	static int count = 0;
	if (count == 0) {
	CppUtil::ClockTimer::start();
	}
	*/

	static int frameIdx = 0;
	static int buffStart = 0;
	if (frameIdx >= numFrames || initiate) {
		frameIdx = 0;
		buffStart = 0;
		resetCountOfPreviewImages(false);
		initiate = false;
	}

next_frame:

	if (frameIdx >= 0 && frameIdx < numFrames)
	{
		auto line = preview.getPatternFrame(frameIdx)->getLineTraceFirst();
		if (line->isHidden() || line->isLineHD()) {
			frameIdx++;
			goto next_frame;
		}

		if (impl().isNotGrabbingImage) {
			if (!triggerLineTracesToPreview(frameIdx, false)) {
				return false;
			}
		}
		else {
			if (!assignGrabBufferToPreview(frameIdx, buffStart, false)) {
				return false;
			}

			if (!triggerLineTracesToPreview(frameIdx, false)) {
				return false;
			}

			int numLines = processGrabBufferToPreview(frameIdx, buffStart, false);
			if (numLines <= 0) {
				return false;
			}
			buffStart += numLines;
		}
		if (!isPreviewing()) {
			return false;
		}
		frameIdx++;
	}

	/*
	count++;
	if (count > 100) {
	double msec = CppUtil::ClockTimer::elapsedMsec();
	LogD() << "Preview Grab 100 elapsed: " << msec;
	count = 0;
	}
	*/
	return true;
}


bool oct_scan::ScanGrab::processGrabEnface(bool& initiate)
{
	EnfaceScan& enface = getPattern()->getEnfaceScan();
	int numFrames = enface.getNumberOfPatternFrames();

	if (checkIfScanOptimizing()) {
		return true;
	}

	static int frameIdx = 0;
	/*
	if (frameIdx == 0) {
		CppUtil::ClockTimer::start();
	}
	*/
	if (frameIdx >= numFrames || initiate) {
		frameIdx = 0;
		resetCountOfEnfaceImages();
		initiate = false;
		// double msec = CppUtil::ClockTimer::elapsedMsec();
		// LogD() << "Grab enface elapsed: " << msec;
	}


	if (frameIdx >= 0 && frameIdx < numFrames)
	{
		if (impl().isNotGrabbingImage) {
			if (!triggerLineTracesToEnface(frameIdx)) {
				return false;
			}
		}
		else {
			if (!assignGrabBufferToEnface(frameIdx, 0)) {
				return false;
			}

			if (!triggerLineTracesToEnface(frameIdx)) {
				return false;
			}

			int numLines = processGrabBufferToEnface(frameIdx, 0);
			if (numLines <= 0) {
				return false;
			}
		}

		if (!isPreviewing()) {
			return false;
		}

		frameIdx++;
	}

	/*
	count++;
	if (count > 100) {
	double msec = CppUtil::ClockTimer::elapsedMsec();
	LogD() << "Enface Grab 100 elapsed: " << msec;
	count = 0;
	}
	*/
	return true;
}


bool oct_scan::ScanGrab::processGrabPreviewHD(void)
{
	StopWatch::start();
	impl().tickMsrAcqStart = GetTickCount64();

	/*
	if (impl().scanDesc->getPattern().isAngioScan()) {
		return true;
	}
	*/

	PreviewScan& preview = getPattern()->getPreviewScan();
	int numFrames = preview.getNumberOfPatternFrames();
	int buffStart = 0;
	int numLines = 0;

	for (int frameIdx = 0; frameIdx < numFrames; frameIdx++)
	{
		auto line = preview.getPatternFrame(frameIdx)->getLineTraceFirst();
		LogD() << "preview frame: " << frameIdx << ", lineHD: " << line->isLineHD();

		if (!line->isLineHD()) {
			continue;
		}

		if (impl().isNotGrabbingImage) {
			return false;
		}

		if (!assignGrabBufferToPreview(frameIdx, buffStart, true)) {
			return false;
		}

		if (!triggerLineTracesToPreview(frameIdx, true)) {
			return false;
		}

		/*
		if (frameIdx == (numFrames - 1)) {
			if (!ScanFunc::uploadPatternProfiles(*getPattern(), true)) {
				return false;
			}
		}
		*/

		int numLines = processGrabBufferToPreview(frameIdx, buffStart, true);
		if (numLines <= 0) {
			return false;
		}

		buffStart += numLines;
	}

	impl().tickMsrAcqEnd = GetTickCount64();
	double msec = StopWatch::getElapsedMillis();
	LogD() << "Grab preview HD elapsed: " << msec;
	return true;
}


bool oct_scan::ScanGrab::proceedToMeasure(void)
{
	if (!isMeasuring() || isGrabErrorOverLimit()) {
		return false;
	}

	if (!updateGrabberTimeStepToMeasure()) {
		return false;
	}

	/*
	// Hamamatsu CCD USB camera.
	if (!GlobalSettings::useUsbCmosCameraEnable()) {
		int height = getPattern()->getMeasureScanPoints();
		changeLineCameraFrameHeight(height);
	}
	*/

	/*
	if (!ScanFunc::uploadPatternProfiles(*getPattern(), true)) {
		return false;
	}
	*/

	if (impl().scanProtocol->isNotImageGrabbing) {
		return false;
	}

	if (impl().scanProtocol->isPhasingEnface) {
		if (!ScanFunc::updatePatternBuffers(*getPattern(), true)) {
			return false;
		}
	}

	ScanFunc::prepareChainSetup(impl().scanProtocol->getMeasure(),
								true);

	bool result = true;
	if (processGrabMeasure()) {
		
		// double msec = CppUtil::ClockTimer::elapsedMsec();
		// LogD() << "Process measure elapsed: " << msec;
	}
	else {
		result = false;
	}
	LogD() << "Process grab measure result: " << result;

	impl().measureRun = false;
	return result;
}


bool oct_scan::ScanGrab::processGrabMeasure(void)
{
	int numFrames = getPattern()->getMeasureScan().getNumberOfPatternFrames();
	int buffStart = 0;
	int buffCount = 0;
	int backLines = 0;
	int waitCount = 0;
	int trackResetCount = 0;
	bool trackExpired = false;
	bool targetRemaped = false;
	bool grabCanceled = false;

	std::int16_t offsetX, offsetY;
	std::int16_t startX, startY;
	if (!getMainboard()->getGalvanoDynamicOffset(offsetX, offsetY)) {
		// return false;
		offsetX = 0;
		offsetY = 0;
	}

	StopWatch::start();
	impl().tickMsrAcqStart = GetTickCount64();

	for (int frameIdx = 0; frameIdx < numFrames; frameIdx++)
	{
		auto frame = getPattern()->getMeasureScan().getPatternFrame(frameIdx);
		auto trace = frame->getLineTraceFirst();

		LogD() << "---------------------------------------------------------------";
		LogD() << "Grab Measure, frameIdx: " << frameIdx << ", traceId: " << trace->getTraceId();
		if (!ScanFunc::uploadTraceProfile(*trace)) {
			return false;
		}

//	retrace:
		int index = getIndexOfMeasureImageNext(false);
		getUsb3Grabber()->setStartMeasureBuffer(buffStart);
		
		int lines = assignGrabBufferToMeasure(frameIdx, buffCount);
		if (lines <= 0) {
			return false;
		}
		buffCount += lines;

		if (!triggerLineTracesToMeasure(frameIdx, false, offsetX, offsetY, startX, startY)) {
			return false;
		}

		int numLines = processGrabBufferToMeasure(frameIdx, buffStart);
		if (numLines <= 0) {
			return false;
		}
		buffStart += numLines;

		notifyMeasureFrameReceived();

		if (frameIdx < (numFrames - 1)) {
			if (isMeasureCanceled()) {
				grabCanceled = true;
				LogD() << "Measure scan grabbing is being canceled";
				// Exit grabbing loop.
				break;
			}
		}
	}

	impl().tickMsrAcqEnd = GetTickCount64();
	double msec = StopWatch::getElapsedMillis();
	LogD() << "Acquisition of measure frames elapsed: " << msec;

	if (!getMainboard()->setGalvanoDynamicOffset(offsetX, offsetY)) {
		return false;
	}

	msec = StopWatch::getElapsedMillis();
	LogD() << "Grab measure elapsed: " << msec;

	notifyMeasureFrameReceived();

	bool result = grabCanceled ? false : true;
	return result;
}


bool oct_scan::ScanGrab::assignGrabBufferToPreview(int frameIdx, int buffIdx, bool repeat)
{
	PatternFrame* frame = getPattern()->getPreviewScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to preview buffer, frameIdx: " << frameIdx;
		return false;
	}

	int sizeY = frame->getLateralSizeFirst();
	// int numLines = frame->getLineTraceListSize();
	int numLines = frame->getNumberOfLineTraces(repeat);
	
	getUsb3Grabber()->setSourceImageSizeY(sizeY);
	getUsb3Grabber()->clearGrabbedPreviewBuffer();

	if (repeat) {
		LogD() << "Assign usb3 buffers to preview-hd, frameIdx: " << frameIdx << ", buffIdx: " << buffIdx << ", numLines: " << numLines;
	}

	bool res = getUsb3Grabber()->grabPreviewProcess(buffIdx, numLines);
	if (!res) {
		LogD() << "Process grab preview usb3 buffers failed!";
	}
	return res;
}


int oct_scan::ScanGrab::assignGrabBufferToMeasure(int frameIdx, int buffIdx)
{
	PatternFrame* frame = getPattern()->getMeasureScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to measure buffer, frameIdx: " << frameIdx;
		return false;
	}

	int sizeY = frame->getLateralSizeFirst();
	int numLines = frame->getNumberOfLineTraces(true);

	getUsb3Grabber()->setSourceImageSizeY(sizeY);
	getUsb3Grabber()->clearGrabbedMeasureBuffer();

	LogD() << "Assign grab measure usb3 buffers, frameIdx: " << frameIdx << ", buffIdx: " << buffIdx << ", numLines: " << numLines;

	bool res = getUsb3Grabber()->grabMeasureProcess(buffIdx, numLines);
	if (!res) {
		LogD() << "Process grab measure usb3 buffers failed!";
	}
	return numLines;
}


bool oct_scan::ScanGrab::assignGrabBufferToEnface(int frameIdx, int buffIdx)
{
	PatternFrame* frame = getPattern()->getEnfaceScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to enface buffer, frameIdx: " << frameIdx;
		return false;
	}

	int sizeY = frame->getLateralSizeFirst();
	int numLines = frame->getLineTraceListSize();

	getUsb3Grabber()->setSourceImageSizeY(sizeY);
	getUsb3Grabber()->clearGrabbedEnfaceBuffer();

	bool res = getUsb3Grabber()->grabEnfaceProcess(buffIdx, numLines);
	if (!res) {
		LogD() << "Grab enface usb3 buffers failed!";
	}
	return res;
}


bool oct_scan::ScanGrab::triggerLineTracesToPreview(int frameIdx, bool repeat)
{
	PatternFrame* frame = getPattern()->getPreviewScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to trigger preview, frameIdx: " << frameIdx;
		return false;
	}

	int offsetX = frame->getPositionIntervalX();
	int offsetY = frame->getPositionIntervalY();
	// int numLines = frame->getLineTraceListSize();
	int numLines = frame->getNumberOfLineTraces(repeat);

	LineTrace* line = frame->getLineTraceFirst();
	if (line == nullptr) {
		LogD() << "Empty frame to trigger preview, frameIdx: " << frameIdx;
		return false;
	}

	auto traceId = line->getTraceId();
	short firstX = line->getPositionFirstX();
	short firstY = line->getPositionFirstY();
	short lastX = line->getPositionLastX();
	short lastY = line->getPositionLastY();
	float x1 = line->getStartX();
	float y1 = line->getStartY();
	float x2 = line->getCloseX();
	float y2 = line->getCloseY();
	bool res = true;

	if (repeat) {
		LogD() << "Trigger preview-hd line, trace-id: " << traceId << ", frame_idx: " << frameIdx << ", n_lines: " << numLines;
		LogD() << "Galvano x-move, pos1: " << firstX << ", pos2: " << lastX << ", interval: " << offsetX << ", x1: " << x1 << " = > x2: " << x2;
		LogD() << "Galvano y-move, pos1: " << firstY << ", pos2: " << lastY << ", interval: " << offsetY << ", y1: " << y1 << " = > y2: " << y2;
	}

	impl().galvano->slewMoveXY(firstX, firstY);
	res = triggerTraceProfile(*line, numLines, offsetX, offsetY);
	if (!res) {
		LogD() << "Trigger trace profile to preview failed!";
	}
	return res;
}


bool oct_scan::ScanGrab::triggerLineTracesToMeasure(int frameIdx, bool inferOffset, std::int16_t dynOffsetX, std::int16_t dynOffsetY, std::int16_t& startX, std::int16_t& startY)
{
	PatternFrame* frame = getPattern()->getMeasureScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to trigger measure, frameIdx: " << frameIdx;
		return false;
	}

	LineTrace* line = frame->getLineTraceFirst();
	if (line == nullptr) {
		LogD() << "Empty frame to trigger measure, frameIdx: " << frameIdx;
		return false;
	}

	bool repeat = !(line->isRasterX() || line->isRasterY());
	int offsetX = frame->getPositionIntervalX();
	int offsetY = frame->getPositionIntervalY();
	int numLines = frame->getNumberOfLineTraces(repeat);

	auto traceId = line->getTraceId();
	short firstX = line->getPositionFirstX();
	short firstY = line->getPositionFirstY();
	short lastX = line->getPositionLastX();
	short lastY = line->getPositionLastY();
	float x1 = line->getStartX();
	float y1 = line->getStartY();
	float x2 = line->getCloseX();
	float y2 = line->getCloseY();
	bool res = true;

	if (inferOffset) {
		if (frameIdx >= TRACE_ID_MAX_LINES) {
			int startIdx = frameIdx % TRACE_ID_MAX_LINES;
			PatternFrame* sframe = getPattern()->getMeasureScan().getPatternFrame(startIdx);
			LineTrace* sline = sframe->getLineTraceFirst();
			auto movePosX = firstX - sline->getPositionX(0) + dynOffsetX;
			auto movePosY = firstY - sline->getPositionY(0) + dynOffsetY;

			if (startIdx == 0) {
				if (!getMainboard()->setGalvanoDynamicOffset(movePosX, movePosY)) {
					return false;
				}
			}
		}
	}

	LogD() << "Trigger measure line, trace-id: " << traceId << ", frame_idx : " << frameIdx << ", n_lines : " << numLines;
	LogD() << "Galvano x-move, pos1: " << firstX << ", pos2: " << lastX << ", interval: " << offsetX << ", x1: " << x1 << " = > x2: " << x2;
	LogD() << "Galvano y-move, pos1: " << firstY << ", pos2: " << lastY << ", interval: " << offsetY << ", y1: " << y1 << " = > y2: " << y2;
	
	impl().galvano->slewMoveXY(firstX, firstY);
	res = triggerTraceProfile(*line, numLines, offsetX, offsetY);
	if (!res) {
		LogD() << "Trigger trace profile to measure failed!";
	}

	startX = firstX;
	startY = firstY;
	return res;
}


bool oct_scan::ScanGrab::triggerLineTracesToEnface(int frameIdx)
{
	PatternFrame* frame = getPattern()->getEnfaceScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to trigger enface, frameIdx: " << frameIdx;
		return false;
	}

	int offsetX = frame->getPositionIntervalX();
	int offsetY = frame->getPositionIntervalY();
	int numLines = frame->getLineTraceListSize();

	LineTrace* line = frame->getLineTraceFirst();
	if (line == nullptr) {
		LogD() << "Empty frame to trigger enface, frameIdx: " << frameIdx;
		return false;
	}

	short firstX = line->getPositionX(0);
	short firstY = line->getPositionY(0);
	bool res = true;

	impl().galvano->slewMoveXY(firstX, firstY);
	res = triggerTraceProfile(*line, numLines, offsetX, offsetY);
	if (!res) {
		LogD() << "Trigger trace profile to enface failed!";
	}
	return res;
}


bool oct_scan::ScanGrab::triggerTraceProfile(oct_pattern::LineTrace & line, short numLines, short offsetX, short offsetY)
{
	if (!isInitiated()) {
		return false;
	}

	Galvanometer* galvano = impl().galvano;

	int traceId = line.getTraceId();
	bool res = false;

	if (line.isLine() || line.isCircle()) {
		res = galvano->scanMoveXY(traceId);
	}
	else if (line.isRasterX()) {
		return galvano->scanRepeatX(traceId, numLines, offsetX, offsetY);
	}
	else if (line.isRasterY()) {
		return galvano->scanRepeatY(traceId, numLines, offsetX, offsetY);
	}
	else if (line.isRasterFastX()) {
		res = galvano->scanRepeatFastX(traceId, numLines, offsetX, offsetY);
	}
	else if (line.isRasterFastY()) {
		res = galvano->scanRepeatFastY(traceId, numLines, offsetX, offsetY);
	}
	return res;
}


int oct_scan::ScanGrab::processGrabBufferToPreview(int frameIdx, int buffIdx, bool repeat)
{
	int numLines = 0;

	PatternFrame* frame = getPattern()->getPreviewScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to grab preview, frameIdx: " << frameIdx;
		return 0;
	}

	bool res = false;

	// numLines = frame->getLineTraceListSize();
	numLines = frame->getNumberOfLineTraces(repeat);

	res = getUsb3Grabber()->grabPreviewProcessWait(buffIdx, numLines);
	if (res) {
		if (repeat) {
			LogD() << "Preview-hd buffer grabbed, buffIdx: " << buffIdx << ", numLines: " << numLines;
		}
	}
	else {
		LogD() << "Grab preview buffer failed!, buffIdx: " << buffIdx << ", numLines: " << numLines;
		return 0;
	}
	return numLines;
	
}


int oct_scan::ScanGrab::processGrabBufferToMeasure(int frameIdx, int buffIdx)
{
	int numLines = 0;

	PatternFrame* frame = getPattern()->getMeasureScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to grab measure, frameIdx: " << frameIdx;
		return 0;
	}

	bool res = false;
	numLines = frame->getNumberOfLineTraces(true);

	res = getUsb3Grabber()->grabMeasureProcessWait(buffIdx, numLines);
	if (res) {
		LogD() << "Measure usb3 buffer grabbed, buffIdx: " << buffIdx << ", numLines: " << numLines;
	}
	else {
		LogD() << "Grab measure usb3 buffer failed!, buffIdx: " << buffIdx << ", numLines: " << numLines;
		return 0;
	}
	return numLines;
}


int oct_scan::ScanGrab::processGrabBufferToEnface(int frameIdx, int buffIdx)
{
	int numLines = 0;

	PatternFrame* frame = getPattern()->getEnfaceScan().getPatternFrame(frameIdx);
	if (frame == nullptr) {
		LogD() << "Invalid frame to grab enface, frameIdx: " << frameIdx;
		return 0;
	}

	bool res = false;
	numLines = frame->getLineTraceListSize();

	res = getUsb3Grabber()->grabEnfaceProcessWait(buffIdx, numLines);
	if (res) {
		// LogD() << "Preview buffer grabbed, buffIdx: " << buffIdx << ", numLines: " << numLines;
	}
	else {
		LogD() << "Grab enface usb3 buffer failed!, buffIdx: " << buffIdx << ", numLines: " << numLines;
		return 0;
	}
	return numLines;
}


bool oct_scan::ScanGrab::updateGrabberTimeStepToMeasure(void)
{
	if (impl().scanProtocol->getMeasure().isSpeedSlower()) {
		if (!setGrabberTimeStepToSlower()) {
			return false;
		}
		LogD() << "Measure scan, slower speed";
	}
	else if (impl().scanProtocol->getMeasure().isSpeedNormal()) {
		if (!setGrabberTimeStepToNormal()) {
			return false;
		}
		LogD() << "Measure scan, normal speed";
	}
	else if (impl().scanProtocol->getMeasure().isSpeedCustom()) {
		if (!setGrabberTimeStepToCustom()) {
			return false;
		}
		LogD() << "Measure scan, custom speed";
	}
	else {
		if (!setGrabberTimeStepToFaster()) {
			return false;
		}
		LogD() << "Measure scan, faster speed";
	}

	return true;
}


bool oct_scan::ScanGrab::setGrabberTimeStepToFaster(bool isNotGrabbingImage)
{
	if (isScanSpeedFaster()) {
		return true;
	}

	if (!getUsb3Grabber()->setCameraSpeedToFaster()) {
		return false;
	}

	auto timestep = TRIGGER_TIME_STEP_AT_FASTER_SPEED;
	getGalvanometer()->setTriggerTimeStep(timestep);

	d_ptr->scanSpeed = OctScanSpeed::FASTER;
	LogD() << "Scan speed changed to faster, galvano timestep: " << timestep;
	return true;
}

bool oct_scan::ScanGrab::setGrabberTimeStepToNormal(bool isNotGrabbingImage)
{
	if (isScanSpeedNormal()) {
		return true;
	}

	if (!getUsb3Grabber()->setCameraSpeedToNormal()) {
		return false;
	}
	auto timestep = TRIGGER_TIME_STEP_AT_NORMAL_SPEED;
	getGalvanometer()->setTriggerTimeStep(timestep);

	d_ptr->scanSpeed = OctScanSpeed::NORMAL;
	LogD() << "Scan speed changed to normal, galvano timestep: " << timestep;
	return true;
}


bool oct_scan::ScanGrab::setGrabberTimeStepToSlower(bool isNotGrabbingImage)
{
	if (isScanSpeedSlower()) {
		return true;
	}

	if (!getUsb3Grabber()->setCameraSpeedToSlower()) {
		return false;
	}
	auto timestep = TRIGGER_TIME_STEP_AT_SLOWER_SPEED;
	getGalvanometer()->setTriggerTimeStep(timestep);

	d_ptr->scanSpeed = OctScanSpeed::SLOWER;
	LogD() << "Scan speed changed to slower, galvano timestep: " << timestep;
	return true;
}


bool oct_scan::ScanGrab::setGrabberTimeStepToCustom(bool isNotGrabbingImage)
{
	if (isScanSpeedCustom()) {
		return true;
	}

	if (!getUsb3Grabber()->setCameraSpeedToFaster()) {
		return false;
	}

	getGalvanometer()->setTriggerTimeStep(TRIGGER_TIME_STEP_AT_CUSTOM_SPEED);
	d_ptr->scanSpeed = OctScanSpeed::CUSTOM;
	return true;
}


bool oct_scan::ScanGrab::isScanSpeedFaster(void)
{
	return (d_ptr->scanSpeed == OctScanSpeed::FASTER);
}


bool oct_scan::ScanGrab::isScanSpeedNormal(void)
{
	return (d_ptr->scanSpeed == OctScanSpeed::NORMAL);
}


bool oct_scan::ScanGrab::isScanSpeedSlower(void)
{
	return (d_ptr->scanSpeed == OctScanSpeed::SLOWER);
}


bool oct_scan::ScanGrab::isScanSpeedCustom(void)
{
	return (d_ptr->scanSpeed == OctScanSpeed::CUSTOM);
}



bool oct_scan::ScanGrab::setUsbCameraExposureTime(float expTime)
{
	bool result = getUsb3Grabber()->setLineCameraExposureTime(expTime);
	return result;
}

float oct_scan::ScanGrab::getUsbCameraExposureTime(void)
{
	auto result = getUsb3Grabber()->getLineCameraExposureTime();
	return result;
}


void oct_scan::ScanGrab::increaseGrabErrorCount(void)
{
	impl().countOfGrabErrors++;
	return;
}


void oct_scan::ScanGrab::clearGrabErrorCount(void)
{
	impl().countOfGrabErrors = 0;
	return;
}


bool oct_scan::ScanGrab::isGrabErrorOverLimit(void)
{
	if (impl().countOfGrabErrors > SCANNER_GRAB_ERROR_COUNT_LIMIT) {
		return true;
	}
	return false;
}


bool oct_scan::ScanGrab::isGrabErrorOverDelay(void)
{
	if (impl().countOfGrabErrors > SCANNER_GRAB_ERROR_COUNT_DELAY) {
		return true;
	}
	return false;
}


bool oct_scan::ScanGrab::isPreviewImagesRepeated(void)
{
	return d_ptr->repeatOfPreviews;
}


void oct_scan::ScanGrab::resetCountOfPreviewImages(bool repeat)
{
	d_ptr->countOfPreviews = 0;
	d_ptr->repeatOfPreviews = repeat;
	return;
}


void oct_scan::ScanGrab::resetCountOfMeasureImages(void)
{
	d_ptr->countOfMeasures = 0;
	return;
}


void oct_scan::ScanGrab::resetCountOfEnfaceImages(void)
{
	d_ptr->countOfEnfaces = 0;
	return;
}


int oct_scan::ScanGrab::getIndexOfPreviewImageNext(void)
{
	int index = d_ptr->countOfPreviews;
	d_ptr->countOfPreviews++;
	return index;
}


int oct_scan::ScanGrab::getIndexOfMeasureImageNext(bool autoinc)
{
	int index = d_ptr->countOfMeasures;
	if (autoinc) {
		d_ptr->countOfMeasures++;
	}
	return index;
}


int oct_scan::ScanGrab::getIndexOfEnfaceImageNext(void)
{
	int index = d_ptr->countOfEnfaces;
	d_ptr->countOfEnfaces++;
	return index;
}


void oct_scan::ScanGrab::retreatIndexOfMeasureImage(int size)
{
	int index = d_ptr->countOfMeasures;
	index = max(index - size, 0);
	d_ptr->countOfMeasures = index;
	return;
}

void oct_scan::ScanGrab::onOctPreviewBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index)
{
	int idxOfImage = getIndexOfPreviewImageNext();
	getProcess()->receiveDataForPreview(buff, size_y, idxOfImage);
	return;
}

void oct_scan::ScanGrab::onOctMeasureBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index)
{
	int idxOfImage = getIndexOfMeasureImageNext();
	getProcess()->receiveDataForMeasure(buff, size_y, idxOfImage);
	return;
}

void oct_scan::ScanGrab::onOctEnfaceBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index)
{
	int idxOfImage = getIndexOfEnfaceImageNext();
	getProcess()->receiveDataForEnface(buff, size_y, idxOfImage);
	return;
}

oct_pattern::PatternPlan * oct_scan::ScanGrab::getPattern(void)
{
	return impl().pattern;
}


sig_chain::OctProcess * oct_scan::ScanGrab::getProcess(void)
{
	return impl().process;
}


oct_grab::Usb3Grabber * oct_scan::ScanGrab::getUsb3Grabber(void) 
{
	return impl().usb3Grab;
}


wso_device::MainBoard * oct_scan::ScanGrab::getMainboard(void)
{
	return impl().board;
}


wso_device::Galvanometer * oct_scan::ScanGrab::getGalvanometer(void)
{
	return impl().galvano;
}


ScanGrab::ScanGrabImpl & oct_scan::ScanGrab::impl(void)
{
	return *d_ptr;
}

