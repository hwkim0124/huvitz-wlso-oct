#pragma once

#include "LsoScanDefines.h"
#include <cstddef>

namespace wso_domain
{
	constexpr int LSO_CAPTURE_FRAME_ROI_MAX = 255;

	struct ColorCameraSettingParam
	{
	public:
		unsigned int roi_max_width;
		unsigned int roi_max_Height;
		unsigned int roi_x_width;
		unsigned int roi_y_height;
		unsigned int roi_x_offset;
		unsigned int roi_y_offset;
		unsigned int exposure_time;
		unsigned int acqusitionMode;
		unsigned int acqusitionFrameCount;
		unsigned int pixelFormat;
		unsigned int binningHorizontal;
		unsigned int binningVertical;
		float		 gain;
		unsigned int adcDepthIndex;

	public:
		ColorCameraSettingParam() {
			roi_max_width = 0;
			roi_max_Height = 0;
			roi_x_width = 0;
			roi_y_height = 0;
			roi_x_offset = 0;
			roi_y_offset = 0;
			exposure_time = 0;
			acqusitionMode = 0;
			acqusitionFrameCount = 0;
			pixelFormat = 0;
			binningHorizontal = 0;
			binningVertical = 0;
			gain = 0.0f;
			adcDepthIndex = 0;
		}
	};

	// --- A-1 extended camera contract -------------------------------------------------
	// The 14-field ColorCameraSettingParam above is frozen: every pre-existing export and
	// the Agent read it, so it is NOT widened. The rest of the DevKit ColorCameraConfig
	// (ExposureAuto, burst count, binning selector/modes, test pattern, throughput limit,
	// shutter mode, ISP, the five trigger nodes, the FrameStart trigger delay and the four
	// GPIO lines) travels in this separate, size- and version-stamped struct instead.
	//
	// Enum values are carried as their GenApi SYMBOLIC NAMES, not as SDK integers: the
	// numbers differ per model and per SDK release, while the names are the SFNC contract
	// the DevKit configs are already written in. An EMPTY name means "this preset does not
	// state this node" - the node is left exactly as the device has it, which is how the
	// existing start paths keep owning the nodes no preset claims.
	// Numeric fields use a negative value for the same "not stated" meaning; 0 is a legal
	// burst count / throughput / trigger delay and therefore never a sentinel.
	constexpr int LSO_CAMERA_LINE_COUNT = 4;
	constexpr int LSO_CAMERA_NAME_MAX = 32;
	constexpr int LSO_CAMERA_FIELD_MAX = 40;
	constexpr unsigned int LSO_CAMERA_CONFIG_VERSION = 1;

	// One GPIO line row (Line0..Line3). LineSelector is the row index, not a stored field,
	// so a single Line value and the four-row table cannot disagree.
	struct ColorCameraLineConfig
	{
		char mode[LSO_CAMERA_NAME_MAX];     // LineMode symbolic, "" = not stated
		char source[LSO_CAMERA_NAME_MAX];   // LineSource symbolic, "" = not stated
		int  inverter;                      // 0 / 1, negative = not stated
	};

	// Field order is fixed by the ABI: the 8-byte members come first so the layout carries
	// no implementation-defined padding and the managed mirror can be pinned by size.
	struct ColorCameraExtendedParam
	{
		unsigned int structSize;                 // sizeof(ColorCameraExtendedParam)
		unsigned int version;                    // LSO_CAMERA_CONFIG_VERSION
		long long    acquisitionBurstFrameCount; // negative = not stated
		long long    deviceLinkThroughput;       // DeviceLinkThroughputLimit, negative = not stated
		double       triggerDelayUs;             // TriggerDelay in us, negative = not stated
		int          ispEnable;                  // 0 / 1, negative = not stated
		int          reserved0;                  // explicit pad - keeps the 8-byte alignment visible
		char exposureAuto[LSO_CAMERA_NAME_MAX];
		char binningSelector[LSO_CAMERA_NAME_MAX];
		char binningHorizontalMode[LSO_CAMERA_NAME_MAX];
		char binningVerticalMode[LSO_CAMERA_NAME_MAX];
		char testPattern[LSO_CAMERA_NAME_MAX];
		char sensorShutterMode[LSO_CAMERA_NAME_MAX];
		char triggerMode[LSO_CAMERA_NAME_MAX];
		char triggerSource[LSO_CAMERA_NAME_MAX];
		char triggerActivation[LSO_CAMERA_NAME_MAX];
		char triggerSelector[LSO_CAMERA_NAME_MAX];
		char triggerOverlap[LSO_CAMERA_NAME_MAX];
		char lineSelector[LSO_CAMERA_NAME_MAX];  // which row the editor left selected
		ColorCameraLineConfig lines[LSO_CAMERA_LINE_COUNT];
	};

