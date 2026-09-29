#include "pch.h"
#include "EyeAligner.h"
#include "PupilDetector.h"
#include "PupilTracker.h"
#include "CorneaFrameStore.h"
#include "AxisController.h"

#include "MainBoard.h"
#include "StepMotor.h"
#include "CorneaCamera.h"


using namespace eye_align;


std::mutex EyeAligner::singleMutex_;


namespace
{
	using Clock = chrono::steady_clock;

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

	int elapsedMs(Clock::time_point since)
	{
		return static_cast<int>(chrono::duration_cast<chrono::milliseconds>(Clock::now() - since).count());
	}

	IrCameraId sideCameraOf(EyeSide side)
	{
		// OD (right eye) is watched by the LEFT camera, OS by the RIGHT camera.
		return (side == EyeSide::OS) ? IrCameraId::CORNEA_RIGHT : IrCameraId::CORNEA_LEFT;
	}

	const char* phaseName(EyeAlignPhase phase)
	{
		switch (phase) {
		case EyeAlignPhase::IDLE: return "IDLE";
		case EyeAlignPhase::TO_READY_Z: return "TO_READY_Z";
		case EyeAlignPhase::TO_EYE_X: return "TO_EYE_X";
		case EyeAlignPhase::SWING_TO_TILT: return "SWING_TO_TILT";
		case EyeAlignPhase::SEARCHING: return "SEARCHING";
		case EyeAlignPhase::CENTERING_XY: return "CENTERING_XY";
		case EyeAlignPhase::APPROACHING: return "APPROACHING";
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
		case EyeAlignEvent::SINGLE_ALIGN_STARTED: return "SINGLE_ALIGN_STARTED";
		case EyeAlignEvent::MANUAL_ALIGN_STARTED: return "MANUAL_ALIGN_STARTED";
		case EyeAlignEvent::ALIGN_CANCELLED: return "ALIGN_CANCELLED";
		case EyeAlignEvent::ALIGN_FAILED: return "ALIGN_FAILED";
		case EyeAlignEvent::READY_POSITION_REACHED: return "READY_POSITION_REACHED";
		case EyeAlignEvent::EYE_SIDE_POSITION_REACHED: return "EYE_SIDE_POSITION_REACHED";
		case EyeAlignEvent::SWING_AT_TILT: return "SWING_AT_TILT";
		case EyeAlignEvent::SEARCH_STARTED: return "SEARCH_STARTED";
		case EyeAlignEvent::EYE_FOUND: return "EYE_FOUND";
		case EyeAlignEvent::EYE_NOT_FOUND: return "EYE_NOT_FOUND";
		case EyeAlignEvent::XY_CENTERED: return "XY_CENTERED";
		case EyeAlignEvent::APPROACH_STARTED: return "APPROACH_STARTED";
		case EyeAlignEvent::LOWER_EYE_FOUND: return "LOWER_EYE_FOUND";
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
		}
		return "?";
	}

	EyeAlignEvent endOfEvent(MotorType type, int dir)
	{
		switch (type) {
		case MotorType::STAGE_X: return (dir == X_DIR_LEFT) ? EyeAlignEvent::END_OF_LEFT : EyeAlignEvent::END_OF_RIGHT;
		case MotorType::STAGE_Y: return (dir == Y_DIR_DOWN) ? EyeAlignEvent::END_OF_BOTTOM : EyeAlignEvent::END_OF_TOP;
		case MotorType::STAGE_Z: return (dir == Z_DIR_FORWARD) ? EyeAlignEvent::END_OF_FORWARD : EyeAlignEvent::END_OF_BACKWARD;
		case MotorType::SWING: return (dir == SWING_DIR_LEFT) ? EyeAlignEvent::END_OF_SWING_LEFT : EyeAlignEvent::END_OF_SWING_RIGHT;
		default: return EyeAlignEvent::NONE;
		}
	}

	int cameraIndex(IrCameraId camId)
	{
		const int index = static_cast<int>(camId);
		return (index >= 0 && index < NUM_IR_CAMERAS) ? index : -1;
	}
}


struct EyeAligner::EyeAlignerImpl
{
	// board
	MainBoard* board = nullptr;
	CorneaCamera* camera = nullptr;
	bool initialized = false;

	// configuration
	mutable mutex cfgMutex;
	EyeAlignConfigParam cfg;		// published by setConfig()
	EyeAlignConfigParam run;		// snapshot used by the running worker

	// run control
	mutex startMutex;
	atomic<bool> running{ false };
	atomic<bool> stopping{ false };
	atomic<bool> holdRequested{ false };
	atomic<bool> held{ false };
	atomic<EyeAlignMode> mode{ EyeAlignMode::MANUAL };
	atomic<EyeAlignPhase> phase{ EyeAlignPhase::IDLE };
	atomic<EyeAlignResult> result{ EyeAlignResult::NONE };
	EyeSide eyeSide = EyeSide::OD;
	IrCameraId sideCam = IrCameraId::CORNEA_LEFT;
	bool manual = false;

	thread threadWork;
	thread threadEvent;

	// event queue (worker -> dispatcher)
	mutex evMutex;
	condition_variable evCondVar;
	deque<EventRecord> evQueue;
	bool evStop = false;

