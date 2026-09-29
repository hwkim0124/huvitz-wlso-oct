#include "pch.h"
#include "EyeAlignment.h"
#include "Hardware.h"
#include "OctScanning.h"


using namespace wso_system;


std::mutex EyeAlignment::singleMutex_;


struct EyeAlignment::EyeAlignmentImpl
{
	bool initialized = false;
	CorneaCamera* camera = nullptr;
};


EyeAlignment::EyeAlignment() :
	d_ptr(make_unique<EyeAlignmentImpl>())
{}


EyeAlignment::~EyeAlignment()
{}


EyeAlignment* wso_system::EyeAlignment::getInstance(void)
{
	// Static local variable initialization is thread-safe and happens only once.
	static EyeAlignment instance;
	return &instance;
}

bool wso_system::EyeAlignment::initializeEyeAlignment(void)
{
	auto* board = Hardware::getInstance()->getMainBoard();
	if (board == nullptr) {
		return false;
	}

	auto* aligner = EyeAligner::getInstance();
	if (!aligner->initializeEyeAligner(board)) {
		return false;
	}

	// Tap the cornea frames before they reach the managed preview callback. The sink
	// copies the frame into the aligner's mailbox and returns; it is a no-op while no
	// alignment is running.
	impl().camera = board->getCorneaCamera();
	if (impl().camera) {
		impl().camera->setFrameSink([aligner](IrCameraId camId, const std::uint8_t* bits, int width, int height) {
			aligner->onCorneaFrame(camId, bits, width, height);
		});
	}

	applyConfiguration();
	impl().initialized = true;
	LogI() << "Eye alignment initialized";
	return true;
}

void wso_system::EyeAlignment::releaseEyeAlignment(void)
{
	if (auto* aligner = EyeAligner::getInstance(); aligner) {
		aligner->cancel(true);
	}
	if (impl().camera) {
		impl().camera->setFrameSink(nullptr);
		impl().camera = nullptr;
	}
	if (auto* aligner = EyeAligner::getInstance(); aligner) {
		aligner->releaseEyeAligner();
	}
	impl().initialized = false;
}

bool wso_system::EyeAlignment::isInitialized(void) const
{
	return impl().initialized;
}

bool wso_system::EyeAlignment::applyConfiguration(void)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* settings = config->getAlignmentSettings(); settings) {
			EyeAligner::getInstance()->setConfig(*settings->getParam());
			return true;
		}
	}
	return false;
}

EyeAlignResult wso_system::EyeAlignment::startAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback)
{
	if (!impl().initialized) {
		return EyeAlignResult::NOT_INITIALIZED;
	}
	// Never move the stage while an OCT auto-optimisation is driving motors.
	if (auto* oct = OctScanning::getInstance(); oct && oct->isAutoOptimizing()) {
		return EyeAlignResult::BUSY;
	}
	return EyeAligner::getInstance()->startAlignment(mode, eyeSide, callback);
}

bool wso_system::EyeAlignment::cancelAlignment(void)
{
	auto* aligner = EyeAligner::getInstance();
	const bool wasRunning = aligner->isRunning();
	aligner->cancel(true);
	return wasRunning;
}

bool wso_system::EyeAlignment::holdAlignment(void)
{
	return EyeAligner::getInstance()->hold();
}

bool wso_system::EyeAlignment::resumeAlignment(void)
{
	return EyeAligner::getInstance()->resume();
}

bool wso_system::EyeAlignment::isHeld(void) const
{
	return EyeAligner::getInstance()->isHeld();
}

bool wso_system::EyeAlignment::isAligning(void) const
{
	return EyeAligner::getInstance()->isRunning();
}

EyeAlignMode wso_system::EyeAlignment::getMode(void) const
{
	return EyeAligner::getInstance()->getMode();
}

EyeAlignPhase wso_system::EyeAlignment::getPhase(void) const
{
	return EyeAligner::getInstance()->getPhase();
}

EyeAlignResult wso_system::EyeAlignment::getResult(void) const
{
	return EyeAligner::getInstance()->getResult();
}

bool wso_system::EyeAlignment::fetchStatus(EyeAlignStatus* status) const
{
	return EyeAligner::getInstance()->fetchStatus(status);
}

bool wso_system::EyeAlignment::isPupilDetected(IrCameraId camId) const
{
	return EyeAligner::getInstance()->isPupilDetected(camId);
}

bool wso_system::EyeAlignment::getPupilResult(IrCameraId camId, PupilDetectResult* result) const
{
	return EyeAligner::getInstance()->getPupilResult(camId, result);
}

bool wso_system::EyeAlignment::detectPupilInImage(const std::uint8_t* gray, int width, int height, IrCameraId camId, PupilDetectResult* result) const
{
	if (result == nullptr) {
		return false;
	}
	*result = EyeAligner::getInstance()->detectPupilInImage(gray, width, height, camId);
	return true;
}

bool wso_system::EyeAlignment::setPupilOverride(IrCameraId camId, const PupilDetectResult& result)
{
	EyeAligner::getInstance()->setPupilOverride(camId, &result);
	return true;
}

bool wso_system::EyeAlignment::clearPupilOverride(IrCameraId camId)
{
	EyeAligner::getInstance()->setPupilOverride(camId, nullptr);
	return true;
}

EyeAlignment::EyeAlignmentImpl& wso_system::EyeAlignment::impl(void) const
{
	return *d_ptr;
}
