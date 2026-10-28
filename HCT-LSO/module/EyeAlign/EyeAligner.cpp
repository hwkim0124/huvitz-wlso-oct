#include "pch.h"
#include "EyeAligner.h"
#include "PupilDetector.h"
#include "PupilCenterModel.h"
#include "PupilTracker.h"
#include "CorneaFrameStore.h"
#include "AxisController.h"

#include "MainBoard.h"
#include "StepMotor.h"
#include "CorneaCamera.h"
#include "CorneaIrLed.h"


using namespace eye_align;


std::mutex EyeAligner::singleMutex_;


namespace
{
	using Clock = std::chrono::steady_clock;

	struct EventRecord
	{
		int32_t event;
		int32_t mode;
		int32_t phase;
		int32_t arg;
		int32_t posX;
		int32_t posY;
		int32_t posZ;
		int32_t posSwing;
	};

	int elapsedMilliseconds(Clock::time_point since)
	{
		return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - since).count());
	}

	IrCameraId sideCameraForEyeSide(EyeSide side)
	{
		// OD (right eye) is watched by the RIGHT camera, OS (left eye) by the LEFT camera.
		// The lower camera serves both eyes; the other side camera is not used at all.
		return (side == EyeSide::OS) ? IrCameraId::CORNEA_LEFT : IrCameraId::CORNEA_RIGHT;
	}

	const char* phaseName(EyeAlignPhase phase)
	{
		switch (phase) {
		case EyeAlignPhase::IDLE: return "IDLE";
		case EyeAlignPhase::TO_READY_Z: return "TO_READY_Z";
		case EyeAlignPhase::TO_EYE_X: return "TO_EYE_X";
		case EyeAlignPhase::SWING_TO_TILT: return "SWING_TO_TILT";
		case EyeAlignPhase::SEARCHING: return "SEARCHING";
		case EyeAlignPhase::CENTERING_Y: return "CENTERING_Y";
		case EyeAlignPhase::APPROACHING: return "APPROACHING";
		case EyeAlignPhase::CENTERING_X: return "CENTERING_X";
		case EyeAlignPhase::CENTERING_Z: return "CENTERING_Z";
		case EyeAlignPhase::FOCUSED: return "FOCUSED";
		case EyeAlignPhase::MONITORING: return "MONITORING";
		case EyeAlignPhase::HELD: return "HELD";
		case EyeAlignPhase::CANCELED: return "CANCELED";
		case EyeAlignPhase::FAILED: return "FAILED";
		}
		return "?";
	}

	const char* eventName(EyeAlignEvent event)
	{
		switch (event) {
		case EyeAlignEvent::NONE: return "NONE";
		case EyeAlignEvent::FULL_ALIGN_STARTED: return "FULL_ALIGN_STARTED";
		case EyeAlignEvent::AUTO_ALIGN_STARTED: return "AUTO_ALIGN_STARTED";
		case EyeAlignEvent::MANUAL_ALIGN_STARTED: return "MANUAL_ALIGN_STARTED";
		case EyeAlignEvent::ALIGN_CANCELLED: return "ALIGN_CANCELLED";
		case EyeAlignEvent::ALIGN_FAILED: return "ALIGN_FAILED";
		case EyeAlignEvent::READY_POSITION_REACHED: return "READY_POSITION_REACHED";
		case EyeAlignEvent::EYE_SIDE_POSITION_REACHED: return "EYE_SIDE_POSITION_REACHED";
		case EyeAlignEvent::SWING_AT_TILT: return "SWING_AT_TILT";
		case EyeAlignEvent::SEARCH_STARTED: return "SEARCH_STARTED";
		case EyeAlignEvent::SIDE_EYE_FOUND: return "SIDE_EYE_FOUND";
		case EyeAlignEvent::SIDE_EYE_NOT_FOUND: return "SIDE_EYE_NOT_FOUND";
		case EyeAlignEvent::Y_CENTERED: return "Y_CENTERED";
		case EyeAlignEvent::APPROACH_STARTED: return "APPROACH_STARTED";
		case EyeAlignEvent::LOWER_EYE_FOUND: return "LOWER_EYE_FOUND";
		case EyeAlignEvent::LOWER_EYE_NOT_FOUND: return "LOWER_EYE_NOT_FOUND";
		case EyeAlignEvent::X_CENTERED: return "X_CENTERED";
		case EyeAlignEvent::Z_CENTERED: return "Z_CENTERED";
		case EyeAlignEvent::FOCUS_ALIGNED: return "FOCUS_ALIGNED";
		case EyeAlignEvent::FOCUS_LOST: return "FOCUS_LOST";
		case EyeAlignEvent::EYE_LOST: return "EYE_LOST";
		case EyeAlignEvent::ALIGN_HELD: return "ALIGN_HELD";
		case EyeAlignEvent::ALIGN_RESUMED: return "ALIGN_RESUMED";
		case EyeAlignEvent::EYE_TOO_CLOSE: return "EYE_TOO_CLOSE";
		case EyeAlignEvent::END_OF_LEFT: return "END_OF_LEFT";
		case EyeAlignEvent::END_OF_RIGHT: return "END_OF_RIGHT";
		case EyeAlignEvent::END_OF_TOP: return "END_OF_TOP";
		case EyeAlignEvent::END_OF_BOTTOM: return "END_OF_BOTTOM";
		case EyeAlignEvent::END_OF_FORWARD: return "END_OF_FORWARD";
		case EyeAlignEvent::END_OF_BACKWARD: return "END_OF_BACKWARD";
		case EyeAlignEvent::END_OF_SWING_LEFT: return "END_OF_SWING_LEFT";
		case EyeAlignEvent::END_OF_SWING_RIGHT: return "END_OF_SWING_RIGHT";
		case EyeAlignEvent::MOTOR_MOVE_FAILED: return "MOTOR_MOVE_FAILED";
		case EyeAlignEvent::PHASE_TIMEOUT: return "PHASE_TIMEOUT";
		case EyeAlignEvent::ALIGN_TIMEOUT: return "ALIGN_TIMEOUT";
		}
		return "?";
	}

	EyeAlignEvent endOfTravelEvent(MotorType type, int direction)
	{
		switch (type) {
		case MotorType::STAGE_X: return (direction == X_DIRECTION_LEFT) ? EyeAlignEvent::END_OF_LEFT : EyeAlignEvent::END_OF_RIGHT;
		case MotorType::STAGE_Y: return (direction == Y_DIRECTION_DOWN) ? EyeAlignEvent::END_OF_BOTTOM : EyeAlignEvent::END_OF_TOP;
		case MotorType::STAGE_Z: return (direction == Z_DIRECTION_FORWARD) ? EyeAlignEvent::END_OF_FORWARD : EyeAlignEvent::END_OF_BACKWARD;
		case MotorType::SWING: return (direction == SWING_DIRECTION_LEFT) ? EyeAlignEvent::END_OF_SWING_LEFT : EyeAlignEvent::END_OF_SWING_RIGHT;
		default: return EyeAlignEvent::NONE;
		}
	}

	int cameraIndex(IrCameraId cameraId)
	{
		const int index = static_cast<int>(cameraId);
		return (index >= 0 && index < IR_CAMERA_COUNT) ? index : -1;
	}
}


struct EyeAligner::EyeAlignerImpl
{
	// board
	MainBoard* board = nullptr;
	CorneaCamera* camera = nullptr;
	bool initialized = false;

	// configuration
	mutable std::mutex configMutex;
	EyeAlignConfigParam config;		// published by setConfig()
	EyeAlignConfigParam runConfig;		// snapshot used by the running worker

	// run control
	std::mutex startMutex;
	std::atomic<bool> running{ false };
	std::atomic<bool> stopping{ false };
	std::atomic<bool> skipBackOff{ false };	// a second cancel() (or the destructor): end without the Z retreat
	std::atomic<bool> holdRequested{ false };
	std::atomic<bool> held{ false };
	std::atomic<EyeAlignMode> mode{ EyeAlignMode::MANUAL };
	std::atomic<EyeAlignPhase> phase{ EyeAlignPhase::IDLE };
	std::atomic<EyeAlignResult> result{ EyeAlignResult::NONE };
	EyeSide eyeSide = EyeSide::OD;
	IrCameraId sideCameraId = IrCameraId::CORNEA_RIGHT;
	bool manual = false;

	std::thread workerThread;
	std::thread eventThread;

	// event queue (worker -> dispatcher)
	std::mutex eventMutex;
	std::condition_variable eventCondition;
	std::deque<EventRecord> eventQueue;
	bool eventThreadStop = false;

	// frames
	CorneaFrameStore frames;
	std::vector<uint8_t> sideFrame;
	std::vector<uint8_t> lowerFrame;
	int sideFrameWidth = DEFAULT_FRAME_WIDTH, sideFrameHeight = DEFAULT_FRAME_HEIGHT;
	int lowerFrameWidth = DEFAULT_FRAME_WIDTH, lowerFrameHeight = DEFAULT_FRAME_HEIGHT;
	uint64_t sideFrameSequence = 0, lowerFrameSequence = 0;

