#pragma once

#include "PatientDefines.h"

#include <string>
#include <map>


namespace wso_domain
{
	// Line Camera
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int LINE_CAMERA_CCD_PIXELS = 2048;
	constexpr unsigned short LINE_CAMERA_PIXEL_VALUE_MAX = 4096;
	constexpr int BSCAN_LATERAL_SIZE_MAX = 1024; // 2048;


	// Default Pattern preview, enface settings. 
	///////////////////////////////////////////////////////////////////////////////////////////
	constexpr int PATTERN_PREVIEW_LATERAL_POINTS = 1024; // 512;
	// constexpr int PATTERN_PREVIEW_AXIAL_POINTS = 512;
	constexpr int PATTERN_PREVIEW_ASCAN_POINTS = PATTERN_PREVIEW_LATERAL_POINTS;
	constexpr int PATTERN_PREVIEW_IMAGE_WIDTH = PATTERN_PREVIEW_LATERAL_POINTS;
	// constexpr int PATTERN_PREVIEW_IMAGE_HEIGHT = PATTERN_PREVIEW_AXIAL_POINTS; // within fft output line size. 

	constexpr int PATTERN_PREVIEW_AVERAGE_SIZE = 5;
	constexpr int PATTERN_PREVIEW_HISTORY_SIZE = 10;
	constexpr float PATTERN_PREVIEW_RANGE_X = 6.0f;

	constexpr float PATTERN_ENFACE_RANGE_X = 6.0f;
	constexpr float PATTERN_ENFACE_RANGE_Y = 6.0f;
	constexpr int PATTERN_ENFACE_ASCAN_POINTS = 256;
	constexpr int PATTERN_ENFACE_BSCAN_LINES = 64; // 96; // 128;

	constexpr int PATTERN_ENFACE_IMAGE_WIDTH = PATTERN_ENFACE_ASCAN_POINTS;
	constexpr int PATTERN_ENFACE_IMAGE_HEIGHT = PATTERN_ENFACE_BSCAN_LINES;


	// OCT Parameters
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr double RESAMPLE_INIT_PARAM1 = 800.752136;
	constexpr double RESAMPLE_INIT_PARAM2 = 0.047172;
	constexpr double RESAMPLE_INIT_PARAM3 = -0.000003;
	constexpr double RESAMPLE_INIT_PARAM4 = 0.000000;

	constexpr double PHASE_SHIFT_INIT_PARAM1 = -9.2;
	constexpr double PHASE_SHIFT_INIT_PARAM2 = +2.3;
	constexpr double PHASE_SHIFT_INIT_PARAM3 = 0.0;


	// OCT Calibration Parameters 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int OCT_SPECTROMETER_COEFFS_SIZE = 4;
	constexpr int OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE = 3;


	// Trigger Step by Scan speed. 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr float TRIGGER_TIME_STEP_AT_SLOWER_SPEED = 66.6f; // 78.0
	constexpr float TRIGGER_TIME_STEP_AT_NORMAL_SPEED = 38.4f; // 38.6  42.0;
	constexpr float TRIGGER_TIME_STEP_AT_FASTER_SPEED = 12.5f; // 14.8f; // 16.0; // 15.0;
	constexpr float TRIGGER_TIME_STEP_AT_CUSTOM_SPEED = 12.5f; // 14.8f; //  8.5; // 15.0;

	constexpr float EXPOSURE_TIME_AT_SLOWER_SPEED_USB3 = 64.0f;
	constexpr float EXPOSURE_TIME_AT_NORMAL_SPEED_USB3 = 37.0f; // 13.7f; // 37.0f;
	constexpr float EXPOSURE_TIME_AT_FASTER_SPEED_USB3 = 11.5f; // 13.7f; // 7.5f; // 13.7f;
	constexpr float EXPOSURE_TIME_AT_CUSTOM_SPEED_USB3 = 11.5f;// 13.7f;

	constexpr int TRIGGER_FORE_PADDING_POINTS_AT_FASTEST = 31;		// 500us // 16
	constexpr int TRIGGER_FORE_PADDING_POINTS_AT_FASTER = 15;		// 500us // 42
	constexpr int TRIGGER_FORE_PADDING_POINTS_AT_NORMAL = 9;		// 500us // 78

	constexpr int TRIGGER_POST_PADDING_POINTS_AT_FASTEST = 2;		// 500us // 16
	constexpr int TRIGGER_POST_PADDING_POINTS_AT_FASTER = 2;		// 500us // 42
	constexpr int TRIGGER_POST_PADDING_POINTS_AT_NORMAL = 2;		// 500us // 78

	// Pattern description Name
	/////////////////////////////////////////////////////////////////////////////////////////////
	constexpr const char* PATTERN_MACULAR_POINT_NAME = "Macular Point";
	constexpr const char* PATTERN_MACULAR_LINE_NAME = "Macular Line";
	constexpr const char* PATTERN_MACULAR_CROSS_NAME = "Macular Cross";
	constexpr const char* PATTERN_MACULAR_CUBE_NAME = "Macular Cube";
	constexpr const char* PATTERN_MACULAR_RASTER_NAME = "Macular Raster";
	constexpr const char* PATTERN_MACULAR_RADIAL_NAME = "Macular Radial";
	constexpr const char* PATTERN_MACULAR_ANGIO_NAME = "Macular Angio";

