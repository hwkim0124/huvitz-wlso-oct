#pragma once

#ifdef RETSEGM_EXPORTS
#define RETSEGM_DLL_API __declspec(dllexport)
#else
#define RETSEGM_DLL_API __declspec(dllimport)
#endif

#include "wso_domain.h"
#include "cpp_util.h"

namespace ret_segm
{
	constexpr int SAMPLE_IMAGE_WIDTH = 256;
	constexpr int SAMPLE_IMAGE_HEIGHT = (768 / 2);
}