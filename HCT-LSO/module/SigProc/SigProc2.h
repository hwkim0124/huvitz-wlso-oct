#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)


#ifdef SIGPROC_EXPORTS
#define SIGPROC_DLL_API		__declspec(dllexport)
#else
#define SIGPROC_DLL_API		__declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

#include "wso_domain.h"
#include "cpp_util.h"


namespace sig_proc
{
	using namespace wso_domain;
	using namespace cpp_util;	
	using namespace std;

	constexpr const char* BACKGROUND_FILE_NAME = "background.bin";

	// FBG Specification
	////////////////////////////////////////////////////////////////////////////////////////
	constexpr const char* FBG_UNKNOWN_NAME = "Unknown";
	constexpr const char* FBG_CLASS1_NAME = "3054-1";
	constexpr const char* FBG_CLASS2_NAME = "3054-2";
	constexpr const char* FBG_CLASS3_NAME = "3054-3";
	constexpr const char* FBG_CLASS4_NAME = "4160-1";
	constexpr const char* FBG_CLASS5_NAME = "4160-2";
	constexpr const char* FBG_CLASS6_NAME = "5435-2";

	enum {
		FBG_CLASS1 = 0, FBG_CLASS2, FBG_CLASS3, FBG_CLASS4, FBG_CLASS5, FBG_CLASS6, NUMBER_OF_FBG_CLASSES = 6
	};

	constexpr int FBG_PEAK_WINDOW_SIZE = 50;
	constexpr int FBG_PEAK_THRESHOLD_MAX = (LINE_CAMERA_PIXEL_VALUE_MAX - 1);
	constexpr int FBG_PEAK_THRESHOLD_INIT = ((int)(FBG_PEAK_THRESHOLD_MAX * 0.90f));

	constexpr float WAVELENGTH_CENTER_OF_SOURCE = 840.0f;
	constexpr int WAVELENGTH_FUNCTION_DEGREE = 3;
	constexpr int WAVELENGTH_FUNCTION_COEFFS_SIZE = (WAVELENGTH_FUNCTION_DEGREE + 1);


	// Resampling Parameters 
	////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int RESAMPLE_DETECTOR_PIXELS = LINE_CAMERA_CCD_PIXELS;
	constexpr int RESAMPLE_ZERO_PADDING_SCALE = 1;
	constexpr int RESAMPLE_PADDED_PIXELS = (RESAMPLE_DETECTOR_PIXELS * RESAMPLE_ZERO_PADDING_SCALE);
	constexpr int RESAMPLE_WAVELENGTH_FUNCTION_DEGREE = 4;

	constexpr double RESAMPLE_WAVELENGTH_PARAM_1 = 800.752136;
	constexpr double RESAMPLE_WAVELENGTH_PARAM_2 = 0.047172;
	constexpr double RESAMPLE_WAVELENGTH_PARAM_3 = -0.000003;
	constexpr double RESAMPLE_WAVELENGTH_PARAM_4 = 0.000000;


	constexpr int RESAMPLE_KVALUE_START_INDEX = 1;
	constexpr int RESAMPLE_KVALUE_END_INDEX = (RESAMPLE_PADDED_PIXELS - 2);


	// Dispersion Compensation
	/////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int PHASE_SHIFT_PARAMETERS = 2;

	constexpr int PHASE_SHIFT_PARAMETER_SIZE = 3;
	constexpr double PHASE_SHIFT_PARAM1_SCALER = 1000000000.0;		// pow(10, 9)
	constexpr double PHASE_SHIFT_PARAM2_SCALER = 10000000000000.0;	// pow(10, 13)
	constexpr double PHASE_SHIFT_PARAM3_SCALER = 100000000000000000.0; // pow(10, 17)

}