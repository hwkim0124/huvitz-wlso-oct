#pragma once

#include <cstdint>

// Shared types of the EyeAlign module (auto alignment of the optical head to the eye).
// Every struct here crosses the P/Invoke boundary: keep all members int32 / float so the
// layout is blittable and matches the C# mirror in WsoNativeLib/EyeAlignDefines.cs
// (LayoutKind.Sequential, no pack pragma, same field order).

namespace wso_domain
{
	enum class EyeAlignMode : int
	{
		MANUAL = 0,		// detector only, operator moves the head; FOCUS_ALIGNED when centred on both cameras
		AUTO = 1,		// ready Z -> eye-side X -> swing tilt -> search -> centre -> follow, eye side given at start
		FULL = 2		// same as AUTO, always starts with the right eye (OD)
	};

	enum class EyeAlignPhase : int
	{
		IDLE = 0,
		TO_READY_Z,			// 1   Z -> readyPosZ
		TO_EYE_X,			// 2   X -> eyePosX_OD | eyePosX_OS
		SWING_TO_TILT,		// 3   swing -> swingTiltPos_OD | swingTiltPos_OS
		SEARCHING,			// 4a  Z forward to searchLimitPosZ, the side camera watched for the pupil
		CENTERING_Y,		// 4b  Z stopped, Y centres the side-camera pupil vertically
		APPROACHING,		// 5   Z forward until the lower-camera pupil sits on its vertical target
		CENTERING_X,		// 6   X centres the lower-camera pupil horizontally
		CENTERING_Z,		// 7   Z centres the lower-camera pupil vertically
		FOCUSED,			// 8   lower pupil centred, no motion (MANUAL: centred by the operator)
		MONITORING,			// MANUAL only: detecting, not yet centred on both images, no motion
		HELD,				// holdEyeAlignment(): motors stopped, detection and events continue, no motion
		CANCELED,
		FAILED
	};

	// Reason carried by ALIGN_CANCELLED / ALIGN_FAILED and returned by start refusals.
	enum class EyeAlignResult : int
	{
		NONE = 0,
		// cancel reasons (the head backs off to readyPosZ, except on USER_CANCEL)
		USER_CANCEL,
		SIDE_EYE_NOT_FOUND,				// side camera: no pupil at the search limit
		PUPIL_LOST,
		LOWER_EYE_NOT_FOUND,		// lower camera: no pupil by the time Z is at minSafePosZ
		ALIGN_TIMEOUT,				// alignTimeoutMs elapsed before the first FOCUS_ALIGNED
		// start refusals
		NOT_INITIALIZED,
		PREVIEW_NOT_RUNNING,
		BUSY,
		MOTOR_NOT_READY,
		// failures
		MOTOR_MOVE_FAILED,
		LIMIT_REACHED,				// a position phase (ready Z, eye X, swing) stopped at an end switch short of its target
		EYE_TOO_CLOSE,				// not raised any more (Z holds at minSafePosZ instead); kept for the C# mirror
		TIMEOUT,
		INTERNAL_ERROR
	};