	// frames
	CorneaFrameStore frames;
	vector<uint8_t> sideFrame;
	vector<uint8_t> lowerFrame;
	int sideW = DEFAULT_FRAME_WIDTH, sideH = DEFAULT_FRAME_HEIGHT;
	int lowerW = DEFAULT_FRAME_WIDTH, lowerH = DEFAULT_FRAME_HEIGHT;
	uint64_t sideSeq = 0, lowerSeq = 0;

	// detection
	PupilDetector detSide;
	PupilDetector detLower;
	PupilTracker trkSide;
	PupilTracker trkLower;

	mutable mutex resultMutex;
	array<PupilDetectResult, NUM_IR_CAMERAS> latest{};
	array<Clock::time_point, NUM_IR_CAMERAS> latestTime{};

	mutable mutex overrideMutex;
	array<optional<PupilDetectResult>, NUM_IR_CAMERAS> overrides{};

	// motion: the only four motors this module may touch
	AxisController axX;
	AxisController axY;
	AxisController axZ;
	AxisController axSwing;

	// run state (worker thread only)
	Clock::time_point runStart;
	Clock::time_point phaseStart;
	int loopCount = 0;
	int zForwardSign = +1;			// sign of (searchLimitPosZ - readyPosZ)
	bool eyeFound = false;
	bool lowerFound = false;
	bool eyeLostReported = false;
	int settleCount = 0;
	EyeAlignPhase resumePhase = EyeAlignPhase::CENTERING_Z;
	bool endRequested = false;
	EyeAlignPhase endPhase = EyeAlignPhase::CANCELED;
	EyeAlignResult endReason = EyeAlignResult::USER_CANCEL;
	float errX = 0.0f, errY = 0.0f, errZ = 0.0f;

	// status snapshot for fetchStatus()
	mutable mutex statusMutex;
	EyeAlignStatus status{};
};


EyeAligner::EyeAligner() :
	d_ptr(make_unique<EyeAlignerImpl>())
{}


EyeAligner::~EyeAligner()
{
	try {
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

	auto& d = impl();
	d.board = board;
	d.camera = board->getCorneaCamera();
	d.axX = AxisController(board->getXstageMotor(), MotorType::STAGE_X, "StageX");
	d.axY = AxisController(board->getYstageMotor(), MotorType::STAGE_Y, "StageY");
	d.axZ = AxisController(board->getZstageMotor(), MotorType::STAGE_Z, "StageZ");
	d.axSwing = AxisController(board->getSwingMotor(), MotorType::SWING, "Swing");
	d.initialized = true;

	LogI() << "EyeAligner initialized, camera: " << (d.camera ? "ok" : "none")
		<< ", motors X/Y/Z/Swing: " << d.axX.isValid() << d.axY.isValid() << d.axZ.isValid() << d.axSwing.isValid();
	return true;
}

bool eye_align::EyeAligner::isInitialized(void) const
{
	return impl().initialized;
}

void eye_align::EyeAligner::releaseEyeAligner(void)
{
	cancel(true);
	auto& d = impl();
	d.frames.setAccepting(false);
	d.board = nullptr;
	d.camera = nullptr;
	d.initialized = false;
}

void eye_align::EyeAligner::setConfig(const EyeAlignConfigParam& config)
{
	lock_guard<mutex> lock(impl().cfgMutex);
	impl().cfg = config;
}

EyeAlignConfigParam eye_align::EyeAligner::getConfig(void) const
{
	lock_guard<mutex> lock(impl().cfgMutex);
	return impl().cfg;
}


// Run control
////////////////////////////////////////////////////////////////////////////////////////////////////

EyeAlignResult eye_align::EyeAligner::startAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback)
{
	auto& d = impl();
	lock_guard<mutex> startLock(d.startMutex);

	if (!d.initialized) {
		return EyeAlignResult::NOT_INITIALIZED;
	}
	if (d.running) {
		return EyeAlignResult::BUSY;
	}
	// The previous run may have ended on its own; its threads must be joined before
	// the thread objects are reused.
	joinThreads();

	if (d.camera == nullptr || !d.camera->isPlaying()) {
		return EyeAlignResult::PREVIEW_NOT_RUNNING;
	}

	d.manual = (mode == EyeAlignMode::MANUAL);
	if (!d.manual) {
		if (!d.axX.isReady() || !d.axY.isReady() || !d.axZ.isReady() || !d.axSwing.isReady()) {
			return EyeAlignResult::MOTOR_NOT_READY;
		}
	}

	{
		lock_guard<mutex> lock(d.cfgMutex);
		d.run = d.cfg;
	}

	if (mode == EyeAlignMode::FULL) {
		eyeSide = EyeSide::OD;			// FULL always starts with the right eye
	}
	else if (eyeSide != EyeSide::OS) {
		eyeSide = EyeSide::OD;
	}
	d.eyeSide = eyeSide;
	d.sideCam = sideCameraOf(eyeSide);
	d.mode = mode;

	if (auto* registry = CallbackRegistry::getInstance(); registry) {
		registry->setEyeAlignEventOccurred(callback);
	}

	// detection
	d.detSide.setCameraId(d.sideCam);
	d.detSide.setParam(eyeSide == EyeSide::OS ? d.run.detRight : d.run.detLeft);
	d.detLower.setCameraId(IrCameraId::CORNEA_LOWER);
	d.detLower.setParam(d.run.detLower);

	PupilTrackerConfig trackerConfig;
	trackerConfig.detectConsecutive = d.run.detectConsecutive;
	trackerConfig.unlockAfterMisses = d.run.lostFramesToStop;
	trackerConfig.maxJumpPx = d.run.maxJumpPx;
	trackerConfig.smoothingAlpha = d.run.smoothingAlpha;
	d.trkSide.configure(trackerConfig);
	d.trkLower.configure(trackerConfig);
	d.trkSide.reset();
	d.trkLower.reset();
	{
		lock_guard<mutex> lock(d.resultMutex);
		d.latest.fill(PupilDetectResult());
		d.latestTime.fill(Clock::time_point());
	}

	// frames
	d.frames.clear();
	d.sideW = d.lowerW = DEFAULT_FRAME_WIDTH;
	d.sideH = d.lowerH = DEFAULT_FRAME_HEIGHT;
	d.sideSeq = d.lowerSeq = 0;

	// run state
	d.loopCount = 0;
	d.zForwardSign = (d.run.searchLimitPosZ >= d.run.readyPosZ) ? +1 : -1;
	d.eyeFound = false;
	d.lowerFound = false;
	d.eyeLostReported = false;
	d.settleCount = 0;
	d.resumePhase = EyeAlignPhase::CENTERING_Z;
	d.endRequested = false;
	d.endPhase = EyeAlignPhase::CANCELED;
	d.endReason = EyeAlignResult::USER_CANCEL;
	d.errX = d.errY = d.errZ = 0.0f;
	d.result = EyeAlignResult::NONE;
	d.phase = d.manual ? EyeAlignPhase::MONITORING : EyeAlignPhase::TO_READY_Z;

	{
		lock_guard<mutex> lock(d.evMutex);
		d.evQueue.clear();
		d.evStop = false;
	}
	d.stopping = false;
	d.holdRequested = false;
	d.held = false;
	d.runStart = d.phaseStart = Clock::now();
	d.frames.setAccepting(true);
	d.running = true;

	d.threadEvent = thread(&EyeAligner::threadEventFunction, this);
	d.threadWork = thread(&EyeAligner::threadWorkFunction, this);

	LogI() << "EyeAligner started, mode: " << static_cast<int>(mode) << ", eye side: " << static_cast<int>(eyeSide);
	return EyeAlignResult::NONE;
}

