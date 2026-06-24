#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)


#ifdef OCTSCAN_EXPORTS
#define OCTSCAN_DLL_API		__declspec(dllexport)
#else
#define OCTSCAN_DLL_API		__declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>


#include "wso_domain.h"
#include "wso_device.h"
#include "cpp_util.h"

namespace oct_scan
{
	using namespace wso_domain;
	using namespace wso_device;
	using namespace cpp_util;
	using namespace std;


	// Scanner 
	/////////////////////////////////////////////////////////////////////////////////////
	constexpr int SCANNER_GRAB_ERROR_COUNT_DELAY = 5;
	constexpr int SCANNER_GRAB_ERROR_COUNT_LIMIT = (SCANNER_GRAB_ERROR_COUNT_DELAY * 5);
	constexpr int SCANNER_GRAB_ERROR_DELAY_TIME = 50;

	constexpr int SCANNER_WAIT_PREVIEW_PROCESSED_COUNT_MAX = 500;
	constexpr int SCANNER_WAIT_PREVIEW_PROCESSED_DELAY = 20;
	constexpr int SCANNER_WAIT_MEASURE_PROCESSED_COUNT_MAX = 1000; // 500;
	constexpr int SCANNER_WAIT_MEASURE_PROCESSED_DELAY = 25; // 20;
	constexpr int SCANNER_WAIT_MEASURE_PROCESSED_DELAY_NEXT = 100;

	// Trigger Step by Scan speed. 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr float TRIGGER_TIME_STEP_AT_SLOWER_SPEED = 66.6f; // 78.0f;
	constexpr float TRIGGER_TIME_STEP_AT_NORMAL_SPEED = 38.4f; // 38.6f; //  42.0;
	constexpr float TRIGGER_TIME_STEP_AT_FASTER_SPEED = 12.5f; // 14.8f; 
	constexpr float TRIGGER_TIME_STEP_AT_CUSTOM_SPEED = 14.8f; //  16.0; // 15.0;

	constexpr float EXPOSURE_TIME_AT_NORMAL_SPEED_USB3 = 64.0f;
	constexpr float EXPOSURE_TIME_AT_FASTER_SPEED_USB3 = 37.0f;
	constexpr float EXPOSURE_TIME_AT_FASTEST_SPEED_USB3 = 11.5f; // 13.7f;

	constexpr int TRIGGER_FORE_PADDING_POINTS_AT_FASTER = 31;		// 500us // 16
	constexpr int TRIGGER_FORE_PADDING_POINTS_AT_NORMAL = 15;		// 500us // 42
	constexpr int TRIGGER_FORE_PADDING_POINTS_AT_SLOWER = 9;		// 500us // 78

	constexpr int TRIGGER_POST_PADDING_POINTS_AT_FASTER = 2;		// 500us // 16
	constexpr int TRIGGER_POST_PADDING_POINTS_AT_NORMAL = 2;		// 500us // 42
	constexpr int TRIGGER_POST_PADDING_POINTS_AT_SLOWER = 2;		// 500us // 78


	// Scan Optimizer 
	///////////////////////////////////////////////////////////////////////////////////////
	constexpr int OBTAIN_NEXT_PREVIEW_WAIT_DELAY = 5;
	constexpr int OBTAIN_NEXT_PREVIEW_RETRY_MAX = 100;
	constexpr int OBTAIN_NEXT_SPLIT_FOCUS_WAIT_DELAY = 5;
	constexpr int OBTAIN_NEXT_SPLIT_FOCUS_RETRAY_MAX = 100;

	constexpr float SCAN_AUTO_QINDEX_TO_SIGNAL = 1.5f;
	constexpr float SCAN_AUTO_QINDEX_TO_TARGET = 3.5f;
	constexpr float SCAN_AUTO_QINDEX_TO_TARGET_LENS_BACK = 1.5f;
	constexpr float SCAN_AUTO_QINDEX_TO_CONFIRM = 5.5f;
	constexpr float SCAN_AUTO_QINDEX_TO_COMPLETE = 9.5f;

