#include "CaptureSequence.h"
#include "DeviceDefines.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

using namespace wso_domain;
using namespace wso_system::capture;

namespace
{
	bool isDefinedLight(int value)
	{
		switch (value) {
		case (int)LightType::LSO_WHITE_LED:
		case (int)LightType::RETINA_IR_LED:
		case (int)LightType::CORNEA_IR_LEFT_LED:
		case (int)LightType::CORNEA_IR_RIGHT_LED:
		case (int)LightType::EXT_FIXATION_LED:
		case (int)LightType::LSO_BLUE_LED:
		case (int)LightType::LSO_GREEN_LED:
			return true;
		default:
			return false;
		}
	}

	bool isDefinedSequenceMode(int value)
	{
		return value == LSO_SCAN_TRIG_FIRST || value == LSO_SCAN_GALVANO_FIRST;
	}

	bool isDefinedFundusLightMode(int value)
	{
		return value == FUNDUS_LIGHT_TRIGGER || value == FUNDUS_LIGHT_CONTINUOUS;
	}

	bool reject(CaptureValidationReason* out, CaptureValidationReason reason)
	{
		if (out) { *out = reason; }
		return false;
	}

	// Fixed-width name compare. A caller may fill all 32 bytes without a terminator, so
	// every read is bounded by the field width rather than by a NUL that may not be there.
	bool nameEquals(const char* field, const char* value)
	{
		return std::strncmp(field, value, LSO_CAMERA_NAME_MAX) == 0;
	}

	// Stated but different from what the board wiring needs. An unstated field is not a
	// claim, so it can never conflict.
	bool statedAndNot(const char* field, const char* required)
	{
		return isCameraNameStated(field) && !nameEquals(field, required);
	}

	bool isLineName(const char* name)
	{
		for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
			const char expected[] = { 'L', 'i', 'n', 'e', (char)('0' + i), '\0' };
			if (nameEquals(name, expected)) { return true; }
		}
		return false;
	}
}

ColorCameraExtendedParam wso_system::capture::emptyCameraExtendedParam(void)
{
	ColorCameraExtendedParam ext{};
	ext.structSize = (unsigned int)sizeof(ColorCameraExtendedParam);
	ext.version = LSO_CAMERA_CONFIG_VERSION;
	ext.acquisitionBurstFrameCount = -1;
	ext.deviceLinkThroughput = -1;
	ext.triggerDelayUs = -1.0;
	ext.ispEnable = -1;
	for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
		ext.lines[i].inverter = -1;
	}
	return ext;
}

bool wso_system::capture::isCameraNameStated(const char* name)
{
	return name != nullptr && name[0] != '\0';
}

bool wso_system::capture::isCameraTriggerContractStated(const ColorCameraExtendedParam* ext)
{
	return ext != nullptr
		&& isCameraNameStated(ext->triggerSource)
		&& isCameraNameStated(ext->triggerSelector)
		&& isCameraNameStated(ext->triggerActivation)
		&& isCameraNameStated(ext->triggerOverlap);
}

ColorCameraExtendedParam wso_system::capture::resolveStartCameraConfig(const ColorCameraExtendedParam& ext,
	bool hwTriggered)
{
	ColorCameraExtendedParam resolved = ext;
	if (!isCameraTriggerContractStated(&resolved)) {
		// The start path still owns the trigger nodes; nothing to fill in.
		return resolved;
	}
	if (isCameraNameStated(resolved.triggerMode)) {
		// The preset owns TriggerMode too, and checkCameraStartContract has already refused
		// it if it disagrees with how this acquisition is driven.
		return resolved;
	}

	const char* mode = hwTriggered ? LSO_CAMERA_HW_TRIGGER_MODE : "Off";
	std::snprintf(resolved.triggerMode, LSO_CAMERA_NAME_MAX, "%s", mode);
	return resolved;
}

ColorCameraExtendedParam wso_system::capture::resolvePreviewCameraConfig(const LsoFundusPreviewRequest& request)
{
	// The preview is hardware-triggered by contract, so the drive mode is always board-gated.
	ColorCameraExtendedParam resolved = resolveStartCameraConfig(request.cameraExt, true);

	// One owner for TriggerDelay on this path. The Toolkit deliberately leaves the block's
	// delay unstated so the validated top-level field is the single source; this is where
	// that field becomes the write, so it goes out through the same checked apply as
	// everything else instead of relying on a start step that may not run.
	if (resolved.triggerDelayUs < 0.0) {
		resolved.triggerDelayUs = (double)request.triggerDelayUs;
	}
	return resolved;
}