void eye_align::EyeAligner::cancel(bool wait)
{
	auto& d = impl();
	if (d.running) {
		d.stopping = true;
		d.frames.wake();
	}
	if (wait) {
		joinThreads();
	}
}

void eye_align::EyeAligner::joinThreads(void)
{
	auto& d = impl();
	// Never join a thread from itself (cancel() called from within the event callback).
	if (d.threadWork.joinable() && d.threadWork.get_id() != this_thread::get_id()) {
		d.threadWork.join();
	}
	if (d.threadEvent.joinable() && d.threadEvent.get_id() != this_thread::get_id()) {
		d.threadEvent.join();
	}
}

bool eye_align::EyeAligner::hold(void)
{
	auto& d = impl();
	if (!d.running || d.manual || d.stopping) {
		return false;
	}
	switch (d.phase.load()) {
	case EyeAlignPhase::CENTERING_XY:
	case EyeAlignPhase::APPROACHING:
	case EyeAlignPhase::CENTERING_Z:
	case EyeAlignPhase::FOCUSED:
	case EyeAlignPhase::HELD:
		break;
	default:
		return false;
	}
	d.holdRequested = true;
	d.frames.wake();
	return true;
}

bool eye_align::EyeAligner::resume(void)
{
	auto& d = impl();
	if (!d.running || !d.holdRequested) {
		return false;
	}
	d.holdRequested = false;
	d.frames.wake();
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
	lock_guard<mutex> lock(impl().statusMutex);
	*status = impl().status;
	return true;
}


// Pupil results
////////////////////////////////////////////////////////////////////////////////////////////////////

bool eye_align::EyeAligner::isPupilDetected(IrCameraId camId) const
{
	PupilDetectResult result;
	return getPupilResult(camId, &result) && result.found != 0;
}

bool eye_align::EyeAligner::getPupilResult(IrCameraId camId, PupilDetectResult* result) const
{
	const int index = cameraIndex(camId);
	if (index < 0 || result == nullptr) {
		return false;
	}
	lock_guard<mutex> lock(impl().resultMutex);
	const auto& stamp = impl().latestTime[index];
	if (stamp == Clock::time_point() || elapsedMs(stamp) > PUPIL_RESULT_STALE_MS) {
		return false;
	}
	*result = impl().latest[index];
	return true;
}

PupilDetectResult eye_align::EyeAligner::detectPupilInImage(const std::uint8_t* gray, int width, int height, IrCameraId camId) const
{
	PupilDetectParam param;
	{
		lock_guard<mutex> lock(impl().cfgMutex);
		switch (camId) {
		case IrCameraId::CORNEA_LEFT: param = impl().cfg.detLeft; break;
		case IrCameraId::CORNEA_RIGHT: param = impl().cfg.detRight; break;
		default: param = impl().cfg.detLower; break;
		}
	}
	PupilDetector detector(camId, param);
	return detector.detect(gray, width, height, 0, 0);
}

