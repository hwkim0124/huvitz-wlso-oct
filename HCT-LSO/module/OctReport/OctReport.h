#pragma once


// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)


#ifdef OCTREPORT_EXPORTS
#define OCTREPORT_DLL_API __declspec(dllexport)
#else
#define OCTREPORT_DLL_API __declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

#include "wso_domain.h"
#include "oct_result.h"
#include "cpp_util.h"


namespace oct_report
{
	using namespace wso_domain;
	using namespace oct_result;
	using namespace cpp_util;
	using namespace std;

	const float FOVEA_CENTER_LINE_START = 0.20f;
	const float FOVEA_CENTER_LINE_CLOSE = 0.80f;
	const float FOVEA_CENTER_XPOS_START = 0.20f;
	const float FOVEA_CENTER_XPOS_CLOSE = 0.80f;
	const float FOVEA_CENTER_DISC_WIDTH = 0.15f;

	const float FOVEA_CENTER_DISC_RADIUS_IN_MM = 1.2f;
	const float FOVEA_CENTER_DISC_NERVE_LIMIT = 45.0f;

	const float FOVEA_CENTER_DISC_DIFF_RATIO_MIN = 0.25f;
	const float FOVEA_CENTER_DISC_DIFF_RATIO_MAX = 0.75f;
	const float FOVEA_DISC_DEPTH_DIFF_MAX = 2.0f;
	const float FOVEA_DISC_BOTTOM_OFFSET = 15.0f;
	const float FOVEA_DISC_BOTTOM_DIFF_MAX = 450.0f;
	const float FOVEA_DISC_AREA_DIFF_MAX = 1.5f;

	const float FOVEA_OPTIC_DISC_RANGE_X = 7.5f;

}