bool wso_system::capture::statesAnyCameraLine(const ColorCameraExtendedParam* ext)
{
	if (ext == nullptr) {
		return false;
	}
	for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
		if (isCameraNameStated(ext->lines[i].mode)
			|| isCameraNameStated(ext->lines[i].source)
			|| ext->lines[i].inverter >= 0) {
			return true;
		}
	}
	return false;
}

int wso_system::capture::checkCameraStartContract(const ColorCameraExtendedParam* ext, bool hwTriggered)
{
	if (ext == nullptr) {
		return CAM_START_STRUCT_SIZE;
	}
	if (ext->structSize != (unsigned int)sizeof(ColorCameraExtendedParam)) {
		return CAM_START_STRUCT_SIZE;
	}
	if (ext->version != LSO_CAMERA_CONFIG_VERSION) {
		return CAM_START_VERSION;
	}

	// Names must be names of lines, or the GPIO editor is pointing at nothing.
	if (isCameraNameStated(ext->lineSelector) && !isLineName(ext->lineSelector)) {
		return CAM_START_LINE_SELECTOR;
	}

	// All-or-nothing: the start either installs this block's whole trigger wiring, or its
	// own constants. Claiming part of it would leave the rest to those constants - a silent
	// overwrite of nodes the operator did not state but did change around.
	// The two GPIO rows the trigger contract uses are part of the same claim: the hardware
	// trigger setup writes Line2 and Line3 too, so a preset that states them without owning
	// the group would have them overwritten a moment later.
	const bool claimsTrigger = isCameraNameStated(ext->triggerMode)
		|| isCameraNameStated(ext->triggerSource)
		|| isCameraNameStated(ext->triggerSelector)
		|| isCameraNameStated(ext->triggerActivation)
		|| isCameraNameStated(ext->triggerOverlap)
		|| isCameraNameStated(ext->lines[2].mode) || isCameraNameStated(ext->lines[2].source)
		|| isCameraNameStated(ext->lines[3].mode) || isCameraNameStated(ext->lines[3].source);
	if (claimsTrigger && !isCameraTriggerContractStated(ext)) {
		return CAM_START_TRIGGER_GROUP;
	}

	// The board exposure written into the scan profile is the camera's read-back taken
	// once, right after the preset is applied. An exposure that keeps moving afterwards
	// makes that number a guess, so Auto is refused at START - never silently switched Off,
	// and never refused at Apply, where tuning it is the point.
	if (statedAndNot(ext->exposureAuto, "Off")) {
		return CAM_START_EXPOSURE_AUTO;
	}

	// TriggerMode is the one field whose required value depends on how the acquisition is
	// driven: the board gates the exposures (On) or the camera free-runs (Off).
	if (statedAndNot(ext->triggerMode, hwTriggered ? LSO_CAMERA_HW_TRIGGER_MODE : "Off")) {
		return CAM_START_TRIGGER_MODE;
	}

	if (!hwTriggered) {
		// Free-running: the trigger nodes are inert while TriggerMode is Off, so nothing
		// else about them can conflict.
		return CAM_START_OK;
	}

	if (statedAndNot(ext->triggerSource, LSO_CAMERA_HW_TRIGGER_SOURCE)) {
		return CAM_START_TRIGGER_SOURCE;
	}
	if (statedAndNot(ext->triggerActivation, LSO_CAMERA_HW_TRIGGER_ACTIVATION)) {
		return CAM_START_TRIGGER_ACTIVATION;
	}
	if (statedAndNot(ext->triggerSelector, LSO_CAMERA_HW_TRIGGER_SELECTOR)) {
		return CAM_START_TRIGGER_SELECTOR;
	}
	if (statedAndNot(ext->triggerOverlap, LSO_CAMERA_HW_TRIGGER_OVERLAP)) {
		return CAM_START_TRIGGER_OVERLAP;
	}

	// Line3 carries the board trigger into the camera; driving it as an output means the
	// trigger never arrives and the acquisition simply times out.
	if (statedAndNot(ext->lines[LSO_CAMERA_HW_TRIGGER_LINE_INDEX].mode, LSO_CAMERA_HW_TRIGGER_LINE_MODE)) {
		return CAM_START_TRIGGER_LINE_MODE;
	}

	// One FrameStart trigger yields one frame. A burst count above 1 only means something
	// under a FrameBurstStart selector, so stating both is a request for more frames than
	// the sequence will ever wait for - which is exactly the completion-condition mismatch
	// this gate exists to catch, rather than discovering it as a capture that never ends.
	if (ext->acquisitionBurstFrameCount > 1) {
		return CAM_START_BURST_COUNT;
	}

	return CAM_START_OK;
}

