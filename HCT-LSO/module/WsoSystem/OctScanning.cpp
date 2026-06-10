#include "pch.h"
#include "OctScanning.h"
#include "Hardware.h"
#include "Fixation.h"
#include "Controller.h"
#include "OctArchive.h"

using namespace wso_system;
using namespace std;


std::mutex OctScanning::singleMutex_;


struct OctScanning::OctScanningImpl
{
	MainBoard* mainboard;
	Galvanometer* galvano; 
	Usb3Grabber* grabber;

	OctScanProtocol protocol;
	PatternPlan pattern;
	OctProcess process;

	bool initialized;

	OctScanningImpl() {
		initializeOctScanningImpl();
	}

	void initializeOctScanningImpl(void) {
		initialized = false;
		mainboard = nullptr;
		galvano = nullptr;
		grabber = nullptr;
	}
};



wso_system::OctScanning::OctScanning() :
	d_ptr(std::make_unique<OctScanningImpl>())
{
}


wso_system::OctScanning::~OctScanning()
{
}


OctScanning* wso_system::OctScanning::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static OctScanning instance;
	return &instance;
}


bool wso_system::OctScanning::initializeOctScanning(void)
{
	ChainSetup::initializeChainSetup();

	if (auto usb3 = Hardware::getInstance()->getUsb3Grabber(); usb3) {
		impl().grabber = usb3;
		if (!impl().process.initializeOctProcess()) {
			return false;
		}
	}
	else {
		return false;
	}

	if (auto board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto grab = ScanGrab::getInstance(); grab) {
			grab->initiateScanGrab(board, impl().grabber);
		}
		if (auto post = ScanPost::getInstance(); post) {
			post->initiateScanPost(board, &impl().process);
		}
		ScanFunc::initializeScanFunc(board, impl().grabber);

		if (auto optim = ScanOptimizer::getInstance(); optim) {
			optim->initializeScanOptimizer(board);
		}
		if (auto archive = OctArchive::getInstance(); archive) {
			archive->initializeOctArchive();
		}

		impl().mainboard = board;
		impl().galvano = board->getGalvanometer();
		impl().initialized = true;
	}
	return isInitialized();
}


void wso_system::OctScanning::releaseOctScanning(void)
{
	impl().process.releaseOctProcess();

	if (auto grab = ScanGrab::getInstance(); grab) {
		grab->releaseScanGrab();
	}
	if (auto post = ScanPost::getInstance(); post) {
		post->releaseScanPost();
	}

	if (auto archive = OctArchive::getInstance(); archive) {
		archive->releaseOctArchive();
	}

	impl().initialized = false;
	return ;
}

bool wso_system::OctScanning::isInitialized(void) const
{
	return impl().initialized;
}


bool wso_system::OctScanning::prepareScan(const OctProtocolInitParam& param)
{
	if (isWorking()) {
		return false;
	}

	// releaseScanCallbacks();

	impl().protocol = OctScanProtocol();
	impl().protocol.setupAsInitParam(param);
	impl().pattern = PatternPlan();

	bool init_scan = param.isFirstScanOnSide;
	bool not_align = param.isNotFocusAlign;

	if (!setScanDevicesPrepared(init_scan, not_align)) {
		return false;
	}
	
	if (auto optim = ScanOptimizer::getInstance(); optim) {
		optim->initializeScanOptimizer(impl().mainboard);
	}
	return true;
}


bool wso_system::OctScanning::startScan(OctPreviewImageCaptured clb_preview, OctEnfaceImageCaptured clb_enface)
{
	if (isWorking()) {
		return false;
	}

	LogD() << "Oct scanning started";
	if (auto p = CallbackRegistry::getInstance(); p) {
		p->setOctPreviewImageCaptured(clb_preview);
		p->setOctEnfaceImageCaptured(clb_enface);
	}

	if (auto p = OctScanStatus::getInstance(); p) {
		p->isCapturingToMeasure(false);
	}

	if (!beginScanProtocol()) {
		return false;
	}
	return true;
}


