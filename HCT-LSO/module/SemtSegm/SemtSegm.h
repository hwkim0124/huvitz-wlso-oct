#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)


#ifdef SEMTSEGM_EXPORTS
#define SEMTSEGM_DLL_API __declspec(dllexport)
#else
#define SEMTSEGM_DLL_API __declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

#include "wso_domain.h"
#include "ret_segm.h"
#include "cpp_util.h"


namespace semt_segm
{
	using namespace wso_domain;
	using namespace ret_segm;
	using namespace cpp_util;
	using namespace std;

	using OcularImage = ret_segm::SegmImage;
	using OcularLayer = ret_segm::SegmLayer;

	constexpr int SAMPLE_WIDTH = 256;
	constexpr int SAMPLE_HEIGHT = (768 / 2);
}