	// What actually happened to one node. A setter that returned without throwing is NOT
	// evidence: every write is followed by a read-back, and the three outcomes a caller
	// must be able to tell apart (node absent, write refused, value came back different)
	// have their own status.
	enum ColorCameraFieldStatus : int
	{
		CAM_FIELD_NOT_REQUESTED = 0,   // the preset does not state this node
		CAM_FIELD_APPLIED = 1,         // written and read back equal
		CAM_FIELD_UNSUPPORTED = 2,     // node or enum entry not implemented by this camera
		CAM_FIELD_LOCKED = 3,          // node exists but is not writable in this state
		CAM_FIELD_WRITE_FAILED = 4,    // the SDK refused the write
		CAM_FIELD_MISMATCH = 5,        // written, but the read-back differs
		// Written successfully, and the device chose a nearby legal value: the request was
		// outside the node's range or off its increment, so it was clamped/aligned first and
		// the read-back matches THAT target. The requested and actual values are both in the
		// row. This is not a failure - refusing a start over a benign quantisation would
		// break a configuration the camera is perfectly able to run - but it is never
		// reported as a plain success either, because the operator asked for a value the
		// device does not have.
		CAM_FIELD_ADJUSTED = 6,
	};

	struct ColorCameraFieldResult
	{
		char node[LSO_CAMERA_NAME_MAX];
		char requested[LSO_CAMERA_NAME_MAX];
		char actual[LSO_CAMERA_NAME_MAX];
		int  status;                    // ColorCameraFieldStatus
	};

	struct ColorCameraApplyResult
	{
		unsigned int structSize;
		unsigned int version;
		int fieldCount;                 // rows written into fields[]
		int failedCount;                // rows whose status is not APPLIED/NOT_REQUESTED
		ColorCameraFieldResult fields[LSO_CAMERA_FIELD_MAX];
	};

	// One entry of an enum node, as the camera reports it.
	struct ColorCameraEnumValue
	{
		char name[LSO_CAMERA_NAME_MAX];
	};

	// Why a start (Preview / Capture) refuses the stated camera configuration. Apply is
	// deliberately NOT gated by these: tuning may write any value the device accepts, and
	// only starting an acquisition needs the board's trigger contract to hold. A stated
	// value is never silently replaced - the conflict is reported with the field name.
	enum CameraStartContract : int
	{
		CAM_START_OK = 0,
		CAM_START_STRUCT_SIZE = 1,
		CAM_START_VERSION = 2,
		CAM_START_TRIGGER_MODE = 3,
		CAM_START_TRIGGER_SOURCE = 4,
		CAM_START_TRIGGER_ACTIVATION = 5,
		CAM_START_TRIGGER_SELECTOR = 6,
		CAM_START_TRIGGER_OVERLAP = 7,
		CAM_START_TRIGGER_LINE_MODE = 8,
		CAM_START_EXPOSURE_AUTO = 9,
		CAM_START_BURST_COUNT = 10,
		CAM_START_LINE_SELECTOR = 11,
		CAM_START_LINE_NAME = 12,
		// The trigger group is all-or-nothing: a preset either owns TriggerSource,
		// TriggerSelector, TriggerActivation and TriggerOverlap together (and then the start
		// writes none of them), or states none of them (and then the start's own hardware
		// trigger setup owns all of them). Stating only some would mean mixing the two, and
		// the constants would silently replace whatever was left unstated.
		CAM_START_TRIGGER_GROUP = 13,
	};