const char* wso_system::capture::describeCameraStartContract(int contract)
{
	switch (contract) {
	case CAM_START_OK:                   return "camera configuration is compatible with the start";
	case CAM_START_STRUCT_SIZE:          return "camera configuration structSize mismatch (managed/native ABI drift)";
	case CAM_START_VERSION:              return "camera configuration version is not supported";
	case CAM_START_TRIGGER_MODE:         return "TriggerMode conflicts with the selected drive mode";
	case CAM_START_TRIGGER_SOURCE:       return "TriggerSource is not the board trigger line (Line3)";
	case CAM_START_TRIGGER_ACTIVATION:   return "TriggerActivation is not RisingEdge as the board drives it";
	case CAM_START_TRIGGER_SELECTOR:     return "TriggerSelector is not FrameStart as the board drives it";
	case CAM_START_TRIGGER_OVERLAP:      return "TriggerOverlap is not ReadOut as the board drives it";
	case CAM_START_TRIGGER_LINE_MODE:    return "Line3 LineMode is not Input, so the board trigger cannot reach the camera";
	case CAM_START_EXPOSURE_AUTO:        return "ExposureAuto is not Off, so the board exposure cannot follow the camera";
	case CAM_START_BURST_COUNT:          return "AcquisitionBurstFrameCount above 1 does not match one frame per FrameStart trigger";
	case CAM_START_LINE_SELECTOR:        return "LineSelector is not one of Line0..Line3";
	case CAM_START_LINE_NAME:            return "a GPIO line row names a line that does not exist";
	case CAM_START_TRIGGER_GROUP:        return "the trigger group is stated only in part - state TriggerSource, TriggerSelector, TriggerActivation and TriggerOverlap together, or none of them";
	default:                             return "unknown camera start contract code";
	}
}

void SequenceState::armStart(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	cancelled_ = false;
	acquired_.clear();
}

void SequenceState::requestCancel(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	cancelled_ = true;
}

bool SequenceState::isCancelled(void) const
{
	std::lock_guard<std::mutex> guard(lock_);
	return cancelled_;
}

void SequenceState::acquire(SequenceResource resource)
{
	std::lock_guard<std::mutex> guard(lock_);
	if (std::find(acquired_.begin(), acquired_.end(), resource) == acquired_.end()) {
		acquired_.push_back(resource);
	}
}

bool SequenceState::holds(SequenceResource resource) const
{
	std::lock_guard<std::mutex> guard(lock_);
	return std::find(acquired_.begin(), acquired_.end(), resource) != acquired_.end();
}

std::vector<SequenceResource> SequenceState::teardownPlan(void) const
{
	std::lock_guard<std::mutex> guard(lock_);
	return std::vector<SequenceResource>(acquired_.rbegin(), acquired_.rend());
}

void SequenceState::releaseAll(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	acquired_.clear();
}

void SequenceState::release(SequenceResource resource)
{
	std::lock_guard<std::mutex> guard(lock_);
	acquired_.erase(std::remove(acquired_.begin(), acquired_.end(), resource), acquired_.end());
}

void DiagnosticGate::arm(int stepId)
{
	std::lock_guard<std::mutex> guard(lock_);
	armedStep_ = stepId;
	arrived_ = false;
	released_ = false;
}

bool DiagnosticGate::waitArrival(int timeoutMs)
{
	if (timeoutMs < 0) { timeoutMs = 0; }
	std::unique_lock<std::mutex> guard(lock_);
	return cv_.wait_for(guard, std::chrono::milliseconds(timeoutMs), [this] { return arrived_; });
}

void DiagnosticGate::release(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	released_ = true;
	cv_.notify_all();
}