	enum class EyeAlignEvent : int
	{
		NONE = 0,
		// lifecycle (arg = EyeSide)
		FULL_ALIGN_STARTED,
		AUTO_ALIGN_STARTED,
		MANUAL_ALIGN_STARTED,
		// terminal (arg = EyeAlignResult); fired exactly once per run, always last
		ALIGN_CANCELLED,
		ALIGN_FAILED,
		// sequence progress
		READY_POSITION_REACHED,		// step 1 done
		EYE_SIDE_POSITION_REACHED,	// step 2 done (arg = EyeSide)
		SWING_AT_TILT,				// step 3 done (arg = EyeSide)
		SEARCH_STARTED,				// step 4a: Z advancing toward searchLimitPosZ
		SIDE_EYE_FOUND,					// side pupil seen detectConsecutive frames (arg = IrCameraId)
		SIDE_EYE_NOT_FOUND,				// search limit reached with no side lock (arg = IrCameraId)
		Y_CENTERED,					// step 4b done
		APPROACH_STARTED,			// step 5: Z advancing toward the lower-camera pupil
		LOWER_EYE_FOUND,			// lower pupil seen detectConsecutive frames (arg = IrCameraId)
		LOWER_EYE_NOT_FOUND,		// Z at minSafePosZ with no lower lock (arg = IrCameraId)
		X_CENTERED,					// step 6 done
		Z_CENTERED,					// step 7 done
		FOCUS_ALIGNED,				// step 8: entered FOCUSED (raised on every entry)
		FOCUS_LOST,					// left FOCUSED (raised on every exit)
		EYE_LOST,					// pupil missing lostFramesToStop frames -> all motors stopped (arg = IrCameraId)
		ALIGN_HELD,					// holdEyeAlignment() accepted
		ALIGN_RESUMED,				// resumeEyeAlignment(): following continues
		EYE_TOO_CLOSE,				// Z at minSafePosZ: raised once per excursion, Z holds there, run continues
		// stage limits (arg = MotorType): raised once per excursion, the run continues
		// (except a position phase whose target lies beyond the switch: LIMIT_REACHED)
		END_OF_LEFT,
		END_OF_RIGHT,
		END_OF_TOP,
		END_OF_BOTTOM,
		END_OF_FORWARD,
		END_OF_BACKWARD,
		END_OF_SWING_LEFT,
		END_OF_SWING_RIGHT,
		// errors
		MOTOR_MOVE_FAILED,			// (arg = MotorType)
		PHASE_TIMEOUT,				// a position phase overran its timeout (arg = EyeAlignPhase)
		ALIGN_TIMEOUT				// alignTimeoutMs elapsed before the first FOCUS_ALIGNED (arg = EyeAlignPhase)
	};

	// Per-camera geometry of the pupil target. Deliberately holds no detection tuning:
	// the pupil-centre network settings (model path, presence threshold) live in the
	// native-only PupilModelConfig (EyeAlignModelConfig.h).
	struct PupilDetectParam
	{
		int32_t targetX = -1;		// desired pupil centre in the image, -1 = image centre
		int32_t targetY = -1;
		int32_t signH = +1;			// horizontal image error -> motor direction sign (+1 / -1)
		int32_t signV = +1;			// vertical image error -> motor direction sign (+1 / -1)
	};

	struct EyeAlignConfigParam
	{
		// positions (motor steps); Z counts up toward the eye
		int32_t readyPosZ = -12000;			// step 1: safe distance from the eye; Z never retreats past it during a run
		int32_t swingTiltPos_OD = -1500;		// step 3 tilt for the right eye
		int32_t swingTiltPos_OS = +1500;		// step 3 tilt for the left eye
		int32_t eyePosX_OD = +15000;			// step 2 X position for the right eye
		int32_t eyePosX_OS = -15000;			// step 2 X position for the left eye
		int32_t searchLimitPosZ = -4000;	// step 4a always advances Z to here; no side lock there cancels (EYE_NOT_FOUND)
		int32_t minSafePosZ = 0;			// Z never goes forward past this (moves are clipped); EYE_TOO_CLOSE once per excursion
		int32_t posToleranceSteps = 100;		// a position move or a jog toward a target counts as arrived within this band
		int32_t stepZ = 500;				// steps 4a and 5 advance Z by this many steps per frame while no pupil is locked

		// pixel control: one tolerance per centring step
		float toleranceYPx = 24.0f;			// step 4b: side-camera pupil vertically on target
		float approachTolZPx = 18.0f;		// step 5: lower-camera pupil vertically on target (ends the approach)
		float toleranceXPx = 6.0f;			// steps 6 and 8: lower-camera pupil horizontally on target
		float toleranceZPx = 8.0f;			// steps 7 and 8: lower-camera pupil vertically on target
		float startDeadbandPx = 10.0f;		// MANUAL: |error| above this counts as drifted from the target (keep >= the tolerances)
		float stepsPerPxX = 10.0f;			// X/Y/Z move by position: offset steps = error px * ratio, at most
		float stepsPerPxY = 20.0f;			// maxJumpPx * 0.8 px worth of steps per frame so the tracker keeps its lock
		float stepsPerPxZ = 10.0f;

