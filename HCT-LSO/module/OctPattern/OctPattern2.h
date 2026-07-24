#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)


#ifdef OCTPATTERN_EXPORTS
#define OCTPATTERN_DLL_API __declspec(dllexport)
#else
#define OCTPATTERN_DLL_API __declspec(dllimport)
#endif


#include "wso_domain.h"
#include "cpp_util.h"


#include <memory>
#include <vector>
#include <chrono>
#include <mutex>


namespace oct_pattern
{
	using namespace wso_domain;
	using namespace cpp_util;
	using namespace std;

	constexpr int TRIGGER_FORE_PADDING_POINTS = 8; // 5;
	constexpr int TRIGGER_POST_PADDING_POINTS = 2; // 5;
	constexpr float TRIGGER_FORE_PADDING_RATIO = 0.1f;
	constexpr float TRIGGER_POST_PADDING_RATIO = 0.1f;

	constexpr float PATTERN_SCAN_RANGE_X_MIN = 0.0f;// 2.0f;
	constexpr float PATTERN_SCAN_RANGE_X_MAX = 16.5f; // 12.0f;// 12.0f;
	constexpr float PATTERN_SCAN_RANGE_Y_MIN = 0.0f;// 2.0f;
	constexpr float PATTERN_SCAN_RANGE_Y_MAX = 12.0f; // 9.0f;// 9.0f;
	constexpr float PATTERN_SCAN_ANGLE_MIN = 0.0f;
	constexpr float PATTERN_SCAN_ANGLE_MAX = 179.0f;

	constexpr int PATTERN_SCAN_POINTS_MIN = 128;
	constexpr int PATTERN_SCAN_POINTS_MAX = 2048;
	constexpr int PATTERN_SCAN_LINES_MIN = 1;
	constexpr int PATTERN_SCAN_LINES_MAX = 512; //  256;

	constexpr float PATTERN_SCAN_CENTER_X = 0.0f;
	constexpr float PATTERN_SCAN_CENTER_Y = 0.0f;


	constexpr int PATTERN_PREVIEW_LINE_NUM_POINTS = 1024; // 512;
	constexpr int PATTERN_PREVIEW_MULTI_NUM_POINTS = 512;


	// Line Trace id (Index of Trajectory)
	/////////////////////////////////////////////////////////////////////////////////////
	constexpr int TRACE_ID_START = 0;
	constexpr int TRACE_ID_CLOSE = 31;
	constexpr int TRACE_ID_MAX_LINES = (TRACE_ID_CLOSE + 1);
	constexpr int TRACE_ID_MAX_REPEATS = (TRACE_ID_MAX_LINES / 2);

	constexpr int TRACE_ID_PREVIEW_HIDDEN = 0;
	constexpr int TRACE_ID_PREVIEW_START = 2;
	constexpr int TRACE_ID_PREVIEW_POINT = 2;
	constexpr int TRACE_ID_PREVIEW_LINE = 2;
	constexpr int TRACE_ID_PREVIEW_CUBE = 2;
	constexpr int TRACE_ID_PREVIEW_MULTI = 2;
	constexpr int TRACE_ID_PREVIEW_LINE_HD = 7;
	constexpr int TRACE_ID_PREVIEW_MULTI_HD = 7;

	constexpr int TRACE_ID_ENFACE_START = 13;
	constexpr int TRACE_ID_ENFACE_CUBE = 13; 

	constexpr int TRACE_ID_MEASURE_START = 0;
	constexpr int TRACE_ID_MEASURE_LINE = 0;
	constexpr int TRACE_ID_MEASURE_CIRCLE = 0;
	constexpr int TRACE_ID_MEASURE_CROSS = 0;
	constexpr int TRACE_ID_MEASURE_RADIAL = 0;
	constexpr int TRACE_ID_MEASURE_RASTER = 0;
	constexpr int TRACE_ID_MEASURE_CUBE = 0;

	// Pattern Frame
	//////////////////////////////////////////////////////////////////////////////////////
	constexpr int PATTERN_FRAMES_ENFACE_MAX = (TRACE_ID_MAX_LINES - TRACE_ID_ENFACE_START);
	constexpr int PATTERN_FRAMES_MEASURE_MAX = (TRACE_ID_MAX_LINES - TRACE_ID_MEASURE_START);
}