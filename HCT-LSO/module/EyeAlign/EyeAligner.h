#pragma once

#include "EyeAlign2.h"

#include <memory>


namespace wso_device
{
	class MainBoard;
}


namespace eye_align
{
	// Auto alignment of the optical head to the patient's eye.
	//
	// Threads
	//   camera thread  -> onCorneaFrame(): copies the frame into CorneaFrameStore, returns.
	//   worker thread  -> one control iteration per new frame: detect, track, drive the
	//                     X/Y/Z stage and swing motors (velocity jog only), run the phase
	//                     machine, enqueue events.
	//   dispatch thread-> pops events and calls CallbackRegistry::runEyeAlignEventOccurred,
	//                     so a slow managed handler never stalls the control loop.
	//
	// The aligner controls ONLY the X, Y, Z stage motors and the swing motor. It never
	// touches any other motor, the cameras' settings or the lights.
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

		// Starts a run. SINGLE and MANUAL use eyeSide (OD when unknown); FULL forces OD.
		// Returns EyeAlignResult::NONE on success, otherwise the refusal reason.
		EyeAlignResult startAlignment(EyeAlignMode mode, EyeSide eyeSide, EyeAlignEventOccurred callback);
		// Stops every axis, ends the run with ALIGN_CANCELLED(USER_CANCEL). With wait the
		// call returns after both threads have finished (the terminal event was delivered).
		void cancel(bool wait = true);

		// Freezes the motors while a measurement runs; detection and events continue.
		// Accepted only after the eye was found (CENTERING_XY .. FOCUSED) in SINGLE/FULL.
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
		bool isPupilDetected(IrCameraId camId) const;
		bool getPupilResult(IrCameraId camId, PupilDetectResult* result) const;

		// Runs the detector of camId on an arbitrary image (offline harness).
		PupilDetectResult detectPupilInImage(const std::uint8_t* gray, int width, int height, IrCameraId camId) const;

		// Test hook: while set, the worker uses this result for camId instead of the
		// detector. nullptr clears. Lets the whole sequence run before the algorithm exists.
		void setPupilOverride(IrCameraId camId, const PupilDetectResult* result);
		void clearPupilOverrides(void);

		// Frame sink registered on the cornea camera (camera thread).
		void onCorneaFrame(IrCameraId camId, const std::uint8_t* bits, int width, int height);

	protected:
		void threadWorkFunction(void);
		void threadEventFunction(void);
		void joinThreads(void);

		bool loopOnce(void);
		void updateDetections(void);
		void pollAxes(void);
		bool checkAxisFaults(void);
		bool applyHoldRequest(void);
		bool applyLostPupilRule(void);
		bool processPhase(void);

		bool phaseToReadyZ(void);
		bool phaseToEyeX(void);
		bool phaseSwingToTilt(void);
		bool phaseSearching(void);
		bool phaseCenteringXY(void);
		bool phaseApproaching(void);
		bool phaseCenteringZ(void);
		bool phaseFocused(void);
		bool phaseMonitoring(void);

		void changePhase(EyeAlignPhase phase);
		void raiseEvent(EyeAlignEvent event, int arg = 0);
		void requestEnd(EyeAlignPhase terminal, EyeAlignResult reason);
		void finishRun(void);
		void backOffToReady(void);

		void controlAxis(class AxisController& axis, float error, bool hasFix);
		void stopAllAxes(void);
		bool zPassed(int limitPos) const;
		bool isCentered(void) const;
		bool anyAxisJogging(void) const;
		void updateStatusSnapshot(void);

	private:
		struct EyeAlignerImpl;
		std::unique_ptr<EyeAlignerImpl> d_ptr;
		EyeAlignerImpl& impl(void) const;
	};
}
