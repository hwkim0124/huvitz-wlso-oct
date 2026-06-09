#pragma once

#include "OctAngio.h"

#include "sig_chain.h"

#include <iterator>
#include <algorithm>
#include <numeric>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <numbers>

#include <boost/format.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/tracking.hpp>


#include <json\json.hpp>
using json = nlohmann::json;


namespace oct_angio
{
	using namespace sig_chain;
	using namespace cv;

	constexpr int ANGIO_GABOR_FILTER_ORIENTS = 12;
	constexpr float ANGIO_GABOR_FILTER_SIGMA = 3.0f;
	constexpr float ANGIO_GABOR_FILTER_DIVIDER = 1.5f;
	constexpr float ANGIO_GABOR_FILTER_WEIGHT = 0.25f;

	constexpr float ANGIO_ENHANCE_PARAM = 1.0f;
	constexpr float ANGIO_BIAS_FIELD_SIGMA = 15.0f;

	constexpr float ANGIO_MOTION_THRESHOLD = 1.5f;
	constexpr float ANGIO_MOTION_OVER_POINTS = 0.5f;
	constexpr float ANGIO_MOTION_DIST_RATIO = 1.15f;

	constexpr int ANGIO_AVERAGE_OFFSET = 3;

	constexpr OcularLayerType DECORR_UPPER_LAYER_TYPE = OcularLayerType::ILM;
	constexpr OcularLayerType DECORR_LOWER_LAYER_TYPE = OcularLayerType::BRM;
	constexpr float DECORR_UPPER_LAYER_OFFSET = -150.0f;
	constexpr float DECORR_LOWER_LAYER_OFFSET = +240.0f;

	constexpr OcularLayerType VASCULAR_UPPER_LAYER_TYPE = OcularLayerType::NFL;
	constexpr OcularLayerType VASCULAR_LOWER_LAYER_TYPE = OcularLayerType::IPL;
	constexpr float VASCULAR_UPPER_LAYER_OFFSET = -0.0f;
	constexpr float VASCULAR_LOWER_LAYER_OFFSET = +15.0f;

	constexpr OcularLayerType VASCULAR_UPPER_LAYER_TYPE2 = OcularLayerType::ILM;
	constexpr OcularLayerType VASCULAR_LOWER_LAYER_TYPE2 = OcularLayerType::OPL;
	constexpr float VASCULAR_UPPER_LAYER_OFFSET2 = -0.0f;
	constexpr float VASCULAR_LOWER_LAYER_OFFSET2 = +0.0f;

	constexpr OcularLayerType VARIANCE_UPPER_LAYER_TYPE = OcularLayerType::ILM; // IOS;
	constexpr OcularLayerType VARIANCE_LOWER_LAYER_TYPE = OcularLayerType::IOS; // OPR; // OcularLayerType::BRM;
	constexpr float VARIANCE_UPPER_LAYER_OFFSET = -0.0f;
	constexpr float VARIANCE_LOWER_LAYER_OFFSET = +0.0f; // +0.0f;

	constexpr OcularLayerType VARIANCE_UPPER_LAYER_TYPE2 = OcularLayerType::IOS;
	constexpr OcularLayerType VARIANCE_LOWER_LAYER_TYPE2 = OcularLayerType::BRM; // OcularLayerType::BRM;
	constexpr float VARIANCE_UPPER_LAYER_OFFSET2 = -0.0f;
	constexpr float VARIANCE_LOWER_LAYER_OFFSET2 = +15.0f; // +0.0f;

	constexpr OcularLayerType SHADOWED_UPPER_LAYER_TYPE = OcularLayerType::OPL; // OcularLayerType::OPL;
	constexpr OcularLayerType SHADOWED_LOWER_LAYER_TYPE = OcularLayerType::OPR; // OcularLayerType::BRM;
	constexpr float SHADOWED_UPPER_LAYER_OFFSET = -0.0f;
	constexpr float SHADOWED_LOWER_LAYER_OFFSET = +9.0f; // +0.0f;

	constexpr OcularLayerType SHADOWED_UPPER_LAYER_TYPE2 = OcularLayerType::IPL; // OcularLayerType::OPL;
	constexpr OcularLayerType SHADOWED_LOWER_LAYER_TYPE2 = OcularLayerType::OPR; // OcularLayerType::BRM;
	constexpr float SHADOWED_UPPER_LAYER_OFFSET2 = -0.0f;
	constexpr float SHADOWED_LOWER_LAYER_OFFSET2 = +9.0f; // +0.0f;

	constexpr OcularLayerType SUPERFICIAL_UPPER_LAYER_TYPE = OcularLayerType::ILM;
	constexpr OcularLayerType SUPERFICIAL_LOWER_LAYER_TYPE = OcularLayerType::IPL;
	constexpr float SUPERFICIAL_UPPER_LAYER_OFFSET = -0.0f;
	constexpr float SUPERFICIAL_LOWER_LAYER_OFFSET = +0.0f;

	constexpr OcularLayerType NONVASCULAR_UPPER_LAYER_TYPE = OcularLayerType::IOS;
	constexpr OcularLayerType NONVASCULAR_LOWER_LAYER_TYPE = OcularLayerType::RPE;
	constexpr float NONVASCULAR_UPPER_LAYER_OFFSET = -15.0f;
	constexpr float NONVASCULAR_LOWER_LAYER_OFFSET = +25.0f;

	constexpr float DIFFER_NORM_RANGE_MIN = 50.0f;
	constexpr float DIFFER_NORM_RANGE_MAX = 450.0f;
	constexpr float DIFFER_NORM_RANGE_MIN_OUTER = 100.0f;
	constexpr float DIFFER_NORM_RANGE_MAX_OUTER = 900.0f;

	constexpr float DECORR_NORM_RANGE_MIN = 0.025f;
	constexpr float DECORR_NORM_RANGE_MAX = 0.35f;
	constexpr float DECORR_NORM_RANGE_MIN_OUTER = 0.035f;
	constexpr float DECORR_NORM_RANGE_MAX_OUTER = 0.55f;

	constexpr float BSCAN_IMAGE_MEAN = 180.0f;

	constexpr int FILE_HEADER_SIZE = 256;
	constexpr int FILE_DSIZES_SIZE = 1024;
	constexpr int FILE_DATA_HEIGHT = 768;

	constexpr int DECORR_IMAGE_HEIGHT = 768;
	constexpr float DECORR_VALUE_MIN = 0.0001f;
	constexpr float DECORR_REFLECT_WEIGHT = 0.25f;
	constexpr float DECORR_PROJECT_REMOVE_RANGE = 6.0f;



}