	// The board wiring this instrument's hardware-triggered paths depend on. They are the
	// values setupHwTriggerSetting() has always written; naming them here is what lets a
	// preset state them explicitly instead of having them overwritten after the fact.
	constexpr const char* LSO_CAMERA_HW_TRIGGER_MODE = "On";
	constexpr const char* LSO_CAMERA_HW_TRIGGER_SOURCE = "Line3";
	constexpr const char* LSO_CAMERA_HW_TRIGGER_ACTIVATION = "RisingEdge";
	constexpr const char* LSO_CAMERA_HW_TRIGGER_SELECTOR = "FrameStart";
	constexpr const char* LSO_CAMERA_HW_TRIGGER_OVERLAP = "ReadOut";
	constexpr const char* LSO_CAMERA_HW_TRIGGER_LINE_MODE = "Input";
	constexpr int LSO_CAMERA_HW_TRIGGER_LINE_INDEX = 3;   // Line3 carries the board trigger

	// Which struct getLsoCameraStructSize() is being asked about. A managed mirror can then
	// compare Marshal.SizeOf against the shipped DLL, not against a comment.
	enum LsoCameraStructId : int
	{
		LSO_STRUCT_CAMERA_SETTING = 0,
		LSO_STRUCT_CAMERA_EXTENDED = 1,
		LSO_STRUCT_CAMERA_LINE = 2,
		LSO_STRUCT_CAMERA_FIELD_RESULT = 3,
		LSO_STRUCT_CAMERA_APPLY_RESULT = 4,
		LSO_STRUCT_FUNDUS_CAPTURE_REQUEST = 5,
		LSO_STRUCT_FUNDUS_PREVIEW_REQUEST = 6,
	};

	// ABI sizes, pinned so the managed mirror can assert the same numbers without loading
	// the DLL. A field added or reordered breaks these here and in TestCameraPreset.
	constexpr unsigned int LSO_CAMERA_LINE_CONFIG_SIZE = 68;
	constexpr unsigned int LSO_CAMERA_EXTENDED_PARAM_SIZE = 696;
	constexpr unsigned int LSO_CAMERA_FIELD_RESULT_SIZE = 100;
	constexpr unsigned int LSO_CAMERA_APPLY_RESULT_SIZE = 4016;

	static_assert(sizeof(ColorCameraLineConfig) == LSO_CAMERA_LINE_CONFIG_SIZE,
		"ColorCameraLineConfig ABI changed - update the managed mirror and the pinned size");
	static_assert(sizeof(ColorCameraExtendedParam) == LSO_CAMERA_EXTENDED_PARAM_SIZE,
		"ColorCameraExtendedParam ABI changed - update the managed mirror and the pinned size");
	static_assert(sizeof(ColorCameraFieldResult) == LSO_CAMERA_FIELD_RESULT_SIZE,
		"ColorCameraFieldResult ABI changed - update the managed mirror and the pinned size");
	static_assert(sizeof(ColorCameraApplyResult) == LSO_CAMERA_APPLY_RESULT_SIZE,
		"ColorCameraApplyResult ABI changed - update the managed mirror and the pinned size");

	// Result/stage codes for the color-camera capture one-call.
	// start return value: OK / BUSY / PARAM / SCANNER (pre-acquisition failures;
	// no completion callback follows a non-OK start).
	// completion callback: OK / CAMERA / CANCELLED / FAULT (how the acquisition ended).
	enum ColorCaptureStage : int
	{
		CAP_STAGE_OK = 0,
		CAP_STAGE_BUSY = 1,
		CAP_STAGE_PARAM = 2,
		CAP_STAGE_SCANNER = 3,
		CAP_STAGE_CAMERA = 4,
		CAP_STAGE_CANCELLED = 5,
		CAP_STAGE_FAULT = 9,
	};

	enum ColorCaptureTriggerMode : int
	{
		COLOR_CAPTURE_NORMAL = 0,
		COLOR_CAPTURE_HW_TRIGGER = 1,
	};

	// Board-global scan sequence values accepted by the fundus one-call requests.
	// These mirror the DevKit command and WsoLsoScan.LsoScanSequenceMode.
	enum LsoFundusScanSequenceMode : int
	{
		LSO_SCAN_TRIG_FIRST = 0,
		LSO_SCAN_GALVANO_FIRST = 1,
	};

	// LED drive mode values accepted by the fundus one-call requests.
	enum LsoFundusLightMode : int
	{
		FUNDUS_LIGHT_TRIGGER = 0,
		FUNDUS_LIGHT_CONTINUOUS = 1,
	};