void eye_align::EyeAligner::setPupilOverride(IrCameraId camId, const PupilDetectResult* result)
{
	const int index = cameraIndex(camId);
	if (index < 0) {
		return;
	}
	lock_guard<mutex> lock(impl().overrideMutex);
	if (result) {
		PupilDetectResult copy = *result;
		copy.camId = static_cast<int32_t>(camId);
		impl().overrides[index] = copy;
	}
	else {
		impl().overrides[index].reset();
	}
}

void eye_align::EyeAligner::clearPupilOverrides(void)
{
	lock_guard<mutex> lock(impl().overrideMutex);
	for (auto& item : impl().overrides) {
		item.reset();
	}
}

void eye_align::EyeAligner::onCorneaFrame(IrCameraId camId, const std::uint8_t* bits, int width, int height)
{
	// CorneaFrameStore::push() returns immediately while nothing is running.
	impl().frames.push(camId, bits, width, height);
}


// Threads
////////////////////////////////////////////////////////////////////////////////////////////////////

void eye_align::EyeAligner::threadWorkFunction(void)
{
	auto& d = impl();
	try {
		switch (d.mode.load()) {
		case EyeAlignMode::FULL: raiseEvent(EyeAlignEvent::FULL_ALIGN_STARTED, static_cast<int>(d.eyeSide)); break;
		case EyeAlignMode::SINGLE: raiseEvent(EyeAlignEvent::SINGLE_ALIGN_STARTED, static_cast<int>(d.eyeSide)); break;
		default: raiseEvent(EyeAlignEvent::MANUAL_ALIGN_STARTED, static_cast<int>(d.eyeSide)); break;
		}

		if (!d.manual) {
			// Fresh positions before the first jogToward() decision.
			d.axX.poll();
			d.axY.poll();
			d.axZ.poll();
			d.axSwing.poll();
		}

		while (!d.stopping) {
			d.frames.waitForNewFrame(d.stopping, LOOP_MAX_WAIT_MS);
			if (d.stopping) {
				break;
			}
			if (!loopOnce()) {
				break;
			}
		}
	}
	catch (...) {
		LogE() << "EyeAligner: unhandled exception in the control loop";
		if (!d.endRequested) {
			requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::INTERNAL_ERROR);
		}
	}
	finishRun();
}

void eye_align::EyeAligner::threadEventFunction(void)
{
	auto& d = impl();
	for (;;) {
		EventRecord record{};
		{
			unique_lock<mutex> lock(d.evMutex);
			d.evCondVar.wait(lock, [&] { return !d.evQueue.empty() || d.evStop; });
			if (d.evQueue.empty()) {
				break;		// evStop and nothing left to deliver
			}
			record = d.evQueue.front();
			d.evQueue.pop_front();
		}
		if (auto* registry = CallbackRegistry::getInstance(); registry) {
			registry->runEyeAlignEventOccurred(record.event, record.mode, record.phase, record.arg,
				record.posX, record.posY, record.posZ, record.posSwing);
		}
	}
}

void eye_align::EyeAligner::finishRun(void)
{
	auto& d = impl();
	try {
		stopAllAxes();
		d.frames.setAccepting(false);

		if (!d.endRequested) {
			// Left the loop because cancel() set the stop flag.
			d.endPhase = EyeAlignPhase::CANCELED;
			d.endReason = EyeAlignResult::USER_CANCEL;
		}
		d.result = d.endReason;

		// A user cancel returns promptly; the other endings retreat Z to the ready
		// position so the head is never left close to the eye.
		if (!d.manual && d.run.backOffOnEnd != 0 && d.endReason != EyeAlignResult::USER_CANCEL) {
			backOffToReady();
		}
		stopAllAxes();
	}
	catch (...) {
		LogE() << "EyeAligner: exception while finishing the run";
	}

	d.held = false;
	d.holdRequested = false;
	d.phase = d.endPhase;
	updateStatusSnapshot();

	const auto terminal = (d.endPhase == EyeAlignPhase::CANCELED) ? EyeAlignEvent::ALIGN_CANCELLED : EyeAlignEvent::ALIGN_FAILED;
	raiseEvent(terminal, static_cast<int>(d.endReason));
	LogI() << "EyeAligner finished, phase: " << phaseName(d.endPhase) << ", reason: " << static_cast<int>(d.endReason);

	d.running = false;
	{
		lock_guard<mutex> lock(d.evMutex);
		d.evStop = true;
	}
	d.evCondVar.notify_all();
}

void eye_align::EyeAligner::backOffToReady(void)
{
	auto& d = impl();
	if (!d.axZ.isValid()) {
		return;
	}
	const auto start = Clock::now();
	while (!d.stopping && elapsedMs(start) < d.run.readyTimeoutMs) {
		d.axZ.poll();
		if (d.axZ.isAtLimitInDirection() || d.axZ.hasCommandFailed()) {
			break;
		}
		if (d.axZ.jogToward(d.run.readyPosZ, d.run.posToleranceSteps)) {
			break;
		}
		this_thread::sleep_for(chrono::milliseconds(50));
	}
	d.axZ.stop();
}


// Control loop
////////////////////////////////////////////////////////////////////////////////////////////////////