	// detection
	PupilDetector sideDetector;
	PupilDetector lowerDetector;
	PupilTracker sideTracker;
	PupilTracker lowerTracker;

	// pupil-centre network shared by both detectors and the offline path
	mutable std::mutex modelMutex;
	PupilModelConfig modelConfig;
	std::shared_ptr<PupilCenterModel> model;

	mutable std::mutex resultMutex;
	std::array<PupilDetectResult, IR_CAMERA_COUNT> latest{};
	std::array<Clock::time_point, IR_CAMERA_COUNT> latestResultTime{};

	mutable std::mutex overrideMutex;
	std::array<std::optional<PupilDetectResult>, IR_CAMERA_COUNT> overrides{};

	// motion: the only four motors this module may touch
	AxisController axisX;
	AxisController axisY;
	AxisController axisZ;
	AxisController axisSwing;
	std::atomic<AxisController*> movingAxis{ nullptr };	// axis inside a blocking move; cancel() aborts it

	// run state (worker thread only)
	Clock::time_point runStartTime;
	Clock::time_point phaseStartTime;
	Clock::time_point phaseDwellUntil;		// processPhase() idles until this instant
	Clock::time_point alignDeadline;		// runStartTime + alignTimeoutMs, pushed back by every hold
	Clock::time_point searchDeadline;		// search start + searchTimeoutMs, pushed back by every hold (steps 4 and 5a)
	Clock::time_point heldSince;			// start of the current hold
	Clock::time_point limitReachedAt;		// epoch: the current phase has not yet seen Z at its limit
	bool focusReached = false;				// first FOCUS_ALIGNED of the run delivered (timeout no longer applies)
	int loopCount = 0;
	int zForwardSign = +1;			// sign of (searchLimitPosZ - readyPosZ)
	bool eyeLostReported = false;
	// Stage limits are reported once per excursion (checkAxisLimits): the flag is set
	// when the limit event fires and clears once the axis has retreated from the limit.
	struct LimitReport { bool minEnd = false; bool maxEnd = false; };
	LimitReport limitX, limitY, limitZ;
	bool safeZReported = false;		// Z at/past minSafePosZ (EYE_TOO_CLOSE)
	int settleCount = 0;
	EyeAlignPhase resumePhase = EyeAlignPhase::CENTERING_Z;
	std::atomic<bool> endRequested{ false };	// read by cancel() on the caller's thread
	EyeAlignPhase endPhase = EyeAlignPhase::CANCELED;
	EyeAlignResult endReason = EyeAlignResult::USER_CANCEL;
	float errorX = 0.0f, errorY = 0.0f, errorZ = 0.0f;

	// status snapshot for fetchStatus()
	mutable std::mutex statusMutex;
	EyeAlignStatus status{};
};


EyeAligner::EyeAligner() :
	d_ptr(std::make_unique<EyeAlignerImpl>())
{}


EyeAligner::~EyeAligner()
{
	try {
		impl().skipBackOff = true;		// the board may be gone: no retreat move at exit
		cancel(true);
	}
	catch (...) {
	}
}


EyeAligner* eye_align::EyeAligner::getInstance(void)
{
	// Static local variable initialization is thread-safe and happens only once.
	static EyeAligner instance;
	return &instance;
}


// Initialisation / configuration
////////////////////////////////////////////////////////////////////////////////////////////////////

bool eye_align::EyeAligner::initializeEyeAligner(wso_device::MainBoard* board)
{
	if (board == nullptr) {
		return false;
	}
	cancel(true);

	auto& state = impl();
	state.board = board;
	state.camera = board->getCorneaCamera();
	state.axisX = AxisController(board->getXstageMotor(), MotorType::STAGE_X, "StageX");
	state.axisY = AxisController(board->getYstageMotor(), MotorType::STAGE_Y, "StageY");
	state.axisZ = AxisController(board->getZstageMotor(), MotorType::STAGE_Z, "StageZ");
	state.axisSwing = AxisController(board->getSwingMotor(), MotorType::SWING, "Swing");
	state.initialized = true;

	LogI() << "EyeAligner initialized, camera: " << (state.camera ? "ok" : "none")
		<< ", motors X/Y/Z/Swing: " << state.axisX.isValid() << state.axisY.isValid() << state.axisZ.isValid() << state.axisSwing.isValid();
	return true;
}

bool eye_align::EyeAligner::isInitialized(void) const
{
	return impl().initialized;
}

void eye_align::EyeAligner::releaseEyeAligner(void)
{
	cancel(true);
	auto& state = impl();
	state.frames.setAccepting(false);
	// Release the inference session deterministically while the process is healthy.
	state.sideDetector.setModel(nullptr);
	state.lowerDetector.setModel(nullptr);
	{
		std::lock_guard<std::mutex> lock(state.modelMutex);
		state.model.reset();
	}
	state.board = nullptr;
	state.camera = nullptr;
	state.initialized = false;
}

void eye_align::EyeAligner::setModelConfig(const PupilModelConfig& config)
{
	auto& state = impl();
	std::lock_guard<std::mutex> lock(state.modelMutex);
	const bool reload = !state.model
		|| state.model->getConfig().modelPath != config.modelPath
		|| state.model->getConfig().intraOpThreads != config.intraOpThreads;
	state.modelConfig = config;
	if (reload) {
		// Eager load on the caller's thread (configuration time), so the session creation
		// never stalls the first frame of the control loop.
		state.model = std::make_shared<PupilCenterModel>(config);
		state.model->load();
	}
	else {
		state.model->setPresenceThreshold(config.presenceThreshold);
	}
}

PupilModelConfig eye_align::EyeAligner::getModelConfig(void) const
{
	std::lock_guard<std::mutex> lock(impl().modelMutex);
	return impl().modelConfig;
}

std::shared_ptr<PupilCenterModel> eye_align::EyeAligner::acquireModel(void) const
{
	auto& state = impl();
	std::lock_guard<std::mutex> lock(state.modelMutex);
	if (!state.model) {
		state.model = std::make_shared<PupilCenterModel>(state.modelConfig);
		state.model->load();
	}
	return state.model;
}

void eye_align::EyeAligner::setConfig(const EyeAlignConfigParam& config)
{
	std::lock_guard<std::mutex> lock(impl().configMutex);
	impl().config = config;
}

EyeAlignConfigParam eye_align::EyeAligner::getConfig(void) const
{
	std::lock_guard<std::mutex> lock(impl().configMutex);
	return impl().config;
}


// Run control
////////////////////////////////////////////////////////////////////////////////////////////////////

EyeAlignResult eye_align::EyeAligner::startAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback)
{
	auto& state = impl();
	std::lock_guard<std::mutex> startLock(state.startMutex);

	if (!state.initialized) {
		return EyeAlignResult::NOT_INITIALIZED;
	}
	if (state.running) {
		return EyeAlignResult::BUSY;
	}
	// The previous run may have ended on its own; its threads must be joined before
	// the thread objects are reused.
	joinThreads();

	if (state.camera == nullptr || !state.camera->isPlaying()) {
		return EyeAlignResult::PREVIEW_NOT_RUNNING;
	}

	state.manual = (mode == EyeAlignMode::MANUAL);
	if (!state.manual) {
		if (!state.axisX.isReady() || !state.axisY.isReady() || !state.axisZ.isReady() || !state.axisSwing.isReady()) {
			return EyeAlignResult::MOTOR_NOT_READY;
		}
	}

	{
		std::lock_guard<std::mutex> lock(state.configMutex);
		state.runConfig = state.config;
	}

	if (mode == EyeAlignMode::FULL) {
		eyeSide = EyeSide::OD;			// FULL always starts with the right eye
	}
	else if (eyeSide != EyeSide::OS) {
		eyeSide = EyeSide::OD;
	}
	state.eyeSide = eyeSide;
	state.sideCameraId = sideCameraForEyeSide(eyeSide);
	state.mode = mode;

	if (auto* registry = CallbackRegistry::getInstance(); registry) {
		registry->setEyeAlignEventOccurred(callback);
	}

	// detection
	state.sideDetector.setCameraId(state.sideCameraId);
	state.sideDetector.setParam(state.sideCameraId == IrCameraId::CORNEA_LEFT ? state.runConfig.detLeft : state.runConfig.detRight);
	state.lowerDetector.setCameraId(IrCameraId::CORNEA_LOWER);
	state.lowerDetector.setParam(state.runConfig.detLower);
	{
		const auto model = acquireModel();
		state.sideDetector.setModel(model);
		state.lowerDetector.setModel(model);
	}

	PupilTrackerConfig trackerConfig;
	trackerConfig.lockAfterConsecutiveHits = state.runConfig.detectConsecutive;
	trackerConfig.unlockAfterConsecutiveMisses = state.runConfig.lostFramesToStop;
	trackerConfig.maxCenterJumpPixels = state.runConfig.maxJumpPx;
	trackerConfig.smoothingAlpha = state.runConfig.smoothingAlpha;
	state.sideTracker.configure(trackerConfig);
	state.lowerTracker.configure(trackerConfig);
	state.sideTracker.reset();
	state.lowerTracker.reset();
	{
		std::lock_guard<std::mutex> lock(state.resultMutex);
		state.latest.fill(PupilDetectResult());
		state.latestResultTime.fill(Clock::time_point());
	}

	// frames: only the side camera of this eye and the lower camera are consumed; the
	// store drops every other camera on the camera thread (no copy, no worker wake-up).
	state.frames.clear();
	state.frames.setEnabledCameras({ state.sideCameraId, IrCameraId::CORNEA_LOWER });
	state.sideFrameWidth = state.lowerFrameWidth = DEFAULT_FRAME_WIDTH;
	state.sideFrameHeight = state.lowerFrameHeight = DEFAULT_FRAME_HEIGHT;
	state.sideFrameSequence = state.lowerFrameSequence = 0;

	// run state
	state.loopCount = 0;
	state.zForwardSign = (state.runConfig.searchLimitPosZ >= state.runConfig.readyPosZ) ? +1 : -1;
	state.movingAxis = nullptr;
	state.eyeLostReported = false;
	state.limitX = state.limitY = state.limitZ = {};
	state.safeZReported = false;
	state.settleCount = 0;
	state.focusReached = false;
	state.limitReachedAt = Clock::time_point();
	state.resumePhase = EyeAlignPhase::CENTERING_Z;
	state.endRequested = false;
	state.endPhase = EyeAlignPhase::CANCELED;
	state.endReason = EyeAlignResult::USER_CANCEL;
	state.errorX = state.errorY = state.errorZ = 0.0f;
	state.result = EyeAlignResult::NONE;
	state.phase = state.manual ? EyeAlignPhase::MONITORING : EyeAlignPhase::TO_READY_Z;

	{
		std::lock_guard<std::mutex> lock(state.eventMutex);
		state.eventQueue.clear();
		state.eventThreadStop = false;
	}
	state.stopping = false;
	state.skipBackOff = false;
	state.holdRequested = false;
	state.held = false;
	state.runStartTime = state.phaseStartTime = state.phaseDwellUntil = state.heldSince = Clock::now();
	state.alignDeadline = (state.runConfig.alignTimeoutMs > 0)
		? state.runStartTime + std::chrono::milliseconds(state.runConfig.alignTimeoutMs)
		: Clock::time_point::max();
	state.searchDeadline = Clock::time_point::max();		// armed when the search starts
	state.frames.setAccepting(true);
	state.running = true;

	state.eventThread = std::thread(&EyeAligner::runEventDispatch, this);
	state.workerThread = std::thread(&EyeAligner::runControlLoop, this);

	LogI() << "EyeAligner started, mode: " << static_cast<int>(mode) << ", eye side: " << static_cast<int>(eyeSide);
	return EyeAlignResult::NONE;
}