bool wso_system::OctScanning::closeScan(bool measure, OctScanProtocolCompleted clb_protocol)
{
	if (!isWorking()) {
		return false;
	}

	LogD() << "Oct scanning closing...";
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctScanProtocolCompleted(clb_protocol);
	}

	if (auto* p = ScanOptimizer::getInstance(); p) {
		p->ceaseOptimizing();
	}

	closePreviewScan(measure);
	setScanDevicesReleased();

	// if (!measure) {
		releaseScanCallbacks();
	// }

	// This closing guarantees that only the scanner grabbing has been stopped. 
	// The post-processing thread may still be running. 
	// Note that even not in the measure mode, the preview image or its raw data 
	// can be processing in the background. 
	// GUI should wait for the post-processing thread to be completed via the callback. 
	// Any method to stop all threads completely should be considered.  
	return true;
}

void wso_system::OctScanning::cancelScan(void)
{
	closeScan(false, nullptr);
	return;
}

bool wso_system::OctScanning::isWorking(void) const
{
	bool flag = isGrabbing() || isPostProcessing();
	return flag;
}


bool wso_system::OctScanning::isGrabbing(void) const
{
	bool flag = false;

	if (auto* p = ScanGrab::getInstance(); p) {
		flag |= p->isGrabbing();
	}
	return flag;
}

bool wso_system::OctScanning::isPostProcessing(void) const
{
	bool flag = false;

	if (auto* p = ScanPost::getInstance(); p) {
		flag |= p->isPosting();
	}
	return flag;
}

bool wso_system::OctScanning::isAutoOptimizing(void) const
{
	if (auto* p = ScanOptimizer::getInstance(); p) {
		return p->isOptimizing();
	}
	return false;
}

bool wso_system::OctScanning::isPreviewing(void) const
{
	if (auto* p = ScanGrab::getInstance(); p) {
		return p->isPreviewing();
	}
	return false;
}

bool wso_system::OctScanning::isMeasuring(void) const
{
	if (auto* p = ScanGrab::getInstance(); p) {
		return p->isMeasuring();
	}
	return false;
}

bool wso_system::OctScanning::startAutoDiopterFocus(OctAutoFocusOptimized clb_focus)
{
	if (!isGrabbing()) {
		return false;
	}
	if (isAutoOptimizing()) {
		return false;
	}

	if (auto* p = ScanOptimizer::getInstance(); p) {
		if (!p->initiateAutoDiopterFocus(clb_focus)) {
			cancelAutoOptimizing();
			return false;
		}
	}
	return true;
}

bool wso_system::OctScanning::startAutoReference(OctAutoReferOptimized clb_refer)
{
	if (!isGrabbing()) {
		return false;
	}
	if (isAutoOptimizing()) {
		return false;
	}

	if (auto* p = ScanOptimizer::getInstance(); p) {
		bool on_cornea = impl().protocol.isCorneaScan();
		if (!p->initiateAutoReference(clb_refer, on_cornea)) {
			cancelAutoOptimizing();
			return false;
		}
	}
	return true;
}

bool wso_system::OctScanning::startAutoPolarization(OctAutoPolarOptimized clb_polar)
{
	if (!isGrabbing()) {
		return false;
	}
	if (isAutoOptimizing()) {
		return false;
	}

	if (auto* p = ScanOptimizer::getInstance(); p) {
		if (!p->initiateAutoPolarization(clb_polar)) {
			cancelAutoOptimizing();
			return false;
		}
	}
	return true;
}

bool wso_system::OctScanning::startAutoScanOptimize(OctAutoScanOptimized clb_scan)
{
	if (!isGrabbing()) {
		return false;
	}
	if (isAutoOptimizing()) {
		return false;
	}

	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctAutoScanOptimized(clb_scan);
	}

	if (auto* p = ScanOptimizer::getInstance(); p) {
		bool on_cornea = impl().protocol.isCorneaScan();
		if (!p->initiateAutoScanOptimize(on_cornea)) {
			cancelAutoOptimizing();
			return false;
		}
	}
	return true;
}

bool wso_system::OctScanning::cancelAutoOptimizing(void)
{
	if (auto* p = ScanOptimizer::getInstance(); p) {
		p->ceaseOptimizing();
	}

	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctAutoScanOptimized(nullptr);
	}
	return true;
}



