#pragma once

// Device-free half of the fundus capture one-call: request validation, the cancellation +
// acquired-resource ledger, and the test-only barrier. Nothing here talks to Spinnaker, the
// main board or Hardware, so it links into a plain console target and its rules can be
// verified with no instrument attached (program/TestCaptureSequence).

#include "LsoCameraStruct.h"
#include "LsoScanDefines.h"

#include <condition_variable>
#include <mutex>
#include <vector>

namespace wso_system
{
	namespace capture
	{
		using namespace wso_domain;

		// Resources the sequence acquires, in acquisition order. Teardown is the reverse of
		// whatever was actually acquired - never a fixed list, so a failure half-way through
		// cannot undo something that was never done.
		enum class SequenceResource : int
		{
			FrameCallback = 0,
			CaptureLight = 1,
			CameraAcquisition = 2,
			ScannerGrabbing = 3,
			// Preview-only. The preview replaces the camera's geometry and tunables
			// (pixel format, binning, ROI, exposure, gain) before anything else runs, so it
			// is the first thing acquired and therefore the last thing put back.
			CameraParams = 4,
		};

		// Cancellation state + resource ledger for one capture session.
		// Cancellation is armed EXACTLY ONCE per session by armStart(), which the starting
		// thread calls while it holds the capture-state lock. No later step clears it: a stop
		// that lands between "start accepted" and "worker running" must still be observed,
		// which is exactly what a clear inside the worker's camera-begin used to lose.
		class SequenceState
		{
		public:
			// Begins a session: clears cancellation and the ledger. Only entry point that
			// may clear cancellation.
			void armStart(void);

			void requestCancel(void);
			bool isCancelled(void) const;

			void acquire(SequenceResource resource);
			bool holds(SequenceResource resource) const;
			void release(SequenceResource resource);

			// Acquired resources in reverse acquisition order.
			std::vector<SequenceResource> teardownPlan(void) const;

			// Marks the ledger empty again once the teardown has run.
			void releaseAll(void);

		private:
			mutable std::mutex lock_;
			bool cancelled_ = false;
			std::vector<SequenceResource> acquired_;
		};

		// Test-only injection point. Arms one step; the worker parks there until the test
		// releases it, so a stop-versus-arm race is reproduced by an event instead of by
		// timing luck. One-shot, and self-releasing after kBarrierAutoReleaseMs so a QA run
		// that dies before releasing can never hang the instrument.
		// kBarrierAutoReleaseMs is deliberately longer than the arrival wait a test uses and
		// shorter than the terminal wait, so "parked at the barrier" stays observable while
		// the normal auto-release is not mistaken for a failure.
		class DiagnosticGate
		{
		public:
			static constexpr int kBarrierAutoReleaseMs = 7000;

			// Arms the barrier at stepId (CAP_STEP_NONE disarms). Clears a stale arrival.
			void arm(int stepId);

			// Blocks until the worker reaches the armed step, bounded by timeoutMs.
			bool waitArrival(int timeoutMs);

			// Lets the parked worker continue.
			void release(void);

			// Called by the worker at each step boundary. Returns immediately unless this
			// step is the armed one.
			void waitAtStep(int stepId);

			// Signalled once, where the sequence observes cancellation. The only supported
			// way for a test to know the stop was seen - elapsed time is not evidence.
			void signalCancelLatched(void);
			bool waitCancelLatched(int timeoutMs);

			// Clears ONLY the cancel latch. This is what a starting session calls: a
			// barrier armed before the capture started must survive session arming,
			// because arming the step first is exactly how the race is injected.
			void clearCancelLatch(void);

			// Clears arm/arrival/latch state (test setup).
			void reset(void);

		private:
			std::mutex lock_;
			std::condition_variable cv_;
			int armedStep_ = CAP_STEP_NONE;
			bool arrived_ = false;
			bool released_ = false;
			bool cancelLatched_ = false;
		};

		// Focused device seam for the request-owned illumination sequence. The driver applies
		// mode before intensity/on, reports every refused board command, and restores a
		// different lamp in the same order. A same-lamp restore only turns it off.
		struct FundusLightOps
		{
			bool (*setMode)(void* ctx, int light, int mode) = nullptr;
			bool (*setIntensity)(void* ctx, int light, unsigned short intensity) = nullptr;
			bool (*turnOn)(void* ctx, int light) = nullptr;
			bool (*turnOff)(void* ctx, int light) = nullptr;
		};

		bool runFundusLightApply(const FundusLightOps& ops, void* ctx,
			int light, int mode, unsigned short intensity);
		bool runFundusLightRestore(const FundusLightOps& ops, void* ctx,
			int workingLight, int restoreLight, int restoreMode, unsigned short restoreIntensity);