void eye_align::EyeAligner::cancel(bool wait)
{
	auto& state = impl();
	if (state.running) {
		if (state.stopping.exchange(true) || state.endRequested) {
			state.skipBackOff = true;		// cancelled again, or during an automatic ending: no retreat
		}
		state.frames.wake();
		// The worker may be blocked inside a position move (a phase move or the
		// retreat to readyPosZ); a stop makes it return.
		if (AxisController* axis = state.movingAxis.load(); axis != nullptr) {
			axis->abortMove();
		}
	}
	if (wait) {
		joinThreads();
	}
}

void eye_align::EyeAligner::joinThreads(void)
{
	auto& state = impl();
	// Never join a thread from itself (cancel() called from within the event callback).
	if (state.workerThread.joinable() && state.workerThread.get_id() != std::this_thread::get_id()) {
		state.workerThread.join();
	}
	if (state.eventThread.joinable() && state.eventThread.get_id() != std::this_thread::get_id()) {
		state.eventThread.join();
	}
}

bool eye_align::EyeAligner::hold(void)
{
	auto& state = impl();
	if (!state.running || state.manual || state.stopping) {
		return false;
	}
	switch (state.phase.load()) {
	case EyeAlignPhase::SEARCHING:
	case EyeAlignPhase::CENTERING_Y:
	case EyeAlignPhase::APPROACHING:
	case EyeAlignPhase::CENTERING_X:
	case EyeAlignPhase::CENTERING_Z:
	case EyeAlignPhase::FOCUSED:
	case EyeAlignPhase::HELD:
		break;
	default:
		return false;
	}
	state.holdRequested = true;
	state.frames.wake();
	return true;
}

bool eye_align::EyeAligner::resume(void)
{
	auto& state = impl();
	if (!state.running || !state.holdRequested) {
		return false;
	}
	state.holdRequested = false;
	state.frames.wake();
	return true;
}

bool eye_align::EyeAligner::isHeld(void) const
{
	return impl().held;
}

bool eye_align::EyeAligner::isRunning(void) const
{
	return impl().running;
}

bool eye_align::EyeAligner::isCancelling(void) const
{
	return impl().running && impl().stopping;
}

EyeAlignMode eye_align::EyeAligner::getMode(void) const
{
	return impl().mode;
}

EyeAlignPhase eye_align::EyeAligner::getPhase(void) const
{
	return impl().phase;
}

EyeAlignResult eye_align::EyeAligner::getResult(void) const
{
	return impl().result;
}

bool eye_align::EyeAligner::fetchStatus(EyeAlignStatus* status) const
{
	if (status == nullptr) {
		return false;
	}
	std::lock_guard<std::mutex> lock(impl().statusMutex);
	*status = impl().status;
	return true;
}


// Pupil results
////////////////////////////////////////////////////////////////////////////////////////////////////

bool eye_align::EyeAligner::isPupilDetected(IrCameraId cameraId) const
{
	PupilDetectResult result;
	return getPupilResult(cameraId, &result) && result.found != 0;
}

bool eye_align::EyeAligner::getPupilResult(IrCameraId cameraId, PupilDetectResult* result) const
{
	const int index = cameraIndex(cameraId);
	if (index < 0 || result == nullptr) {
		return false;
	}
	std::lock_guard<std::mutex> lock(impl().resultMutex);
	const auto& resultTime = impl().latestResultTime[index];
	if (resultTime == Clock::time_point() || elapsedMilliseconds(resultTime) > PUPIL_RESULT_STALE_MS) {
		return false;
	}
	*result = impl().latest[index];
	return true;
}

PupilDetectResult eye_align::EyeAligner::detectPupilInImage(const std::uint8_t* gray, int width, int height, IrCameraId cameraId) const
{
	PupilDetectParam param;
	{
		std::lock_guard<std::mutex> lock(impl().configMutex);
		switch (cameraId) {
		case IrCameraId::CORNEA_LEFT: param = impl().config.detLeft; break;
		case IrCameraId::CORNEA_RIGHT: param = impl().config.detRight; break;
		default: param = impl().config.detLower; break;
		}
	}
	PupilDetector detector(cameraId, param);
	detector.setModel(acquireModel());
	return detector.detect(gray, width, height, 0, 0);
}

void eye_align::EyeAligner::setPupilOverride(IrCameraId cameraId, const PupilDetectResult* result)
{
	const int index = cameraIndex(cameraId);
	if (index < 0) {
		return;
	}
	std::lock_guard<std::mutex> lock(impl().overrideMutex);
	if (result) {
		PupilDetectResult overrideResult = *result;
		overrideResult.camId = static_cast<int32_t>(cameraId);
		impl().overrides[index] = overrideResult;
	}
	else {
		impl().overrides[index].reset();
	}
}

void eye_align::EyeAligner::clearPupilOverrides(void)
{
	std::lock_guard<std::mutex> lock(impl().overrideMutex);
	for (auto& item : impl().overrides) {
		item.reset();
	}
}

void eye_align::EyeAligner::onCorneaFrame(IrCameraId cameraId, const std::uint8_t* pixels, int width, int height)
{
	// CorneaFrameStore::push() returns immediately while nothing is running.
	impl().frames.push(cameraId, pixels, width, height);
}


// Threads
////////////////////////////////////////////////////////////////////////////////////////////////////

void eye_align::EyeAligner::runControlLoop(void)
{
	auto& state = impl();
	try {
		switch (state.mode.load()) {
		case EyeAlignMode::FULL: raiseEvent(EyeAlignEvent::FULL_ALIGN_STARTED, static_cast<int>(state.eyeSide)); break;
		case EyeAlignMode::AUTO: raiseEvent(EyeAlignEvent::AUTO_ALIGN_STARTED, static_cast<int>(state.eyeSide)); break;
		default: raiseEvent(EyeAlignEvent::MANUAL_ALIGN_STARTED, static_cast<int>(state.eyeSide)); break;
		}

		setupCorneaLeds();

		if (!state.manual) {
			// Fresh positions before the first move decision.
			state.axisX.poll();
			state.axisY.poll();
			state.axisZ.poll();
			state.axisSwing.poll();
		}

		while (!state.stopping) {
			state.frames.waitForNewFrame(state.stopping, CONTROL_LOOP_MAX_WAIT_MS);
			if (state.stopping) {
				break;
			}
			if (!runControlIteration()) {
				break;
			}
		}
	}
	catch (...) {
		LogE() << "EyeAligner: unhandled exception in the control loop";
		if (!state.endRequested) {
			requestRunEnd(EyeAlignPhase::FAILED, EyeAlignResult::INTERNAL_ERROR);
		}
	}
	finishRun();
}

