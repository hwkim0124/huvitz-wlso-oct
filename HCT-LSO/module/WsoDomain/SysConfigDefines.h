#pragma once
#include "WsoDomain2.h"

#include <functional>
#include <string>

namespace wso_domain
{
	const int OCT_GALVANO_SCALE_NUM = 8;
	const float OCT_GALVANO_SCALE_MAX = 5.0f;
	const float OCT_GALVANO_SCALE_MIN = 0.1f;

	struct CorneaCameraConfigParam {
		float again_left = 0.0f;
		float again_right = 0.0f;
		float again_lower = 0.0f;
		float dgain_left = 0.0f;
		float dgain_right = 0.0f;
		float dgain_lower = 0.0f;
	};

	struct LcdFixationParam {
		int row = 0;
		int col = 0;
		int bright = 0;
		int blink = 0;
		int period = 0;
		int ontime = 0;
		int fixtype = 0;
	};

}