#pragma once

#include "EyeAlign2.h"

#include <memory>


namespace wso_device
{
	class MainBoard;
}


namespace eye_align
{
	class PupilCenterModel;

	// Auto alignment of the optical head to the patient's eye.
	//
	// Sequence (AUTO / FULL)
	//   1  Z -> readyPosZ          2  X -> eye side        3  swing -> tilt
	//      (2: the swing goes to zero first when the head stands on the other eye's side)
	//   4a Z -> searchLimitPosZ: stepZ per frame until the side camera locks, then one
	//      position move for the rest of the way (skipped when the lower camera is
	//      locked too with its pupil above the 3/4 line); no lock at the limit cancels
	//   4b Y centres the side-camera pupil vertically (toleranceYPx)
	//   5  Z advances (stepZ per frame) until the lower camera locks, then follows the
	//      pupil's vertical error (approachTolZPx), forward moves clipped at minSafePosZ
	//   6  X centres the lower-camera pupil horizontally (toleranceXPx)
	//   7  Z centres it vertically (toleranceZPx); X off again -> 6
	//   8  FOCUSED: no motion; X or Z outside its tolerance -> FOCUS_LOST -> 7
	//   The whole run up to the first FOCUS_ALIGNED is bounded by alignTimeoutMs. An
	//   automatic cancel (EYE_NOT_FOUND, LOWER_EYE_NOT_FOUND, ALIGN_TIMEOUT, PUPIL_LOST)
	//   backs Z off to readyPosZ; a user cancel stops at once.
	//
	// Threads
	//   camera thread  -> onCorneaFrame(): copies the frame into CorneaFrameStore, returns.
	//   worker thread  -> one control iteration per new frame: detect, track, drive the
	//                     motors, run the phase machine, enqueue events. The position
	//                     phases (ready Z, eye-side X, swing tilt) each run one blocking
	//                     position move; the search and approach phases advance Z by
	//                     stepZ per frame, and the centring phases move one axis by a
	//                     short blocking position move (error px * stepsPerPx); every
	//                     move is re-measured on the next frame.
	//   dispatch thread-> pops events and calls CallbackRegistry::runEyeAlignEventOccurred,
	//                     so a slow managed handler never stalls the control loop.
	//
	// The aligner controls ONLY the X, Y, Z stage motors, the swing motor and the two
	// cornea IR LEDs. It never touches any other motor or the cameras' settings.
	class EYEALIGN_DLL_API EyeAligner
	{
	public:
		EyeAligner();
		virtual ~EyeAligner();

		EyeAligner(const EyeAligner& rhs) = delete;
		EyeAligner& operator=(const EyeAligner& rhs) = delete;

		static EyeAligner* getInstance(void);
		static std::mutex singleMutex_;

	public:
		// Caches the four motors and the cornea camera. Safe to call again after a
		// board re-initialisation.
		bool initializeEyeAligner(wso_device::MainBoard* board);
		bool isInitialized(void) const;
		// Cancels a running alignment and forgets the board.
		void releaseEyeAligner(void);

		void setConfig(const EyeAlignConfigParam& config);
		EyeAlignConfigParam getConfig(void) const;

		// Pupil-centre network (model path, presence threshold, threads). Loading the model
		// happens here, on the caller's thread, when the path or thread count changed; a
		// threshold-only change applies without a reload.
		void setModelConfig(const PupilModelConfig& config);
		PupilModelConfig getModelConfig(void) const;

		// Starts a run. AUTO and MANUAL use eyeSide (OD when unknown); FULL forces OD.
		// Returns EyeAlignResult::NONE on success, otherwise the refusal reason.
		EyeAlignResult startAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback);
		// Stops every axis, ends the run with ALIGN_CANCELLED(USER_CANCEL). With wait the
		// call returns after both threads have finished (the terminal event was delivered).
		void cancel(bool wait = true);

		// Freezes the motors while a measurement runs; detection and events continue.
		// Accepted only after the eye was found (CENTERING_Y .. FOCUSED) in AUTO/FULL;
		// resume() continues in the phase that was held (the alignment timeout excludes
		// the held time).
		bool hold(void);
		bool resume(void);
		bool isHeld(void) const;

		bool isRunning(void) const;
		bool isCancelling(void) const;
		EyeAlignMode getMode(void) const;
		EyeAlignPhase getPhase(void) const;
		EyeAlignResult getResult(void) const;
		bool fetchStatus(EyeAlignStatus* status) const;

		// Latest detector result per camera (polled by the UI). A result older than
		// PUPIL_RESULT_STALE_MS, or produced while nothing was running, reads as not found.
		bool isPupilDetected(IrCameraId cameraId) const;
		bool getPupilResult(IrCameraId cameraId, PupilDetectResult* result) const;

		// Runs the detector of cameraId on an arbitrary image (offline harness).
		PupilDetectResult detectPupilInImage(const std::uint8_t* gray, int width, int height, IrCameraId cameraId) const;

		// Test hook: while set, the worker uses this result for cameraId instead of the
		// detector. nullptr clears. Lets the whole sequence run before the algorithm exists.
		void setPupilOverride(IrCameraId cameraId, const PupilDetectResult* result);
		void clearPupilOverrides(void);