	// Scan Optimizer
	/////////////////////////////////////////////////////////////////////////////////////
	constexpr float AUTO_FOCUS_QINDEX_TO_COMPLETE = 9.5f;
	constexpr float AUTO_FOCUS_QINDEX_TO_SIGNAL = 1.0f; // 2.0f; // 3.5f;
	constexpr float AUTO_FOCUS_QINDEX_DIFF_TO_RETRACE = 0.15f;

	constexpr float AutoFocusQualitySteps[7] = {
		9.5f, 8.5f, 7.5f, 6.5f, 5.5f, 4.5f, 3.5f
	};

	constexpr float AutoFocusDioptSteps[7] = {
		0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.5f, 3.5f
	};
	constexpr float AUTO_FOCUS_DIOPT_STEP_MIN = AutoFocusDioptSteps[0];
	constexpr float AUTO_FOCUS_DIOPT_STEP_MAX = AutoFocusDioptSteps[6];
	constexpr float AUTO_FOCUS_DIOPT_STEP_FIRST = AutoFocusDioptSteps[3];
	constexpr int AUTO_FOCUS_DIOPT_STEP_SIZE = 7;

	constexpr int AUTO_FOCUS_PLUS_DIRECTION = +1;
	constexpr int AUTO_FOCUS_MINUS_DIRECTION = -1;


	// Auto Scan Measure
	//////////////////////////////////////////////////////////////////////////////////////
	constexpr int AUTO_SCAN_FOCUS_DELAY_COUNT = 9;
	constexpr int AUTO_SCAN_FOCUS_DELAY_TIME = 50;


	// Auto Reference
	//////////////////////////////////////////////////////////////////////////////////////
	constexpr int AutoReferPointSteps[7] = {
		// 50, 100, 150, 200, 250
		// 10, 20, 30, 40, 50
		// 10, 25, 50, 75, 100, 150, 200
		10, 25, 50, 75, 100, 150, 200
	};

	constexpr int AutoReferMoveSteps[7] = {
		// 25, 50, 150, 250, 500
		// 100, 200, 300, 400, 500
		// 25, 50, 100, 150, 200, 300, 500
		25, 50, 75, 100, 150, 300, 500
	};

	constexpr int AUTO_REFER_MOVE_STEP_MIN = AutoReferMoveSteps[0];
	constexpr int AUTO_REFER_MOVE_STEP_MAX = AutoReferMoveSteps[6];
	constexpr int AUTO_REFER_MOVE_STEP_SIZE = 7;
	constexpr int AUTO_REFER_MOVE_STEP_FIND = AUTO_REFER_MOVE_STEP_MAX * 3;

	constexpr int AUTO_REFER_CENTER_POINT = 256;
	constexpr int AUTO_REFER_CENTER_OFFSET = 8; // 5;

	constexpr int AUTO_REFER_UPPER_DIRECTION = +1;
	constexpr int AUTO_REFER_LOWER_DIRECTION = -1;


	// Auto Leveling
	//////////////////////////////////////////////////////////////////////////////////////
	constexpr int AutoLevelPointSteps[5] = {
		50, 100, 150, 200, 250
	};

	constexpr int AutoLevelMoveSteps[5] = {
		25, 50, 150, 250, 500
	};

	constexpr int AUTO_LEVEL_MOVE_STEP_MIN = AutoLevelMoveSteps[0];
	constexpr int AUTO_LEVEL_MOVE_STEP_MAX = (AutoLevelMoveSteps[4] * 2);
	constexpr int AUTO_LEVEL_MOVE_STEP_SIZE = 5;

	constexpr int AUTO_LEVEL_CENTER_POINT = 256;
	constexpr int AUTO_LEVEL_CENTER_OFFSET = 5;

	constexpr float AUTO_POLAR_QINDEX_DIFF_TO_RETRACE = 0.05f;

	constexpr float AUTO_POLAR_MOVE_STEP_MAX = 30;
	constexpr float AUTO_POLAR_MOVE_STEP_FIRST = 15;
	constexpr float AUTO_POLAR_MOVE_STEP_MIN = 5;

