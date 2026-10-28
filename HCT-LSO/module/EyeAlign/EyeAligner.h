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
	//   4  Z -> searchLimitPosZ: searchStepZ per frame until the side camera locks, which
	//      goes straight to 5a; no lock at the limit: Z stops, waits 5 s, then cancels
	//   5a Z advances (approachStepZ per frame) until the side-camera pupil is inside the
	//      middle half of the image width or the lower-camera pupil is above 3/4 of the
	//      image height; Z rests without a side lock or at minSafePosZ; searchTimeoutMs
	//      from the start of 4 without the pupil coming in cancels
	//   5b Y centres the side-camera pupil vertically (toleranceYPx), Z at rest: centerStepY
	//      per frame from the outer quarters of the image height, fine steps inside
	//      the middle half; Y rests without a pupil fix or at an end switch; on target -> 7
	//   6  X centres the lower-camera pupil horizontally (toleranceXPx): centerStepX per
	//      frame from the outer quarters of the image width, fine steps inside the middle
	//      half; rests without a fix or at an end switch; on target -> 7
	//   7  Z centres it vertically (toleranceZPx): centerStepZ per frame from the outer
	//      quarters of the image height, fine steps inside the middle half; rests without
	//      a fix or at a limit; on target with X on target too -> 8, otherwise -> 6
	//   8  FOCUSED: no motion; X or Z outside its tolerance -> FOCUS_LOST -> 7
	//   The whole run up to the first FOCUS_ALIGNED is bounded by alignTimeoutMs. Every
	//   ending - a user cancel, an automatic cancel (SIDE_EYE_NOT_FOUND, ALIGN_TIMEOUT,
	//   PUPIL_LOST) or a failure - backs Z off to readyPosZ (backOffOnEnd); a second
	//   cancel during that retreat stops it where it is.
	//
	// Threads
	//   camera thread  -> onCorneaFrame(): copies the frame into CorneaFrameStore, returns.
	//   worker thread  -> one control iteration per new frame: detect, track, drive the
	//                     motors, run the phase machine, enqueue events. The position
	//                     phases (ready Z, eye-side X, swing tilt) each run one blocking
	//                     position move; the search and approach phases advance Z by
	//                     searchStepZ / approachStepZ per frame, and the centring phases move one axis by a
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
		// Stops every axis, backs Z off to readyPosZ (backOffOnEnd, AUTO/FULL) and ends
		// the run with ALIGN_CANCELLED(USER_CANCEL). Called again during the retreat it
		// aborts that move too. With wait the call returns after both threads have
		// finished (the retreat is done and the terminal event was delivered).
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
		// Step 5a end condition: the side-camera pupil inside the middle half of the image
		// width, or the lower-camera pupil above 3/4 of the image height.
		bool isPupilInApproachRange(void) const;
		bool phaseCenteringX(void);
		bool phaseCenteringZ(void);
		bool phaseFocused(void);
		bool phaseMonitoring(void);
		// Z has arrived at a limit (no further forward move is required) when it is
		// within posToleranceSteps of it or past it, in the forward direction.
		bool hasZReached(int limitPos) const;
		// Z at a limit with no lock on the camera that should see the pupil: waits
		// waitMs from the first call, then returns true (time to cancel).
		bool hasLockGraceExpired(int waitMs);

		// dwellMs > 0 keeps processPhase() from running the new phase until the dwell
		// has elapsed; the phase timeout counts from the end of the dwell.
		void changePhase(EyeAlignPhase phase, int dwellMs = 0);
		void raiseEvent(EyeAlignEvent event, int arg = 0);
		void requestRunEnd(EyeAlignPhase terminal, EyeAlignResult reason);
		void finishRun(void);
		void backOffToReadyPosition(void);

		// Cornea IR illumination: turns both LEDs on (no intensity control). Runs at the
		// start of every run.
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
		// True when a relative move of offset would go into the axis's active end switch.
		static bool movesIntoEnd(const class AxisController& axis, int offset);
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
