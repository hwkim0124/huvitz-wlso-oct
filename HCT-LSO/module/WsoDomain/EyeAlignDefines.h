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
		SINGLE = 1,		// ready Z -> eye-side X -> swing tilt -> search -> centre -> follow, eye side given at start
		FULL = 2		// same as SINGLE, always starts with the right eye (OD)
	};

	enum class EyeAlignPhase : int
	{
		IDLE = 0,
		TO_READY_Z,			// P1  Z -> readyPosZ
		TO_EYE_X,			// P2  X -> eyePosX_OD | eyePosX_OS
		SWING_TO_TILT,		// 1   swing -> swingTiltPos_OD | swingTiltPos_OS
		SEARCHING,			// 2   Z forward to searchLimitPosZ, waiting for the side-camera pupil
		CENTERING_XY,		// 3   Z stopped, X/Y centre the side-camera pupil
		APPROACHING,		// 4   Z forward, X/Y tracking, waiting for the lower-camera pupil
		CENTERING_Z,		// 5   Z centres the lower-camera pupil, X/Y tracking
		FOCUSED,			// 6/7 all centred, still following (MANUAL: centred by the operator)
		MONITORING,			// MANUAL only: detecting, not yet centred on both images, no motion
		HELD,				// holdEyeAlignment(): motors stopped, detection and events continue, no motion
		CANCELED,
		FAILED
	};

	// Reason carried by ALIGN_CANCELLED / ALIGN_FAILED and returned by start refusals.
	enum class EyeAlignResult : int
	{
		NONE = 0,
		// cancel reasons
		USER_CANCEL,
		EYE_NOT_FOUND,
		PUPIL_LOST,
		// start refusals
		NOT_INITIALIZED,
		PREVIEW_NOT_RUNNING,
		BUSY,
		MOTOR_NOT_READY,
		// failures
		MOTOR_MOVE_FAILED,
		LIMIT_REACHED,
		EYE_TOO_CLOSE,
		TIMEOUT,
		INTERNAL_ERROR
	};

	enum class EyeAlignEvent : int
	{
		NONE = 0,
		// lifecycle (arg = EyeSide)
		FULL_ALIGN_STARTED,
		SINGLE_ALIGN_STARTED,
		MANUAL_ALIGN_STARTED,
		// terminal (arg = EyeAlignResult); fired exactly once per run, always last
		ALIGN_CANCELLED,
		ALIGN_FAILED,
		// sequence progress
		READY_POSITION_REACHED,		// P1 done
		EYE_SIDE_POSITION_REACHED,	// P2 done (arg = EyeSide)
		SWING_AT_TILT,				// step 1 done (arg = EyeSide)
		SEARCH_STARTED,				// step 2: Z advancing toward searchLimitPosZ
		EYE_FOUND,					// side pupil seen detectConsecutive frames (arg = IrCameraId)
		EYE_NOT_FOUND,				// search limit reached with no find (arg = IrCameraId)
		XY_CENTERED,				// step 3 done
		APPROACH_STARTED,			// step 4: Z advancing with X/Y tracking
		LOWER_EYE_FOUND,			// step 5: lower pupil seen detectConsecutive frames (arg = IrCameraId)
		FOCUS_ALIGNED,				// step 6: entered FOCUSED (raised on every entry)
		FOCUS_LOST,					// step 7: left FOCUSED (raised on every exit)
		EYE_LOST,					// pupil missing lostFramesToStop frames -> all motors stopped (arg = IrCameraId)
		ALIGN_HELD,					// holdEyeAlignment() accepted
		ALIGN_RESUMED,				// resumeEyeAlignment(): following continues
		EYE_TOO_CLOSE,				// Z reached minSafePosZ before centring
		// stage limits (arg = MotorType)
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
		PHASE_TIMEOUT				// (arg = EyeAlignPhase)
	};

	// Per-camera geometry of the pupil target. Deliberately holds no detection tuning
	// (thresholds, radii, ...): those arrive with the detector algorithm in a struct of
	// their own.
	struct PupilDetectParam
	{
		int32_t targetX = -1;		// desired pupil centre in the image, -1 = image centre
		int32_t targetY = -1;
		int32_t signH = +1;			// horizontal image error -> motor direction sign (+1 / -1)
		int32_t signV = +1;			// vertical image error -> motor direction sign (+1 / -1)
	};

	struct EyeAlignConfigParam
	{
		// positions (motor steps)
		int32_t readyPosZ = 0;				// P1: safe distance from the eye
		int32_t swingTiltPos_OD = 0;		// step 1 tilt for the right eye
		int32_t swingTiltPos_OS = 0;		// step 1 tilt for the left eye
		int32_t eyePosX_OD = 0;				// P2 X position for the right eye
		int32_t eyePosX_OS = 0;				// P2 X position for the left eye
		int32_t searchLimitPosZ = 0;		// step 2 stops here when no pupil was found
		int32_t minSafePosZ = 0;			// steps 4-5 never go past this (EYE_TOO_CLOSE)
		int32_t posToleranceSteps = 20;		// a jog toward a target position stops within this band

		// pixel control
		float tolerancePx = 6.0f;			// "centred" for X/Y (side camera)
		float toleranceZPx = 8.0f;			// "centred" for Z (lower camera)
		float startDeadbandPx = 10.0f;		// start a jog when |error| exceeds this
		float stopDeadbandPx = 4.0f;		// stop the jog when |error| falls below this (hysteresis)

		// frame counting (the control loop runs once per camera frame)
		int32_t detectConsecutive = 3;		// consecutive detections to declare EYE_FOUND / LOWER_EYE_FOUND
		int32_t lostFramesToStop = 5;		// frames without pupil -> stop all motors (EYE_LOST)
		int32_t lostFramesToCancel = 300;	// frames without pupil -> ALIGN_CANCELLED(PUPIL_LOST), ~10 s at 30 fps
		int32_t settleFrames = 5;			// frames centred and stopped -> FOCUSED
		float maxJumpPx = 60.0f;			// tracker rejects a centre jump larger than this
		float smoothingAlpha = 0.5f;		// EMA weight of the newest detection

		// timeouts (ms) of the position phases
		int32_t readyTimeoutMs = 10000;
		int32_t swingTimeoutMs = 10000;
		int32_t eyeXTimeoutMs = 10000;
		int32_t searchTimeoutMs = 20000;

		int32_t backOffOnEnd = 1;			// on cancel/fail jog Z back to readyPosZ (swing left as is)

		PupilDetectParam detLeft;
		PupilDetectParam detRight;
		PupilDetectParam detLower;
	};

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
		float errX = 0.0f;			// signed pixel errors after sign mapping
		float errY = 0.0f;
		float errZ = 0.0f;
		int32_t lostFrames = 0;		// consecutive frames without a locked pupil
		int32_t elapsedMs = 0;		// since the run started
	};
}