bool eye_align::EyeAligner::loopOnce(void)
{
	auto& d = impl();
	d.loopCount++;

	updateDetections();

	if (!d.manual) {
		pollAxes();
		if (!checkAxisFaults()) {
			return false;
		}
	}
	if (!applyHoldRequest()) {
		return false;
	}
	if (!applyLostPupilRule()) {
		return false;
	}
	if (!processPhase()) {
		return false;
	}

	updateStatusSnapshot();
	return !d.endRequested;
}

void eye_align::EyeAligner::updateDetections(void)
{
	auto& d = impl();

	auto detectFrame = [&](IrCameraId camId, const PupilDetector& detector, const vector<uint8_t>& frame, int width, int height, uint64_t seq) {
		PupilDetectResult result;
		bool overridden = false;
		{
			lock_guard<mutex> lock(d.overrideMutex);
			const int index = cameraIndex(camId);
			if (index >= 0 && d.overrides[index].has_value()) {
				result = *d.overrides[index];
				result.frameSeq = static_cast<int32_t>(seq);
				overridden = true;
			}
		}
		if (!overridden) {
			result = detector.detect(frame.data(), width, height, 0, static_cast<int>(seq));
		}
		const int index = cameraIndex(camId);
		if (index >= 0) {
			lock_guard<mutex> lock(d.resultMutex);
			d.latest[index] = result;
			d.latestTime[index] = Clock::now();
		}
		return result;
	};

	if (d.frames.takeLatest(d.sideCam, d.sideFrame, d.sideW, d.sideH, d.sideSeq)) {
		const auto result = detectFrame(d.sideCam, d.detSide, d.sideFrame, d.sideW, d.sideH, d.sideSeq);
		d.trkSide.update(result);
		if (d.trkSide.justLocked()) {
			d.eyeFound = true;
			raiseEvent(EyeAlignEvent::EYE_FOUND, static_cast<int>(d.sideCam));
		}
	}
	if (d.frames.takeLatest(IrCameraId::CORNEA_LOWER, d.lowerFrame, d.lowerW, d.lowerH, d.lowerSeq)) {
		const auto result = detectFrame(IrCameraId::CORNEA_LOWER, d.detLower, d.lowerFrame, d.lowerW, d.lowerH, d.lowerSeq);
		d.trkLower.update(result);
		if (d.trkLower.justLocked()) {
			d.lowerFound = true;
			raiseEvent(EyeAlignEvent::LOWER_EYE_FOUND, static_cast<int>(IrCameraId::CORNEA_LOWER));
		}
	}

	// Signed pixel errors after the per-camera sign mapping. A positive mapped error
	// jogs the axis in POSITIVE_STEP_DIR; the config signs absorb the mirror modes.
	const auto& ps = d.detSide.getParam();
	const auto& pl = d.detLower.getParam();
	d.errX = (d.trkSide.cx() - d.detSide.getTargetX(d.sideW)) * static_cast<float>(ps.signH);
	d.errY = (d.trkSide.cy() - d.detSide.getTargetY(d.sideH)) * static_cast<float>(ps.signV);
	d.errZ = (d.trkLower.cy() - d.detLower.getTargetY(d.lowerH)) * static_cast<float>(pl.signV);
}

void eye_align::EyeAligner::pollAxes(void)
{
	auto& d = impl();
	const bool idleTurn = (d.loopCount % IDLE_AXIS_POLL_PERIOD) == 0;
	for (AxisController* axis : { &d.axX, &d.axY, &d.axZ, &d.axSwing }) {
		if (axis->isValid() && (axis->isJogging() || idleTurn)) {
			axis->poll();
		}
	}
}

bool eye_align::EyeAligner::checkAxisFaults(void)
{
	auto& d = impl();
	for (AxisController* axis : { &d.axX, &d.axY, &d.axZ, &d.axSwing }) {
		if (!axis->isValid()) {
			continue;
		}
		if (axis->hasCommandFailed()) {
			stopAllAxes();
			raiseEvent(EyeAlignEvent::MOTOR_MOVE_FAILED, static_cast<int>(axis->getType()));
			requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::MOTOR_MOVE_FAILED);
			return false;
		}
		if (axis->isAtLimitInDirection()) {
			const int dir = axis->getDirection();
			axis->stop();
			stopAllAxes();
			raiseEvent(endOfEvent(axis->getType(), dir), static_cast<int>(axis->getType()));
			requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::LIMIT_REACHED);
			return false;
		}
	}
	return true;
}

bool eye_align::EyeAligner::applyHoldRequest(void)
{
	auto& d = impl();
	if (d.manual) {
		return true;
	}
	if (d.holdRequested && !d.held) {
		stopAllAxes();
		const auto current = d.phase.load();
		d.resumePhase = (current == EyeAlignPhase::CENTERING_XY || current == EyeAlignPhase::APPROACHING)
			? current : EyeAlignPhase::CENTERING_Z;
		d.held = true;
		changePhase(EyeAlignPhase::HELD);
		raiseEvent(EyeAlignEvent::ALIGN_HELD);
	}
	else if (!d.holdRequested && d.held) {
		d.held = false;
		changePhase(d.resumePhase);
		raiseEvent(EyeAlignEvent::ALIGN_RESUMED);
	}
	return true;
}