void eye_align::EyeAligner::runEventDispatch(void)
{
	auto& state = impl();
	for (;;) {
		EventRecord record{};
		{
			std::unique_lock<std::mutex> lock(state.eventMutex);
			state.eventCondition.wait(lock, [&] { return !state.eventQueue.empty() || state.eventThreadStop; });
			if (state.eventQueue.empty()) {
				break;		// eventThreadStop and nothing left to deliver
			}
			record = state.eventQueue.front();
			state.eventQueue.pop_front();
		}
		if (auto* registry = CallbackRegistry::getInstance(); registry) {
			registry->runEyeAlignEventOccurred(record.event, record.mode, record.phase, record.arg,
				record.posX, record.posY, record.posZ, record.posSwing);
		}
	}
}

void eye_align::EyeAligner::finishRun(void)
{
	auto& state = impl();
	try {
		stopAllAxes();
		state.frames.setAccepting(false);

		if (!state.endRequested) {
			// Left the loop because cancel() set the stop flag.
			state.endPhase = EyeAlignPhase::CANCELED;
			state.endReason = EyeAlignResult::USER_CANCEL;
		}
		state.result = state.endReason;

		// Every ending - a user cancel included - retreats Z to the ready position so
		// the head is never left close to the eye; a second cancel() aborts the retreat.
		if (!state.manual && state.runConfig.backOffOnEnd != 0) {
			backOffToReadyPosition();
		}
		stopAllAxes();
	}
	catch (...) {
		LogE() << "EyeAligner: exception while finishing the run";
	}

	state.held = false;
	state.holdRequested = false;
	state.phase = state.endPhase;
	updateStatusSnapshot();

	const auto terminalEvent = (state.endPhase == EyeAlignPhase::CANCELED) ? EyeAlignEvent::ALIGN_CANCELLED : EyeAlignEvent::ALIGN_FAILED;
	raiseEvent(terminalEvent, static_cast<int>(state.endReason));
	LogI() << "EyeAligner finished, phase: " << phaseName(state.endPhase) << ", reason: " << static_cast<int>(state.endReason);

	state.running = false;
	{
		std::lock_guard<std::mutex> lock(state.eventMutex);
		state.eventThreadStop = true;
	}
	state.eventCondition.notify_all();
}

void eye_align::EyeAligner::backOffToReadyPosition(void)
{
	auto& state = impl();
	if (!state.axisZ.isValid()) {
		return;
	}
	// One blocking position move back to readyPosZ (the motor's own wait bounds it).
	// The run's cancel() has usually set stopping already, so that flag does not stop
	// the retreat; a further cancel() sets skipBackOff and aborts the move through
	// movingAxis, like any phase move.
	state.axisZ.poll();
	if (state.skipBackOff || std::abs(state.axisZ.getPosition() - state.runConfig.readyPosZ) <= state.runConfig.posToleranceSteps) {
		return;
	}
	state.movingAxis = &state.axisZ;
	const bool reached = state.axisZ.moveTo(state.runConfig.readyPosZ);
	state.movingAxis = nullptr;
	if (state.skipBackOff) {
		LogI() << "EyeAligner: back-off to the ready position aborted at " << state.axisZ.getPosition();
	}
	else if (!reached || std::abs(state.axisZ.getPosition() - state.runConfig.readyPosZ) > state.runConfig.posToleranceSteps) {
		LogW() << "EyeAligner: back-off to the ready position ended at " << state.axisZ.getPosition()
			<< " (target " << state.runConfig.readyPosZ << ")";
	}
}


// Control loop
////////////////////////////////////////////////////////////////////////////////////////////////////

bool eye_align::EyeAligner::runControlIteration(void)
{
	auto& state = impl();
	state.loopCount++;

	updateDetections();

	if (!state.manual) {
		pollAxes();
		if (!checkAxisFaults()) {
			return false;
		}
		checkAxisLimits();
	}
	if (!applyHoldRequest()) {
		return false;
	}
	// After the hold handling (a resume pushes the deadline back first) and before the
	// lost-pupil rule and the phase (no new move after expiry; the first requestRunEnd
	// of an iteration wins, so this order makes the timeout the reported reason).
	if (!checkAlignTimeout()) {
		return false;
	}
	if (!applyLostPupilRule()) {
		return false;
	}
	if (!processPhase()) {
		return false;
	}

	updateStatusSnapshot();
	return !state.endRequested;
}

void eye_align::EyeAligner::updateDetections(void)
{
	auto& state = impl();

	auto detectFrame = [&](IrCameraId cameraId, const PupilDetector& detector, const std::vector<uint8_t>& frame, int width, int height, uint64_t frameSequence) {
		PupilDetectResult result;
		bool overridden = false;
		{
			std::lock_guard<std::mutex> lock(state.overrideMutex);
			const int index = cameraIndex(cameraId);
			if (index >= 0 && state.overrides[index].has_value()) {
				result = *state.overrides[index];
				result.frameSeq = static_cast<int32_t>(frameSequence);
				overridden = true;
			}
		}
		if (!overridden) {
			result = detector.detect(frame.data(), width, height, 0, static_cast<int>(frameSequence));
		}
		const int index = cameraIndex(cameraId);
		if (index >= 0) {
			std::lock_guard<std::mutex> lock(state.resultMutex);
			state.latest[index] = result;
			state.latestResultTime[index] = Clock::now();
		}
		return result;
	};

	if (state.frames.takeLatest(state.sideCameraId, state.sideFrame, state.sideFrameWidth, state.sideFrameHeight, state.sideFrameSequence)) {
		const auto result = detectFrame(state.sideCameraId, state.sideDetector, state.sideFrame, state.sideFrameWidth, state.sideFrameHeight, state.sideFrameSequence);
		state.sideTracker.update(result);
		if (state.sideTracker.justLocked()) {
			raiseEvent(EyeAlignEvent::SIDE_EYE_FOUND, static_cast<int>(state.sideCameraId));
		}
	}
	if (state.frames.takeLatest(IrCameraId::CORNEA_LOWER, state.lowerFrame, state.lowerFrameWidth, state.lowerFrameHeight, state.lowerFrameSequence)) {
		const auto result = detectFrame(IrCameraId::CORNEA_LOWER, state.lowerDetector, state.lowerFrame, state.lowerFrameWidth, state.lowerFrameHeight, state.lowerFrameSequence);
		state.lowerTracker.update(result);
		if (state.lowerTracker.justLocked()) {
			raiseEvent(EyeAlignEvent::LOWER_EYE_FOUND, static_cast<int>(IrCameraId::CORNEA_LOWER));
		}
	}
	// Signed pixel errors after the per-camera sign mapping. A positive mapped error
	// moves the axis in POSITIVE_STEP_DIRECTION; the config signs absorb the mirror
	// modes. Y comes from the side camera (vertical), X and Z from the lower camera
	// (horizontal / vertical: the pupil rises in the lower image as Z advances).
	const auto& sideParam = state.sideDetector.getParam();
	const auto& lowerParam = state.lowerDetector.getParam();
	state.errorY = (state.sideTracker.centerY() - state.sideDetector.getTargetY(state.sideFrameHeight)) * static_cast<float>(sideParam.signV);
	state.errorX = (state.lowerTracker.centerX() - state.lowerDetector.getTargetX(state.lowerFrameWidth)) * static_cast<float>(lowerParam.signH);
	state.errorZ = (state.lowerTracker.centerY() - state.lowerDetector.getTargetY(state.lowerFrameHeight)) * static_cast<float>(lowerParam.signV);
}

void eye_align::EyeAligner::pollAxes(void)
{
	auto& state = impl();
	const bool isIdlePollTurn = (state.loopCount % IDLE_AXIS_POLL_PERIOD) == 0;
	// Z is polled every iteration while a phase moves it toward a limit: the limit tests
	// and the end-switch pre-check of stepAxis() must not run on ten-frame-old data on
	// the frames where no move happened (a move refreshes the position on its return).
	const auto phase = state.phase.load();
	const bool zActive = phase == EyeAlignPhase::SEARCHING || phase == EyeAlignPhase::APPROACHING || phase == EyeAlignPhase::CENTERING_Z;
	for (AxisController* axis : { &state.axisX, &state.axisY, &state.axisZ, &state.axisSwing }) {
		const bool pollNow = axis->isJogging() || isIdlePollTurn || (zActive && axis == &state.axisZ);
		if (axis->isValid() && pollNow) {
			axis->poll();
		}
	}
}

bool eye_align::EyeAligner::checkAxisFaults(void)
{
	auto& state = impl();
	for (AxisController* axis : { &state.axisX, &state.axisY, &state.axisZ, &state.axisSwing }) {
		if (!axis->isValid()) {
			continue;
		}
		if (axis->hasCommandFailed()) {
			stopAllAxes();
			raiseEvent(EyeAlignEvent::MOTOR_MOVE_FAILED, static_cast<int>(axis->getMotorType()));
			requestRunEnd(EyeAlignPhase::FAILED, EyeAlignResult::MOTOR_MOVE_FAILED);
			return false;
		}
	}
	return true;
}