	// Steps of the fundus capture sequence, in execution order. Also the step id the
	// diagnostic barrier takes, so a test can pause the worker exactly where a race lives.
	enum LsoCaptureStep : int
	{
		CAP_STEP_NONE = -1,
		CAP_STEP_VALIDATE = 0,
		CAP_STEP_APPLY_CAMERA = 1,
		CAP_STEP_UPLOAD_PROFILE = 2,
		CAP_STEP_CAPTURE_LIGHT = 3,
		CAP_STEP_BEGIN_CAMERA = 4,
		CAP_STEP_WAIT_ARM = 5,
		CAP_STEP_START_SCANNER = 6,
		CAP_STEP_ACQUIRE = 7,
		CAP_STEP_TEARDOWN = 8,
	};

	// One reason per rejection gate. The gates are distinguishable by value (and by the
	// error text derived from it), so a new gate cannot silently borrow another's identity
	// and a device-free test can tell exactly which rule fired.
	enum CaptureValidationReason : int
	{
		CAP_REQ_VALID = 0,
		CAP_REQ_NULL = 1,
		CAP_REQ_STRUCT_SIZE = 2,
		CAP_REQ_TRIGGER_MODE = 3,
		CAP_REQ_MONO_MODE = 4,
		CAP_REQ_PATTERN_ID = 5,
		CAP_REQ_SAMPLE_SIZE_ZERO = 6,
		CAP_REQ_ACQ_FRAME_SIZE_ZERO = 7,
		CAP_REQ_SUB_FRAME_SIZE_ZERO = 8,
		CAP_REQ_FRAME_COUNT_RANGE = 9,
		CAP_REQ_SUB_FRAME_COMPOSE = 10,
		CAP_REQ_CAPTURE_LIGHT = 11,
		CAP_REQ_RESTORE_LIGHT = 12,
		CAP_REQ_PREVIEW_ACQ_MODE = 13,
		CAP_REQ_PREVIEW_EXPOSURE = 14,
		// The extended camera contract itself is malformed (size/version drift).
		CAP_REQ_CAMERA_CONFIG = 15,
		// The stated camera configuration cannot drive this acquisition. Which field
		// conflicts is a CameraStartContract code - validateLsoCameraStartContract()
		// returns it, and the last-capture-error text names it.
		CAP_REQ_CAMERA_CONTRACT = 16,
		CAP_REQ_SEQUENCE_MODE = 17,
		CAP_REQ_WORKING_LIGHT_MODE = 18,
		CAP_REQ_RESTORE_LIGHT_MODE = 19,
	};

	// Request for the fundus capture one-call. The whole device sequence is owned by the
	// callee, so its inputs are the whole sequence: scan profile (always uploaded before the
	// board is started), camera preset, illumination and the illumination restored on every
	// exit path. There are no "ignore this field" sentinels and no optional-step flags: one
	// request means one full sequence.
	// structSize is the ABI drift guard - a managed mirror that gained or lost a field is
	// rejected instead of being read past.
	// camera.acqusitionMode / camera.acqusitionFrameCount are NOT inputs: the frame count is
	// owned by profile.acqFrameSize * profile.subFrameSize and the callee overwrites both
	// camera fields with the derived values before applying the preset.
	struct LsoFundusCaptureRequest
	{
		unsigned int   structSize = 0;              // sizeof(LsoFundusCaptureRequest)
		int            triggerMode = 0;             // ColorCaptureTriggerMode
		int            scanPatternId = 0;           // 0 .. LSO_SCANNER_PATTERN_NUM_MAX-1
		int            sequenceMode = 0;            // LsoFundusScanSequenceMode
		short          preMoveOffset = 0;            // GALVANO_FIRST only; normalized to 0 otherwise
		unsigned short preMoveDelayUs = 0;           // GALVANO_FIRST only; normalized to 0 otherwise
		int            captureLight = 0;            // LightType applied for the capture
		int            captureLightMode = 0;        // LsoFundusLightMode applied before turn-on
		// LightType left on after the sequence, on success, cancel and failure alike. The
		// working light is always switched off first, so setting this equal to captureLight
		// means "leave the illumination off".
		int            restoreLight = 0;
		int            restoreLightMode = 0;        // applied before a different restore light turns on
		// Intensity is board state that outlives one capture, and the device reports it from
		// an in-process cache that starts at 0 (LED_*_INIT_VALUE = 0), so 0 here means
		// "leave the intensity the board already has" - only a non-zero value is written.
		unsigned short captureLightIntensity = 0;
		unsigned short restoreLightIntensity = 0;
		unsigned int   subFrameCompose = 0;         // 0 = per-frame callback, else == profile.subFrameSize
		LsoScannerControlParam  profile;            // uploaded to the board; owns the frame count
		ColorCameraSettingParam camera;
		// The rest of the A-1 camera configuration. Applied with `camera` by the same
		// checked apply, in node-dependency order, BEFORE the camera is armed - and the
		// nodes it states are not written again afterwards, so a stated trigger/GPIO value
		// survives the start instead of being replaced by setupHwTriggerSetting()'s
		// constants. Fields it leaves unstated stay owned by that start path.
		ColorCameraExtendedParam cameraExt;
	};