bool eye_align::EyeAligner::applyLostPupilRule(void)
{
	auto& d = impl();
	if (!d.eyeFound) {
		return true;		// the rule starts once the eye has been found
	}

	const int lostSide = d.trkSide.lostFrames();
	const int lostLower = d.lowerFound ? d.trkLower.lostFrames() : 0;
	const int lost = std::max(lostSide, lostLower);
	const IrCameraId lostCam = (lostSide >= lostLower) ? d.sideCam : IrCameraId::CORNEA_LOWER;

	if (lost >= d.run.lostFramesToStop) {
		if (!d.eyeLostReported) {
			d.eyeLostReported = true;
			if (!d.manual) {
				stopAllAxes();
			}
			raiseEvent(EyeAlignEvent::EYE_LOST, static_cast<int>(lostCam));
			if (d.phase == EyeAlignPhase::FOCUSED) {
				raiseEvent(EyeAlignEvent::FOCUS_LOST);
				changePhase(d.manual ? EyeAlignPhase::MONITORING : EyeAlignPhase::CENTERING_Z);
			}
		}
		if (!d.manual && lost >= d.run.lostFramesToCancel) {
			requestEnd(EyeAlignPhase::CANCELED, EyeAlignResult::PUPIL_LOST);
			return false;
		}
	}
	else if (d.eyeLostReported && lost == 0) {
		d.eyeLostReported = false;
	}
	return true;
}

bool eye_align::EyeAligner::processPhase(void)
{
	auto& d = impl();
	switch (d.phase.load()) {
	case EyeAlignPhase::TO_READY_Z: return phaseToReadyZ();
	case EyeAlignPhase::TO_EYE_X: return phaseToEyeX();
	case EyeAlignPhase::SWING_TO_TILT: return phaseSwingToTilt();
	case EyeAlignPhase::SEARCHING: return phaseSearching();
	case EyeAlignPhase::CENTERING_XY: return phaseCenteringXY();
	case EyeAlignPhase::APPROACHING: return phaseApproaching();
	case EyeAlignPhase::CENTERING_Z: return phaseCenteringZ();
	case EyeAlignPhase::FOCUSED: return d.manual ? phaseMonitoring() : phaseFocused();
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

bool eye_align::EyeAligner::phaseToReadyZ(void)
{
	auto& d = impl();
	if (d.axZ.jogToward(d.run.readyPosZ, d.run.posToleranceSteps)) {
		raiseEvent(EyeAlignEvent::READY_POSITION_REACHED, static_cast<int>(d.eyeSide));
		changePhase(EyeAlignPhase::TO_EYE_X);
		return true;
	}
	if (elapsedMs(d.phaseStart) > d.run.readyTimeoutMs) {
		d.axZ.stop();
		raiseEvent(EyeAlignEvent::PHASE_TIMEOUT, static_cast<int>(EyeAlignPhase::TO_READY_Z));
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::TIMEOUT);
		return false;
	}
	return true;
}

bool eye_align::EyeAligner::phaseToEyeX(void)
{
	auto& d = impl();
	const int target = (d.eyeSide == EyeSide::OS) ? d.run.eyePosX_OS : d.run.eyePosX_OD;
	if (d.axX.jogToward(target, d.run.posToleranceSteps)) {
		raiseEvent(EyeAlignEvent::EYE_SIDE_POSITION_REACHED, static_cast<int>(d.eyeSide));
		changePhase(EyeAlignPhase::SWING_TO_TILT);
		return true;
	}
	if (elapsedMs(d.phaseStart) > d.run.eyeXTimeoutMs) {
		d.axX.stop();
		raiseEvent(EyeAlignEvent::PHASE_TIMEOUT, static_cast<int>(EyeAlignPhase::TO_EYE_X));
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::TIMEOUT);
		return false;
	}
	return true;
}

bool eye_align::EyeAligner::phaseSwingToTilt(void)
{
	auto& d = impl();
	const int target = (d.eyeSide == EyeSide::OS) ? d.run.swingTiltPos_OS : d.run.swingTiltPos_OD;
	if (d.axSwing.jogToward(target, d.run.posToleranceSteps)) {
		raiseEvent(EyeAlignEvent::SWING_AT_TILT, static_cast<int>(d.eyeSide));
		changePhase(EyeAlignPhase::SEARCHING);
		raiseEvent(EyeAlignEvent::SEARCH_STARTED);
		return true;
	}
	if (elapsedMs(d.phaseStart) > d.run.swingTimeoutMs) {
		d.axSwing.stop();
		raiseEvent(EyeAlignEvent::PHASE_TIMEOUT, static_cast<int>(EyeAlignPhase::SWING_TO_TILT));
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::TIMEOUT);
		return false;
	}
	return true;
}