	constexpr const char* PATTERN_DISC_POINT_NAME = "Disc Point";
	constexpr const char* PATTERN_DISC_RASTER_NAME = "Disc Raster";
	constexpr const char* PATTERN_DISC_RADIAL_NAME = "Disc Radial";
	constexpr const char* PATTERN_DISC_CUBE_NAME = "Disc Cube";
	constexpr const char* PATTERN_DISC_CIRCLE_NAME = "Disc Circle";
	constexpr const char* PATTERN_DISC_ANGIO_NAME = "Disc Angio";

	constexpr const char* PATTERN_FUNDUS_LINE_NAME = "Fundus Line";
	constexpr const char* PATTERN_FUNDUS_CROSS_NAME = "Fundus Cross";
	constexpr const char* PATTERN_FUNDUS_CUBE_NAME = "Fundus Cube";
	constexpr const char* PATTERN_FUNDUS_RASTER_NAME = "Fundus Raster";
	constexpr const char* PATTERN_FUNDUS_ANGIO_NAME = "Fundus Angio";

	constexpr const char* PATTERN_CORNEA_POINT_NAME = "Anterior Point";
	constexpr const char* PATTERN_CORNEA_LINE_NAME = "Anterior Line";
	constexpr const char* PATTERN_CORNEA_RADIAL_NAME = "Anterior Radial";
	constexpr const char* PATTERN_CORNEA_CUBE_NAME = "Anterior 3D";

	constexpr const char* PATTERN_ANTERIOR_WIDE_NAME = "Anterior Full";
	constexpr const char* PATTERN_ANTERIOR_AL_NAME = "Axial Length";
	constexpr const char* PATTERN_ANTERIOR_LT_NAME = "Lens Thickness";
	constexpr const char* PATTERN_CALIBRATION_POINT_NAME = "Calibration Point";
	constexpr const char* PATTERN_TOPOGRAPHY_NAME = "Topography";



	enum class OctScanSpeed : int {
		UNKNOWN = 0,
		SLOWER = 1,
		NORMAL = 2,
		FASTER = 3,
		CUSTOM = 4,
		N_SCAN_SPEED_MODE = 4
	};


	enum class OctCompensationLensMode : int {
		Zero = 0,
		Minus,
		Plus
	};

	enum class OctPatternCode : int {
		UNKNOWN = 0,
		CALIBRATION_POINT = 1,
		MACULAR_POINT = 1001,
		MACULAR_LINE = 1002, 
		MACULAR_CROSS = 1003, 
		MACULAR_RASTER = 1004,
		MACULAR_RADIAL = 1005,
		MACULAR_CUBE = 1007, 
		MACULAR_ANGIO = 1108,
		DISC_POINT = 2001,
		DISC_LINE = 2002,
		DISC_CROSS = 2003,
		DISC_RASTER = 2004,
		DISC_RADIAL = 2005, 
		DISC_CIRCLE = 2006, 
		DISC_CUBE = 2007, 
		DISC_ANGIO = 2108, 
		FUNDUS_LINE = 3002,
		FUNDUS_RASTER = 3004,
		FUNDUS_CUBE = 3007,
		FUNDUS_ANGIO = 3108, 
		CORNEA_POINT = 4001,
		CORNEA_LINE = 4002,
		CORNEA_RADIAL = 4005,
		CORNEA_CUBE = 4007,
	};

	enum class OctPatternType : int {
		UNKNOWN = 0, 
		POINT = 1, 
		LINE = 2,
		CROSS = 3,
		RASTER = 4,
		RADIAL = 5,
		CIRCLE = 6,
		CUBE = 7,
		ANGIO = 8,
		MULTI_LINE = 9,
		VERT_LINE = 12, 
		VERT_RASTER = 14,
		VERT_CUBE = 17,
		VERT_ANGIO = 18, 
		VERT_MULTI_LINE = 19,
	};

	enum class OctScanDirection : int {
		X_TO_Y = 0,
		Y_TO_X = 1,
	};

	enum class OctAscanSize : int {
		UNKNOWN = 0,
		ASCAN_256 = 256,
		ASCAN_384 = 384,
		ASCAN_512 = 512,
		ASCAN_1024 = 1024,
	};

	enum class OctBscanSize : int {
		UNKNOWN = 0,
		BSCAN_1 = 1,
		BSCAN_10 = 10,
		BSCAN_12 = 12,
		BSCAN_18 = 18,
		BSCAN_24 = 24,
		BSCAN_64 = 64,
		BSCAN_96 = 96,
		BSCAN_128 = 128,
		BSCAN_256 = 256,
		BSCAN_384 = 384,
		BSCAN_512 = 512,
		BSCAN_1024 = 1024,
	};

	enum class LineTraceType {
		Line = 0,
		Radial,
		Circle,
		HorzRaster,
		VertRaster,
		HorzRasterFast,
		VertRasterFast
	};

}