void DiagnosticGate::waitAtStep(int stepId)
{
	std::unique_lock<std::mutex> guard(lock_);
	if (armedStep_ != stepId) {
		return;
	}

	arrived_ = true;
	cv_.notify_all();

	// Bounded on purpose: an unreleased barrier resolves itself instead of parking a
	// worker that owns camera + scanner state for good.
	cv_.wait_for(guard, std::chrono::milliseconds(kBarrierAutoReleaseMs), [this] { return released_; });

	armedStep_ = CAP_STEP_NONE;   // one-shot
	released_ = false;
}

void DiagnosticGate::signalCancelLatched(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	cancelLatched_ = true;
	cv_.notify_all();
}

bool DiagnosticGate::waitCancelLatched(int timeoutMs)
{
	if (timeoutMs < 0) { timeoutMs = 0; }
	std::unique_lock<std::mutex> guard(lock_);
	return cv_.wait_for(guard, std::chrono::milliseconds(timeoutMs), [this] { return cancelLatched_; });
}

void DiagnosticGate::clearCancelLatch(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	cancelLatched_ = false;
}

void DiagnosticGate::reset(void)
{
	std::lock_guard<std::mutex> guard(lock_);
	armedStep_ = CAP_STEP_NONE;
	arrived_ = false;
	released_ = false;
	cancelLatched_ = false;
}

bool wso_system::capture::runFundusLightApply(const FundusLightOps& ops, void* ctx,
	int light, int mode, unsigned short intensity)
{
	if (ops.setMode == nullptr || ops.turnOn == nullptr) {
		return false;
	}
	if (!ops.setMode(ctx, light, mode)) {
		return false;
	}
	if (intensity > 0) {
		if (ops.setIntensity == nullptr || !ops.setIntensity(ctx, light, intensity)) {
			return false;
		}
	}
	return ops.turnOn(ctx, light);
}

bool wso_system::capture::runFundusLightRestore(const FundusLightOps& ops, void* ctx,
	int workingLight, int restoreLight, int restoreMode, unsigned short restoreIntensity)
{
	if (ops.turnOff == nullptr) {
		return false;
	}
	const bool workingOff = ops.turnOff(ctx, workingLight);
	if (!workingOff || restoreLight == workingLight) {
		return workingOff;
	}

	// Do not add another illuminated lamp after a refused stop. A failed restore may
	// itself have partially reached the board, so it also needs an explicit OFF attempt.
	try {
		if (runFundusLightApply(ops, ctx, restoreLight, restoreMode, restoreIntensity)) {
			return true;
		}
	}
	catch (...) {
	}
	ops.turnOff(ctx, restoreLight);
	return false;
}

namespace
{
	// One step of the preview start. Most resources are acquired only after success. Camera
	// params are different: applying them can fail after partial writes, so that resource is
	// reserved before the step and its snapshot is restored on either outcome.
	bool runPreviewStep(SequenceState& state, bool (*step)(void*), void* ctx,
		const SequenceResource* acquires, bool reserveBeforeRun)
	{
		if (reserveBeforeRun && acquires) {
			state.acquire(*acquires);
		}
		if (step == nullptr) {
			return false;
		}
		if (!step(ctx)) {
			return false;
		}
		if (!reserveBeforeRun && acquires) {
			state.acquire(*acquires);
		}
		return true;
	}
}

PreviewStep wso_system::capture::runPreviewStart(SequenceState& state, const PreviewOps& ops, void* ctx)
{
	// Acquisition order == the order the steps run in, so the reverse is the teardown.
	// The board (StartScanner) is deliberately last: the camera must already be streaming
	// and armed on the trigger line before the board starts issuing exposures.
	constexpr SequenceResource kCameraParams = SequenceResource::CameraParams;
	constexpr SequenceResource kLight = SequenceResource::CaptureLight;
	constexpr SequenceResource kCallback = SequenceResource::FrameCallback;
	constexpr SequenceResource kCamera = SequenceResource::CameraAcquisition;
	constexpr SequenceResource kScanner = SequenceResource::ScannerGrabbing;

	struct Plan
	{
		PreviewStep step;
		bool (*run)(void*);
		const SequenceResource* acquires;
		bool reserveBeforeRun;
	};

	// The profile upload owns no resource: the board slot has no previous value this
	// sequence could put back, so there is nothing to undo for it.
	const Plan plan[] = {
		{ PreviewStep::ApplySequence,        ops.applySequence,         nullptr,        false },
		{ PreviewStep::ApplyCameraParams,    ops.applyCameraParams,     &kCameraParams, true },
		{ PreviewStep::UploadProfile,        ops.uploadProfile,         nullptr,        false },
		{ PreviewStep::PreviewLight,         ops.previewLight,          &kLight,         true },
		{ PreviewStep::ConnectCallback,      ops.connectCallback,       &kCallback,      false },
		{ PreviewStep::BeginTriggeredCamera, ops.beginTriggeredCamera,  &kCamera,        false },
		{ PreviewStep::StartScanner,         ops.startScanner,          &kScanner,       false },
	};

	for (const Plan& entry : plan) {
		bool succeeded = false;
		try {
			succeeded = runPreviewStep(state, entry.run, ctx, entry.acquires, entry.reserveBeforeRun);
		}
		catch (...) {
			succeeded = false;
		}
		if (!succeeded) {
			runPreviewStop(state, ops, ctx);
			return entry.step;
		}
	}

	return PreviewStep::Done;
}