bool wso_system::OctScanning::beginScanProtocol(void)
{
	auto proto = &impl().protocol;
	auto plan = &impl().pattern;

	// Update pattern plan. 
	if (!plan->buildPatternPlan(*proto)) {
		return false;
	}

	// Initialize chain setup for preview scanning. 
	auto& measure = proto->getMeasure();
	ScanFunc::prepareChainSetup(measure, false);

	// Additinonally, angio setup initialized here. 
	if (ChainSetup::isAngioPattern() || ChainSetup::isScanAmplitudesStored()) {
		AngioSetup::setAngiogramLayout(
			measure.getNumberOfScanLines(),
			measure.getNumberOfScanPoints(),
			measure.getScanOverlaps(),
			measure.isVerticalScan()
		);
	}

	// Galvano time step should be set before pattern profiles update.
	if (!adjustScanSpeedToPreview()) {
		return false;
	}

	// Update grabber profiles.
	if (!ScanFunc::updatePatternPositions(proto->getEyeSide(), *plan, proto->getMeasureScanSpeed())) {
		return false;
	}
	if (!ScanFunc::uploadPatternProfiles(*plan, false)) {
		return false;
	}
	if (!ScanFunc::updatePatternBuffers(*plan, false, true)) {
		return false;
	}

	if (!proto->isNotImageGrabbing && !proto->isPhasingEnface) {
		if (!ScanFunc::updatePatternBuffers(*plan, true, false)) {
			return false;
		}
	}

	// Update process features. 
	if (!updatePatternLayout()) {
		return false;
	}

	// Start grabbing for preview. 
	bool res = startPreviewScan(proto->isPhasingEnface);
	return res;
}

bool wso_system::OctScanning::initScanProcessing(bool enface)
{
	auto res = impl().process.startProcess(enface);
	return res;
}


bool wso_system::OctScanning::setScanDevicesPrepared(bool init_scan, bool not_align)
{
	/*
	if (!Hardware::getInstance()->initializeOctScanner()) {
		return false;
	}
	*/

	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (!board->prepareDevicesForOctScan()) {
			return false;
		}
	}

	if (!turnOnInternalFixationByPattern()) {
		return false;
	}
	/*
	if (!adjustScanSpeedToPreview()) {
		return false;
	}
    */

	if (!alignScanFocusToReadyPosition(init_scan, not_align)) {
		return false;
	}
	return true;
}

bool wso_system::OctScanning::setScanDevicesReleased(void)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (!board->releaseDevicesForOctScan()) {
			return false;
		}
	}
	return true;
}

bool wso_system::OctScanning::alignScanFocusToReadyPosition(bool init_scan, bool not_align)
{
	if (auto control = Controller::getInstance(); control) {
		auto proto = &impl().protocol;

		if (not_align) {
			return true;
		}
		if (init_scan) {
			// At the initial measure. 
			bool anter = proto->isCorneaScan();
			auto diopt = proto->diopter;
			control->moveOctReferenceToOrigin(anter);
			control->changeOctDiopterFocus(diopt);
			control->moveOctPolarizationToOrigin();
			// control->moveFixationToOrigin();
		}
		else {
			// At the subsequent measures.
			if (proto->isCorneaScan()) {
				control->moveOctReferenceToOrigin(true);
			}
		}
		return true;
	}
	return false;
}
bool wso_system::OctScanning::startPreviewScan(bool enface)
{
	if (!isInitialized()) {
		return false;
	}

	stopGrabbing(false);

	auto proto = &impl().protocol;
	if (!proto->isNotImageGrabbing) {
		if (!initScanProcessing(enface)) {
			return false;
		}
	}

	if (!adjustScanSpeedToPreview()) {
		return false;
	}

	if (auto grab = ScanGrab::getInstance(); grab) {
		if (!grab->grabPreview(&impl().pattern, &impl().process, proto->isNotImageGrabbing)) {
			return false;
		}

		initiateMeasureResult();
		return true;
	}
	return false;
}

bool wso_system::OctScanning::closePreviewScan(bool measure)
{
	if (!isGrabbing()) {
		return false;
	}

	if (measure) {
		captureMeasureResult();
	}

	stopGrabbing(measure);
	return true;
}