	// Request for the fundus preview one-call. Same ownership rule as the capture request:
	// the profile is always uploaded, illumination is explicit, and the preview is continuous
	// by contract (a fixed-frame profile is rejected rather than silently overwritten).
	//
	// The preview drives the camera from the board trigger, exactly like the DevKit preview
	// path (BfsCameraConfigPreview.json: TriggerMode On / FrameStart / Line3 / RisingEdge /
	// ReadOut / TriggerDelay 288, Mono8 + 2x2 binning, its OWN exposure and gain - 1013us
	// and 27.0457dB there, which are NOT the capture config's values). That is not an
	// optional mode: one request means the whole triggered sequence, so there is no
	// trigger-on/off flag here. exposureTimeUs and gain are the camera tunables the preview
	// installs and puts back on stop; the geometry comes from the camera preset below
	// (monoMode only decides the fallback geometry when the preset carries no ROI).
	struct LsoFundusPreviewRequest
	{
		unsigned int   structSize = 0;              // sizeof(LsoFundusPreviewRequest)
		int            monoMode = 0;                // 0 = SensorMono8, 1 = BayerSwGray
		int            scanPatternId = 0;           // 0 .. LSO_SCANNER_PATTERN_NUM_MAX-1
		int            sequenceMode = 0;            // LsoFundusScanSequenceMode
		short          preMoveOffset = 0;            // GALVANO_FIRST only; normalized to 0 otherwise
		unsigned short preMoveDelayUs = 0;           // GALVANO_FIRST only; normalized to 0 otherwise
		int            previewLight = 0;            // LightType applied for the preview
		int            previewLightMode = 0;        // LsoFundusLightMode applied before turn-on
		int            restoreLight = 0;            // LightType restored on stop
		int            restoreLightMode = 0;        // applied before a different restore light turns on
		unsigned short previewLightIntensity = 0;   // 0 = leave the configured intensity
		unsigned short restoreLightIntensity = 0;   // 0 = leave the configured intensity
		// Camera exposure for the preview. Rejected while zero: unlike a light intensity,
		// which is board state the request may legitimately leave alone, a zero exposure is
		// never a value the camera can run - it is a caller that forgot to fill the field.
		unsigned int   exposureTimeUs = 0;
		float          gain = 0.0f;                 // camera gain in dB; <= 0 keeps the current gain
		// FrameStart trigger delay in microseconds (DevKit preview/capture configs: 288).
		// 0 is a legal delay, so it is not a "leave it alone" sentinel - the previous value
		// is saved and put back on stop either way.
		unsigned int   triggerDelayUs = 0;
		LsoScannerControlParam profile;             // acquisitionMode must be 0 (continuous)
		// Camera preset the preview installs: pixel format, binning, ROI, acquisition mode
		// and ADC depth are taken from here instead of being hardcoded, so a Preview preset
		// edited in the Toolkit is what the camera actually runs.
		// exposure_time / gain are NOT read from here: exposureTimeUs and gain above are the
		// validated fields (a zero exposure is a rejected request), and one owner per value
		// is what keeps them from disagreeing.
		// roi_max_* are device-reported and ignored on input.
		// A zero-sized ROI means "this preset carries no window": the preview then installs
		// the monoMode full-FOV geometry, exactly as it did before presets existed.
		ColorCameraSettingParam camera;
		// Same contract as the capture request: the preview installs the stated trigger and
		// GPIO nodes itself, and startTriggeredLiveMode() then only begins the acquisition
		// and verifies arming instead of re-writing the hardcoded trigger constants.
		//
		// triggerDelayUs above stays the validated top-level field and the Toolkit leaves
		// cameraExt.triggerDelayUs unstated, so one value never has two owners. The callee
		// folds the top-level value into this block before applying it
		// (capture::resolvePreviewCameraConfig): with the trigger group owned by the preset
		// the camera start only arms, so the block is what actually writes the node.
		ColorCameraExtendedParam cameraExt;
	};