bool wso_system::capture::runPreviewStop(SequenceState& state, const PreviewOps& ops, void* ctx)
{
	bool released = true;
	if (ops.release != nullptr) {
		for (const SequenceResource resource : state.teardownPlan()) {
			try {
				if (ops.release(ctx, resource)) {
					state.release(resource);
				}
				else {
					released = false;
				}
			}
			catch (...) {
				released = false;
			}
		}
	}
	else if (!state.teardownPlan().empty()) {
		released = false;
	}
	return released;
}

namespace
{
	unsigned int alignDownTo(unsigned int value, unsigned int step)
	{
		if (step < 1) { step = 1; }
		return value - (value % step);
	}
}

PreviewParamPlan wso_system::capture::planMonoPreviewParams(const ColorCameraSettingParam& current,
	int monoMode, unsigned int exposureTimeUs, float gain)
{
	PreviewParamPlan plan;
	plan.params = current;
	plan.geometry = true;
	plan.squareRoi = true;

	// roi_max is binning-relative, so convert back to the sensor size before taking the
	// full FOV; the centered square is finished from the actually-applied Width/Height.
	const unsigned int binH = (current.binningHorizontal < 1) ? 1 : current.binningHorizontal;
	const unsigned int binV = (current.binningVertical < 1) ? 1 : current.binningVertical;
	const unsigned int sensorW = current.roi_max_width * binH;
	const unsigned int sensorH = current.roi_max_Height * binV;
	const bool hasSensor = (sensorW > 0) && (sensorH > 0);

	if (monoMode == 1) {
		plan.params.pixelFormat = 4;            // BayerRG8
		plan.params.binningHorizontal = 1;
		plan.params.binningVertical = 1;
		if (hasSensor) {
			plan.params.roi_x_width = alignDownTo(sensorW, 16);
			plan.params.roi_y_height = alignDownTo(sensorH, 2);
			plan.params.roi_x_offset = 0;
			plan.params.roi_y_offset = 0;
		}
	}
	else {
		plan.params.pixelFormat = 0;            // Mono8 (forces binning >= 2)
		plan.params.binningHorizontal = 2;
		plan.params.binningVertical = 2;
		if (hasSensor) {
			plan.params.roi_x_width = alignDownTo(sensorW / 2, 16);
			plan.params.roi_y_height = alignDownTo(sensorH / 2, 2);
			plan.params.roi_x_offset = 0;
			plan.params.roi_y_offset = 0;
		}
	}

	if (exposureTimeUs > 0) {
		plan.params.exposure_time = exposureTimeUs;
		plan.tunables = true;
	}
	if (gain > 0.0f) {
		plan.params.gain = gain;
		plan.tunables = true;
	}

	return plan;
}