void wso_system::OctScanning::stopGrabbing(bool measure)
{
	if (auto grab = ScanGrab::getInstance(); grab) {
		auto proto = &impl().protocol;
		// Busy waiting for grabbing finished.
		grab->grabMeasure(measure, proto);

		if (measure) {
			bool res = grab->isMeasureCompleted();

			// Start off a thread for image processing. 
			if (auto post = ScanPost::getInstance(); post) {
				post->postMeasure(res);
			}
		}
	}
	return;
}

void wso_system::OctScanning::releaseScanCallbacks(void)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctSpectrumDataCaptured(nullptr);
		p->setOctResampledDataCaptured(nullptr);
		p->setOctIntensityDataCaptured(nullptr);

		p->setOctPreviewImageCaptured(nullptr);
		p->setOctEnfaceImageCaptured(nullptr);
	}
	return;
}

bool wso_system::OctScanning::initiateMeasureResult(void)
{
	if (auto archive = OctArchive::getInstance(); archive) {
		return archive->initiateProtocolResult(&impl().protocol, &impl().pattern);
	}
	return false;
}

bool wso_system::OctScanning::captureMeasureResult(void)
{
	if (auto p = impl().mainboard; p) {
		impl().protocol.eyeSide = p->getEyeSide();

		/*
		OctCorneaImageDescript image;
		if (p->getCorneaCamera()->captureFrame(image)) {
			if (auto archive = OctArchive::getInstance(); archive) {
				if (archive->assignCorneaImage(image)) {
					return true;
				}
			}
		}
		*/
	}
	return false;
}


bool wso_system::OctScanning::turnOnInternalFixationByPattern(void)
{
	auto proto = &impl().protocol;
	auto eyeSide = proto->getEyeSide();
	auto target = proto->getFixationTarget();
	if (auto* fix = Fixation::getInstance(); fix) {
		if (fix->turnOnInternalLed(eyeSide, target)) {
			return true;
		}
	}
	return false;
}

bool wso_system::OctScanning::adjustScanSpeedToPreview(void)
{
	auto proto = &impl().protocol;
	auto no_image = proto->isNotImageGrabbing;
	auto speed = proto->getPreviewScanSpeed();

	if (updateGrabberTimeStep(speed, no_image)) {
		return true;
	}
	return false;
}

bool wso_system::OctScanning::adjustScanSpeedToMeasure(void)
{
	auto proto = &impl().protocol;
	auto no_image = proto->isNotImageGrabbing;
	auto speed = proto->getMeasureScanSpeed();

	if (updateGrabberTimeStep(speed, no_image)) {
		return true;
	}
	return false;
}

bool wso_system::OctScanning::updateGrabberTimeStep(OctScanSpeed speed, bool not_image_grab)
{
	if (isGrabbing() || isPostProcessing()) {
		return false;
	}
	if (auto* p = ScanGrab::getInstance(); p) {
		bool res = false;
		switch (speed) {
		using enum OctScanSpeed;
		case SLOWER:
			res = p->setGrabberTimeStepToSlower(not_image_grab);
			break;
		case NORMAL:
			res = p->setGrabberTimeStepToNormal(not_image_grab);
			break;
		case FASTER:
			res = p->setGrabberTimeStepToFaster(not_image_grab);
			break;
		}

		if (res) {
			ChainSetup::clearBackgroundSpectrum();
			return true;
		}
	}
	return false;
}

bool wso_system::OctScanning::updatePatternLayout(void)
{
	auto proto = &impl().protocol;
	auto pattern = &impl().pattern;

	auto points = pattern->getPreviewScanPoints();
	auto lines = pattern->getPreviewScanLines();
	if (!impl().process.setPreviewFeature(lines, points)) {
		return false;
	}
	points = pattern->getMeasureScanPoints();
	lines = pattern->getMeasureScanLines();
	if (!impl().process.setMeasureFeature(lines, points)) {
		return false;
	}
	if (proto->isPhasingEnface) {
		points = pattern->getEnfaceScanPoints();
		lines = pattern->getEnfaceScanLines();
		if (!impl().process.setEnfaceFeature(lines, points)) {
			return false;
		}
	}

	return true;
}

OctScanning::OctScanningImpl& wso_system::OctScanning::impl(void) const
{
	return *d_ptr;
}