void eye_align::EyeAligner::checkAxisLimits(void)
{
	auto& state = impl();
	// The one place that reports stage limits. Each limit is reported once per
	// excursion: the event fires when the axis is seen at the limit and again only
	// after it has retreated and come back. Reaching a limit never ends the run; the
	// moves are clipped (Z at minSafePosZ) or skipped (end switches) instead. The
	// search limit is the searching phase's own business (EYE_NOT_FOUND).
	auto report = [this](bool& reported, bool atLimit, EyeAlignEvent event, int arg) {
		if (atLimit && !reported) {
			reported = true;
			raiseEvent(event, arg);
		}
		else if (!atLimit) {
			reported = false;
		}
	};
	auto checkEnds = [&](const AxisController& axis, auto& limit) {
		if (!axis.isValid()) {
			return;
		}
		const MotorType type = axis.getMotorType();
		report(limit.minEnd, axis.isAtMinimumEnd(), endOfTravelEvent(type, 0), static_cast<int>(type));
		report(limit.maxEnd, axis.isAtMaximumEnd(), endOfTravelEvent(type, 1), static_cast<int>(type));
	};
	checkEnds(state.axisX, state.limitX);
	checkEnds(state.axisY, state.limitY);
	checkEnds(state.axisZ, state.limitZ);
	if (state.axisZ.isValid()) {
		// EYE_TOO_CLOSE with hysteresis: set once Z has reached minSafePosZ (within the
		// position band, where forward moves are clipped away), cleared only after Z has
		// retreated clearly from it, so Z following the pupil around the limit reports once.
		const int distance = state.zForwardSign * (state.runConfig.minSafePosZ - state.axisZ.getPosition());
		if (hasZReached(state.runConfig.minSafePosZ)) {
			if (!state.safeZReported) {
				state.safeZReported = true;
				raiseEvent(EyeAlignEvent::EYE_TOO_CLOSE, 0);
			}
		}
		else if (distance > 2 * state.runConfig.posToleranceSteps) {
			state.safeZReported = false;
		}
	}
}

int eye_align::EyeAligner::clipZ(int offset) const
{
	const auto& state = impl();
	// Z stays inside [readyPosZ, minSafePosZ] in the forward direction: a forward offset
	// is shortened to land on minSafePosZ, a backward one to land on readyPosZ. The last
	// posToleranceSteps before a bound are not worth a move (the motor's own arrival band
	// would stop short anyway), so such an offset becomes 0.
	const int sign = state.zForwardSign;
	const int forward = sign * offset;
	if (forward == 0) {
		return 0;
	}
	const int direction = (forward > 0) ? +1 : -1;
	const int bound = (forward > 0) ? state.runConfig.minSafePosZ : state.runConfig.readyPosZ;
	const int remaining = std::max(0, direction * sign * (bound - state.axisZ.getPosition()));
	if (remaining <= state.runConfig.posToleranceSteps) {
		return 0;
	}
	return sign * direction * std::min(std::abs(forward), remaining);
}

bool eye_align::EyeAligner::applyHoldRequest(void)
{
	auto& state = impl();
	if (state.manual) {
		return true;
	}
	if (state.holdRequested && !state.held) {
		stopAllAxes();
		// hold() accepts only the phases that can simply continue afterwards, FOCUSED
		// included: resuming into FOCUSED lets phaseFocused() decide whether the eye
		// drifted meanwhile instead of re-announcing FOCUS_ALIGNED unconditionally.
		state.resumePhase = state.phase.load();
		state.held = true;
		state.heldSince = Clock::now();
		changePhase(EyeAlignPhase::HELD);
		raiseEvent(EyeAlignEvent::ALIGN_HELD);
	}
	else if (!state.holdRequested && state.held) {
		state.held = false;
		const auto heldFor = Clock::now() - state.heldSince;		// held time does not count
		if (state.alignDeadline != Clock::time_point::max()) {
			state.alignDeadline += heldFor;
		}
		if (state.searchDeadline != Clock::time_point::max()) {
			state.searchDeadline += heldFor;
		}
		changePhase(state.resumePhase);
		raiseEvent(EyeAlignEvent::ALIGN_RESUMED);
	}
	return true;
}

const PupilTracker* eye_align::EyeAligner::watchedTracker(IrCameraId& cameraId) const
{
	const auto& state = impl();
	// The side camera matters until Y is centred, the lower camera from the X centring on;
	// each only once it has locked at least once. HELD watches the phase it resumes
	// into. MANUAL keeps both and reports the one that has been missing longer.
	const PupilTracker* side = state.sideTracker.everLocked() ? &state.sideTracker : nullptr;
	const PupilTracker* lower = state.lowerTracker.everLocked() ? &state.lowerTracker : nullptr;
	cameraId = state.sideCameraId;
	if (state.manual) {
		if (lower != nullptr && (side == nullptr || lower->lostFrames() > side->lostFrames())) {
			cameraId = IrCameraId::CORNEA_LOWER;
			return lower;
		}
		return side;
	}
	auto phase = state.phase.load();
	if (phase == EyeAlignPhase::HELD) {
		phase = state.resumePhase;
	}
	switch (phase) {
	case EyeAlignPhase::SEARCHING:
	case EyeAlignPhase::APPROACHING:
	case EyeAlignPhase::CENTERING_Y:
		return side;
	case EyeAlignPhase::CENTERING_X:
	case EyeAlignPhase::CENTERING_Z:
	case EyeAlignPhase::FOCUSED:
		cameraId = IrCameraId::CORNEA_LOWER;
		return lower;
	default:
		return nullptr;
	}
}

bool eye_align::EyeAligner::applyLostPupilRule(void)
{
	auto& state = impl();
	IrCameraId lostCameraId = state.sideCameraId;
	const PupilTracker* tracker = watchedTracker(lostCameraId);
	if (tracker == nullptr) {
		return true;		// the rule starts once the watched camera has found the eye
	}
	const int lostFrames = tracker->lostFrames();

	if (lostFrames >= state.runConfig.lostFramesToStop) {
		if (!state.eyeLostReported) {
			state.eyeLostReported = true;
			if (!state.manual) {
				stopAllAxes();
			}
			raiseEvent(EyeAlignEvent::EYE_LOST, static_cast<int>(lostCameraId));
			if (state.phase == EyeAlignPhase::FOCUSED) {
				raiseEvent(EyeAlignEvent::FOCUS_LOST);
				changePhase(state.manual ? EyeAlignPhase::MONITORING : EyeAlignPhase::CENTERING_Z);
			}
		}
		if (!state.manual && lostFrames >= state.runConfig.lostFramesToCancel) {
			requestRunEnd(EyeAlignPhase::CANCELED, EyeAlignResult::PUPIL_LOST);
			return false;
		}
	}
	else if (state.eyeLostReported && lostFrames == 0) {
		state.eyeLostReported = false;
	}
	return true;
}

bool eye_align::EyeAligner::checkAlignTimeout(void)
{
	auto& state = impl();
	if (state.manual || state.held || state.focusReached || Clock::now() < state.alignDeadline) {
		return true;
	}
	raiseEvent(EyeAlignEvent::ALIGN_TIMEOUT, static_cast<int>(state.phase.load()));
	requestRunEnd(EyeAlignPhase::CANCELED, EyeAlignResult::ALIGN_TIMEOUT);
	return false;
}

bool eye_align::EyeAligner::processPhase(void)
{
	auto& state = impl();
	if (Clock::now() < state.phaseDwellUntil) {
		return true;		// end-of-phase dwell: no motion until it elapses
	}
	switch (state.phase.load()) {
	case EyeAlignPhase::TO_READY_Z: return phaseToReadyZ();
	case EyeAlignPhase::TO_EYE_X: return phaseToEyeX();
	case EyeAlignPhase::SWING_TO_TILT: return phaseSwingToTilt();
	case EyeAlignPhase::SEARCHING: return phaseSearching();
	case EyeAlignPhase::CENTERING_Y: return phaseCenteringY();
	case EyeAlignPhase::APPROACHING: return phaseApproaching();
	case EyeAlignPhase::CENTERING_X: return phaseCenteringX();
	case EyeAlignPhase::CENTERING_Z: return phaseCenteringZ();
	case EyeAlignPhase::FOCUSED: return state.manual ? phaseMonitoring() : phaseFocused();
	case EyeAlignPhase::MONITORING: return phaseMonitoring();
	case EyeAlignPhase::HELD: return true;
	case EyeAlignPhase::IDLE:
	case EyeAlignPhase::CANCELED:
	case EyeAlignPhase::FAILED:
		return false;
	}
	return false;
}


// Phases
////////////////////////////////////////////////////////////////////////////////////////////////////