PreviewParamPlan wso_system::capture::planPreviewParams(const ColorCameraSettingParam& current,
	const LsoFundusPreviewRequest& request)
{
	// Start from the monoMode geometry: it is the fallback for a preset that carries no
	// window, and it already applies the request's validated exposure/gain.
	PreviewParamPlan plan = planMonoPreviewParams(current, request.monoMode,
		request.exposureTimeUs, request.gain);

	const ColorCameraSettingParam& preset = request.camera;

	plan.params.pixelFormat = preset.pixelFormat;
	plan.params.binningHorizontal = (preset.binningHorizontal < 1) ? 1 : preset.binningHorizontal;
	plan.params.binningVertical = (preset.binningVertical < 1) ? 1 : preset.binningVertical;

	// A zero-sized window is not something a camera can run, so it means "this preset
	// carries no ROI" - keep the derived full-FOV window and let the square crop finish it.
	// A preset that DOES carry a window owns it: cropping that to a centered square would
	// throw away the very ROI the preset exists to state.
	if (preset.roi_x_width > 0 && preset.roi_y_height > 0) {
		plan.params.roi_x_width = preset.roi_x_width;
		plan.params.roi_y_height = preset.roi_y_height;
		plan.params.roi_x_offset = preset.roi_x_offset;
		plan.params.roi_y_offset = preset.roi_y_offset;
		plan.squareRoi = false;
	}

	plan.params.acqusitionMode = preset.acqusitionMode;
	plan.params.acqusitionFrameCount = preset.acqusitionFrameCount;
	plan.acquisition = true;

	plan.params.adcDepthIndex = preset.adcDepthIndex;
	plan.adcDepth = true;

	// roi_max_* are device-reported; a request never dictates them.
	plan.params.roi_max_width = current.roi_max_width;
	plan.params.roi_max_Height = current.roi_max_Height;

	return plan;
}

ColorCameraSettingParam wso_system::capture::resolveCaptureCameraParams(
	const ColorCameraSettingParam& requested, unsigned int totalFrameCount, bool hwTriggered)
{
	ColorCameraSettingParam resolved = requested;
	if (hwTriggered) {
		resolved.acqusitionMode = 0;
		return resolved;
	}

	resolved.acqusitionMode = totalFrameCount == 1U ? 1U : 2U;
	resolved.acqusitionFrameCount = totalFrameCount;
	return resolved;
}

LsoScannerControlParam wso_system::capture::resolveCaptureScannerProfile(
	const LsoScannerControlParam& requested, double cameraExposureUs)
{
	LsoScannerControlParam resolved = requested;
	resolved.exposureTimeUs = (float)cameraExposureUs;
	return resolved;
}

ColorCameraSettingParam wso_system::capture::mergePreviewRestore(const ColorCameraSettingParam& live,
	const ColorCameraSettingParam& saved, const PreviewParamPlan& plan)
{
	ColorCameraSettingParam restore = live;
	if (plan.geometry) {
		restore.pixelFormat = saved.pixelFormat;
		restore.binningHorizontal = saved.binningHorizontal;
		restore.binningVertical = saved.binningVertical;
		restore.roi_x_width = saved.roi_x_width;
		restore.roi_y_height = saved.roi_y_height;
		restore.roi_x_offset = saved.roi_x_offset;
		restore.roi_y_offset = saved.roi_y_offset;
	}
	if (plan.tunables) {
		restore.exposure_time = saved.exposure_time;
		restore.gain = saved.gain;
	}
	if (plan.acquisition) {
		restore.acqusitionMode = saved.acqusitionMode;
		restore.acqusitionFrameCount = saved.acqusitionFrameCount;
	}
	if (plan.adcDepth) {
		restore.adcDepthIndex = saved.adcDepthIndex;
	}
	return restore;
}