bool eye_align::EyeAligner::phaseSearching(void)
{
	auto& d = impl();
	// Step 2: Z advances toward the eye whether or not a pupil is seen. The only way
	// out is the side camera locking (eye found) or the search limit (cancel).
	if (d.trkSide.isLocked()) {
		d.axZ.stop();
		changePhase(EyeAlignPhase::CENTERING_XY);
		return true;
	}
	if (zPassed(d.run.minSafePosZ)) {
		d.axZ.stop();
		raiseEvent(EyeAlignEvent::EYE_TOO_CLOSE);
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::EYE_TOO_CLOSE);
		return false;
	}
	if (zPassed(d.run.searchLimitPosZ) || elapsedMs(d.phaseStart) > d.run.searchTimeoutMs) {
		d.axZ.stop();
		raiseEvent(EyeAlignEvent::EYE_NOT_FOUND, static_cast<int>(d.sideCam));
		requestEnd(EyeAlignPhase::CANCELED, EyeAlignResult::EYE_NOT_FOUND);
		return false;
	}
	d.axZ.jog(Z_DIR_FORWARD);
	return true;
}

bool eye_align::EyeAligner::phaseCenteringXY(void)
{
	auto& d = impl();
	// Step 3: Z stays still while X/Y bring the side-camera pupil to the target.
	const bool fix = d.trkSide.hasFix();
	controlAxis(d.axX, d.errX, fix);
	controlAxis(d.axY, d.errY, fix);

	const bool centered = fix
		&& std::fabs(d.errX) <= d.run.tolerancePx
		&& std::fabs(d.errY) <= d.run.tolerancePx
		&& !d.axX.isJogging() && !d.axY.isJogging();
	if (centered) {
		raiseEvent(EyeAlignEvent::XY_CENTERED);
		if (d.trkLower.isLocked()) {
			changePhase(EyeAlignPhase::CENTERING_Z);		// lower camera already sees the pupil
		}
		else {
			changePhase(EyeAlignPhase::APPROACHING);
			raiseEvent(EyeAlignEvent::APPROACH_STARTED);
		}
	}
	return true;
}

bool eye_align::EyeAligner::phaseApproaching(void)
{
	auto& d = impl();
	// Step 4: Z advances while X/Y keep the side-camera pupil centred.
	const bool fix = d.trkSide.hasFix();
	controlAxis(d.axX, d.errX, fix);
	controlAxis(d.axY, d.errY, fix);

	if (d.trkLower.isLocked()) {
		d.axZ.stop();
		changePhase(EyeAlignPhase::CENTERING_Z);
		return true;
	}
	if (zPassed(d.run.minSafePosZ)) {
		d.axZ.stop();
		stopAllAxes();
		raiseEvent(EyeAlignEvent::EYE_TOO_CLOSE);
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::EYE_TOO_CLOSE);
		return false;
	}
	if (d.eyeLostReported) {
		d.axZ.stop();		// the lost-pupil rule holds every motor until the pupil is back
	}
	else {
		d.axZ.jog(Z_DIR_FORWARD);
	}
	return true;
}

bool eye_align::EyeAligner::phaseCenteringZ(void)
{
	auto& d = impl();
	// Step 5: Z centres the lower-camera pupil, X/Y keep tracking.
	const bool sideFix = d.trkSide.hasFix();
	const bool lowerFix = d.trkLower.hasFix();
	controlAxis(d.axX, d.errX, sideFix);
	controlAxis(d.axY, d.errY, sideFix);
	controlAxis(d.axZ, d.errZ, lowerFix);

	if (d.axZ.isJogging() && d.axZ.getDirection() == Z_DIR_FORWARD && zPassed(d.run.minSafePosZ)) {
		stopAllAxes();
		raiseEvent(EyeAlignEvent::EYE_TOO_CLOSE);
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::EYE_TOO_CLOSE);
		return false;
	}

	if (isCentered() && !anyAxisJogging()) {
		d.settleCount++;
		if (d.settleCount >= d.run.settleFrames) {
			changePhase(EyeAlignPhase::FOCUSED);		// step 6
			raiseEvent(EyeAlignEvent::FOCUS_ALIGNED);
		}
	}
	else {
		d.settleCount = 0;
	}
	return true;
}

bool eye_align::EyeAligner::phaseFocused(void)
{
	auto& d = impl();
	// Step 7: keep following. A jog that becomes necessary means the eye moved off
	// the working position: report FOCUS_LOST and re-centre (back to step 5).
	const bool sideFix = d.trkSide.hasFix();
	const bool lowerFix = d.trkLower.hasFix();
	controlAxis(d.axX, d.errX, sideFix);
	controlAxis(d.axY, d.errY, sideFix);
	controlAxis(d.axZ, d.errZ, lowerFix);

	if (d.axZ.isJogging() && d.axZ.getDirection() == Z_DIR_FORWARD && zPassed(d.run.minSafePosZ)) {
		stopAllAxes();
		raiseEvent(EyeAlignEvent::EYE_TOO_CLOSE);
		requestEnd(EyeAlignPhase::FAILED, EyeAlignResult::EYE_TOO_CLOSE);
		return false;
	}

	if (anyAxisJogging()) {
		raiseEvent(EyeAlignEvent::FOCUS_LOST);
		changePhase(EyeAlignPhase::CENTERING_Z);
	}
	return true;
}

