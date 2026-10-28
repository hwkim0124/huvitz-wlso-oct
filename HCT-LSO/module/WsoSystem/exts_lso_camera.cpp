#include "pch.h"
#include "exts_lso_camera.h"
#include "exts_device_lights.h"
#include "exts_lso_scanner.h"
#include "CaptureSequence.h"
#include "Hardware.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace wso_system;
using namespace std;



void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraLive(ColorCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraFrameCaptured(clbFrame);
			p->startLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSingleFrameLive(ColorCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraFrameCaptured(clbFrame);
			p->startSingleFrameLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSeqROILive(ColorCameraSeqROIFrameCaptured clbFrame, const LsoCaptureFrameSeqROIPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraSeqROIFrameCaptured(clbFrame);
			p->setCaptureSequencerPreset(*preset);
			p->startFrameSeqROILiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSwTriggerOffsetROILive(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraOffsetROIFrameCaptured(clbFrame);
			p->setCaptureOffsetRoiPreset(*preset);
			p->startFrameOffsetROILiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSwTriggerOffsetROICapture(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraOffsetROIFrameCaptured(clbFrame);
			p->setCaptureOffsetRoiPreset(*preset);
			p->startFrmaeOffsetROICaptureMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraRollSwTrigOverlapLive(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraRollSwTrigOverlapFrameCaptured(clbFrame);
			p->setRollSwTrigOverlapPreset(*preset);
			p->startFrameRollSwTrigOverlabLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraRollSwTrigOverlapCapture(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraRollSwTrigOverlapFrameCaptured(clbFrame);
			p->setRollSwTrigOverlapPreset(*preset);
			p->startFrameRollSwTrigOverlabCaptureMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSwTriggerLive(ColorCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraSwTriggerFrameCaptured(clbFrame);
			p->startSwTriggerLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::stopColorCameraSwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->stopSwTriggerLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::shootColorCameraSwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->shootSwTrigger();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraHwTriggerLive(ColorCameraImageCaptured clbImage)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraImageCaptured(clbImage);
			p->startHwTriggerLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::stopColorCameraHwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->stopHwTriggerLiveMode();
		}
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isColorCameraIsStreaming(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->isCameraStreaming();
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::setupColorCameraHwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->setupHwTriggerSetting();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraOriginal(ColorCameraImageCaptured clbImage)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraImageCaptured(clbImage);
			p->startOriginalMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::pauseColorCameraOriginal(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->pauseOriginalMode();
			inst->disconnectColorCameraImageCaptured();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::pauseColorCameraLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->pauseLiveMode();
			inst->disconnectColorCameraFrameCaptured();
			inst->disconnectColorCameraImageCaptured();
		}
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isColorCameraLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			auto result = p->isLiveMode();
			return result;
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::getColorCameraSettingParam(ColorCameraSettingParam* param)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			if (param) {
				auto sets = p->getCameraSettings();
				memcpy(param, &sets, sizeof(ColorCameraSettingParam));
			}
			return;
		}
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setColorCameraSettingParam(const ColorCameraSettingParam* param)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			if (param == nullptr) {
				return false;
			}

			// Session guard: structural fields are camera-locked while streaming/capturing.
			// Apply tunables only (live tuning keeps working); report false when structural
			// fields were requested so the caller knows they were not applied.
			if (p->isLiveMode() || p->isCaptureMode()) {
				const auto& cur = p->getCameraSettings();
				bool structuralChanged =
					cur.pixelFormat != param->pixelFormat ||
					cur.binningHorizontal != param->binningHorizontal ||
					cur.binningVertical != param->binningVertical ||
					cur.roi_x_width != param->roi_x_width ||
					cur.roi_y_height != param->roi_y_height ||
					cur.roi_x_offset != param->roi_x_offset ||
					cur.roi_y_offset != param->roi_y_offset ||
					cur.acqusitionMode != param->acqusitionMode ||
					cur.acqusitionFrameCount != param->acqusitionFrameCount ||
					cur.adcDepthIndex != param->adcDepthIndex;

				try {
					p->setExposureTime(param->exposure_time);
					p->setGain(param->gain);
				}
				catch (...) {
					return false;
				}
				return !structuralChanged;
			}

			p->setCameraSettings(*param);
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setColorCameraExtendedParam(const ColorCameraExtendedParam* param, ColorCameraApplyResult* result)
{
	if (param == nullptr) {
		return false;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->applyExtendedSettings(*param, result);
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setColorCameraFullSettingParam(const ColorCameraSettingParam* base, const ColorCameraExtendedParam* ext, ColorCameraApplyResult* result)
{
	if (base == nullptr || ext == nullptr) {
		return false;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			// Structural verification and the extended fields are separate verdicts inside
			// the device layer; this entry point is the tuning Apply, which only succeeds
			// when BOTH held - the caller saves the preset on that answer.
			bool baseVerified = false;
			const bool extendedOk = p->applyFullSettings(*base, *ext, result, &baseVerified);
			return extendedOk && baseVerified;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getColorCameraExtendedParam(ColorCameraExtendedParam* param)
{
	if (param == nullptr) {
		return false;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->readExtendedSettings(param);
		}
	}
	*param = capture::emptyCameraExtendedParam();
	return false;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getColorCameraEnumEntries(const char* nodeName, ColorCameraEnumValue* buffer, int maxCount)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->enumerateNodeEntries(nodeName, buffer, maxCount);
		}
	}
	return 0;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getColorCameraNodeRange(const char* nodeName, double* minValue, double* maxValue, double* increment, bool* writable)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->queryNodeRange(nodeName, minValue, maxValue, increment, writable);
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getColorCameraNodeValue(const char* nodeName, double* value)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->queryNodeValue(nodeName, value);
		}
	}
	return false;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::validateLsoCameraStartContract(const ColorCameraExtendedParam* param, bool hwTriggered)
{
	return capture::checkCameraStartContract(param, hwTriggered);
}

int WSOSYSTEM_DLL_API __stdcall wso_system::describeLsoCameraStartContract(int contract, char* buf, int len)
{
	const char* text = capture::describeCameraStartContract(contract);
	const int n = (int)std::strlen(text);
	if (buf == nullptr || len <= 0) {
		return n;
	}
	const int copied = (n > len - 1) ? (len - 1) : n;
	std::memcpy(buf, text, copied);
	buf[copied] = '\0';
	return copied;
}

unsigned int WSOSYSTEM_DLL_API __stdcall wso_system::getLsoCameraStructSize(int which)
{
	switch (which) {
	case LSO_STRUCT_CAMERA_SETTING:         return (unsigned int)sizeof(ColorCameraSettingParam);
	case LSO_STRUCT_CAMERA_EXTENDED:        return (unsigned int)sizeof(ColorCameraExtendedParam);
	case LSO_STRUCT_CAMERA_LINE:            return (unsigned int)sizeof(ColorCameraLineConfig);
	case LSO_STRUCT_CAMERA_FIELD_RESULT:    return (unsigned int)sizeof(ColorCameraFieldResult);
	case LSO_STRUCT_CAMERA_APPLY_RESULT:    return (unsigned int)sizeof(ColorCameraApplyResult);
	case LSO_STRUCT_FUNDUS_CAPTURE_REQUEST: return (unsigned int)sizeof(LsoFundusCaptureRequest);
	case LSO_STRUCT_FUNDUS_PREVIEW_REQUEST: return (unsigned int)sizeof(LsoFundusPreviewRequest);
	default:                                return 0;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::squareColorCameraRoi(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->squareRoi();
		}
	}
	return false;
}

// --- One-call mono square preview: shared orchestration for Toolkit and Agent ---
namespace {
	ColorCameraSettingParam s_savedMonoParam;
	bool s_hasSavedMono = false;
	// Which field groups the install replaced. The restore path puts back exactly those and
	// leaves the rest at whatever the camera currently holds, so tuning done during a
	// preview that did not own that field still survives the stop. What each preview
	// replaces is decided by capture::planPreviewParams / planMonoPreviewParams, which is
	// device-free and checked by program/TestCaptureSequence.
	capture::PreviewParamPlan s_monoPreviewPlan;

	// The extended (A-1) node state as it was before the preview installed its own, and
	// whether it could be read at all. The preview now owns trigger/GPIO nodes that outlive
	// one session, so the stop path puts back the values it found instead of leaving the
	// preview's wiring on the camera for whatever runs next.
	ColorCameraExtendedParam s_savedMonoExt;
	bool s_hasSavedMonoExt = false;

	// Snapshots the current camera settings for restore and installs `plan`.
	// `ext` is the A-1 configuration to install alongside the geometry; null means this
	// caller states none (the legacy free-running mono preview), and then nothing outside
	// the 14 fields is touched - exactly the behaviour that path always had.
	// Returns false when either the structural fields or a stated extended field did not
	// take. Preview parity must not arm a camera after logging that it holds a different
	// pixel format, binning, ROI, acquisition mode or ADC depth.
	bool applyPreviewParams(wso_device::ColorCamera* cam,
		const ColorCameraSettingParam& current, const capture::PreviewParamPlan& plan,
		const ColorCameraExtendedParam* ext, ColorCameraApplyResult* result = nullptr)
	{
		s_savedMonoParam = current;
		s_hasSavedMono = true;
		s_monoPreviewPlan = plan;

		bool ok = true;
		if (ext != nullptr && ext->structSize == (unsigned int)sizeof(ColorCameraExtendedParam)) {
			// Snapshot BEFORE the first write, so the ledger restores what was there. The
			// GPIO rows are only walked when this preview will actually change one: reading
			// a row means writing LineSelector, and that is device state to spend only when
			// there is something to put back.
			s_hasSavedMonoExt = cam->readExtendedSettings(&s_savedMonoExt,
				capture::statesAnyCameraLine(ext));
			bool baseVerified = false;
			ok = cam->applyFullSettings(plan.params, *ext, result, &baseVerified);
			if (!baseVerified) {
				WsoLogWarn("[FUNDUS-PRV] structural camera settings differ from the preset - see the [SET] log");
			}
			ok = ok && baseVerified;
		}
		else {
			cam->setCameraSettings(plan.params);
		}

		if (plan.squareRoi) {
			// Only a geometry this code derived (full FOV) is cropped to a centered square.
			// A preset that states its own window keeps it.
			cam->squareRoi();
		}
		return ok;
	}

	// Legacy mono aiming geometry (no preset): mode 0 = Mono8 + 2x2 binning, 1 = BayerRG8 +
	// 1x1, both full FOV cropped to a centered square, exposure/gain left alone.
	void installMonoPreviewParams(wso_device::ColorCamera* cam, int mode,
		unsigned int exposureTimeUs, float gain)
	{
		const ColorCameraSettingParam current = cam->getCameraSettings();
		applyPreviewParams(cam, current,
			capture::planMonoPreviewParams(current, mode, exposureTimeUs, gain), nullptr);
	}

	// Fundus preview: the request's camera preset owns pixel format, binning, ROI,
	// acquisition mode and ADC depth; the validated exposureTimeUs / gain own the tunables;
	// and `resolvedExt` owns the trigger, GPIO and sensor nodes the 14 fields cannot carry.
	// The preview is hardware-triggered by contract, so `resolvedExt` is the request's block
	// with TriggerMode already filled in for that drive mode when the preset owns the group.
	bool installPreviewParams(wso_device::ColorCamera* cam, const LsoFundusPreviewRequest& req,
		const ColorCameraExtendedParam& resolvedExt, ColorCameraApplyResult* result)
	{
		const ColorCameraSettingParam current = cam->getCameraSettings();
		return applyPreviewParams(cam, current, capture::planPreviewParams(current, req), &resolvedExt, result);
	}

	// Minimal restore: exactly the groups the install replaced.
	void restoreMonoPreviewParams(void)
	{
		if (!s_hasSavedMono && !s_hasSavedMonoExt) {
			return;
		}
		if (auto* inst = Hardware::getInstance(); inst) {
			if (auto* cam = inst->getMainBoard()->getColorCamera(); cam) {
				if (s_hasSavedMono) {
					const ColorCameraSettingParam live = cam->getCameraSettings();
					const ColorCameraSettingParam restore =
						capture::mergePreviewRestore(live, s_savedMonoParam, s_monoPreviewPlan);
					cam->setCameraSettings(restore);
				}
				if (s_hasSavedMonoExt) {
					// Only nodes that could actually be READ come back in the snapshot, so
					// this writes back values the device really had - it never invents one.
					cam->applyExtendedSettings(s_savedMonoExt, nullptr);
				}
			}
		}
		s_hasSavedMono = false;
		s_hasSavedMonoExt = false;
		s_monoPreviewPlan = capture::PreviewParamPlan{};
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraMonoPreview(int mode, ColorCameraFrameCaptured cb)
{
	try {
		auto* inst = Hardware::getInstance();
		if (!inst) { return false; }
		auto* mb = inst->getMainBoard();
		if (!mb) { return false; }
		auto* cam = mb->getColorCamera();
		if (!cam) { return false; }

		// Re-entry guard: if a live or capture is already running, do not snapshot
		// preview params as "saved" (and do not fight the running session).
		if (cam->isLiveMode() || cam->isCaptureMode()) { return false; }

		// 1-3) snapshot for restore, install the mono aiming geometry, centered-square ROI.
		//      Exposure/gain are left alone here: this export's contract is that live tuning
		//      survives, and it drives the camera free-running rather than from the board.
		installMonoPreviewParams(cam, mode, 0, 0.0f);

		// 4) IR aiming illumination (reuse device-lights export)
		turnOnLight(LightType::RETINA_IR_LED);

		// 5) LSO scanner IR pattern grabbing (reuse scanner export; IR pattern id = 1)
		startLsoScannerGrabbing(1, 1);

		// 6) start color-camera live with the caller's frame callback
		inst->connectColorCameraFrameCaptured(cb);
		cam->startLiveMode();
		return true;
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopColorCameraMonoPreview(void)
{
	try {
		// reverse order: live pause (+ callback disconnect) -> scanner pause -> IR off -> restore params
		pauseColorCameraLive();
		pauseLsoScannerGrabbing(1);
		turnOffLight(LightType::RETINA_IR_LED);

		restoreMonoPreviewParams();
		return true;
	}
	catch (...) {
		return false;
	}
}

// --- One-call color capture: shared orchestration for Toolkit and Agent ---
namespace {
	std::atomic<bool> s_capturing{ false };
	std::mutex s_captureErrorLock;
	std::string s_lastCaptureError;

	// Serializes every start/stop across both capture exports. Without it two concurrent
	// starts can both pass the busy check and assign a joinable std::thread (terminate),
	// and a start racing a stop can join the same worker twice.
	// The worker thread itself never takes this lock, so joining under it cannot deadlock.
	std::mutex s_captureStateLock;

	// Joined on the next start/stop and at teardown, so the thread object never
	// destructs joinable. Consumers must stop a running capture before shutdown.
	struct CaptureWorker {
		std::thread t;
		~CaptureWorker() {
			if (t.joinable()) {
				t.join();
			}
		}
	};
	CaptureWorker s_captureWorker;

	void setLastCaptureError(const std::string& msg)
	{
		std::lock_guard<std::mutex> lock(s_captureErrorLock);
		s_lastCaptureError = msg;
	}

	bool joinCaptureWorker()
	{
		if (s_captureWorker.t.joinable()) {
			if (s_captureWorker.t.get_id() == std::this_thread::get_id()) {
				return false;
			}
			s_captureWorker.t.join();
		}
		return true;
	}

	// Callers must hold s_captureStateLock.
	// Legacy core: the established semantics of the four pre-existing capture exports -
	// Normal takes its frame count from the device readback, HW trigger from the requested
	// count, illumination is the fixed IR-off/white-on pair and the board profile is left
	// alone. Frozen on purpose: those exports are also consumed by the Agent, so their
	// defaults live here and are NOT exposed as knobs on the fundus request.
	int startColorCaptureLegacyCore(const ColorCameraSettingParam* param, int scanPatternId, bool hwTrigger, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

	// Fundus one-call session state. Declared here (not next to the core at the end of the
	// file) because stopColorCameraColorCapture below must be able to signal cancellation.
	capture::SequenceState s_fundusState;
	capture::DiagnosticGate s_fundusGate;
	bool s_fundusPreviewActive = false; // Also true while a failed preview cleanup is pending.
}

int WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraColorCapture(const ColorCameraSettingParam* param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
{
	std::lock_guard<std::mutex> lock(s_captureStateLock);
	return startColorCaptureLegacyCore(param, scanPatternId, false, cb, onCompleted);
}

int WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraColorCaptureEx(const ColorCameraSettingParam* param, int scanPatternId, ColorCaptureTriggerMode triggerMode, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
{
	if (triggerMode != COLOR_CAPTURE_NORMAL && triggerMode != COLOR_CAPTURE_HW_TRIGGER) {
		setLastCaptureError("invalid trigger mode");
		return CAP_STAGE_PARAM;
	}
	std::lock_guard<std::mutex> lock(s_captureStateLock);
	return startColorCaptureLegacyCore(param, scanPatternId, triggerMode == COLOR_CAPTURE_HW_TRIGGER, cb, onCompleted);
}

// Shared device sequence for both capture exports (per-frame and composed).
namespace {
int startColorCaptureLegacyCore(const ColorCameraSettingParam* param, int scanPatternId, bool hwTrigger, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
{
	const unsigned char fundusMode = 0; // Legacy color-only API has no mode request.
	bool scannerStarted = false;
	bool callbackConnected = false;
	bool cameraArmed = false;
	bool workerStarted = false;
	wso_device::ColorCamera* cam = nullptr;

	try {
		if (param == nullptr) { setLastCaptureError("capture param is null"); return CAP_STAGE_PARAM; }
		auto* inst = Hardware::getInstance();
		if (!inst) { setLastCaptureError("hardware not ready"); return CAP_STAGE_FAULT; }
		auto* mb = inst->getMainBoard();
		if (!mb) { setLastCaptureError("main board not ready"); return CAP_STAGE_FAULT; }
		cam = mb->getColorCamera();
		if (!cam) { setLastCaptureError("color camera not ready"); return CAP_STAGE_FAULT; }

		// Re-entry guard: one color-camera session at a time (live, aiming or capture).
		if (cam->isLiveMode() || cam->isCaptureMode() || s_capturing || s_fundusPreviewActive) {
			setLastCaptureError("busy: a live/capture session is active");
			return CAP_STAGE_BUSY;
		}

		if (!joinCaptureWorker()) {
			setLastCaptureError("capture callback re-entry is not allowed");
			return CAP_STAGE_BUSY;
		}

		// 0) arm the session: clears cancellation here, on the starting thread, under the
		//    caller's capture-state lock. beginCaptureFrames used to clear it on the worker,
		//    which lost a stop issued right after an accepted start.
		cam->armCaptureSession();

		// 1) aiming IR off. (White LED stays scan-gated / manual - values are not owned here.)
		turnOffLight(LightType::RETINA_IR_LED);
		turnOnLight(LightType::LSO_WHITE_LED);

		// Verify the settings that will actually run. HW trigger always uses a
		// Continuous camera stream; its requested count remains the callback bound.
		ColorCameraSettingParam appliedParam = *param;
		if (hwTrigger) {
			appliedParam.acqusitionMode = 0;
		}
		if (!cam->setCameraSettingsChecked(appliedParam)) {
			setLastCaptureError("param apply/verify failed - see [COLOR-CAP]/[SET] log for req vs actual");
			return CAP_STAGE_PARAM;
		}

		const unsigned int totalFrameCount = hwTrigger
			? ((param->acqusitionMode == 1) ? 1U : param->acqusitionFrameCount)
			: ((cam->getAcquisitionMode() == 1) ? 1U : cam->getAcquisitionFrameCount());
		if (totalFrameCount == 0) {
			setLastCaptureError("capture frame count is zero");
			return CAP_STAGE_PARAM;
		}

		inst->connectColorCameraImageCaptured(cb);
		callbackConnected = true;
		setLastCaptureError("");
		s_capturing = true;

		auto failStart = [&](int stage, const std::string& message) {
			setLastCaptureError(message);
			if (cameraArmed) { cam->abortCaptureFrames(); }
			if (scannerStarted) { pauseLsoScannerGrabbing(scanPatternId); }
			if (callbackConnected) { inst->disconnectColorCameraImageCaptured(); }
			s_capturing = false;
			return stage;
		};

		// Keep Normal's established scanner-first free-run sequence. HW setup and its
		// bounded readiness wait run on the worker so the UI caller is never blocked.
		if (!hwTrigger) {
			if (!startLsoScannerGrabbing(scanPatternId, fundusMode)) {
				return failStart(CAP_STAGE_SCANNER,
					"scanner grabbing start failed (patternId=" + std::to_string(scanPatternId) + ")");
			}
			scannerStarted = true;
			if (!cam->beginCaptureFrames(false)) {
				// A live/aiming session that won the race against the guard above is BUSY,
				// not a device fault - keep the stage the consumer sees accurate.
				const bool busy = cam->isLiveMode() || cam->isCaptureMode();
				return failStart(busy ? CAP_STAGE_BUSY : CAP_STAGE_CAMERA,
					busy ? "color camera busy (live/aiming or capture in progress)"
					     : "camera acquisition start failed");
			}
			cameraArmed = true;
		}

		s_captureWorker.t = std::thread([cam, scanPatternId, totalFrameCount, hwTrigger, onCompleted, fundusMode] {
			int frames = 0;
			int result = CAP_STAGE_OK;
			bool workerScannerStarted = !hwTrigger;

			// ponytail: single knob. The HW spec quotes 200ms, but that budget is unverified on
			// this instrument. Widened while the arm time is being measured - waitUntilTriggerArmed
			// logs the elapsed ms on success, so tighten this back once a real number exists.
			constexpr unsigned int kArmTimeoutMs = 2000;

			if (hwTrigger) {
				if (!cam->beginCaptureFrames(true)) {
					// Completion codes are OK/CAMERA/CANCELLED/FAULT by contract, so a
					// live-mode collision is reported as CAMERA with an accurate message.
					setLastCaptureError(cam->isLiveMode()
						? "color camera busy (live/aiming in progress)"
						: "camera acquisition start failed");
					result = CAP_STAGE_CAMERA;
				}
				else if (!cam->waitUntilTriggerArmed(kArmTimeoutMs)) {
					// Arming can fail either by timeout or because the status node is unusable;
					// the previous text claimed a timeout for both. The log names which.
					setLastCaptureError("camera trigger arming failed (FrameTriggerWait, budget "
						+ std::to_string(kArmTimeoutMs) + "ms) - see [COLOR-CAP] log");
					cam->abortCaptureFrames();
					result = CAP_STAGE_CAMERA;
				}
				else if (!startLsoScannerGrabbing(scanPatternId, fundusMode)) {
					setLastCaptureError("scanner grabbing start failed (patternId=" + std::to_string(scanPatternId) + ")");
					cam->abortCaptureFrames();
					result = CAP_STAGE_SCANNER;
				}
				else {
					workerScannerStarted = true;
				}
			}

			if (result == CAP_STAGE_OK) {
				result = cam->acquireCaptureFrames(&frames, totalFrameCount, hwTrigger);
			}

			if (workerScannerStarted) {
				pauseLsoScannerGrabbing(scanPatternId);
			}
			if (auto* inst2 = Hardware::getInstance(); inst2) {
				inst2->disconnectColorCameraImageCaptured();
			}

			if (onCompleted) {
				onCompleted(result, frames);
			}
			s_capturing = false;
		});
		workerStarted = true;

		return CAP_STAGE_OK;
	}
	catch (...) {
		s_capturing = false;

		// The worker owns the teardown once it runs; if it never started (e.g. thread
		// creation failed) the device would keep scanning with the callback attached.
		if (!workerStarted) {
			try {
				if (cameraArmed) {
					cam->abortCaptureFrames();
				}
				if (callbackConnected) {
					if (auto* inst = Hardware::getInstance(); inst) {
						inst->disconnectColorCameraImageCaptured();
					}
				}
				if (scannerStarted) {
					pauseLsoScannerGrabbing(scanPatternId);
				}
			}
			catch (...) {
			}
		}

		setLastCaptureError("unexpected exception in startColorCameraColorCapture");
		return CAP_STAGE_FAULT;
	}
}
} // namespace

// --- Native subframe composition: one image per capture for consumers that must not stitch ---
namespace {
	// Single capture session at a time (process singleton), so file-static state is safe here.
	ColorCameraImageCaptured s_composedConsumerFrameCb = nullptr;
	ColorCaptureCompleted s_composedConsumerCompletedCb = nullptr;
	std::vector<unsigned char> s_composedBuffer;
	int s_composedWidth = 0;
	int s_composedHeight = 0;
	int s_composedBpp = 0;
	int s_composedPixelFormat = 0;
	int s_composedFlipMode = 0;
	int s_composedBands = 0;			// requested band count (0 = derive from the frame count)
	int s_composedBandsResolved = 0;	// band count actually used by the running capture
	int s_composedFilled = 0;
	bool s_composedResetPending = false;

	// Runs on the native acquisition thread: writes this subframe's band into the composite.
	void __stdcall composedFrameThunk(unsigned char* bits, int32_t width, int32_t height, int32_t frameCount, int32_t totalFrameCount, int32_t flipMode, int32_t pixelFormat, int32_t bytesPerPixel)
	{
		if (bits == nullptr || width <= 0 || height <= 0 || bytesPerPixel <= 0 || frameCount < 0) {
			return;
		}

		const int bands = (s_composedBands > 0)
			? s_composedBands
			: ((totalFrameCount > 0) ? (int)totalFrameCount : 1);
		s_composedBandsResolved = bands;
		if (frameCount >= bands) {
			return; // extra acquisition frames beyond the band count are not composed
		}

		const size_t rowBytes = (size_t)width * (size_t)bytesPerPixel;
		const size_t frameBytes = rowBytes * (size_t)height;

		// Clear once per capture, not only when the geometry changes: a reused buffer of the
		// same size would otherwise still hold the previous capture's rows.
		if (s_composedBuffer.size() != frameBytes) {
			s_composedBuffer.assign(frameBytes, 0);
			s_composedResetPending = false;
		}
		else if (s_composedResetPending) {
			std::fill(s_composedBuffer.begin(), s_composedBuffer.end(), (unsigned char)0);
			s_composedResetPending = false;
		}

		s_composedWidth = width;
		s_composedHeight = height;
		s_composedBpp = bytesPerPixel;
		s_composedPixelFormat = pixelFormat;
		s_composedFlipMode = flipMode;

		const int segH = height / bands;
		if (segH <= 0) {
			return;
		}

		const int y0 = frameCount * segH;
		if (y0 >= height) {
			return;
		}

		// Last band takes the remaining rows so the composite covers the full frame height.
		int bandH = (frameCount == bands - 1) ? (height - y0) : segH;
		if (bandH > height - y0) {
			bandH = height - y0;
		}

		std::memcpy(s_composedBuffer.data() + (size_t)y0 * rowBytes,
			bits + (size_t)y0 * rowBytes,
			(size_t)bandH * rowBytes);
		++s_composedFilled;
	}

	// Runs on the native worker thread: emits the composite once, then forwards completion.
	void __stdcall composedCompletedThunk(int32_t result, int32_t framesAcquired)
	{
		ColorCameraImageCaptured frameCb = s_composedConsumerFrameCb;
		ColorCaptureCompleted completedCb = s_composedConsumerCompletedCb;
		s_composedConsumerFrameCb = nullptr;
		s_composedConsumerCompletedCb = nullptr;

		// Only a complete composite is emitted. A cancelled or camera-aborted capture leaves
		// unwritten bands, and emitting it would hand the consumer an image whose remaining
		// rows are the previous capture's pixels - indistinguishable from a fresh one.
		const bool isComplete = s_composedBandsResolved > 0
			&& s_composedFilled == s_composedBandsResolved
			&& !s_composedBuffer.empty();

		WsoLogInfo("[COLOR-CAP] composed bands=" + std::to_string(s_composedFilled)
			+ "/" + std::to_string(s_composedBandsResolved)
			+ " size=" + std::to_string(s_composedWidth) + "x" + std::to_string(s_composedHeight)
			+ (isComplete ? " emit=1" : " emit=0"));

		if (frameCb && isComplete) {
			frameCb(s_composedBuffer.data(), s_composedWidth, s_composedHeight, 0, 1,
				s_composedFlipMode, s_composedPixelFormat, s_composedBpp);
		}

		if (completedCb) {
			completedCb(result, framesAcquired);
		}
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraColorCaptureComposed(const ColorCameraSettingParam* param, int scanPatternId, int subFrameCount, ColorCameraImageCaptured cbComposed, ColorCaptureCompleted onCompleted)
{
	return startColorCameraColorCaptureComposedEx(param, scanPatternId, subFrameCount, COLOR_CAPTURE_NORMAL, cbComposed, onCompleted);
}

int WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraColorCaptureComposedEx(const ColorCameraSettingParam* param, int scanPatternId, int subFrameCount, ColorCaptureTriggerMode triggerMode, ColorCameraImageCaptured cbComposed, ColorCaptureCompleted onCompleted)
{
	if (triggerMode != COLOR_CAPTURE_NORMAL && triggerMode != COLOR_CAPTURE_HW_TRIGGER) {
		setLastCaptureError("invalid trigger mode");
		return CAP_STAGE_PARAM;
	}
	std::lock_guard<std::mutex> lock(s_captureStateLock);

	// Reject a re-entrant start BEFORE touching the composition state: the callbacks stored
	// here belong to a capture that may still be running, and clearing them on the failure
	// path would swallow that capture's completion notice for good.
	if (s_capturing || s_composedConsumerCompletedCb != nullptr) {
		setLastCaptureError("busy: a capture session is active");
		return CAP_STAGE_BUSY;
	}

	s_composedConsumerFrameCb = cbComposed;
	s_composedConsumerCompletedCb = onCompleted;
	s_composedBands = (subFrameCount > 0) ? subFrameCount : 0;
	s_composedBandsResolved = 0;
	s_composedFilled = 0;
	s_composedWidth = 0;
	s_composedHeight = 0;
	s_composedResetPending = true;

	const int stage = startColorCaptureLegacyCore(param, scanPatternId, triggerMode == COLOR_CAPTURE_HW_TRIGGER, composedFrameThunk, composedCompletedThunk);
	if (stage != CAP_STAGE_OK) {
		// No completion callback follows a failed start, so release the consumer callbacks here.
		s_composedConsumerFrameCb = nullptr;
		s_composedConsumerCompletedCb = nullptr;
	}

	return stage;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopColorCameraColorCapture(void)
{
	try {
		if (auto* inst = Hardware::getInstance(); inst) {
			if (auto* cam = inst->getMainBoard()->getColorCamera(); cam) {
				cam->cancelCaptureFrames();
			}
		}

		// The fundus sequence keeps its own session flag, because the camera flag is only
		// polled once the acquisition loop runs - a stop that arrives before the board is
		// started has to be visible to the steps in front of it too.
		s_fundusState.requestCancel();

		// Contract: joins the worker here. Never call stop from the frame/completed callback.
		std::lock_guard<std::mutex> lock(s_captureStateLock);
		// A synchronous start can arm/reset cancellation after the early request above
		// while owning this lock. Repeat it after that start has finished arming.
		s_fundusState.requestCancel();
		if (auto* inst = Hardware::getInstance(); inst) {
			if (auto* cam = inst->getMainBoard()->getColorCamera(); cam) {
				cam->cancelCaptureFrames();
			}
		}
		return joinCaptureWorker();
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isColorCameraCapturing(void)
{
	return s_capturing;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getLastColorCaptureError(char* buf, int len)
{
	std::lock_guard<std::mutex> lock(s_captureErrorLock);
	if (buf == nullptr || len <= 0) {
		return (int)s_lastCaptureError.size();
	}
	int n = (int)s_lastCaptureError.size();
	if (n > len - 1) {
		n = len - 1;
	}
	std::memcpy(buf, s_lastCaptureError.data(), n);
	buf[n] = '\0';
	return n;
}

// --- Fundus one-call: request-owned device sequence ------------------------------------
// Differences from the legacy core above, all of them ownership moves rather than new
// hardware logic: the scan profile is uploaded on every run (startGrabbing only sends
// capture-on, so the board would otherwise scan whatever the last Apply left), the frame
// count comes from that profile instead of the camera, illumination is named by the request
// and restored on every exit path, and cancellation is armed once by the accepted start.
namespace {
	// Intensity 0 means "leave what the board has": LightLed reports its intensity from an
	// in-process cache that starts at 0 (LED_*_INIT_VALUE = 0 in WsoDevice2.h) and is only
	// filled once someone writes it, so writing a read-back 0 would darken a working LED.
	// Board calibration intensity for a fundus light (CalBlk2.LED_Info - the same table the
	// DevKit sends before every preview/capture). 0 when the light has no entry or the
	// calibration block is not loaded.
	unsigned short calibratedFundusLightIntensity(LightType type)
	{
		auto* inst = Hardware::getInstance();
		if (!inst || !inst->getMainBoard() || !inst->getMainBoard()->getHbsDataProfile()) { return 0; }
		const auto* calib = inst->getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource();
		if (!calib) { return 0; }
		switch (type) {
		case LightType::LSO_WHITE_LED:  return calib->LED_Info.WLED_intensity;
		case LightType::RETINA_IR_LED:  return calib->LED_Info.RetIR_intensity;
		case LightType::LSO_BLUE_LED:   return calib->LED_Info.Bled_intensity;
		case LightType::LSO_GREEN_LED:  return calib->LED_Info.Gled_intensity;
		default:             return 0;
		}
	}

	bool fundusSetLightMode(void*, int light, int mode)
	{
		return setLightMode((LightType)light, (unsigned short)mode);
	}

	bool fundusSetLightIntensity(void*, int light, unsigned short intensity)
	{
		return setLightIntensity((LightType)light, intensity);
	}

	bool fundusTurnLightOn(void*, int light)
	{
		return turnOnLight((LightType)light);
	}

	bool fundusTurnLightOff(void*, int light)
	{
		return turnOffLight((LightType)light);
	}

	const capture::FundusLightOps& fundusLightOps(void)
	{
		static const capture::FundusLightOps ops{
			&fundusSetLightMode, &fundusSetLightIntensity, &fundusTurnLightOn, &fundusTurnLightOff
		};
		return ops;
	}

	bool applyFundusLight(int light, int mode, unsigned short intensity, bool intensityOnly = false)
	{
		const LightType type = (LightType)light;
		const unsigned short calibrated = calibratedFundusLightIntensity(type);
		const unsigned short applied = capture::resolveFundusLightIntensity(intensity, calibrated);
		// Firmware 10-16 selects and lights the capture LED from fundus_mode.
		const bool ok = intensityOnly
			? (applied == 0 || setLightIntensity(type, applied))
			: capture::runFundusLightApply(fundusLightOps(), nullptr, light, mode, applied);
		WsoLogInfo("[FUNDUS-LIGHT] type=" + std::to_string(light)
			+ " mode=" + std::to_string(mode)
			+ " requested=" + std::to_string(intensity)
			+ " calibrated=" + std::to_string(calibrated)
			+ " applied=" + std::to_string(applied) + (applied == 0 ? " (board value kept)" : "")
			+ " result=" + std::to_string(ok));
		return ok;
	}

	// Restore path: the working light always goes OFF, then the restore light comes up at
	// its own intensity - unless it is the same light, which then means "leave it off".
	// That way one request field expresses both "hand back to aiming IR" (capture) and
	// "leave the illumination off" (a preview whose restore light is its own light).
	// Runs on success, cancel and every failure path.
	bool restoreFundusLight(int workingLight, int restoreLight, int restoreMode,
		unsigned short restoreIntensity)
	{
		try {
			const unsigned short calibrated = calibratedFundusLightIntensity((LightType)restoreLight);
			const unsigned short applied = capture::resolveFundusLightIntensity(
				restoreIntensity, calibrated);
			return capture::runFundusLightRestore(fundusLightOps(), nullptr,
				workingLight, restoreLight, restoreMode, applied);
		}
		catch (...) {
			return false;
		}
	}

	bool restoreFundusLight(const LsoFundusCaptureRequest& request, bool captureFailed = false)
	{
		// Scanner OFF already returns firmware to PreviewIR. No capture lamp was
		// switched on by the host; restore only the aiming LED's saved intensity.
		// On failure the scanner command may not have reached the board: retain a
		// safe OFF rather than leave the firmware-selected capture lamp lit.
		if (captureFailed && !turnOffLight((LightType)request.captureLight)) {
			return false;
		}
		const unsigned short applied = capture::resolveFundusLightIntensity(
			request.restoreLightIntensity, calibratedFundusLightIntensity((LightType)request.restoreLight));
		return applied == 0 || setLightIntensity((LightType)request.restoreLight, applied);
	}

	// Callers must hold s_captureStateLock.
	int startFundusCaptureCore(const LsoFundusCaptureRequest* request, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
	{
		// 1) Request-only validation, ahead of every device lookup. Keeping this first is
		//    what lets the gates be verified with no instrument attached.
		CaptureValidationReason reason = CAP_REQ_VALID;
		unsigned int totalFrameCount = 0;
		if (!capture::validateFundusCaptureRequest(request, &totalFrameCount, &reason)) {
			setLastCaptureError(std::string("fundus capture request rejected: ")
				+ capture::describeValidationReason(reason));
			return CAP_STAGE_PARAM;
		}

		const LsoFundusCaptureRequest req = *request;
		const bool hwTrigger = (req.triggerMode == COLOR_CAPTURE_HW_TRIGGER);
		wso_device::ColorCamera* cam = nullptr;

		try {
			auto* inst = Hardware::getInstance();
			if (!inst) { setLastCaptureError("hardware not ready"); return CAP_STAGE_FAULT; }
			auto* mb = inst->getMainBoard();
			if (!mb) { setLastCaptureError("main board not ready"); return CAP_STAGE_FAULT; }
			cam = mb->getColorCamera();
			if (!cam) { setLastCaptureError("color camera not ready"); return CAP_STAGE_FAULT; }

			// One color-camera session at a time (live, aiming or capture).
			if (cam->isLiveMode() || cam->isCaptureMode() || s_capturing || s_fundusPreviewActive) {
				setLastCaptureError("busy: a live/capture session is active");
				return CAP_STAGE_BUSY;
			}
			if (!joinCaptureWorker()) {
				setLastCaptureError("capture callback re-entry is not allowed");
				return CAP_STAGE_BUSY;
			}
			// The command is board-global, so every accepted start re-sends the active
			// preset's values while the shared session lock is held. There is no getter and
			// therefore no invented rollback to a guessed previous sequence.
			if (!setLsoScanSequence(req.sequenceMode, req.preMoveOffset, req.preMoveDelayUs)) {
				setLastCaptureError("scan sequence apply failed (mode="
					+ std::to_string(req.sequenceMode) + ")");
				return CAP_STAGE_SCANNER;
			}

			// 2) Arm the session. The ONLY place cancellation is cleared, on this thread,
			//    under s_captureStateLock. Barrier arming is deliberately NOT reset here: a
			//    QA run arms the step before it starts the capture.
			s_fundusState.armStart();
			s_fundusGate.clearCancelLatch();
			cam->armCaptureSession();
			setLastCaptureError("");

			// 3) Camera preset. HW trigger streams Continuous with the preset's camera frame
			//    count while the board profile owns completion. Normal derives both camera
			//    values from that profile's total, preserving its existing behavior.
			ColorCameraSettingParam camera =
				capture::resolveCaptureCameraParams(req.camera, totalFrameCount, hwTrigger);

			s_fundusGate.waitAtStep(CAP_STEP_APPLY_CAMERA);
			// Who owns the trigger nodes. When the preset owns the group, TriggerMode is
			// filled from this capture's drive mode BEFORE the apply, and the acquisition
			// start then writes no trigger node at all - which is how a stated value stops
			// being replaced by setupHwTriggerSetting()'s constants a moment later. When the
			// preset owns none of it, this is the request's block unchanged and the legacy
			// setup keeps running exactly as before.
			const ColorCameraExtendedParam cameraExt =
				capture::resolveStartCameraConfig(req.cameraExt, hwTrigger);
			const bool triggerInstalled = capture::isCameraTriggerContractStated(&cameraExt);

			// The whole camera contract in one checked apply: the 14 structural fields plus
			// every A-1 node the preset states, in node-dependency order. Both verdicts are
			// required - a stated trigger or GPIO value that did not take must not be
			// discovered as a capture that never receives a frame.
			bool baseVerified = false;
			const bool extendedOk = cam->applyFullSettings(camera, cameraExt, nullptr, &baseVerified);
			if (!baseVerified) {
				setLastCaptureError("camera preset apply/verify failed - see [COLOR-CAP]/[SET] log for req vs actual");
				return CAP_STAGE_PARAM;
			}
			if (!extendedOk) {
				setLastCaptureError("camera configuration was not applied - a stated A-1 field is "
					"unsupported, locked or read back different (see the [CAM-EXT] log)");
				return CAP_STAGE_PARAM;
			}

			// 4) The camera owns the exposure, so the profile carries what the camera
			//    actually applied instead of a UI value the device may have quantized.
			const LsoScannerControlParam profile = capture::resolveCaptureScannerProfile(
				req.profile, (double)cam->getExposureTime());

			WsoLogInfo("[FUNDUS-CAP] start patternId=" + std::to_string(req.scanPatternId)
				+ " trigger=" + std::to_string(req.triggerMode)
				+ " acqFrameSize=" + std::to_string(profile.acqFrameSize)
				+ " subFrameSize=" + std::to_string(profile.subFrameSize)
				+ " total=" + std::to_string(totalFrameCount)
				+ " camMode=" + std::to_string(camera.acqusitionMode)
				+ " exposureUs=" + std::to_string((int)profile.exposureTimeUs));

			// 5) Profile upload. Must happen before the board is started: startGrabbing()
			//    only sends capture-on for the slot.
			s_fundusGate.waitAtStep(CAP_STEP_UPLOAD_PROFILE);
			if (!submitLsoScannerControlParam(req.scanPatternId, &profile)) {
				setLastCaptureError("scan profile upload failed (patternId="
					+ std::to_string(req.scanPatternId) + ")");
				return CAP_STAGE_SCANNER;
			}

			// 6) Frame callback, then the illumination named by the request (the legacy path
			//    hard-coded white-on and never restored anything). The ledger order is the
			//    acquisition order, and the teardown is its reverse - see
			//    program/TestCaptureSequence, which pins exactly this order.
			inst->connectColorCameraImageCaptured(cb);
			s_fundusState.acquire(capture::SequenceResource::FrameCallback);

			s_fundusGate.waitAtStep(CAP_STEP_CAPTURE_LIGHT);
			// Reserve before the intensity write so rollback restores the aiming intensity.
			s_fundusState.acquire(capture::SequenceResource::CaptureLight);
			if (!applyFundusLight(req.captureLight, req.captureLightMode,
				req.captureLightIntensity, true)) {
				const bool restored = restoreFundusLight(req);
				inst->disconnectColorCameraImageCaptured();
				s_fundusState.releaseAll();
				setLastCaptureError("capture illumination failed (light="
					+ std::to_string(req.captureLight) + ", cleanup="
					+ std::to_string(restored) + ")");
				return CAP_STAGE_FAULT;
			}
			s_capturing = true;

			s_captureWorker.t = std::thread([cam, req, totalFrameCount, hwTrigger, triggerInstalled, onCompleted] {
				int frames = 0;
				int result = CAP_STAGE_OK;

				// ponytail: single knob. The HW spec quotes 200ms; that budget is still
				// unmeasured on this instrument, so it stays wide while
				// waitUntilTriggerArmed logs the elapsed ms on success.
				constexpr unsigned int kArmTimeoutMs = 2000;

				// Observing cancellation: checked before every irreversible step, and
				// latched so a test can prove the stop was seen rather than infer it.
				auto cancelled = [](const char* where) {
					if (!s_fundusState.isCancelled()) {
						return false;
					}
					s_fundusGate.signalCancelLatched();
					setLastCaptureError(std::string("capture cancelled at ") + where);
					return true;
				};

				if (cancelled("session start")) {
					result = CAP_STAGE_CANCELLED;
				}

				if (result == CAP_STAGE_OK) {
					s_fundusGate.waitAtStep(CAP_STEP_BEGIN_CAMERA);
					if (cancelled("camera begin")) {
						result = CAP_STAGE_CANCELLED;
					}
					// The trigger configuration the preset stated was installed and verified
					// by the apply above, so beginning the acquisition must not write the
					// hardcoded setup over it.
					else if (!cam->beginCaptureFrames(hwTrigger, triggerInstalled)) {
						setLastCaptureError(cam->isLiveMode()
							? "color camera busy (live/aiming in progress)"
							: "camera acquisition start failed");
						result = CAP_STAGE_CAMERA;
					}
					else {
						s_fundusState.acquire(capture::SequenceResource::CameraAcquisition);
					}
				}

				if (result == CAP_STAGE_OK && hwTrigger) {
					s_fundusGate.waitAtStep(CAP_STEP_WAIT_ARM);
					if (cancelled("trigger arming")) {
						result = CAP_STAGE_CANCELLED;
					}
					else if (!cam->waitUntilTriggerArmed(kArmTimeoutMs)) {
						setLastCaptureError("camera trigger arming failed (FrameTriggerWait, budget "
							+ std::to_string(kArmTimeoutMs) + "ms) - see [COLOR-CAP] log");
						result = CAP_STAGE_CAMERA;
					}
				}

				if (result == CAP_STAGE_OK) {
					// Last gate before the board starts scanning and flashing.
					s_fundusGate.waitAtStep(CAP_STEP_START_SCANNER);
					if (cancelled("scanner start")) {
						result = CAP_STAGE_CANCELLED;
					}
					else if (!startLsoScannerGrabbing(req.scanPatternId, req.fundusMode)) {
						setLastCaptureError("scanner grabbing start failed (patternId="
							+ std::to_string(req.scanPatternId) + ")");
						result = CAP_STAGE_SCANNER;
					}
					else {
						s_fundusState.acquire(capture::SequenceResource::ScannerGrabbing);
					}
				}

				if (result == CAP_STAGE_OK) {
					s_fundusGate.waitAtStep(CAP_STEP_ACQUIRE);
					result = cam->acquireCaptureFrames(&frames, totalFrameCount, hwTrigger);
				}

				// Teardown: undo exactly what was acquired, in reverse. A step that never
				// ran has nothing to undo, which is why the ledger drives this instead of a
				// fixed list of "probably running" resources.
				s_fundusGate.waitAtStep(CAP_STEP_TEARDOWN);
				bool teardownOk = true;
				for (const capture::SequenceResource resource : s_fundusState.teardownPlan()) {
					try {
						switch (resource) {
						case capture::SequenceResource::ScannerGrabbing:
							teardownOk = pauseLsoScannerGrabbing(req.scanPatternId) && teardownOk;
							break;
						case capture::SequenceResource::CameraAcquisition:
							cam->abortCaptureFrames();	// idempotent: acquire already aborts
							break;
						case capture::SequenceResource::CaptureLight:
							teardownOk = restoreFundusLight(req,
								result != CAP_STAGE_OK || !teardownOk) && teardownOk;
							break;
						case capture::SequenceResource::FrameCallback:
							if (auto* inst2 = Hardware::getInstance(); inst2) {
								inst2->disconnectColorCameraImageCaptured();
							}
							break;
						}
					}
					catch (...) {
						teardownOk = false;
					}
				}
				s_fundusState.releaseAll();
				if (!teardownOk) {
					setLastCaptureError("fundus capture cleanup failed");
					result = CAP_STAGE_FAULT;
				}

				WsoLogInfo("[FUNDUS-CAP] end result=" + std::to_string(result)
					+ " frames=" + std::to_string(frames)
					+ "/" + std::to_string(totalFrameCount));

				if (onCompleted) {
					onCompleted(result, frames);
				}
				s_capturing = false;
			});

			return CAP_STAGE_OK;
		}
		catch (...) {
			// The worker owns the teardown once it runs; it never started here, so undo
			// whatever the start path had already acquired.
			s_capturing = false;
			for (const capture::SequenceResource resource : s_fundusState.teardownPlan()) {
				try {
					switch (resource) {
					case capture::SequenceResource::ScannerGrabbing:
						pauseLsoScannerGrabbing(req.scanPatternId);
						break;
					case capture::SequenceResource::CameraAcquisition:
						if (cam) { cam->abortCaptureFrames(); }
						break;
					case capture::SequenceResource::CaptureLight:
						restoreFundusLight(req, true);
						break;
					case capture::SequenceResource::FrameCallback:
						if (auto* inst = Hardware::getInstance(); inst) {
							inst->disconnectColorCameraImageCaptured();
						}
						break;
					}
				}
				catch (...) {
				}
			}
			s_fundusState.releaseAll();
			setLastCaptureError("unexpected exception in startLsoFundusCapture");
			return CAP_STAGE_FAULT;
		}
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::startLsoFundusCapture(const LsoFundusCaptureRequest* request, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
{
	std::lock_guard<std::mutex> lock(s_captureStateLock);
	// Validate before the wrapper reads subFrameCompose. On ABI drift even that field may
	// live at another offset, so composition state must not inspect or mutate anything yet.
	CaptureValidationReason reason = CAP_REQ_VALID;
	unsigned int ignoredFrameCount = 0;
	if (!capture::validateFundusCaptureRequest(request, &ignoredFrameCount, &reason)) {
		setLastCaptureError(std::string("fundus capture request rejected: ")
			+ capture::describeValidationReason(reason));
		return CAP_STAGE_PARAM;
	}

	// subFrameCompose routes through the existing native composition, so a consumer that
	// must not stitch gets one image. Validation already pinned the band count to the
	// profile's subFrameSize, so the composite cannot be assembled against a stale count.
	const bool compose = (request != nullptr) && (request->subFrameCompose > 0);
	if (compose) {
		// Reject a re-entrant start BEFORE touching the composition state, or the stored
		// callbacks of a still-running capture would be dropped on the failure path.
		if (s_capturing || s_composedConsumerCompletedCb != nullptr) {
			setLastCaptureError("busy: a capture session is active");
			return CAP_STAGE_BUSY;
		}
		s_composedConsumerFrameCb = cb;
		s_composedConsumerCompletedCb = onCompleted;
		s_composedBands = (int)request->subFrameCompose;
		s_composedBandsResolved = 0;
		s_composedFilled = 0;
		s_composedWidth = 0;
		s_composedHeight = 0;
		s_composedResetPending = true;
	}

	const int stage = compose
		? startFundusCaptureCore(request, composedFrameThunk, composedCompletedThunk)
		: startFundusCaptureCore(request, cb, onCompleted);

	if (compose && stage != CAP_STAGE_OK) {
		s_composedConsumerFrameCb = nullptr;
		s_composedConsumerCompletedCb = nullptr;
	}
	return stage;
}

// --- Fundus one-call preview -----------------------------------------------------------
// Hardware-triggered, like the DevKit preview: the camera is configured (Mono8 + 2x2, the
// preview's own exposure/gain, TriggerMode On / FrameStart / Line3 rising edge / ReadOut
// overlap / trigger delay), the slot profile and the illumination go out, the frame
// callback is connected, the acquisition begins and its arming is verified - and only then
// is the board told to scan. Starting the board first fires triggers at a camera that is
// not waiting for them.
// The step order and the reverse rollback live in capture::runPreviewStart, which
// program/TestCaptureSequence drives with scripted steps, so both are checked with no
// instrument attached.
namespace {
	LsoFundusPreviewRequest s_fundusPreviewRequest;
	capture::SequenceState s_fundusPreviewState;

	// Same budget the capture worker uses. waitUntilTriggerArmed logs the measured ms, so
	// this can be tightened once a real number exists on this instrument.
	constexpr unsigned int kPreviewArmTimeoutMs = 2000;

	struct PreviewContext
	{
		wso_device::ColorCamera* cam = nullptr;
		Hardware* inst = nullptr;
		LsoFundusPreviewRequest req{};
		ColorCameraFrameCaptured cb = nullptr;
		// The block actually applied (TriggerMode resolved for the triggered drive mode) and
		// whether the preset owns the trigger group. When it does, the camera start only
		// arms; when it does not, the legacy hardware trigger setup runs as before.
		ColorCameraExtendedParam resolvedExt{};
		bool triggerInstalled = false;
	};

	PreviewContext& previewCtx(void* ctx) { return *static_cast<PreviewContext*>(ctx); }

	bool previewApplySequence(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);
		if (!setLsoScanSequence(c.req.sequenceMode, c.req.preMoveOffset, c.req.preMoveDelayUs)) {
			setLastCaptureError("preview scan sequence apply failed (mode="
				+ std::to_string(c.req.sequenceMode) + ")");
			WsoLogWarn("[FUNDUS-PRV] scan sequence apply failed");
			return false;
		}
		return true;
	}

	bool previewApplyCameraParams(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);
		try {
			// The preview runs its own preset - pixel format, binning, ROI, acquisition mode,
			// ADC depth, the tunables AND the A-1 trigger/GPIO/sensor nodes - not whatever
			// the last capture left on the camera; restoreMonoPreviewParams puts back
			// exactly what was replaced.
			ColorCameraApplyResult applyResult{};
			if (!installPreviewParams(c.cam, c.req, c.resolvedExt, &applyResult)) {
				std::string detail = "no per-field detail was reported";
				for (int i = 0; i < applyResult.fieldCount && i < LSO_CAMERA_FIELD_MAX; ++i) {
					const ColorCameraFieldResult& field = applyResult.fields[i];
					if (field.status == CAM_FIELD_APPLIED || field.status == CAM_FIELD_NOT_REQUESTED
						|| field.status == CAM_FIELD_ADJUSTED) {
						continue;
					}
					detail = std::string(field.node) + " requested '" + field.requested
						+ "', actual '" + field.actual + "'";
					break;
				}
				setLastCaptureError("preview camera configuration was not applied - "
					+ detail + " (see the [SET]/[CAM-EXT] log)");
				WsoLogWarn("[FUNDUS-PRV] camera configuration failed");
				return false;
			}
			WsoLogInfo("[FUNDUS-PRV] camera params monoMode=" + std::to_string(c.req.monoMode)
				+ " presetPixelFormat=" + std::to_string(c.req.camera.pixelFormat)
				+ " presetBinning=" + std::to_string(c.req.camera.binningHorizontal)
				+ "x" + std::to_string(c.req.camera.binningVertical)
				+ " presetRoi=" + std::to_string(c.req.camera.roi_x_width)
				+ "x" + std::to_string(c.req.camera.roi_y_height)
				+ "+" + std::to_string(c.req.camera.roi_x_offset)
				+ "+" + std::to_string(c.req.camera.roi_y_offset)
				+ " appliedRoi=" + std::to_string(c.cam->getROI_X_Width())
				+ "x" + std::to_string(c.cam->getROI_Y_Height())
				+ " reqExposureUs=" + std::to_string(c.req.exposureTimeUs)
				+ " appliedExposureUs=" + std::to_string(c.cam->getExposureTime())
				+ " reqGain=" + std::to_string(c.req.gain)
				+ " appliedGain=" + std::to_string(c.cam->getGain()));
			return true;
		}
		catch (const std::exception& e) {
			setLastCaptureError("preview camera params failed: " + std::string(e.what()));
			return false;
		}
		catch (...) {
			setLastCaptureError("preview camera params failed");
			return false;
		}
	}

	bool previewUploadProfile(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);

		// The preview owns its slot profile too, so the board scans this preview's
		// trajectory rather than whatever the last capture or Apply left in the slot.
		// DevKit ApplyLsoPatternConfig writes GetColorCameraExposureUs into
		// traj_profile.exposure_time_us and never the JSON ExposureTimeUs (Preview_Bfs
		// stores 44000; the pattern form shows the camera value). The camera reading is
		// taken AFTER the preset above, so it is this preview's exposure, not the previous
		// session's.
		LsoScannerControlParam profile = c.req.profile;
		profile.exposureTimeUs = capture::resolveBoardExposureUs(profile.exposureTimeUs,
			(double)c.cam->getExposureTime());
		WsoLogInfo("[FUNDUS-PRV] start patternId=" + std::to_string(c.req.scanPatternId)
			+ " monoMode=" + std::to_string(c.req.monoMode)
			+ " sweep=" + std::to_string(profile.yGalvoStartPos) + "->" + std::to_string(profile.yGalvoEndPos)
			+ " samples=" + std::to_string(profile.sampleSize)
			+ " reqExposureUs=" + std::to_string(c.req.profile.exposureTimeUs)
			+ " boardExposureUs=" + std::to_string(profile.exposureTimeUs)
			+ " trgSrc=" + std::to_string(profile.triggerSource)
			+ " ledIdx=" + std::to_string(profile.ledOnPosIndex) + "/" + std::to_string(profile.ledOffPosIndex));

		// Not a gate - the board owns what this field means - but the one combination that
		// produces a silently black preview is worth naming in the log: the camera is now
		// driven from the board trigger, so a slot that triggers the IR camera instead
		// (DevKit writes TrigSrc = 0 for a color-camera preview) delivers no frames at all.
		if (profile.triggerSource != 0) {
			WsoLogWarn("[FUNDUS-PRV] slot triggerSource=" + std::to_string(profile.triggerSource)
				+ " is not the color camera (0) - the camera will receive no triggers");
		}

		if (!submitLsoScannerControlParam(c.req.scanPatternId, &profile)) {
			setLastCaptureError("preview scan profile upload failed (patternId="
				+ std::to_string(c.req.scanPatternId) + ")");
			WsoLogWarn("[FUNDUS-PRV] profile upload failed");
			return false;
		}
		return true;
	}

	bool previewApplyLight(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);
		try {
			if (applyFundusLight(c.req.previewLight, c.req.previewLightMode,
				c.req.previewLightIntensity)) {
				return true;
			}
			setLastCaptureError("preview illumination was refused (light="
				+ std::to_string(c.req.previewLight) + ")");
			WsoLogWarn("[FUNDUS-PRV] illumination was refused");
			return false;
		}
		catch (...) {
			setLastCaptureError("preview illumination failed (light="
				+ std::to_string(c.req.previewLight) + ")");
			WsoLogWarn("[FUNDUS-PRV] illumination failed");
			return false;
		}
	}

	bool previewConnectCallback(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);
		c.inst->connectColorCameraFrameCaptured(c.cb);
		return true;
	}

	bool previewBeginTriggeredCamera(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);
		// When the preset owns the trigger group, previewApplyCameraParams already wrote and
		// verified all of it. Letting startTriggeredLiveMode run its hardcoded setup on top
		// would replace exactly those values - the overwrite this work removes - so it is
		// told to arm only.
		if (!c.cam->startTriggeredLiveMode(c.req.triggerDelayUs, kPreviewArmTimeoutMs, c.triggerInstalled)) {
			setLastCaptureError("preview camera trigger start/arming failed (delayUs="
				+ std::to_string(c.req.triggerDelayUs) + ", budget "
				+ std::to_string(kPreviewArmTimeoutMs) + "ms) - see [COLOR-PRV]/[COLOR-CAP] log");
			WsoLogWarn("[FUNDUS-PRV] camera trigger start failed");
			return false;
		}
		return true;
	}

	bool previewStartScanner(void* ctx)
	{
		PreviewContext& c = previewCtx(ctx);
		if (!startLsoScannerGrabbing(c.req.scanPatternId, c.req.fundusMode)) {
			setLastCaptureError("preview scanner grabbing start failed (patternId="
				+ std::to_string(c.req.scanPatternId) + ")");
			WsoLogWarn("[FUNDUS-PRV] scanner start failed");
			return false;
		}
		return true;
	}

	bool previewRelease(void* ctx, capture::SequenceResource resource)
	{
		PreviewContext& c = previewCtx(ctx);
		try {
			switch (resource) {
			case capture::SequenceResource::ScannerGrabbing:
				return pauseLsoScannerGrabbing(c.req.scanPatternId);
			case capture::SequenceResource::CameraAcquisition:
				// Clears liveMode and joins the triggered worker. That worker's per-image
				// wait is bounded, so this returns even though no trigger will ever come
				// once the board is off (or if it was never started).
				c.cam->pauseLiveMode();
				return true;
			case capture::SequenceResource::FrameCallback:
				if (auto* inst = Hardware::getInstance(); inst) {
					inst->disconnectColorCameraFrameCaptured();
				}
				return true;
			case capture::SequenceResource::CaptureLight:
				if (!restoreFundusLight(c.req.previewLight, c.req.restoreLight,
					c.req.restoreLightMode, c.req.restoreLightIntensity)) {
					setLastCaptureError("preview cleanup failed while restoring illumination");
					return false;
				}
				return true;
			case capture::SequenceResource::CameraParams:
				restoreMonoPreviewParams();
				return true;
			}
		}
		catch (...) {
			return false;
		}
		return false;
	}

	capture::PreviewOps previewOps(void)
	{
		capture::PreviewOps ops;
		ops.applySequence = &previewApplySequence;
		ops.applyCameraParams = &previewApplyCameraParams;
		ops.uploadProfile = &previewUploadProfile;
		ops.previewLight = &previewApplyLight;
		ops.connectCallback = &previewConnectCallback;
		ops.beginTriggeredCamera = &previewBeginTriggeredCamera;
		ops.startScanner = &previewStartScanner;
		ops.release = &previewRelease;
		return ops;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::startLsoFundusPreview(const LsoFundusPreviewRequest* request, ColorCameraFrameCaptured cb)
{
	CaptureValidationReason reason = CAP_REQ_VALID;
	if (!capture::validateFundusPreviewRequest(request, &reason)) {
		setLastCaptureError(std::string("fundus preview request rejected: ")
			+ capture::describeValidationReason(reason));
		return false;
	}
	std::lock_guard<std::mutex> lock(s_captureStateLock);

	try {
		auto* inst = Hardware::getInstance();
		if (!inst) { setLastCaptureError("hardware not ready"); return false; }
		auto* mb = inst->getMainBoard();
		if (!mb) { setLastCaptureError("main board not ready"); return false; }
		auto* cam = mb->getColorCamera();
		if (!cam) { setLastCaptureError("color camera not ready"); return false; }

		// Same single-session rule as the capture path: do not fight a running session and
		// do not snapshot its params as the "saved" preview baseline.
		if (cam->isLiveMode() || cam->isCaptureMode() || s_capturing || s_fundusPreviewActive) {
			setLastCaptureError("busy: a live/capture session is active");
			WsoLogWarn("[FUNDUS-PRV] rejected: busy (live=" + std::to_string(cam->isLiveMode())
				+ " capture=" + std::to_string(cam->isCaptureMode())
				+ " capturing=" + std::to_string(s_capturing.load())
				+ " preview=" + std::to_string(s_fundusPreviewActive) + ")");
			return false;
		}

		PreviewContext ctx;
		ctx.cam = cam;
		ctx.inst = inst;
		ctx.req = *request;
		ctx.cb = cb;
		// The preview is hardware-triggered by contract, and the request's validated
		// triggerDelayUs is folded into the block here - when the preset owns the trigger
		// group the camera start only arms, so this is the only thing that writes the delay.
		ctx.resolvedExt = capture::resolvePreviewCameraConfig(*request);
		ctx.triggerInstalled = capture::isCameraTriggerContractStated(&ctx.resolvedExt);

		// The request is stored before the run so a rollback and a later stop restore the
		// illumination and the slot from the same request the start used.
		s_fundusPreviewRequest = ctx.req;
		s_fundusPreviewState.armStart();

		const capture::PreviewOps ops = previewOps();
		const capture::PreviewStep failed = capture::runPreviewStart(s_fundusPreviewState, ops, &ctx);
		if (failed != capture::PreviewStep::Done) {
			// A refused cleanup retains ownership, so stop can retry and start stays busy.
			s_fundusPreviewActive = !s_fundusPreviewState.teardownPlan().empty();
			WsoLogWarn("[FUNDUS-PRV] start failed at step " + std::to_string((int)failed));
			return false;
		}

		s_fundusPreviewActive = true;
		WsoLogInfo("[FUNDUS-PRV] running (camera armed on trigger, scanner on)");
		return true;
	}
	catch (...) {
		// Nothing here can leak: the driver owns the ledger and rolls it back itself.
		setLastCaptureError("unexpected exception in startLsoFundusPreview");
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopLsoFundusPreview(void)
{
	std::lock_guard<std::mutex> lock(s_captureStateLock);
	try {
		if (!s_fundusPreviewActive) {
			// A preview that never started owns nothing; the legacy mono-preview export and
			// the free-running Color Live have their own stop paths.
			return true;
		}
		PreviewContext ctx;
		ctx.inst = Hardware::getInstance();
		ctx.req = s_fundusPreviewRequest;
		if (ctx.inst && ctx.inst->getMainBoard()) {
			ctx.cam = ctx.inst->getMainBoard()->getColorCamera();
		}
		if (!ctx.cam) {
			setLastCaptureError("color camera not ready");
			return false;
		}

		// Same reverse order the failure rollback uses: board off -> camera live stop ->
		// callback -> illumination restore -> camera params restore.
		const capture::PreviewOps ops = previewOps();
		if (!capture::runPreviewStop(s_fundusPreviewState, ops, &ctx)) {
			WsoLogWarn("[FUNDUS-PRV] stopped with cleanup failure");
			return false;
		}
		s_fundusPreviewActive = false;
		WsoLogInfo("[FUNDUS-PRV] stopped");
		return true;
	}
	catch (...) {
		setLastCaptureError("unexpected exception in stopLsoFundusPreview");
		return false;
	}
}

// --- Diagnostics -----------------------------------------------------------------------
int WSOSYSTEM_DLL_API __stdcall wso_system::validateLsoFundusCaptureRequest(const LsoFundusCaptureRequest* request, unsigned int* totalFrameCount)
{
	CaptureValidationReason reason = CAP_REQ_VALID;
	capture::validateFundusCaptureRequest(request, totalFrameCount, &reason);
	return (int)reason;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::validateLsoFundusPreviewRequest(const LsoFundusPreviewRequest* request)
{
	CaptureValidationReason reason = CAP_REQ_VALID;
	capture::validateFundusPreviewRequest(request, &reason);
	return (int)reason;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setLsoCaptureBarrier(int stepId)
{
	if (stepId != CAP_STEP_NONE && (stepId < CAP_STEP_VALIDATE || stepId > CAP_STEP_TEARDOWN)) {
		return false;
	}
	s_fundusGate.arm(stepId);
	return true;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::waitLsoCaptureBarrier(int timeoutMs)
{
	return s_fundusGate.waitArrival(timeoutMs);
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseLsoCaptureBarrier(void)
{
	s_fundusGate.release();
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::waitLsoCaptureCancelLatched(int timeoutMs)
{
	return s_fundusGate.waitCancelLatched(timeoutMs);
}