bool eye_align::EyeAligner::movePhaseAxisTo(AxisController& axis, int target, EyeAlignPhase phase, int timeoutMs)
{
	auto& state = impl();
	// One blocking position move (StepMotor::updatePosition). The worker does not
	// iterate while it runs; cancel() aborts it through movingAxis->abortMove(). The
	// move returns with the motor at rest, so the caller's event carries the final position.
	if (state.stopping) {
		return false;
	}
	state.movingAxis = &axis;
	const bool reached = axis.moveTo(target);
	state.movingAxis = nullptr;
	if (state.stopping) {
		return false;		// aborted by cancel(); finishRun() reports USER_CANCEL
	}
	if (reached && std::abs(axis.getPosition() - target) <= state.runConfig.posToleranceSteps) {
		return true;
	}
	if (axis.isAtMinimumEnd() || axis.isAtMaximumEnd()) {
		// The motor's wait returns on an end switch and reports success; the
		// tolerance check above is what tells us the target was not reached. A
		// position phase cannot continue without its target, so this one does end
		// the run (the limit event itself comes from checkAxisLimits).
		checkAxisLimits();
		requestRunEnd(EyeAlignPhase::FAILED, EyeAlignResult::LIMIT_REACHED);
	}
	else if (elapsedMilliseconds(state.phaseStartTime) > timeoutMs) {
		raiseEvent(EyeAlignEvent::PHASE_TIMEOUT, static_cast<int>(phase));
		requestRunEnd(EyeAlignPhase::FAILED, EyeAlignResult::TIMEOUT);
	}
	else {
		raiseEvent(EyeAlignEvent::MOTOR_MOVE_FAILED, static_cast<int>(axis.getMotorType()));
		requestRunEnd(EyeAlignPhase::FAILED, EyeAlignResult::MOTOR_MOVE_FAILED);
	}
	return false;
}

bool eye_align::EyeAligner::phaseToReadyZ(void)
{
	auto& state = impl();
	// Step 1: Z moves by position to the safe ready distance.
	if (!movePhaseAxisTo(state.axisZ, state.runConfig.readyPosZ, EyeAlignPhase::TO_READY_Z, READY_TIMEOUT_MS)) {
		return false;
	}
	raiseEvent(EyeAlignEvent::READY_POSITION_REACHED, static_cast<int>(state.eyeSide));
	changePhase(EyeAlignPhase::TO_EYE_X, PHASE_END_DWELL_MS);
	return true;
}

bool eye_align::EyeAligner::phaseToEyeX(void)
{
	auto& state = impl();
	// Step 2: X moves by position in front of the selected eye. When the head stands
	// on the other eye's side (past the midpoint between the two eye positions), the
	// swing returns to zero first so a tilted head is not carried across the nose.
	const int target = (state.eyeSide == EyeSide::OS) ? state.runConfig.eyePosX_OS : state.runConfig.eyePosX_OD;
	const int other = (state.eyeSide == EyeSide::OS) ? state.runConfig.eyePosX_OD : state.runConfig.eyePosX_OS;
	const int midpoint = (target + other) / 2;
	const bool onOtherSide = (other > target) ? (state.axisX.getPosition() > midpoint) : (state.axisX.getPosition() < midpoint);
	const bool swingAtZero = std::abs(state.axisSwing.getPosition()) <= state.runConfig.posToleranceSteps;
	if (onOtherSide && !swingAtZero) {
		LogI() << "EyeAligner: head on the other eye's side, swing to zero before the X move";
		if (!movePhaseAxisTo(state.axisSwing, 0, EyeAlignPhase::TO_EYE_X, SWING_TIMEOUT_MS)) {
			return false;
		}
	}
	if (!movePhaseAxisTo(state.axisX, target, EyeAlignPhase::TO_EYE_X, EYE_X_TIMEOUT_MS)) {
		return false;
	}
	raiseEvent(EyeAlignEvent::EYE_SIDE_POSITION_REACHED, static_cast<int>(state.eyeSide));
	changePhase(EyeAlignPhase::SWING_TO_TILT, PHASE_END_DWELL_MS);
	return true;
}

bool eye_align::EyeAligner::phaseSwingToTilt(void)
{
	auto& state = impl();
	// Step 3: the swing moves by position to the tilt of the selected eye.
	const int target = (state.eyeSide == EyeSide::OS) ? state.runConfig.swingTiltPos_OS : state.runConfig.swingTiltPos_OD;
	if (!movePhaseAxisTo(state.axisSwing, target, EyeAlignPhase::SWING_TO_TILT, SWING_TIMEOUT_MS)) {
		return false;
	}
	raiseEvent(EyeAlignEvent::SWING_AT_TILT, static_cast<int>(state.eyeSide));
	changePhase(EyeAlignPhase::SEARCHING, PHASE_END_DWELL_MS);
	// searchTimeoutMs runs from here across the search (4) and the approach (5a).
	state.searchDeadline = state.phaseStartTime + std::chrono::milliseconds(state.runConfig.searchTimeoutMs);
	raiseEvent(EyeAlignEvent::SEARCH_STARTED);
	return true;
}

bool eye_align::EyeAligner::phaseSearching(void)
{
	auto& state = impl();
	// Step 4: Z advances toward the eye by searchStepZ per frame (one
	// blocking position move each) while the side camera is watched. As soon as the
	// side camera locks on the pupil - on the way or during the wait at the limit -
	// the approach starts where Z stands. Without a lock Z steps to searchLimitPosZ,
	// stops there and waits SEARCH_LIMIT_WAIT_MS for one, then cancels
	// (SIDE_EYE_NOT_FOUND). searchTimeoutMs, counted from the search start and shared
	// with the approach, remains the safety net for a Z that never gets there (it fires
	// before alignTimeoutMs).
	if (state.sideTracker.isLocked()) {
		changePhase(EyeAlignPhase::APPROACHING);
		raiseEvent(EyeAlignEvent::APPROACH_STARTED);
		return true;
	}
	if (Clock::now() > state.searchDeadline) {
		raiseEvent(EyeAlignEvent::SIDE_EYE_NOT_FOUND, static_cast<int>(state.sideCameraId));
		requestRunEnd(EyeAlignPhase::CANCELED, EyeAlignResult::SIDE_EYE_NOT_FOUND);
		return false;
	}
	if (hasZReached(state.runConfig.searchLimitPosZ)) {
		// Z rests at the limit (no further step is issued) while the wait runs.
		if (hasLockGraceExpired(SEARCH_LIMIT_WAIT_MS)) {
			raiseEvent(EyeAlignEvent::SIDE_EYE_NOT_FOUND, static_cast<int>(state.sideCameraId));
			requestRunEnd(EyeAlignPhase::CANCELED, EyeAlignResult::SIDE_EYE_NOT_FOUND);
			return false;
		}
		return true;
	}
	// The last step lands on the search limit (the end switches are reported by
	// checkAxisLimits and only skip the step).
	const int toLimit = std::max(0, state.zForwardSign * (state.runConfig.searchLimitPosZ - state.axisZ.getPosition()));
	const int step = std::min(std::max(state.runConfig.searchStepZ, 1), toLimit);
	stepAxis(state.axisZ, clipZ(state.zForwardSign * step));
	return !state.endRequested;
}

bool eye_align::EyeAligner::phaseApproaching(void)
{
	auto& state = impl();
	// Step 5a: Z advances by approachStepZ per frame (one blocking position
	// move each) until the pupil has come in far enough for the Y centring: the
	// side-camera pupil inside the middle half of the image width (between 1/4 and 3/4,
	// APPROACH_SIDE_BAND_FRACTION) or the lower-camera pupil above 3/4 of the image
	// height (APPROACH_LOWER_LINE_FRACTION; it enters from the bottom). Z stands still
	// while the side camera has no lock (the lost-pupil rule stops and eventually
	// cancels) and once it has reached minSafePosZ. searchTimeoutMs, counted from the
	// search start (step 4) and shared with it, without the pupil coming in cancels
	// (SIDE_EYE_NOT_FOUND). X and Y do not move in this phase.
	if (isPupilInApproachRange()) {
		changePhase(EyeAlignPhase::CENTERING_Y);
		return true;
	}
	if (Clock::now() > state.searchDeadline) {
		raiseEvent(EyeAlignEvent::SIDE_EYE_NOT_FOUND, static_cast<int>(IrCameraId::CORNEA_LOWER));
		requestRunEnd(EyeAlignPhase::CANCELED, EyeAlignResult::SIDE_EYE_NOT_FOUND);
		return false;
	}
	if (!state.sideTracker.isLocked() || hasZReached(state.runConfig.minSafePosZ)) {
		return true;		// Z rests: no step is issued (moveBy() blocks, so nothing is in flight)
	}
	const int step = std::max(state.runConfig.approachStepZ, 1);
	stepAxis(state.axisZ, clipZ(state.zForwardSign * step));
	return !state.endRequested;
}


bool eye_align::EyeAligner::isPupilInApproachRange(void) const
{
	const auto& state = impl();
	// Step 5a end condition, evaluated on the latest detections only (hasFix).
	if (state.sideTracker.hasFix()) {
		const float bandPx = state.sideFrameWidth * APPROACH_SIDE_BAND_FRACTION;
		const float pupilX = state.sideTracker.centerX();
		if (pupilX > bandPx && pupilX < state.sideFrameWidth - bandPx) {
			return true;
		}
	}
	if (state.lowerTracker.hasFix()) {
		const float linePx = state.lowerFrameHeight * APPROACH_LOWER_LINE_FRACTION;
		if (state.lowerTracker.centerY() < linePx) {
			return true;
		}
	}
	return false;
}