bool eye_align::EyeAligner::phaseMonitoring(void)
{
	auto& d = impl();
	// MANUAL: no motion. The operator centres the head; report the focused state.
	if (d.phase == EyeAlignPhase::MONITORING) {
		if (isCentered()) {
			d.settleCount++;
			if (d.settleCount >= d.run.settleFrames) {
				changePhase(EyeAlignPhase::FOCUSED);
				raiseEvent(EyeAlignEvent::FOCUS_ALIGNED);
			}
		}
		else {
			d.settleCount = 0;
		}
		return true;
	}

	// FOCUSED (manual): leave when the pupil drifts past the start deadband on any
	// axis. A lost pupil is handled by the lost-pupil rule.
	const bool driftX = d.trkSide.hasFix() && std::fabs(d.errX) > d.run.startDeadbandPx;
	const bool driftY = d.trkSide.hasFix() && std::fabs(d.errY) > d.run.startDeadbandPx;
	const bool driftZ = d.trkLower.hasFix() && std::fabs(d.errZ) > d.run.startDeadbandPx;
	if (driftX || driftY || driftZ) {
		raiseEvent(EyeAlignEvent::FOCUS_LOST);
		changePhase(EyeAlignPhase::MONITORING);
	}
	return true;
}


// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////

void eye_align::EyeAligner::changePhase(EyeAlignPhase phase)
{
	auto& d = impl();
	const auto previous = d.phase.load();
	d.phase = phase;
	d.phaseStart = Clock::now();
	d.settleCount = 0;
	LogD() << "EyeAligner phase: " << phaseName(previous) << " -> " << phaseName(phase);
}

void eye_align::EyeAligner::raiseEvent(EyeAlignEvent event, int arg)
{
	auto& d = impl();
	EventRecord record{};
	record.event = static_cast<int32_t>(event);
	record.mode = static_cast<int32_t>(d.mode.load());
	record.phase = static_cast<int32_t>(d.phase.load());
	record.arg = arg;
	record.posX = d.axX.getPosition();
	record.posY = d.axY.getPosition();
	record.posZ = d.axZ.getPosition();
	record.posSwing = d.axSwing.getPosition();

	{
		lock_guard<mutex> lock(d.evMutex);
		d.evQueue.push_back(record);
	}
	d.evCondVar.notify_one();
	LogD() << "EyeAligner event: " << eventName(event) << ", arg: " << arg;
}

void eye_align::EyeAligner::requestEnd(EyeAlignPhase terminal, EyeAlignResult reason)
{
	auto& d = impl();
	if (d.endRequested) {
		return;
	}
	d.endRequested = true;
	d.endPhase = terminal;
	d.endReason = reason;
}

void eye_align::EyeAligner::controlAxis(AxisController& axis, float error, bool hasFix)
{
	auto& d = impl();
	if (!axis.isValid() || d.held) {
		return;
	}
	if (!hasFix) {
		axis.stop();
		return;
	}
	const int dir = (error > 0.0f) ? POSITIVE_STEP_DIR : (1 - POSITIVE_STEP_DIR);
	if (axis.isJogging()) {
		if (std::fabs(error) < d.run.stopDeadbandPx || dir != axis.getDirection()) {
			axis.stop();
		}
	}
	else if (std::fabs(error) > d.run.startDeadbandPx) {
		axis.jog(dir);
	}
}

void eye_align::EyeAligner::stopAllAxes(void)
{
	auto& d = impl();
	for (AxisController* axis : { &d.axX, &d.axY, &d.axZ, &d.axSwing }) {
		if (axis->isValid()) {
			axis->stop();
		}
	}
}

bool eye_align::EyeAligner::zPassed(int limitPos) const
{
	const auto& d = impl();
	return d.zForwardSign * (d.axZ.getPosition() - limitPos) >= 0;
}

bool eye_align::EyeAligner::isCentered(void) const
{
	const auto& d = impl();
	return d.trkSide.hasFix() && d.trkLower.hasFix()
		&& std::fabs(d.errX) <= d.run.tolerancePx
		&& std::fabs(d.errY) <= d.run.tolerancePx
		&& std::fabs(d.errZ) <= d.run.toleranceZPx;
}

bool eye_align::EyeAligner::anyAxisJogging(void) const
{
	const auto& d = impl();
	return d.axX.isJogging() || d.axY.isJogging() || d.axZ.isJogging() || d.axSwing.isJogging();
}

void eye_align::EyeAligner::updateStatusSnapshot(void)
{
	auto& d = impl();
	EyeAlignStatus status;
	status.mode = static_cast<int32_t>(d.mode.load());
	status.phase = static_cast<int32_t>(d.phase.load());
	status.result = static_cast<int32_t>(d.result.load());
	status.eyeSide = static_cast<int32_t>(d.eyeSide);
	status.sideCamId = static_cast<int32_t>(d.sideCam);
	status.posX = d.axX.getPosition();
	status.posY = d.axY.getPosition();
	status.posZ = d.axZ.getPosition();
	status.posSwing = d.axSwing.getPosition();
	status.side = d.trkSide.lastResult();
	status.lower = d.trkLower.lastResult();
	status.errX = d.errX;
	status.errY = d.errY;
	status.errZ = d.errZ;
	status.lostFrames = d.eyeFound ? std::max(d.trkSide.lostFrames(), d.lowerFound ? d.trkLower.lostFrames() : 0) : 0;
	status.elapsedMs = elapsedMs(d.runStart);

	lock_guard<mutex> lock(d.statusMutex);
	d.status = status;
}

EyeAligner::EyeAlignerImpl& eye_align::EyeAligner::impl(void) const
{
	return *d_ptr;
}
