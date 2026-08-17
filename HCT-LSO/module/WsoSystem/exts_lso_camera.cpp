#include "pch.h"
#include "exts_lso_camera.h"
#include "exts_device_lights.h"
#include "exts_lso_scanner.h"
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
			p->setupHwTriggerSetting(false);
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

	unsigned int alignDownTo(unsigned int value, unsigned int step)
	{
		if (step < 1) { step = 1; }
		return value - (value % step);
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

		// 1) snapshot current settings for restore on stop
		s_savedMonoParam = cam->getCameraSettings();
		s_hasSavedMono = true;

		// 2) build live params (same values as the validated ColorCameraPreview.PlayPreview path).
		//    roi_max is binning-relative, so convert to sensor size then take full FOV; squareRoi()
		//    below finalizes the centered square from the actually-applied Width/Height.
		ColorCameraSettingParam p = s_savedMonoParam;
		unsigned int binH = (p.binningHorizontal < 1) ? 1 : p.binningHorizontal;
		unsigned int binV = (p.binningVertical < 1) ? 1 : p.binningVertical;
		unsigned int sensorW = p.roi_max_width * binH;
		unsigned int sensorH = p.roi_max_Height * binV;
		bool hasSensor = (sensorW > 0) && (sensorH > 0);

		bool bayer = (mode == 1); // 0 = SensorMono8, 1 = BayerSwGray
		if (bayer) {
			p.pixelFormat = 4;            // BayerRG8
			p.binningHorizontal = 1;
			p.binningVertical = 1;
			if (hasSensor) {
				p.roi_x_width = alignDownTo(sensorW, 16);
				p.roi_y_height = alignDownTo(sensorH, 2);
				p.roi_x_offset = 0;
				p.roi_y_offset = 0;
			}
		}
		else {
			p.pixelFormat = 0;            // Mono8 (forces binning >= 2)
			p.binningHorizontal = 2;
			p.binningVertical = 2;
			if (hasSensor) {
				p.roi_x_width = alignDownTo(sensorW / 2, 16);
				p.roi_y_height = alignDownTo(sensorH / 2, 2);
				p.roi_x_offset = 0;
				p.roi_y_offset = 0;
			}
		}
		cam->setCameraSettings(p);

		// 3) native centered-square ROI
		cam->squareRoi();

		// 4) IR aiming illumination (reuse device-lights export)
		turnOnLight(LightType::RETINA_IR_LED);

		// 5) LSO scanner IR pattern grabbing (reuse scanner export; IR pattern id = 1)
		startLsoScannerGrabbing(1);

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

		if (s_hasSavedMono) {
			if (auto* inst = Hardware::getInstance(); inst) {
				if (auto* cam = inst->getMainBoard()->getColorCamera(); cam) {
					// Minimal restore: only the fields the one-call changed (pixel format,
					// binning, ROI). Exposure/gain keep their current values so knob tuning
					// done during the preview survives the stop.
					ColorCameraSettingParam restore = cam->getCameraSettings();
					restore.pixelFormat = s_savedMonoParam.pixelFormat;
					restore.binningHorizontal = s_savedMonoParam.binningHorizontal;
					restore.binningVertical = s_savedMonoParam.binningVertical;
					restore.roi_x_width = s_savedMonoParam.roi_x_width;
					restore.roi_y_height = s_savedMonoParam.roi_y_height;
					restore.roi_x_offset = s_savedMonoParam.roi_x_offset;
					restore.roi_y_offset = s_savedMonoParam.roi_y_offset;
					cam->setCameraSettings(restore);
				}
			}
			s_hasSavedMono = false;
		}
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

	void joinCaptureWorker()
	{
		if (s_captureWorker.t.joinable()) {
			s_captureWorker.t.join();
		}
	}

	// Callers must hold s_captureStateLock.
	int startColorCaptureCore(const ColorCameraSettingParam* param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);
}

int WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraColorCapture(const ColorCameraSettingParam* param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
{
	std::lock_guard<std::mutex> lock(s_captureStateLock);
	return startColorCaptureCore(param, scanPatternId, cb, onCompleted);
}

// Shared device sequence for both capture exports (per-frame and composed).
namespace {
int startColorCaptureCore(const ColorCameraSettingParam* param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted)
{
	bool scannerStarted = false;
	bool callbackConnected = false;
	bool workerStarted = false;

	try {
		auto* inst = Hardware::getInstance();
		if (!inst) { setLastCaptureError("hardware not ready"); return CAP_STAGE_FAULT; }
		auto* mb = inst->getMainBoard();
		if (!mb) { setLastCaptureError("main board not ready"); return CAP_STAGE_FAULT; }
		auto* cam = mb->getColorCamera();
		if (!cam) { setLastCaptureError("color camera not ready"); return CAP_STAGE_FAULT; }
		if (param == nullptr) { setLastCaptureError("capture param is null"); return CAP_STAGE_PARAM; }

		// Re-entry guard: one color-camera session at a time (live, aiming or capture).
		if (cam->isLiveMode() || cam->isCaptureMode() || s_capturing) {
			setLastCaptureError("busy: a live/capture session is active");
			return CAP_STAGE_BUSY;
		}

		joinCaptureWorker(); // reap the previous, already-finished worker

		// 1) aiming IR off. (White LED stays scan-gated / manual - values are not owned here.)
		turnOffLight(LightType::RETINA_IR_LED);

		// 2) apply + verify the preset; a silent partial apply becomes CAP_STAGE_PARAM here.
		if (!cam->setCameraSettingsChecked(*param)) {
			setLastCaptureError("param apply/verify failed - see [COLOR-CAP]/[SET] log for req vs actual");
			return CAP_STAGE_PARAM;
		}

		// 3) scanner pattern grabbing (reuses the scanner export).
		if (!startLsoScannerGrabbing(scanPatternId)) {
			setLastCaptureError("scanner grabbing start failed (patternId=" + std::to_string(scanPatternId) + ")");
			return CAP_STAGE_SCANNER;
		}
		scannerStarted = true;

		// 4) connect the frame callback and run the acquisition on a worker thread,
		//    so the caller returns immediately (no UI-thread blocking).
		inst->connectColorCameraImageCaptured(cb);
		callbackConnected = true;
		setLastCaptureError("");
		s_capturing = true;

		s_captureWorker.t = std::thread([cam, scanPatternId, onCompleted] {
			int frames = 0;
			int result = cam->acquireCaptureFrames(&frames);

			// 5) reverse order natively: scanner pause + callback disconnect happen here,
			//    not from a consumer frame callback (no WSO re-entry from consumers).
			pauseLsoScannerGrabbing(scanPatternId);
			if (auto* inst2 = Hardware::getInstance(); inst2) {
				inst2->disconnectColorCameraImageCaptured();
			}

			s_capturing = false;

			// 6) completion notice - fired exactly once, on this worker thread.
			if (onCompleted) {
				onCompleted(result, frames);
			}
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

	const int stage = startColorCaptureCore(param, scanPatternId, composedFrameThunk, composedCompletedThunk);
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

		// Contract: joins the worker here. Never call stop from the frame/completed callback.
		std::lock_guard<std::mutex> lock(s_captureStateLock);
		joinCaptureWorker();
		return true;
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