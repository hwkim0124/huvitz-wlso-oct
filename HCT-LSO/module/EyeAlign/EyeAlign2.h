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
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
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
	using namespace std;


	// Velocity-mode directions of the stage motors, as used by the exported jog
	// functions in WsoSystem/exts_device_motors.cpp (moveHeadLeft/Right/Up/Down/
	// Forward/Backward, moveSwingLeft/Right).
	/////////////////////////////////////////////////////////////////////////////////////
	constexpr int X_DIR_LEFT = 0;
	constexpr int X_DIR_RIGHT = 1;
	constexpr int Y_DIR_DOWN = 0;
	constexpr int Y_DIR_UP = 1;
	constexpr int Z_DIR_FORWARD = 0;		// toward the eye
	constexpr int Z_DIR_BACKWARD = 1;
	constexpr int SWING_DIR_LEFT = 0;
	constexpr int SWING_DIR_RIGHT = 1;

	// Velocity direction assumed to INCREASE the encoder position. Used by
	// AxisController::jogToward(). Verify on hardware; if a stage counts the other
	// way, flip here (or per axis) once the encoder convention is confirmed.
	constexpr int POSITIVE_STEP_DIR = 1;

	// Number of IrCameraId slots (RETINA, CORNEA_LOWER, CORNEA_LEFT, CORNEA_RIGHT).
	constexpr int NUM_IR_CAMERAS = static_cast<int>(IrCameraId::NUMBER_OF_ITEMS);

	// A pupil result older than this reads as "not detected" from the query exports.
	constexpr int PUPIL_RESULT_STALE_MS = 500;

	// The control loop wakes on every new camera frame; when no frame arrives it still
	// runs once per this period so timeouts, hold/resume and cancel are honoured.
	constexpr int LOOP_MAX_WAIT_MS = 100;

	// Idle (not jogging) axes are polled over USB only every Nth loop iteration to
	// keep the bus free for the camera stream. Jogging axes are polled every iteration.
	constexpr int IDLE_AXIS_POLL_PERIOD = 10;

	// Default frame geometry when no frame has been received yet.
	constexpr int DEFAULT_FRAME_WIDTH = IR_CAMERA_IMAGE_WIDTH;
	constexpr int DEFAULT_FRAME_HEIGHT = IR_CAMERA_IMAGE_HEIGHT;
}