	constexpr int AUTO_POLAR_UPPER_DIRECTION = +1;
	constexpr int AUTO_POLAR_LOWER_DIRECTION = -1;


	// Auto Focus 
	///////////////////////////////////////////////////////////////////////////////////////
	enum class AutoFocusPhase {
		INIT = 0,
		NO_SIGNAL,
		NO_SIGNAL_REVERSE,
		FIRST_GUESS,
		SEARCH,
		SEARCH_REVERSE,
		FORWARD,
		RETRACE,
		FORWARD2,
		RETRACE2,
		COMPLETE,
		CANCELED
	};

	constexpr int AUTO_FOCUS_TO_MINUS_DIOPT = -1;
	constexpr int AUTO_FOCUS_TO_PLUS_DIOPT = +1;

	constexpr float AUTO_FOCUS_PLUS_DIOPT_END = +30.0f;
	constexpr float AUTO_FOCUS_MINUS_DIOPT_END = -30.0f;
	constexpr float AUTO_FOCUS_PLUS_DIOPT_END_ANTERIOR = +30.0f; // +15.0f;
	constexpr float AUTO_FOCUS_MINUS_DIOPT_END_ANTERIOR = -30.0f; // -15.0f;

	constexpr float AUTO_FOCUS_MOVE_STEP_SEARCH = 3.0f; // 2.5f;
	constexpr float AUTO_FOCUS_MOVE_STEP_ALIGN = 1.5f; // 0.5f;
	constexpr float AUTO_FOCUS_MOVE_STEP_CONFIRM = 0.5f;
	constexpr float AUTO_FOCUS_MOVE_STEP_GUESS = 1.5f;

	constexpr int AUTO_FOCUS_RETRY_MAX_FORWARD = 9; // 5;
	constexpr int AUTO_FOCUS_RETRY_MAX_RETRACE = 9; // 5;
	constexpr int AUTO_FOCUS_RETRY_MAX_FIRST_GUESS = 9; // 5; // 3;

	constexpr int AUTO_FOCUS_WAIT_TARGET_SLOWER = 5;
	constexpr int AUTO_FOCUS_WAIT_TARGET_NORMAL = 7;
	constexpr int AUTO_FOCUS_WAIT_TARGET_FASTER = 15;

	constexpr int AUTO_FOCUS_MOMENTS_FORWARD = 5; // 3;
	constexpr int AUTO_FOCUS_MOMENTS_RETRACE = 5; // 3;
	constexpr int AUTO_FOCUS_RESISTS_FORWARD = 3; // 7; // 3;
	constexpr int AUTO_FOCUS_RESISTS_RETRACE = 3; // 7; // 3;
	constexpr int AUTO_FOCUS_RESISTS_FORWARD2 = 1; // 3;
	constexpr int AUTO_FOCUS_RESISTS_RETRACE2 = 1; // 3;

	constexpr int AUTO_FOCUS_FORWARDS_LIMIT2 = 7;
	constexpr int AUTO_FOCUS_RETRACES_LIMIT2 = 7;



	// Auto Polarization 
	///////////////////////////////////////////////////////////////////////////////////////
	enum class AutoPolarPhase {
		INIT = 0,
		NO_SIGNAL,
		NO_SIGNAL_REVERSE,
		FIRST_GUESS,
		FORWARD,
		RETRACE,
		FORWARD2,
		RETRACE2,
		COMPLETE,
		CANCELED
	};

	constexpr int AUTO_POLAR_TO_MINUS_DEGREE = -1;
	constexpr int AUTO_POLAR_TO_PLUS_DEGREE = +1;

	constexpr float AUTO_POLAR_MOVE_STEP_SEARCH = 30.0f;
	constexpr float AUTO_POLAR_MOVE_STEP_ALIGN = 15.0f; // 5.0f;
	constexpr float AUTO_POLAR_MOVE_STEP_CONFIRM = 5.0f; // 5.0f;
	constexpr float AUTO_POLAR_MOVE_STEP_GUESS = 15.0f;