bool wso_system::capture::validateFundusCaptureRequest(const LsoFundusCaptureRequest* request,
	unsigned int* totalFrameCount, CaptureValidationReason* reason)
{
	if (totalFrameCount) { *totalFrameCount = 0; }
	if (reason) { *reason = CAP_REQ_VALID; }

	if (request == nullptr) { return reject(reason, CAP_REQ_NULL); }
	if (request->structSize != (unsigned int)sizeof(LsoFundusCaptureRequest)) {
		return reject(reason, CAP_REQ_STRUCT_SIZE);
	}
	if (request->triggerMode != COLOR_CAPTURE_NORMAL && request->triggerMode != COLOR_CAPTURE_HW_TRIGGER) {
		return reject(reason, CAP_REQ_TRIGGER_MODE);
	}
	if (request->scanPatternId < 0 || request->scanPatternId >= LSO_SCANNER_PATTERN_NUM_MAX) {
		return reject(reason, CAP_REQ_PATTERN_ID);
	}

	if (!isDefinedSequenceMode(request->sequenceMode)) {
		return reject(reason, CAP_REQ_SEQUENCE_MODE);
	}

	// A zero pattern size is not a scan, and the profile is always uploaded on this path -
	// refuse it here instead of writing a zeroed trajectory over a good board slot.
	if (request->profile.sampleSize == 0) { return reject(reason, CAP_REQ_SAMPLE_SIZE_ZERO); }

	// Frame count. Each zero is rejected BEFORE the multiplication, so no max(1, x) style
	// guard can hide it; the product is computed in 64 bit (uint32 * uint16 cannot overflow
	// there) and then bounded by what the callback ABI can carry - frameCount /
	// framesAcquired are signed 32 bit in WsoCallback.cs, so INT32_MAX is the representable
	// limit. That is an ABI bound, not an invented shot-count policy.
	if (request->profile.acqFrameSize == 0) { return reject(reason, CAP_REQ_ACQ_FRAME_SIZE_ZERO); }
	if (request->profile.subFrameSize == 0) { return reject(reason, CAP_REQ_SUB_FRAME_SIZE_ZERO); }
	const unsigned long long product =
		(unsigned long long)request->profile.acqFrameSize * (unsigned long long)request->profile.subFrameSize;
	if (product > 0x7FFFFFFFull) { return reject(reason, CAP_REQ_FRAME_COUNT_RANGE); }

	// Composition bands must match the profile that produced the frames; deriving them from
	// anything else is how a composite ends up holding the previous capture's rows.
	if (request->subFrameCompose != 0 && request->subFrameCompose != (unsigned int)request->profile.subFrameSize) {
		return reject(reason, CAP_REQ_SUB_FRAME_COMPOSE);
	}

	// Only the light identity is a request decision. Intensity is board state that outlives
	// one capture (and the device reports it from a cache that starts at 0), so a zero here
	// means "leave what the board has" - rejecting it would block every capture after a
	// fresh start until someone moved a slider.
	if (!isDefinedLight(request->captureLight)) { return reject(reason, CAP_REQ_CAPTURE_LIGHT); }
	if (!isDefinedLight(request->restoreLight)) { return reject(reason, CAP_REQ_RESTORE_LIGHT); }
	if (!isDefinedFundusLightMode(request->captureLightMode)) {
		return reject(reason, CAP_REQ_WORKING_LIGHT_MODE);
	}
	if (!isDefinedFundusLightMode(request->restoreLightMode)) {
		return reject(reason, CAP_REQ_RESTORE_LIGHT_MODE);
	}

	// The A-1 camera configuration is part of the request, so it is gated here with
	// everything else - before any device is touched. A size/version drift is a different
	// failure from a value the board wiring cannot drive, so they get different reasons.
	const int contract = checkCameraStartContract(&request->cameraExt,
		request->triggerMode == COLOR_CAPTURE_HW_TRIGGER);
	if (contract == CAM_START_STRUCT_SIZE || contract == CAM_START_VERSION) {
		return reject(reason, CAP_REQ_CAMERA_CONFIG);
	}
	if (contract != CAM_START_OK) {
		return reject(reason, CAP_REQ_CAMERA_CONTRACT);
	}

	if (totalFrameCount) { *totalFrameCount = (unsigned int)product; }
	return true;
}