		// Steps of the hardware-triggered fundus preview start, in execution order. The
		// order is the contract: the camera is fully configured and already waiting on the
		// board trigger BEFORE the board is told to scan, which is the DevKit ordering
		// (config -> profile -> LED -> StartAcquisition -> WaitForDriverArmed -> LSO_CAPTURE
		// on). Starting the board first would fire triggers into a camera that is not armed.
		enum class PreviewStep : int
		{
			ApplySequence = 0,			// board-global sequence always follows the active preset
			ApplyCameraParams = 1,		// mono geometry + the preview's own exposure/gain
			UploadProfile = 2,			// board slot always carries this preview's trajectory
			PreviewLight = 3,
			ConnectCallback = 4,		// before any frame can be delivered
			BeginTriggeredCamera = 5,	// trigger install + BeginAcquisition + bounded arm wait
			StartScanner = 6,			// board capture on - the first irreversible board state
			Done = 7,					// every step succeeded
		};

		// Step table for runPreviewStart. The export fills it with the device-touching
		// steps; a device-free test fills it with scripted ones, so the ordering and the
		// failure rollback are checked against the SAME driver the instrument runs instead
		// of a re-implementation of it.
		// Each step returns false on failure. CameraParams is reserved before its apply step
		// because failure can follow partial writes; other resources are acquired on success.
		// release() undoes one reserved/acquired resource and must not throw.
		struct PreviewOps
		{
			bool (*applySequence)(void* ctx) = nullptr;
			bool (*applyCameraParams)(void* ctx) = nullptr;
			bool (*uploadProfile)(void* ctx) = nullptr;
			bool (*previewLight)(void* ctx) = nullptr;
			bool (*connectCallback)(void* ctx) = nullptr;
			bool (*beginTriggeredCamera)(void* ctx) = nullptr;
			bool (*startScanner)(void* ctx) = nullptr;
			bool (*release)(void* ctx, SequenceResource resource) = nullptr;
		};

		// Runs the preview start sequence, recording every success in `state`. On the first
		// failure it releases exactly what was acquired, in reverse, empties the ledger and
		// returns the step that failed; on success it returns PreviewStep::Done and `state`
		// holds what the stop path has to undo.
		PreviewStep runPreviewStart(SequenceState& state, const PreviewOps& ops, void* ctx);

		// Stop path: releases in reverse and retains failed resources for a later stop.
		// Idempotent - resources released successfully are never released twice.
		bool runPreviewStop(SequenceState& state, const PreviewOps& ops, void* ctx);

		// --- Preview camera parameters ------------------------------------------------
		// What a preview install writes to the camera, and which field groups it replaced.
		// The groups are the restore contract: the stop path puts back exactly the groups
		// that were replaced and leaves every other field at its current value, so tuning
		// done during a preview that did not own that field still survives the stop.
		// Deciding this is pure arithmetic over the request bytes plus the camera's current
		// settings, so it lives here (device-free) and not in the export.
		struct PreviewParamPlan
		{
			ColorCameraSettingParam params{};
			bool geometry = false;     // pixelFormat, binningHorizontal/Vertical, roi_x/y_*
			bool tunables = false;     // exposure_time, gain
			bool acquisition = false;  // acqusitionMode, acqusitionFrameCount
			bool adcDepth = false;     // adcDepthIndex
			// Crop the applied window to a centered square afterwards. True only when the
			// geometry was derived here (full FOV); a request that carries its own ROI owns
			// the window, and cropping it would throw that ROI away.
			bool squareRoi = false;
		};

		// Legacy mono-preview geometry: monoMode 0 = Mono8 + 2x2 binning at half the sensor
		// window, 1 = BayerRG8 + 1x1 at the full sensor window, both cropped to a centered
		// square. exposureTimeUs == 0 / gain <= 0 mean "leave that tunable alone", which is
		// what the free-running mono preview export asks for.
		PreviewParamPlan planMonoPreviewParams(const ColorCameraSettingParam& current,
			int monoMode, unsigned int exposureTimeUs, float gain);

		// Preset-driven preview geometry: the request's camera preset owns pixel format,
		// binning, ROI, acquisition mode and ADC depth; exposureTimeUs / gain (the validated
		// top-level fields) own the tunables. A preset with a zero-sized ROI carries no
		// window, so the monoMode geometry above is kept instead.
		PreviewParamPlan planPreviewParams(const ColorCameraSettingParam& current,
			const LsoFundusPreviewRequest& request);

		ColorCameraSettingParam resolveCaptureCameraParams(const ColorCameraSettingParam& requested,
			unsigned int totalFrameCount, bool hwTriggered);

		// Capture submission keeps every requested scanner field and overwrites exposure with
		// the direct camera readback exactly, including zero/non-positive boundary values.
		LsoScannerControlParam resolveCaptureScannerProfile(const LsoScannerControlParam& requested,
			double cameraExposureUs);

