#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// Main Camera
		//////////////////////////////////////////////////////////////////////////////////////////
		void WSOSYSTEM_DLL_API __stdcall startColorCameraLive(ColorCameraFrameCaptured clbFrame);
		void WSOSYSTEM_DLL_API __stdcall pauseColorCameraLive(void);
		bool WSOSYSTEM_DLL_API __stdcall isColorCameraLive(void);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraSingleFrameLive(ColorCameraFrameCaptured clbFrame);
		void WSOSYSTEM_DLL_API __stdcall startColorCameraSeqROILive(ColorCameraSeqROIFrameCaptured clbFrame, const LsoCaptureFrameSeqROIPreset* preset);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraSwTriggerOffsetROILive(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset);
		void WSOSYSTEM_DLL_API __stdcall startColorCameraSwTriggerOffsetROICapture(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraRollSwTrigOverlapLive(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset);
		void WSOSYSTEM_DLL_API __stdcall startColorCameraRollSwTrigOverlapCapture(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraSwTriggerLive(ColorCameraFrameCaptured clbFrame);
		void WSOSYSTEM_DLL_API __stdcall stopColorCameraSwTriggerLive(void);
		void WSOSYSTEM_DLL_API __stdcall shootColorCameraSwTriggerLive(void);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraHwTriggerLive(ColorCameraImageCaptured clbImage);
		void WSOSYSTEM_DLL_API __stdcall stopColorCameraHwTriggerLive(void);
		bool WSOSYSTEM_DLL_API __stdcall isColorCameraIsStreaming(void);
		void WSOSYSTEM_DLL_API __stdcall setupColorCameraHwTriggerLive(void);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraOriginal(ColorCameraImageCaptured clbImage);
		void WSOSYSTEM_DLL_API __stdcall pauseColorCameraOriginal(void);

		void WSOSYSTEM_DLL_API __stdcall getColorCameraSettingParam(ColorCameraSettingParam* pParam);

		// Applies camera settings. While a live/capture session is active, structural fields
		// (pixel format, binning, ROI, acq mode/count, ADC) are camera-locked: tunables
		// (exposure, gain) are still applied so live tuning keeps working, structural fields
		// are ignored and false is returned. Stop the session before structural changes.
		bool WSOSYSTEM_DLL_API __stdcall setColorCameraSettingParam(const ColorCameraSettingParam* pParam);

		// --- A-1 extended camera configuration ---------------------------------------------
		// The 14-field setColorCameraSettingParam above keeps its exact contract; these are
		// additive entry points for the rest of the DevKit ColorCameraConfig.
		//
		// Values are GenApi SYMBOLIC names, so nothing here depends on SDK enum integers.
		// An empty name (or a negative number) means "this caller does not state this node",
		// and the node is then left exactly as the device has it.
		//
		// setColorCameraExtendedParam writes only the extended nodes;
		// setColorCameraFullSettingParam writes the whole camera contract in node-dependency
		// order (TriggerMode Off -> sensor/binning -> the 14 fields -> GPIO -> trigger ->
		// TriggerMode). Both return true only when every STATED field was written AND read
		// back equal - a setter that did not throw is not treated as success - and both fill
		// `result` with the per-field requested/actual/status table the UI shows.
		// `result` may be null. structural (14-field) verification is reported through the
		// result table's "PixelFormat/Binning/ROI" row.
		bool WSOSYSTEM_DLL_API __stdcall setColorCameraExtendedParam(const ColorCameraExtendedParam* param, ColorCameraApplyResult* result);
		bool WSOSYSTEM_DLL_API __stdcall setColorCameraFullSettingParam(const ColorCameraSettingParam* base, const ColorCameraExtendedParam* ext, ColorCameraApplyResult* result);

		// Current device state of the extended nodes. Unreadable nodes come back unstated,
		// so a snapshot taken here never invents a value.
		bool WSOSYSTEM_DLL_API __stdcall getColorCameraExtendedParam(ColorCameraExtendedParam* param);

		// Capability, for an editor that must offer what the camera implements instead of a
		// hardcoded list: the available symbolic entries of an enum node, and the range and
		// increment of an integer or float node. 0 / false means the node is not there.
		int WSOSYSTEM_DLL_API __stdcall getColorCameraEnumEntries(const char* nodeName, ColorCameraEnumValue* buffer, int maxCount);
		bool WSOSYSTEM_DLL_API __stdcall getColorCameraNodeRange(const char* nodeName, double* minValue, double* maxValue, double* increment, bool* writable);
		// Current value of an integer or float node (AcquisitionLineRate, ResultingFrameRate,
		// ...): what the range sibling cannot answer. false = node absent or unreadable.
		bool WSOSYSTEM_DLL_API __stdcall getColorCameraNodeValue(const char* nodeName, double* value);

		// Device-free: whether a stated camera configuration can drive an acquisition on this
		// instrument's wiring. Returns a CameraStartContract code so the caller can name the
		// conflicting field BEFORE it starts anything - which is what keeps a start from
		// silently replacing an operator's value with a constant.
		int WSOSYSTEM_DLL_API __stdcall validateLsoCameraStartContract(const ColorCameraExtendedParam* param, bool hwTriggered);
		int WSOSYSTEM_DLL_API __stdcall describeLsoCameraStartContract(int contract, char* buf, int len);

		// Native sizeof() of the ABI structs, so a managed mirror can be checked against the
		// shipped DLL instead of against a comment. `which` is LsoCameraStructId.
		unsigned int WSOSYSTEM_DLL_API __stdcall getLsoCameraStructSize(int which);

		bool WSOSYSTEM_DLL_API __stdcall squareColorCameraRoi(void);

		// One-call mono square preview: params(Mono8/Bayer) -> squareRoi -> IR LED -> scanner IR grab -> live.
		// mode: 0 = SensorMono8, 1 = BayerSwGray. stop reverses and restores saved params.
		bool WSOSYSTEM_DLL_API __stdcall startColorCameraMonoPreview(int mode, ColorCameraFrameCaptured cb);
		bool WSOSYSTEM_DLL_API __stdcall stopColorCameraMonoPreview(void);

		// One-call color capture: IR off -> preset apply(checked) -> scanner grab ->
		// worker acquisition -> scanner pause -> onCompleted(result, framesAcquired).
		// Returns ColorCaptureStage: CAP_STAGE_OK once the worker started, else the failed
		// pre-acquisition stage (no completion callback follows a non-OK start). For HWTrigger,
		// camera arm/readiness/scanner failures are asynchronous completion results.
		// stop cancels and joins the worker - never call it from the frame/completed callback.
		// isCapturing() remains true through the completion callback. Start from that callback
		// returns BUSY and stop returns false; consumers must marshal and return promptly.
		int WSOSYSTEM_DLL_API __stdcall startColorCameraColorCapture(const ColorCameraSettingParam* param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);
		// triggerMode: COLOR_CAPTURE_NORMAL or COLOR_CAPTURE_HW_TRIGGER.
		int WSOSYSTEM_DLL_API __stdcall startColorCameraColorCaptureEx(const ColorCameraSettingParam* param, int scanPatternId, ColorCaptureTriggerMode triggerMode, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

		// Same device sequence as startColorCameraColorCapture, but the acquired subframes are
		// composed natively and the consumer receives ONE image: subframe j contributes rows
		// [j*segH, (j+1)*segH) of the composite (segH = height / subFrameCount, the last band takes
		// the remainder). cbComposed fires with frameCount=0, totalFrameCount=1 right before
		// onCompleted, and ONLY when every band was written - a cancelled or camera-aborted
		// capture reports through onCompleted alone, with no image.
		// subFrameCount <= 0 derives the band count from the acquired total frame count; frames
		// beyond subFrameCount (extra acquisition frames) are ignored.
		// stop/isCapturing/getLastColorCaptureError are shared with the per-frame capture above.
		int WSOSYSTEM_DLL_API __stdcall startColorCameraColorCaptureComposed(const ColorCameraSettingParam* param, int scanPatternId, int subFrameCount, ColorCameraImageCaptured cbComposed, ColorCaptureCompleted onCompleted);
		int WSOSYSTEM_DLL_API __stdcall startColorCameraColorCaptureComposedEx(const ColorCameraSettingParam* param, int scanPatternId, int subFrameCount, ColorCaptureTriggerMode triggerMode, ColorCameraImageCaptured cbComposed, ColorCaptureCompleted onCompleted);
		bool WSOSYSTEM_DLL_API __stdcall stopColorCameraColorCapture(void);
		bool WSOSYSTEM_DLL_API __stdcall isColorCameraCapturing(void);
		int WSOSYSTEM_DLL_API __stdcall getLastColorCaptureError(char* buf, int len);


		// --- Fundus one-call: the whole device sequence behind a single entry ---------------
		// Owns, in this fixed order: request validation (request bytes only, before any
		// device is touched) -> camera preset apply+verify -> measured exposure written into
		// the profile -> profile upload to the board slot -> capture illumination -> camera
		// acquisition begin -> HW trigger arm wait -> board capture on -> bounded frame
		// acquisition -> teardown of exactly what was acquired, in reverse, including the
		// restore illumination -> onCompleted(result, framesAcquired).
		//
		// Frame count authority is request->profile.acqFrameSize * profile.subFrameSize. The
		// camera's own acquisition mode/count are OUTPUTS here: they are overwritten from the
		// derived count before the checked apply, so there is no camera-versus-board contest.
		// Each factor is rejected while still zero (never hidden by a max(1, x) guard) and
		// the 64-bit product is bounded by what the frame callbacks can carry (INT32_MAX).
		//
		// Start return: CAP_STAGE_OK once the worker started - completion is then reported
		// exactly once - else the failed pre-acquisition stage and no completion callback:
		//   PARAM   request rejected (see getLastColorCaptureError for which gate)
		//   FAULT   hardware / main board / color camera not ready, or illumination failed
		//   BUSY    a live, aiming or capture session is active
		//   SCANNER scan profile upload failed
		// Asynchronous results arrive through onCompleted: OK / CAMERA (begin or arm) /
		// SCANNER (board start) / CANCELLED.
		//
		// Cancellation is armed exactly once, by the accepted start while it holds the
		// capture-state lock; no later step clears it, so a stop issued right after an
		// accepted start is still observed and the board is never started.
		// stop / isCapturing / getLastColorCaptureError are shared with the capture exports
		// above, and the same rule applies: never call stop from a frame/completed callback.
		int WSOSYSTEM_DLL_API __stdcall startLsoFundusCapture(const LsoFundusCaptureRequest* request, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

		// One-call fundus (aiming) preview, HARDWARE-TRIGGERED like the DevKit preview path.
		// Same ownership, in this fixed order:
		//   validation -> camera mono params + centered square ROI + the request's own
		//   exposure/gain -> profile upload (board exposure = the camera's read-back after
		//   that preset) -> preview illumination -> frame callback -> trigger install
		//   (TriggerMode On, FrameStart, Line3, RisingEdge, ReadOut, TriggerDelay) +
		//   BeginAcquisition + bounded FrameTriggerWait arm wait -> board capture on.
		// The board is started LAST, and only after the camera is armed: a trigger issued
		// at a camera that is not waiting for it is lost. There is no trigger on/off flag -
		// one preview request means the whole triggered sequence.
		// The profile must be continuous; a fixed-frame profile is rejected instead of
		// silently overwritten, and a zero camera exposure is rejected (it is not a
		// "leave it alone" sentinel the way a zero light intensity is).
		// A failure at any step releases exactly what was acquired, in reverse - including
		// the camera geometry, exposure/gain, the trigger delay and the Line2 output source
		// - so false means nothing is left running. stop reverses the same ledger and
		// applies the restore illumination; the live worker's per-image wait is bounded, so
		// stopping never blocks on a trigger that will not come.
		// The caller does NOT start or pause the scanner - doing it as well double-drives the
		// board (off/on twice per transition).
		bool WSOSYSTEM_DLL_API __stdcall startLsoFundusPreview(const LsoFundusPreviewRequest* request, ColorCameraFrameCaptured cb);
		bool WSOSYSTEM_DLL_API __stdcall stopLsoFundusPreview(void);

		// --- Diagnostics: request gates and the deterministic race seam ---------------------
		// validate* run the same request-only rules the one-call runs, with no device access,
		// and return CaptureValidationReason. They exist so the gates can be checked against
		// the shipped DLL with no instrument attached.
		int WSOSYSTEM_DLL_API __stdcall validateLsoFundusCaptureRequest(const LsoFundusCaptureRequest* request, unsigned int* totalFrameCount);
		int WSOSYSTEM_DLL_API __stdcall validateLsoFundusPreviewRequest(const LsoFundusPreviewRequest* request);

		// Test-only barrier: arms one LsoCaptureStep, so the worker parks there and a
		// stop-versus-arm race is reproduced by an event instead of by timing luck. One-shot,
		// and self-releasing after 7000 ms so a QA run that dies before releasing cannot hang
		// the instrument. An arm placed BEFORE the capture starts survives session arming.
		// waitLsoCaptureCancelLatched is the only supported way to observe that a stop was
		// seen by the sequence; elapsed time is not evidence.
		bool WSOSYSTEM_DLL_API __stdcall setLsoCaptureBarrier(int stepId);
		bool WSOSYSTEM_DLL_API __stdcall waitLsoCaptureBarrier(int timeoutMs);
		void WSOSYSTEM_DLL_API __stdcall releaseLsoCaptureBarrier(void);
		bool WSOSYSTEM_DLL_API __stdcall waitLsoCaptureCancelLatched(int timeoutMs);

	}
}