bool wso_system::capture::validateFundusPreviewRequest(const LsoFundusPreviewRequest* request,
	CaptureValidationReason* reason)
{
	if (reason) { *reason = CAP_REQ_VALID; }

	if (request == nullptr) { return reject(reason, CAP_REQ_NULL); }
	if (request->structSize != (unsigned int)sizeof(LsoFundusPreviewRequest)) {
		return reject(reason, CAP_REQ_STRUCT_SIZE);
	}
	if (request->monoMode != 0 && request->monoMode != 1) { return reject(reason, CAP_REQ_MONO_MODE); }
	if (request->scanPatternId < 0 || request->scanPatternId >= LSO_SCANNER_PATTERN_NUM_MAX) {
		return reject(reason, CAP_REQ_PATTERN_ID);
	}
	if (!isDefinedSequenceMode(request->sequenceMode)) {
		return reject(reason, CAP_REQ_SEQUENCE_MODE);
	}
	if (request->profile.sampleSize == 0) { return reject(reason, CAP_REQ_SAMPLE_SIZE_ZERO); }

	// The preview is continuous by contract. A fixed-frame profile is rejected rather than
	// silently overwritten, so the caller cannot believe it asked for something else.
	if (request->profile.acquisitionMode != 0) { return reject(reason, CAP_REQ_PREVIEW_ACQ_MODE); }

	// The preview installs its own camera exposure (DevKit BfsCameraConfigPreview: 1013us)
	// instead of inheriting whatever the capture config left behind. Zero is not a "leave
	// it alone" sentinel the way a zero light intensity is - no camera runs a zero
	// exposure, so it can only be a caller that never filled the field.
	// gain and triggerDelayUs are NOT gated: 0dB and a 0us delay are both legal values.
	if (request->exposureTimeUs == 0) { return reject(reason, CAP_REQ_PREVIEW_EXPOSURE); }

	if (!isDefinedLight(request->previewLight)) { return reject(reason, CAP_REQ_CAPTURE_LIGHT); }
	if (!isDefinedLight(request->restoreLight)) { return reject(reason, CAP_REQ_RESTORE_LIGHT); }
	if (!isDefinedFundusLightMode(request->previewLightMode)) {
		return reject(reason, CAP_REQ_WORKING_LIGHT_MODE);
	}
	if (!isDefinedFundusLightMode(request->restoreLightMode)) {
		return reject(reason, CAP_REQ_RESTORE_LIGHT_MODE);
	}

	// The preview is hardware-triggered by contract, so it is always checked against the
	// board-gated wiring.
	const int contract = checkCameraStartContract(&request->cameraExt, true);
	if (contract == CAM_START_STRUCT_SIZE || contract == CAM_START_VERSION) {
		return reject(reason, CAP_REQ_CAMERA_CONFIG);
	}
	if (contract != CAM_START_OK) {
		return reject(reason, CAP_REQ_CAMERA_CONTRACT);
	}

	return true;
}

unsigned short wso_system::capture::resolveFundusLightIntensity(unsigned short requested, unsigned short calibrated)
{
	return requested > 0 ? requested : calibrated;
}

float wso_system::capture::resolveBoardExposureUs(float requestedUs, double cameraUs)
{
	return cameraUs > 0.0 ? (float)cameraUs : requestedUs;
}

const char* wso_system::capture::describeValidationReason(CaptureValidationReason reason)
{
	switch (reason) {
	case CAP_REQ_VALID:                 return "request is valid";
	case CAP_REQ_NULL:                  return "request pointer is null";
	case CAP_REQ_STRUCT_SIZE:           return "request structSize mismatch (managed/native ABI drift)";
	case CAP_REQ_TRIGGER_MODE:          return "trigger mode out of range";
	case CAP_REQ_MONO_MODE:             return "preview mono mode out of range";
	case CAP_REQ_PATTERN_ID:            return "scan pattern id out of range";
	case CAP_REQ_SAMPLE_SIZE_ZERO:      return "scan profile sample size is zero";
	case CAP_REQ_ACQ_FRAME_SIZE_ZERO:   return "scan profile acqFrameSize is zero";
	case CAP_REQ_SUB_FRAME_SIZE_ZERO:   return "scan profile subFrameSize is zero";
	case CAP_REQ_FRAME_COUNT_RANGE:     return "derived frame count exceeds the callback ABI range";
	case CAP_REQ_SUB_FRAME_COMPOSE:     return "subFrameCompose does not match the profile subFrameSize";
	case CAP_REQ_CAPTURE_LIGHT:         return "capture/preview light is not a defined LightType";
	case CAP_REQ_RESTORE_LIGHT:         return "restore light is not a defined LightType";
	case CAP_REQ_PREVIEW_ACQ_MODE:      return "preview profile must be continuous (acquisitionMode 0)";
	case CAP_REQ_PREVIEW_EXPOSURE:      return "preview camera exposure is zero";
	case CAP_REQ_CAMERA_CONFIG:         return "extended camera configuration structSize/version mismatch";
	case CAP_REQ_CAMERA_CONTRACT:       return "stated camera configuration cannot drive this acquisition";
	case CAP_REQ_SEQUENCE_MODE:         return "scan sequence mode out of range";
	case CAP_REQ_WORKING_LIGHT_MODE:    return "capture/preview light mode out of range";
	case CAP_REQ_RESTORE_LIGHT_MODE:    return "restore light mode out of range";
	default:                            return "unknown validation reason";
	}
}