		// Frame sink registered on the cornea camera (camera thread).
		void onCorneaFrame(IrCameraId cameraId, const std::uint8_t* pixels, int width, int height);

	protected:
		// The shared model, created and loaded on first use (lets the offline harness run
		// without the hardware init path). Never nullptr; a model that failed to load
		// simply makes every detection read "not found".
		std::shared_ptr<PupilCenterModel> acquireModel(void) const;

		void runControlLoop(void);
		void runEventDispatch(void);
		void joinThreads(void);

		bool runControlIteration(void);
		void updateDetections(void);
		void pollAxes(void);
		bool checkAxisFaults(void);		// a rejected motor command ends the run (MOTOR_MOVE_FAILED)
		// Reports stage limits (X/Y/Z end switches, Z at minSafePosZ) once per excursion
		// and never ends the run; the search limit stays inside phaseSearching().
		void checkAxisLimits(void);
		// Bounds a Z offset so the move never goes forward past minSafePosZ nor backward
		// past readyPosZ; an offset ending within posToleranceSteps of a bound becomes 0.
		int clipZ(int offset) const;
		bool applyHoldRequest(void);
		bool applyLostPupilRule(void);
		// AUTO/FULL: alignTimeoutMs after the run start (held time excluded) without a
		// first FOCUS_ALIGNED ends the run with ALIGN_CANCELLED(ALIGN_TIMEOUT).
		bool checkAlignTimeout(void);
		bool processPhase(void);

		// One blocking position move. Returns true when the target was reached; otherwise
		// the run end was requested (or cancel() aborted the move) and the caller must
		// return false.
		bool movePhaseAxisTo(class AxisController& axis, int target, EyeAlignPhase phase, int timeoutMs);
		bool phaseToReadyZ(void);
		bool phaseToEyeX(void);
		bool phaseSwingToTilt(void);
		bool phaseSearching(void);
		bool phaseCenteringY(void);
		bool phaseApproaching(void);
		bool phaseCenteringX(void);
		bool phaseCenteringZ(void);
		bool phaseFocused(void);
		bool phaseMonitoring(void);
		// Z has arrived at a limit (no further forward move is required) when it is
		// within posToleranceSteps of it or past it, in the forward direction.
		bool hasZReached(int limitPos) const;
		// Z at a limit with no lock on the camera that should see the pupil: waits
		// SEARCH_LOCK_GRACE_MS from the first call, then returns true (time to cancel).
		bool hasLockGraceExpired(void);

		// dwellMs > 0 keeps processPhase() from running the new phase until the dwell
		// has elapsed; the phase timeout counts from the end of the dwell.
		void changePhase(EyeAlignPhase phase, int dwellMs = 0);
		void raiseEvent(EyeAlignEvent event, int arg = 0);
		void requestRunEnd(EyeAlignPhase terminal, EyeAlignResult reason);
		void finishRun(void);
		void backOffToReadyPosition(void);

		// Cornea IR illumination: turns both LEDs on at runConfig.irLedIntensity. Runs at
		// the start of every run.
		void setupCorneaLeds(void);

		// Outcome of one pixel-error control step of an axis.
		enum class AxisMove
		{
			NONE,		// no fix, or the error is inside the tolerance: nothing to do
			MOVED,		// one position move was issued; the errors are stale until the next frame
			CLIPPED		// a move was needed but the Z bounds reduced it to nothing (Z at a limit)
		};
		// Position move of one axis from its pixel error: when |error| exceeds tolerancePx,
		// one blocking StepMotor::updatePositionByOffset() of error * stepsPerPx steps,
		// capped at MOVE_JUMP_FRACTION * maxJumpPx worth of steps. Z offsets pass clipZ().
		AxisMove controlAxis(class AxisController& axis, float error, bool hasFix, float tolerancePx, float stepsPerPx);
		// One blocking relative move (AxisController::moveBy) tracked in movingAxis so
		// cancel() can abort it; a move into an active end switch is skipped.
		void stepAxis(class AxisController& axis, int offset);
		AxisMove controlAxisY(bool sideHasFix);							// Y from errorY (side camera, toleranceYPx)
		AxisMove controlAxisX(bool lowerHasFix);						// X from errorX (lower camera, toleranceXPx)
		AxisMove controlAxisZ(bool lowerHasFix, float tolerancePx);		// Z from errorZ (lower camera)
		void stopAllAxes(void);
		bool hasZPassed(int limitPos) const;
		// The tracker whose loss matters in the current phase (nullptr: none yet) and its camera.
		const class PupilTracker* watchedTracker(IrCameraId& cameraId) const;
		bool isLowerCentered(void) const;			// lower fix inside toleranceXPx / toleranceZPx
		bool isCentered(void) const;				// MANUAL: lower centred and side fix inside toleranceYPx
		bool hasDriftedFromTarget(void) const;		// |error| > startDeadbandPx on any axis with a fix
		void updateStatusSnapshot(void);

	private:
		struct EyeAlignerImpl;
		std::unique_ptr<EyeAlignerImpl> d_ptr;
		EyeAlignerImpl& impl(void) const;
	};
}