	constexpr unsigned int LSO_FUNDUS_CAPTURE_REQUEST_SIZE = 832;
	constexpr unsigned int LSO_FUNDUS_PREVIEW_REQUEST_SIZE = 840;

	static_assert(sizeof(LsoFundusCaptureRequest) == LSO_FUNDUS_CAPTURE_REQUEST_SIZE,
		"LsoFundusCaptureRequest ABI changed - update the managed mirror and pinned offsets");
	static_assert(sizeof(LsoFundusPreviewRequest) == LSO_FUNDUS_PREVIEW_REQUEST_SIZE,
		"LsoFundusPreviewRequest ABI changed - update the managed mirror and pinned offsets");
	static_assert(offsetof(LsoFundusCaptureRequest, sequenceMode) == 12);
	static_assert(offsetof(LsoFundusCaptureRequest, preMoveOffset) == 16);
	static_assert(offsetof(LsoFundusCaptureRequest, preMoveDelayUs) == 18);
	static_assert(offsetof(LsoFundusCaptureRequest, captureLightMode) == 24);
	static_assert(offsetof(LsoFundusCaptureRequest, restoreLightMode) == 32);
	static_assert(offsetof(LsoFundusCaptureRequest, profile) == 44);
	static_assert(offsetof(LsoFundusCaptureRequest, cameraExt) == 136);
	static_assert(offsetof(LsoFundusPreviewRequest, sequenceMode) == 12);
	static_assert(offsetof(LsoFundusPreviewRequest, preMoveOffset) == 16);
	static_assert(offsetof(LsoFundusPreviewRequest, preMoveDelayUs) == 18);
	static_assert(offsetof(LsoFundusPreviewRequest, previewLightMode) == 24);
	static_assert(offsetof(LsoFundusPreviewRequest, restoreLightMode) == 32);
	static_assert(offsetof(LsoFundusPreviewRequest, profile) == 52);
	static_assert(offsetof(LsoFundusPreviewRequest, cameraExt) == 144);

	struct LsoCaptureFrameROI
	{
		int width = 0;
		int height = 0;
		int offsetX = 0;
		int offsetY = 0;
		int startY = 0;
		int endY = 0;
	};

	// Color Camera, Rolling Shutter, SW Trigger, Overlab
	struct LsoCaptureFrameRollSwTrigOverlap
	{
		int frameWidth = 0;
		int frameHeight = 0;
		int offsetX = 0;
		int offsetY = 0;
		int overlapFrameCount = 0;
		int triggerIntervalMs = 0;
		int brightness = 0;
	};

	//Image Display Setting
	struct LsoImageMask
	{
		int irRadius = 0;
		int colorRadius = 0;
	};

	//Image Adjust Setting
	struct LsoImageAdjust
	{
		int brightness = 0;
	};

	struct LsoCaptureFrameROIPreset
	{
		int frameCount = 0;
		LsoCaptureFrameROI frameRois[LSO_CAPTURE_FRAME_ROI_MAX];
	};

	struct LsoCaptureFrameSeqROIPreset
	{
		int frameCount = 0;
		LsoCaptureFrameROI frameRois[LSO_CAPTURE_FRAME_ROI_MAX];
	};

	struct LsoCaptureFrameOffsetROIPreset
	{
		int frameCount = 0;
		int roiWidth = 0;
		int roiHeight = 0;
		LsoCaptureFrameROI frameRois[LSO_CAPTURE_FRAME_ROI_MAX];
	};

	struct LsoCaptureFrameRollSwTrigOverlapPreset
	{
		LsoCaptureFrameRollSwTrigOverlap overlap;
	};

	struct LsoImageAdjustPreset
	{
		LsoImageAdjust adjust;
	};

	struct LsoImageMaskPreset
	{
		LsoImageMask mask;
	};
}
