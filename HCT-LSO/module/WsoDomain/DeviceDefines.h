#pragma once

namespace wso_domain
{
	// Trajectory Profile
	/////////////////////////////////////////////////////////////////////////////////////////////
	constexpr float TRAJECT_TIME_STEP_IN_US = 42.0f; // 38.0f;  // 10^6, 26KHz
	constexpr short TRAJECT_TRIGGER_DELAY = 1;
	constexpr short TRAJECT_NUMBER_OF_REPEATS = 1;
	constexpr short TRAJECT_CAMERA_TRIGGER_COUNT = 1024;
	constexpr short TRAJECT_CAMERA_TRIGGER_INTERVAL = 1;
	constexpr short TRAJECT_TRIGGER_START_INDEX = 5;
	constexpr short TRAJECT_SAMPLE_SIZE = (TRAJECT_TRIGGER_START_INDEX + TRAJECT_CAMERA_TRIGGER_COUNT);
	constexpr short TRAJECT_SAMPLE_SIZE_MAX = 8192;

	constexpr int TRAJECT_NUMBER_OF_PROFILES = 128; //  32;

	// Galvanometer 
	////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int GALVANO_POSITION_FUNCTION_DEGREE = 5; // 8;
	constexpr int GALVANO_STEP_RANGE_MIN = -32767;
	constexpr int GALVANO_STEP_RANGE_MAX = +32767;
	constexpr int GALVANO_STEP_RANGE_SIZE = (GALVANO_STEP_RANGE_MAX - GALVANO_STEP_RANGE_MIN);
	constexpr int GALVANO_STEP_RANGE_HALF = GALVANO_STEP_RANGE_MAX;

	constexpr int GALVANO_TRACE_POSITION_BUFFER_SIZE = TRAJECT_SAMPLE_SIZE_MAX;
	constexpr int GALVANO_RESP_TIME_IN_US = 350;


	enum class MotorType : unsigned short
	{
		OCT_FOCUS = 0x00,
		OCT_REFER = 0x01,
		OCT_POLAR = 0x02,
		OCT_REFND = 0x03,
		LSO_FOCUS = 0x04,
		RM = 0x05,
		OCT_ANT_LENS = 0x06,
		AP1 = 0x07,
		AP2 = 0x08,
		SWING = 0x09,
		STAGE_X = 0x0A,
		STAGE_Y = 0x0B,
		STAGE_Z = 0x0C,
		HEAD_REST = 0x0F,

		NUM_MOTORS = (HEAD_REST + 1)
	};

	enum class LightType : unsigned short
	{
		LSO_WHITE_LED = 0,
		RETINA_IR_LED = 1,
		CORNEA_IR_LEFT_LED = 2,
		CORNEA_IR_RIGHT_LED = 3,
		OCT_SLD = 0x04,
		UNKNOWN = 0xFF,
		NUM_LIGHT_LEDS = (CORNEA_IR_RIGHT_LED + 1)
	};

	enum class StepMotorType : unsigned char
	{
		OCT_FOCUS = 0x00,
		OCT_REFER = 0x01,
		OCT_POLAR = 0x02,
		OCT_REFND = 0x03,
		LSO_FOCUS = 0x04,
		RM = 0x05,
		OCT_ANT_LENS = 0x06,
		AP1 = 0x07,
		AP2 = 0x08,
		SWING = 0x09,
		STAGE_X = 0x0A,
		STAGE_Y = 0x0B,
		STAGE_Z = 0x0C,
		HEAD_REST = 0x0F,

		NUM_MOTORS = (HEAD_REST + 1)
	};

	enum class StageMotorType : unsigned char
	{
		STAGE_X = 0x09,
		STAGE_Y = 0x05, 
		STAGE_Z = 0x0B,
		NUMBER_OF_ITEMS = (STAGE_Z + 1)
	};


	enum class CorneaCameraType : unsigned char
	{
		LEFT = 0x00,
		RIGHT = 0x01,
		UNKNOWN = 0x99,
		NUMBER_OF_ITEMS = (RIGHT + 1)
	};

	enum class CameraType : unsigned char
	{
		IR_CORNEA_LEFT = 0x00,
		IR_CORNEA_RIGHT = 0x01,
		IR_RETINA = 0x02,
		IR_CORNEA_LOWER = 0x03,
		COLOR_FUNDUS = 0x04,
		NUMBER_OF_ITEMS = (COLOR_FUNDUS + 1)
	};


	enum class LedMode : unsigned char
	{
		Trigger = 0x00,
		Continue = 0x01,
		UNKNOWN = 0x99,
		NUMBER_OF_ITEMS = (Trigger + 1)
	};


	inline int WSODOMAIN_DLL_API LightTypeToIndex(LightType type) {
		int index = static_cast<int>(type);
		if (index < 0 || index >= static_cast<int>(LightType::NUM_LIGHT_LEDS)) {
			return -1;
		}
		return index;
	}
}