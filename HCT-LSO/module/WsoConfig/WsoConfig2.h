#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

#ifdef WSOCONFIG_EXPORTS
#define WSOCONFIG_DLL_API		__declspec(dllexport)
#else
#define WSOCONFIG_DLL_API		__declspec(dllimport)
#endif

#include <cstdint>
#include <cmath>
#include <memory>
#include <vector>
#include <mutex>

#include "wso_domain.h"
#include "wso_board.h"


namespace wso_config
{
	using namespace wso_domain;
	using namespace wso_board;
	using namespace std;

	constexpr const char* SYS_CONFIG_FILE_NAME = "WsoConfig.xml";
	constexpr const char* SYS_CONFIG_FILE_ROOT = "HCT-SLO";
	constexpr const char* SYS_CONFIG_FILE_ROOT_COMMENT = "System Configuration for Huvitz OCT-LSO";

	constexpr const char* CAMERA_SETTING_GROUP = "CameraSetting";
	constexpr const char* FIXATION_SETTING_GROUP = "FixationSetting";
	constexpr const char* GALVANO_SETTING_GROUP = "GalvanoSetting";
	constexpr const char* LSO_CAPTURE_SETTING_GROUP = "LsoCaptureSetting";
	constexpr const char* LSO_DISPLAY_SETTING_GROUP = "LsoDisplaySetting";



	struct FixationPoint {
		int x = 0;
		int y = 0;
	};

	struct InternalFixationPreset {
		bool useLcdFix = false;
		bool useBlinkOn = false;

		int blinkPeriod = 0;
		int blinkOnTime = 0;
		int brightness = 0;
		int fixationType = 0;

		FixationPoint centerOD = { 0, 0 };
		FixationPoint fundusOD = { 0, 0 };
		FixationPoint opticDiscOD = { 0, 0 };
		FixationPoint leftSideOD = { 0, 0 };
		FixationPoint leftTopOD = { 0, 0 };
		FixationPoint leftBottomOD = { 0, 0 };
		FixationPoint rightSideOD = { 0, 0 };
		FixationPoint rightTopOD = { 0, 0 };
		FixationPoint rightBottomOD = { 0, 0 };

		FixationPoint centerOS = { 0, 0 };
		FixationPoint fundusOS = { 0, 0 };
		FixationPoint opticDiscOS = { 0, 0 };
		FixationPoint leftSideOS = { 0, 0 };
		FixationPoint leftTopOS = { 0, 0 };
		FixationPoint leftBottomOS = { 0, 0 };
		FixationPoint rightSideOS = { 0, 0 };
		FixationPoint rightTopOS = { 0, 0 };
		FixationPoint rightBottomOS = { 0, 0 };
	};


	struct StageMotorPosition {
		int x = 0;
		int y = 0;
		int z = 0;
	};

	struct StageReadyPositionPreset
	{
		StageMotorPosition od;
		StageMotorPosition os;
		StageMotorPosition center;
		StageMotorPosition me;
	};

	enum class StagePositionType : int
	{
		NONE = 0,
		OD = 1,
		OS = 2,
		CENTER = 3,
		MODELEYE = 4
	};


}