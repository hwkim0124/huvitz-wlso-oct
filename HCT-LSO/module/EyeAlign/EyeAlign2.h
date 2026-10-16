#pragma once

// Warning disabled caused by any class members with std container classes
// not being exported from a dll.
#pragma warning(disable:4251)


#ifdef EYEALIGN_EXPORTS
#define EYEALIGN_DLL_API		__declspec(dllexport)
#else
#define EYEALIGN_DLL_API		__declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <string>
#include <thread>
#include <vector>


#include "wso_domain.h"
#include "wso_device.h"
#include "cpp_util.h"

namespace eye_align
{
	using namespace wso_domain;
	using namespace wso_device;
	using namespace cpp_util;


	// Velocity-mode directions of the stage motors, as used by the exported jog
	// functions in WsoSystem/exts_device_motors.cpp (moveHeadLeft/Right/Up/Down/
	// Forward/Backward, moveSwingLeft/Right).
	/////////////////////////////////////////////////////////////////////////////////////
	constexpr int X_DIRECTION_LEFT = 1;
	constexpr int X_DIRECTION_RIGHT = 0;
	constexpr int Y_DIRECTION_DOWN = 1;
	constexpr int Y_DIRECTION_UP = 0;
	constexpr int Z_DIRECTION_FORWARD = 1;		// toward the eye
	constexpr int Z_DIRECTION_BACKWARD = 0;
	constexpr int SWING_DIRECTION_LEFT = 0;		// the aligner moves the swing by position;
	constexpr int SWING_DIRECTION_RIGHT = 1;		// kept for the END_OF_SWING_* event mapping

	// Velocity direction assumed to INCREASE the encoder position. Used by
	// AxisController::jogToward() and EyeAligner::controlAxis(). Verify on hardware; if a stage counts the other
	// way, flip here (or per axis) once the encoder convention is confirmed.
	constexpr int POSITIVE_STEP_DIRECTION = 1;

	// A pixel-error position move is capped so one frame shifts the pupil by at most this
	// fraction of maxJumpPx: a larger jump would make the tracker reject the next
	// detection and drop its lock.
	constexpr float MOVE_JUMP_FRACTION = 0.8f;

	// When Z arrives at a limit (search limit, minSafePosZ) with no lock on the camera
	// that should see the pupil, the phase waits this long for one before it cancels:
	// the tracker needs detectConsecutive frames after the pupil becomes visible.
	constexpr int SEARCH_LOCK_GRACE_MS = 3000;

	// Number of IrCameraId slots (RETINA, CORNEA_LOWER, CORNEA_LEFT, CORNEA_RIGHT).
	constexpr int IR_CAMERA_COUNT = static_cast<int>(IrCameraId::NUMBER_OF_ITEMS);

	// A pupil result older than this reads as "not detected" from the query exports.
	constexpr int PUPIL_RESULT_STALE_MS = 500;

	// The control loop wakes on every new camera frame; when no frame arrives it still
	// runs once per this period so timeouts, hold/resume and cancel are honoured.
	constexpr int CONTROL_LOOP_MAX_WAIT_MS = 100;

	// Pause after a position phase completes (READY_POSITION_REACHED, EYE_SIDE_POSITION_REACHED,
	// SWING_AT_TILT) before the next phase starts moving. Lets the stage settle and the
	// operator see the step; cancel, hold and fault checks keep running meanwhile.
	constexpr int PHASE_END_DWELL_MS = 500;

	// Idle (not jogging) axes are polled over USB only every Nth loop iteration to
	// keep the bus free for the camera stream. Jogging axes are polled every iteration.
	constexpr int IDLE_AXIS_POLL_PERIOD = 10;

	// Default frame geometry when no frame has been received yet.
	constexpr int DEFAULT_FRAME_WIDTH = IR_CAMERA_IMAGE_WIDTH;
	constexpr int DEFAULT_FRAME_HEIGHT = IR_CAMERA_IMAGE_HEIGHT;
}