bool eye_align::EyeAligner::phaseCenteringY(void)
{
	auto& state = impl();
	// Step 5b: Z rests where the approach left it while Y brings the side-camera pupil
	// onto its vertical target (targetY of the side camera, the image centre unless
	// configured). The side camera is not used for control after this phase.
	//   - no pupil fix: Y rests (the lost-pupil rule stops and eventually cancels)
	//   - within toleranceYPx: Y_CENTERED -> step 7 (Z centring; X follows from there)
	//   - Y at its end switch on the side the move would go: Y rests (END_OF_TOP /
	//     END_OF_BOTTOM come from checkAxisLimits; the run goes on)
	//   - pupil in the outer quarters of the image height (above 1/4 or below 3/4):
	//     one relative move of centerStepY toward the target per frame. That jump may
	//     exceed the tracker's maxJumpPx, so the side tracker is restarted to re-lock
	//     on the new position
	//   - pupil inside the middle half: capped per-frame steps of controlAxisY()
	if (!state.sideTracker.hasFix()) {
		return true;
	}
	if (std::fabs(state.errorY) <= state.runConfig.toleranceYPx) {
		raiseEvent(EyeAlignEvent::Y_CENTERED);
		state.eyeLostReported = false;		// the lost-pupil rule hands over to the lower camera
		changePhase(EyeAlignPhase::CENTERING_Z);
		return true;
	}
	const int offset = static_cast<int>(std::lround(state.errorY * state.runConfig.stepsPerPxY));
	if (!state.axisY.isValid() || state.held || movesIntoEnd(state.axisY, offset)) {
		return true;
	}
	const float farBandPx = state.sideFrameHeight * CENTERING_Y_FAR_BAND_FRACTION;
	const float pupilY = state.sideTracker.centerY();
	const bool pupilFar = pupilY < farBandPx || pupilY > state.sideFrameHeight - farBandPx;
	if (pupilFar) {
		const int farStep = (offset >= 0 ? +1 : -1) * std::max(state.runConfig.centerStepY, 1);
		stepAxis(state.axisY, farStep);
		state.sideTracker.reset();
		return !state.endRequested;
	}
	controlAxisY(true);
	return !state.endRequested;
}

bool eye_align::EyeAligner::phaseCenteringZ(void)
{
	auto& state = impl();
	// Step 7: Z brings the lower-camera pupil onto its vertical target; X and Y rest.
	//   - no pupil fix on the lower camera: Z rests (the lost-pupil rule stops and
	//     eventually cancels)
	//   - within toleranceZPx: Z_CENTERED; with X on target too -> FOCUSED (step 8),
	//     otherwise -> step 6 (X centring, which comes back here once X is on target)
	//   - Z at a limit on the side the move would go (minSafePosZ / readyPosZ through
	//     clipZ, or an end switch through stepAxis): Z rests; EYE_TOO_CLOSE / END_OF_*
	//     come from checkAxisLimits and the run goes on
	//   - pupil in the outer quarters of the image height (above 1/4 or below 3/4):
	//     one relative move of centerStepZ toward the target per frame, clipped at the
	//     Z bounds. That jump may exceed the tracker's maxJumpPx, so the lower tracker
	//     is restarted to re-lock on the new position
	//   - pupil inside the middle half: capped per-frame steps of controlAxisZ()
	if (!state.lowerTracker.hasFix()) {
		return true;
	}
	if (std::fabs(state.errorZ) <= state.runConfig.toleranceZPx) {
		raiseEvent(EyeAlignEvent::Z_CENTERED);
		if (std::fabs(state.errorX) <= state.runConfig.toleranceXPx) {
			raiseEvent(EyeAlignEvent::X_CENTERED);
			changePhase(EyeAlignPhase::FOCUSED);		// step 8
			state.focusReached = true;				// the alignment timeout no longer applies
			raiseEvent(EyeAlignEvent::FOCUS_ALIGNED);
		}
		else {
			changePhase(EyeAlignPhase::CENTERING_X);
		}
		return true;
	}
	const float farBandPx = state.lowerFrameHeight * CENTERING_Z_FAR_BAND_FRACTION;
	const float pupilY = state.lowerTracker.centerY();
	const bool pupilFar = pupilY < farBandPx || pupilY > state.lowerFrameHeight - farBandPx;
	if (pupilFar && !state.held) {
		const int direction = (state.errorZ * state.runConfig.stepsPerPxZ >= 0.0f) ? +1 : -1;
		const int farStep = clipZ(direction * std::max(state.runConfig.centerStepZ, 1));
		if (farStep != 0) {
			stepAxis(state.axisZ, farStep);
			state.lowerTracker.reset();
		}
		return !state.endRequested;
	}
	controlAxisZ(true, state.runConfig.toleranceZPx);		// CLIPPED or an end switch leave Z at rest
	return !state.endRequested;
}

bool eye_align::EyeAligner::phaseCenteringX(void)
{
	auto& state = impl();
	// Step 6: X brings the lower-camera pupil onto its horizontal target; Y and Z rest.
	//   - no pupil fix on the lower camera: X rests (the lost-pupil rule stops and
	//     eventually cancels)
	//   - within toleranceXPx: X_CENTERED -> step 7 (Z centring)
	//   - X at its end switch on the side the move would go: X rests (END_OF_LEFT /
	//     END_OF_RIGHT come from checkAxisLimits; the run goes on)
	//   - pupil in the outer quarters of the image width (left of 1/4 or right of 3/4):
	//     one relative move of centerStepX toward the target per frame. That jump may
	//     exceed the tracker's maxJumpPx, so the lower tracker is restarted to re-lock
	//     on the new position
	//   - pupil inside the middle half: capped per-frame steps of controlAxisX() by the
	//     lower-camera X error
	if (!state.lowerTracker.hasFix()) {
		return true;
	}
	if (std::fabs(state.errorX) <= state.runConfig.toleranceXPx) {
		raiseEvent(EyeAlignEvent::X_CENTERED);
		changePhase(EyeAlignPhase::CENTERING_Z);
		return true;
	}
	const int offset = static_cast<int>(std::lround(state.errorX * state.runConfig.stepsPerPxX));
	if (movesIntoEnd(state.axisX, offset)) {
		return true;
	}
	const float farBandPx = state.lowerFrameWidth * CENTERING_X_FAR_BAND_FRACTION;
	const float pupilX = state.lowerTracker.centerX();
	const bool pupilFar = pupilX < farBandPx || pupilX > state.lowerFrameWidth - farBandPx;
	if (pupilFar && state.axisX.isValid() && !state.held) {
		const int farStep = (offset >= 0 ? +1 : -1) * std::max(state.runConfig.centerStepX, 1);
		stepAxis(state.axisX, farStep);
		state.lowerTracker.reset();
		return !state.endRequested;
	}
	controlAxisX(true);		// no move while held or without a motor
	return !state.endRequested;
}

bool eye_align::EyeAligner::phaseFocused(void)
{
	auto& state = impl();
	// Step 8: no motion. The pupil leaving the X or Z tolerance means the eye moved off
	// the working position: report FOCUS_LOST and re-centre (back to step 7, then step
	// 6, which returns here once X is on target). The lock, not the per-frame fix, is the
	// condition: a single dropped frame keeps the last smoothed centre and must not
	// churn FOCUS_LOST / FOCUS_ALIGNED; a real loss is handled by the lost-pupil rule.
	if (state.lowerTracker.isLocked() && isLowerCentered()) {
		return true;
	}
	raiseEvent(EyeAlignEvent::FOCUS_LOST);
	changePhase(EyeAlignPhase::CENTERING_Y);
	return true;
}

bool eye_align::EyeAligner::phaseMonitoring(void)
{
	auto& state = impl();
	// MANUAL: no motion. The operator centres the head; report the focused state.
	if (state.phase == EyeAlignPhase::MONITORING) {
		if (isCentered()) {
			state.settleCount++;
			if (state.settleCount >= state.runConfig.settleFrames) {
				changePhase(EyeAlignPhase::FOCUSED);
				raiseEvent(EyeAlignEvent::FOCUS_ALIGNED);
			}
		}
		else {
			state.settleCount = 0;
		}
		return true;
	}

	// FOCUSED (manual): leave when the pupil drifts past the start deadband on any
	// axis. A lost pupil is handled by the lost-pupil rule.
	if (hasDriftedFromTarget()) {
		raiseEvent(EyeAlignEvent::FOCUS_LOST);
		changePhase(EyeAlignPhase::MONITORING);
	}
	return true;
}


// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////