		// Restore value for the stop path: `saved` for every group the plan replaced, `live`
		// for everything else.
		ColorCameraSettingParam mergePreviewRestore(const ColorCameraSettingParam& live,
			const ColorCameraSettingParam& saved, const PreviewParamPlan& plan);

		// --- Extended (A-1) camera contract, request bytes only -----------------------
		// An extended parameter block that states nothing: every enum name empty, every
		// numeric sentinel negative. Applying it writes no node, so it is what a caller
		// with no A-1 configuration of its own passes.
		ColorCameraExtendedParam emptyCameraExtendedParam(void);

		// True when the field is not stated by the preset and must therefore be left to
		// whatever owns it today (the device, or the start path's own default).
		bool isCameraNameStated(const char* name);

		// Whether the STATED camera configuration can drive an acquisition on this
		// instrument's wiring. Device-free: it compares the stated names against the board
		// trigger contract, so a Toolkit can pre-check a preset with no camera attached.
		// hwTriggered selects the contract: the board gates the exposures (Preview and the
		// HW-trigger Capture) versus a free-running Normal capture.
		// Returns a CameraStartContract code; CAM_START_OK means nothing conflicts.
		// Fields the preset leaves unstated never conflict - they are not this preset's
		// claim, and the start path still owns them.
		int checkCameraStartContract(const ColorCameraExtendedParam* ext, bool hwTriggered);

		// Stable, distinct text per contract code, naming the conflicting field.
		const char* describeCameraStartContract(int contract);

		// True when the block owns the whole trigger group (TriggerSource, TriggerSelector,
		// TriggerActivation and TriggerOverlap are all stated). That is the switch between
		// the two ownerships: owned means the start installs the block and then writes no
		// trigger or GPIO node of its own, not owned means the start's existing hardware
		// trigger setup keeps writing its constants exactly as it always did.
		// checkCameraStartContract refuses a block that states only part of the group, so
		// there is no third case where the two would be mixed.
		bool isCameraTriggerContractStated(const ColorCameraExtendedParam* ext);

		// The block a start actually applies. TriggerMode is the one node whose required
		// value depends on how the acquisition is driven, so when the group is owned and the
		// preset left TriggerMode unstated, the drive mode fills it in HERE - before the
		// apply - instead of a start path writing over the applied configuration afterwards.
		// A block that does not own the group is returned unchanged.
		ColorCameraExtendedParam resolveStartCameraConfig(const ColorCameraExtendedParam& ext,
			bool hwTriggered);

		// The block a fundus PREVIEW applies: resolveStartCameraConfig(ext, true), plus the
		// request's validated top-level triggerDelayUs materialised into the block whenever
		// the block does not state one of its own.
		//
		// That materialisation is the whole point. When the preset owns the trigger group,
		// the camera start only arms and writes no trigger node - so if the delay lived only
		// in the top-level field, NOTHING would write it and the board-sync delay would
		// silently vanish. Folding it in here gives the node exactly one owner on this path,
		// and that owner goes through the same checked apply and read-back as every other
		// field. A block that states its own delay keeps it.
		ColorCameraExtendedParam resolvePreviewCameraConfig(const LsoFundusPreviewRequest& request);

		// Whether the block claims any GPIO row. Reading a row means writing LineSelector,
		// so a caller that only needs a restore snapshot can skip that traffic - and the
		// selector churn - when no row is claimed.
		bool statesAnyCameraLine(const ColorCameraExtendedParam* ext);

		// Validates a capture request using the request bytes ONLY - no board read, no camera
		// read, no Hardware access - so it can run (and be tested) before any device exists.
		// On success writes the derived total frame count. reason may be null.
		bool validateFundusCaptureRequest(const LsoFundusCaptureRequest* request,
			unsigned int* totalFrameCount, CaptureValidationReason* reason);

		bool validateFundusPreviewRequest(const LsoFundusPreviewRequest* request,
			CaptureValidationReason* reason);

		// Illumination intensity actually applied for a request: requested if > 0, else the
		// board calibration value (may be 0 = leave the board setting untouched).
		unsigned short resolveFundusLightIntensity(unsigned short requested, unsigned short calibrated);

		// Board traj_profile.exposure_time_us. DevKit ApplyLsoPatternConfig always writes
		// GetColorCameraExposureUs into this field and never the JSON ExposureTimeUs. A
		// positive camera reading wins; a non-positive reading keeps the request so a
		// missing camera does not zero the board field.
		float resolveBoardExposureUs(float requestedUs, double cameraUs);

		// Stable, distinct text per rejection gate, for the last-capture-error surface.
		const char* describeValidationReason(CaptureValidationReason reason);
	}
}