	constexpr int AUTO_POLAR_RETRY_MAX_FORWARD = 9;  // 5; // 5;
	constexpr int AUTO_POLAR_RETRY_MAX_RETRACE = 9;  // 5; // 5;
	constexpr int AUTO_POLAR_RETRY_MAX_FIRST_GUESS = 9; // 5; // 5; // 3;

	constexpr int AUTO_POLAR_WAIT_TARGET_SLOWER = 5;
	constexpr int AUTO_POLAR_WAIT_TARGET_NORMAL = 7;
	constexpr int AUTO_POLAR_WAIT_TARGET_FASTER = 15;

	constexpr int AUTO_POLAR_MOMENTS_FORWARD = 5; // 3;
	constexpr int AUTO_POLAR_MOMENTS_RETRACE = 5; // 3;
	constexpr int AUTO_POLAR_RESISTS_FORWARD = 3; // 7; // 3;
	constexpr int AUTO_POLAR_RESISTS_RETRACE = 3; // 7; // 3;
	constexpr int AUTO_POLAR_RESISTS_FORWARD2 = 1; // 7; // 3;
	constexpr int AUTO_POLAR_RESISTS_RETRACE2 = 1; // 7; // 3;

	constexpr int AUTO_POLAR_FORWARDS_LIMIT2 = 7;
	constexpr int AUTO_POLAR_RETRACES_LIMIT2 = 7;



	// Auto Reference 
	///////////////////////////////////////////////////////////////////////////////////////
	enum class AutoReferPhase {
		INIT = 0,
		NO_SIGNAL,
		NO_SIGNAL_REVERSE,
		CAPTURE,
		FORWARD,
		RETRACE,
		CENTER,
		COMPLETE,
		CANCELED
	};

	constexpr int AUTO_REFER_MOVE_STEP_SEARCH = 450; // 900;
	constexpr int AUTO_REFER_MOVE_STEP_SEARCH_FAST = 700;// 1200;
	constexpr int AUTO_REFER_MOVE_STEP_ALIGN = 150;
	constexpr int AUTO_REFER_MOVE_STEP_CENTER = 50; // 75;

	constexpr int AUTO_REFER_MOVE_DISTANCE_CAPTURE = 2500;
	constexpr float AUTO_REFER_MOVE_RATIO_TO_OFFSET = 3.5f;

	// Forward direction is moving toward the eye.  
	constexpr int AUTO_REFER_TO_FORWARD = -1; // 1;
	// Backward direction is moving away from the eye.
	constexpr int AUTO_REFER_TO_BACKWARD = +1; // -1;

	constexpr int AUTO_REFER_MOMENTS_FORWARD = 5; // 3;
	constexpr int AUTO_REFER_MOMENTS_RETRACE = 5; // 3;
	constexpr int AUTO_REFER_RETRY_MAX_FORWARD = 9; // 5;
	constexpr int AUTO_REFER_RETRY_MAX_RETRACE = 9; // 5;
	constexpr int AUTO_REFER_RETRY_MAX_CENTER = 9; //  5;
	constexpr int AUTO_REFER_RETRY_MAX_CAPTURE = 3; // 5; // 3;// 5;
	constexpr int AUTO_REFER_CENTER_TRACK_MAX = 20; // 15; // 7;

	constexpr int AUTO_REFER_CENTER_LINE = 256;
	constexpr int AUTO_REFER_CENTER_OFFSET_MIN = -50;
	constexpr int AUTO_REFER_CENTER_OFFSET_MAX = 75;
	constexpr int AUTO_REFER_CENTER_LINE_SHIFT = (AUTO_REFER_CENTER_LINE + (AUTO_REFER_CENTER_OFFSET_MIN + AUTO_REFER_CENTER_OFFSET_MAX) / 2);
	constexpr int AUTO_REFER_VALID_POINT_MIN = (AUTO_REFER_CENTER_LINE - 100);
	constexpr int AUTO_REFER_VALID_POINT_MAX = (AUTO_REFER_CENTER_LINE + 150);
	constexpr int AUTO_REFER_CENTER_UPPER_LINE = 128; // 192; // 128;

}