void eye_align::EyeAligner::changePhase(EyeAlignPhase phase, int dwellMs)
{
	auto& state = impl();
	const auto previous = state.phase.load();
	const auto now = Clock::now();
	const auto dwell = std::chrono::milliseconds(std::max(dwellMs, 0));
	state.phase = phase;
	state.phaseDwellUntil = now + dwell;
	state.phaseStartTime = state.phaseDwellUntil;		// the timeout starts when the phase starts acting
	state.settleCount = 0;
	state.limitReachedAt = Clock::time_point();
	if (dwellMs > 0) {
		LogD() << "EyeAligner phase: " << phaseName(previous) << " -> " << phaseName(phase) << " after " << dwellMs << " ms dwell";
	}
	else {
		LogD() << "EyeAligner phase: " << phaseName(previous) << " -> " << phaseName(phase);
	}
}

void eye_align::EyeAligner::raiseEvent(EyeAlignEvent event, int arg)
{
	auto& state = impl();
	EventRecord record{};
	record.event = static_cast<int32_t>(event);
	record.mode = static_cast<int32_t>(state.mode.load());
	record.phase = static_cast<int32_t>(state.phase.load());
	record.arg = arg;
	record.posX = state.axisX.getPosition();
	record.posY = state.axisY.getPosition();
	record.posZ = state.axisZ.getPosition();
	record.posSwing = state.axisSwing.getPosition();

	{
		std::lock_guard<std::mutex> lock(state.eventMutex);
		state.eventQueue.push_back(record);
	}
	state.eventCondition.notify_one();
	LogD() << "EyeAligner event: " << eventName(event) << ", arg: " << arg;
}

void eye_align::EyeAligner::requestRunEnd(EyeAlignPhase terminal, EyeAlignResult reason)
{
	auto& state = impl();
	if (state.endRequested) {
		return;
	}
	state.endRequested = true;
	state.endPhase = terminal;
	state.endReason = reason;
}

void eye_align::EyeAligner::setupCorneaLeds(void)
{
	auto& state = impl();
	if (state.board == nullptr) {
		return;
	}
	// No intensity control here: the LEDs keep the intensity set at start-up (or by the
	// light control panel); a run only makes sure they are on.
	for (CorneaIrLed* led : { state.board->getCorneaIrLeftLed(), state.board->getCorneaIrRightLed() }) {
		if (led != nullptr && !led->turnLightOn()) {
			LogW() << "EyeAligner: failed to turn on " << led->getName();
		}
	}
	LogI() << "EyeAligner: both cornea IR LEDs on";
}

EyeAligner::AxisMove eye_align::EyeAligner::controlAxis(AxisController& axis, float error, bool hasFix, float tolerancePx, float stepsPerPx)
{
	auto& state = impl();
	if (!axis.isValid() || state.held || state.stopping || !hasFix) {
		return AxisMove::NONE;
	}
	if (std::fabs(error) <= tolerancePx) {
		return AxisMove::NONE;
	}
	// One blocking position move of error px * ratio steps: at least one step so a small
	// ratio cannot leave the axis stuck outside the tolerance, at most the steps that
	// shift the pupil by MOVE_JUMP_FRACTION * maxJumpPx so the tracker keeps its lock
	// across the move. A positive mapped error moves the position upward, the same
	// side as POSITIVE_STEP_DIRECTION.
	const float maxOffset = std::max(1.0f, state.runConfig.maxJumpPx * MOVE_JUMP_FRACTION * std::fabs(stepsPerPx));
	const float wanted = std::clamp(error * stepsPerPx, -maxOffset, maxOffset);
	int offset = static_cast<int>(std::lround(wanted));
	if (offset == 0) {
		offset = (wanted >= 0.0f) ? +1 : -1;
	}
	if (&axis == &state.axisZ) {
		offset = clipZ(offset);		// Z stays between readyPosZ and minSafePosZ
		if (offset == 0) {
			return AxisMove::CLIPPED;
		}
	}
	stepAxis(axis, offset);
	return AxisMove::MOVED;
}

void eye_align::EyeAligner::stepAxis(AxisController& axis, int offset)
{
	auto& state = impl();
	if (offset == 0 || state.stopping || state.held) {
		return;
	}
	// A move into an active end switch is skipped: the limit was (or will be)
	// reported by checkAxisLimits, and a rejected command would otherwise end the
	// run through checkAxisFaults.
	if (movesIntoEnd(axis, offset)) {
		return;
	}
	state.movingAxis = &axis;
	axis.moveBy(offset);
	state.movingAxis = nullptr;
	return;
}

bool eye_align::EyeAligner::movesIntoEnd(const AxisController& axis, int offset)
{
	if (offset == 0 || !axis.isValid()) {
		return false;
	}
	const int direction = (offset > 0) ? POSITIVE_STEP_DIRECTION : (1 - POSITIVE_STEP_DIRECTION);
	return (direction == 0) ? axis.isAtMinimumEnd() : axis.isAtMaximumEnd();
}

EyeAligner::AxisMove eye_align::EyeAligner::controlAxisY(bool sideHasFix)
{
	auto& state = impl();
	return controlAxis(state.axisY, state.errorY, sideHasFix, state.runConfig.toleranceYPx, state.runConfig.stepsPerPxY);
}

EyeAligner::AxisMove eye_align::EyeAligner::controlAxisX(bool lowerHasFix)
{
	auto& state = impl();
	return controlAxis(state.axisX, state.errorX, lowerHasFix, state.runConfig.toleranceXPx, state.runConfig.stepsPerPxX);
}

EyeAligner::AxisMove eye_align::EyeAligner::controlAxisZ(bool lowerHasFix, float tolerancePx)
{
	auto& state = impl();
	// controlAxis bounds the Z offset (clipZ); checkAxisLimits reports minSafePosZ.
	return controlAxis(state.axisZ, state.errorZ, lowerHasFix, tolerancePx, state.runConfig.stepsPerPxZ);
}

void eye_align::EyeAligner::stopAllAxes(void)
{
	auto& state = impl();
	for (AxisController* axis : { &state.axisX, &state.axisY, &state.axisZ, &state.axisSwing }) {
		if (axis->isValid()) {
			axis->stop();
		}
	}
}

bool eye_align::EyeAligner::hasZPassed(int limitPos) const
{
	const auto& state = impl();
	return state.zForwardSign * (state.axisZ.getPosition() - limitPos) >= 0;
}

bool eye_align::EyeAligner::hasZReached(int limitPos) const
{
	const auto& state = impl();
	// A blocking move may rest a few steps short of its target (the motor's arrival
	// band), so "at the limit" is the position band, not the exact position.
	return state.zForwardSign * (limitPos - state.axisZ.getPosition()) <= state.runConfig.posToleranceSteps;
}

bool eye_align::EyeAligner::hasLockGraceExpired(int waitMs)
{
	auto& state = impl();
	if (state.limitReachedAt == Clock::time_point()) {
		state.limitReachedAt = Clock::now();		// first frame at the limit: start waiting
		return false;
	}
	return elapsedMilliseconds(state.limitReachedAt) >= waitMs;
}

bool eye_align::EyeAligner::isLowerCentered(void) const
{
	const auto& state = impl();
	return state.lowerTracker.hasFix()
		&& std::fabs(state.errorX) <= state.runConfig.toleranceXPx
		&& std::fabs(state.errorZ) <= state.runConfig.toleranceZPx;
}

bool eye_align::EyeAligner::isCentered(void) const
{
	const auto& state = impl();
	// MANUAL: centred on both cameras.
	return isLowerCentered()
		&& state.sideTracker.hasFix()
		&& std::fabs(state.errorY) <= state.runConfig.toleranceYPx;
}

bool eye_align::EyeAligner::hasDriftedFromTarget(void) const
{
	const auto& state = impl();
	const float deadband = state.runConfig.startDeadbandPx;
	const bool driftX = state.lowerTracker.hasFix() && std::fabs(state.errorX) > deadband;
	const bool driftY = state.sideTracker.hasFix() && std::fabs(state.errorY) > deadband;
	const bool driftZ = state.lowerTracker.hasFix() && std::fabs(state.errorZ) > deadband;
	return driftX || driftY || driftZ;
}

void eye_align::EyeAligner::updateStatusSnapshot(void)
{
	auto& state = impl();
	EyeAlignStatus status;
	status.mode = static_cast<int32_t>(state.mode.load());
	status.phase = static_cast<int32_t>(state.phase.load());
	status.result = static_cast<int32_t>(state.result.load());
	status.eyeSide = static_cast<int32_t>(state.eyeSide);
	status.sideCamId = static_cast<int32_t>(state.sideCameraId);
	status.posX = state.axisX.getPosition();
	status.posY = state.axisY.getPosition();
	status.posZ = state.axisZ.getPosition();
	status.posSwing = state.axisSwing.getPosition();
	status.side = state.sideTracker.lastResult();
	status.lower = state.lowerTracker.lastResult();
	status.errX = state.errorX;
	status.errY = state.errorY;
	status.errZ = state.errorZ;
	IrCameraId watchedCameraId = state.sideCameraId;
	const PupilTracker* watched = watchedTracker(watchedCameraId);
	status.lostFrames = watched ? watched->lostFrames() : 0;
	status.elapsedMs = elapsedMilliseconds(state.runStartTime);

	std::lock_guard<std::mutex> lock(state.statusMutex);
	state.status = status;
}

EyeAligner::EyeAlignerImpl& eye_align::EyeAligner::impl(void) const
{
	return *d_ptr;
}