		// frame counting (the control loop runs once per camera frame)
		int32_t detectConsecutive = 3;		// consecutive detections to declare EYE_FOUND / LOWER_EYE_FOUND
		int32_t lostFramesToStop = 5;		// frames without pupil -> stop all motors (EYE_LOST)
		int32_t lostFramesToCancel = 300;	// frames without pupil -> ALIGN_CANCELLED(PUPIL_LOST), ~10 s at 30 fps
		int32_t settleFrames = 2;			// frames centred and stopped -> FOCUSED
		float maxJumpPx = 60.0f;			// tracker rejects a centre jump larger than this
		float smoothingAlpha = 0.5f;		// EMA weight of the newest detection

		// timeouts (ms)
		int32_t readyTimeoutMs = 10000;		// position phases (steps 1..3)
		int32_t swingTimeoutMs = 10000;		// swing moves by position; its own wait (~10 s) bounds the block
		int32_t eyeXTimeoutMs = 10000;
		int32_t searchTimeoutMs = 20000;	// step 4a safety net for a stalled Z (fires before alignTimeoutMs with the better diagnosis)
		int32_t alignTimeoutMs = 30000;		// whole sequence up to the first FOCUS_ALIGNED -> ALIGN_CANCELLED(ALIGN_TIMEOUT); held time excluded; 0 = no limit

		int32_t backOffOnEnd = 1;			// on an automatic cancel or a failure jog Z back to readyPosZ (swing left as is)
		int32_t irLedIntensity = 3;			// intensity of both cornea IR LEDs (0..255), turned on when a run starts

		// side cameras: only targetY / signV are used (step 4b). signV = -1: the pupil
		// below the target in the side image drives the head UP (Y_DIRECTION_UP).
		PupilDetectParam detLeft{ -1, -1, -1, -1 };
		PupilDetectParam detRight{ -1, -1, +1, -1 };
		PupilDetectParam detLower{ -1, -1, +1, +1 };			// lower camera: targetX / signH drive X, targetY / signV drive Z
	};
	// Mirrored by WsoNativeLib/EyeAlignDefines.cs; the C# side checks Marshal.SizeOf at
	// run time (getEyeAlignConfigStructSize). Update both when the layout changes.
	static_assert(sizeof(PupilDetectParam) == 16, "PupilDetectParam layout changed; update the C# mirror");
	static_assert(sizeof(EyeAlignConfigParam) == 30 * 4 + 3 * sizeof(PupilDetectParam), "EyeAlignConfigParam layout changed; update the C# mirror");

	struct PupilDetectResult
	{
		int32_t camId = 0;			// IrCameraId
		int32_t found = 0;			// 0 / 1
		float cx = 0.0f;			// pupil centre (image pixels)
		float cy = 0.0f;
		float radius = 0.0f;		// pupil radius (image pixels)
		float score = 0.0f;			// detector confidence 0..1
		int32_t frameSeq = 0;		// frame sequence number the result belongs to
	};

	struct EyeAlignStatus
	{
		int32_t mode = 0;			// EyeAlignMode
		int32_t phase = 0;			// EyeAlignPhase
		int32_t result = 0;			// EyeAlignResult
		int32_t eyeSide = 0;		// EyeSide
		int32_t sideCamId = 0;		// IrCameraId of the side camera in use
		int32_t posX = 0;
		int32_t posY = 0;
		int32_t posZ = 0;
		int32_t posSwing = 0;
		PupilDetectResult side;
		PupilDetectResult lower;
		float errX = 0.0f;			// signed pixel errors after sign mapping: X and Z from the lower camera,
		float errY = 0.0f;			// Y from the side camera
		float errZ = 0.0f;
		int32_t lostFrames = 0;		// consecutive frames without a detection on the camera the lost-pupil rule watches
		int32_t elapsedMs = 0;		// since the run started
	};
}
