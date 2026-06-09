#pragma once

#include "ret_segm.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

namespace semt_segm
{
	using namespace ret_segm;

	constexpr float VALID_IMAGE_MEAN_MIN = 9.0f; // 15.0f; // 25.0f;
	constexpr float VALID_IMAGE_MEAN_MAX = 75.0f;

	constexpr float VALID_PEAK_TO_MEAN_RATIO_MIN = 4.5f;
	constexpr float VALID_PEAK_TO_MEAN_RATIO_MIN_CORNEA = 3.0f;
	constexpr float VALID_POOR_SNR_RATIO_MIN = 0.50f; // 0.25f;
}