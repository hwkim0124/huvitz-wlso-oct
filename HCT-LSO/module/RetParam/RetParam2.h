#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)

#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

#include "RetParam.h"

namespace ret_param
{
	using namespace cpp_util;
	using namespace std;


	constexpr int ENFACE_BSCANS_MIN = 50;

	constexpr float DISC_CENTER_RADIUS = (1.0f / 2.0f);
	constexpr float DISC_INNER_RADIUS = (3.0f / 2.0f);
	constexpr float DISC_OUTER_RADIUS = (5.0f / 2.0f);

}