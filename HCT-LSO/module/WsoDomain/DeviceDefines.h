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
	constexpr short TRAJECT_SAMPLE_SIZE_MAX = 16384; // 8192;

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
		RET_MIRROR = 0x05,
		OCT_ANT_LENS = 0x06,
		LSO_FILTER = 0x07,
		AP2 = 0x08,
		SWING = 0x09,
		STAGE_X = 0x0A,
		STAGE_Y = 0x0B,
		STAGE_Z = 0x0C,
		CHIN_REST = 0x0F,
		UNKNOWN = 0xFF,
		NUM_MOTORS = (CHIN_REST + 1)
	};

	enum class LightType : unsigned char
	{
		LSO_WHITE_LED = 0x00,
		RETINA_IR_LED = 0x01,  // IR/ICGA
		CORNEA_IR_LEFT_LED = 0x02,
		CORNEA_IR_RIGHT_LED = 0x03,
		EXT_FIXATION_LED = 0x04, 
		LSO_BLUE_LED = 0x05, // FA/Blue FAF mode
		LSO_GREEN_LED = 0x06, // Green FAF 
		// OCT_SLD = 0x04,
		UNKNOWN = 0xFF,
		NUM_LIGHT_LEDS = (LSO_GREEN_LED + 1)
	};

	enum class LaserType : unsigned char
	{
		OCT_SLD = 0x04,
	};

	enum class StepMotorType : unsigned char
	{
		OCT_FOCUS = 0x00,
		OCT_REFER = 0x01,
		OCT_POLAR = 0x02,
		OCT_REFND = 0x03,
		LSO_FOCUS = 0x04,
		RET_MIRROR = 0x05,
		OCT_ANT_LENS = 0x06,
		LSO_FILTER = 0x07,
		AP2 = 0x08,
		SWING = 0x09,
		STAGE_X = 0x0A,
		STAGE_Y = 0x0B,
		STAGE_Z = 0x0C,
		CHIN_REST = 0x0F,
		UNKNOWN = 0xFF,
		NUM_MOTORS = (CHIN_REST + 1)
	};

	enum class StageMotorType : unsigned char
	{
		SWING = 0x09,
		STAGE_X = 0x0A, // 0x09,
		STAGE_Y = 0x0B, // 0x05, 
		STAGE_Z = 0x0C, // 0x0B,
		NUMBER_OF_ITEMS = 4
	};

	enum class CameraType : unsigned char
	{
		IR_CORNEA_LEFT = 0x00,
		IR_CORNEA_RIGHT = 0x01,
		IR_CORNEA_LOWER = 0x02,
		IR_RETINA = 0x03,
		COLOR_FUNDUS = 0x04,
		NUMBER_OF_ITEMS = (COLOR_FUNDUS + 1)
	};

	enum class IrCameraType : unsigned short
	{
		RETINA = 0x00,
		ANTERIOR = 0x01,
		NUMBER_OF_ITEMS = (ANTERIOR + 1)
	};

	enum class IrCameraId : unsigned char
	{
		RETINA = 0x00,
		CORNEA_LOWER = 0x01,
		CORNEA_LEFT = 0x02,
		CORNEA_RIGHT = 0x03,
		NUMBER_OF_ITEMS = (CORNEA_RIGHT + 1)
	};

	enum class IrCameraViewPreset : unsigned char
	{
		DEFAULT = 0x00,
		LOWER_ONLY = 0x01,
		LEFT_ONLY = 0x02,
		LOWER_LEFT = 0x03,
		RIGHT_ONLY = 0x04,
		LOWER_RIGHT = 0x05,
		LEFT_RIGHT = 0x06,
		LOWER_LEFT_RIGHT = 0x07,
		NUMBER_OF_ITEMS = (LOWER_LEFT_RIGHT + 1)
	};

	enum class IrCameraMirrorMode : unsigned char
	{
		MIRROR_OFF = 0x00,
		MIRROR_ON = 0x01,
		NUMBER_OF_ITEMS = (MIRROR_ON + 1)
	};

	enum class IrCameraOutputMode : unsigned char
	{
		NORMAL = 0x00,
		TEST_PATTERN = 0x01,
		NUMBER_OF_ITEMS = (TEST_PATTERN + 1)
	};

	enum class IrCameraTestPattern : unsigned char
	{
		STANDARD = 0x00,
		TOP_BOTTOM_BAR = 0x04,
		LEFT_RIGHT_BAR = 0x08,
		RANDOM = 0x01,
		SQUARE = 0x12,
		BLACK = 0x03
	};

	enum class IrCameraTestOption : unsigned char
	{
		NONE = 0x00,
		ROLLING_BAR_FLAG = 0x40, 
		TRANSPARENT_FLAG = 0x80 
	};

	enum class IrAnteriorViewWidth : unsigned int
	{
		LOWER_ONLY = 644,
		LEFT_ONLY = 644,
		RIGHT_ONLY = 644,
		LOWER_LEFT = 1288,
		LEFT_RIGHT = 1288,
		LOWER_RIGHT = 1288,
		LOWER_LEFT_RIGHT = 1932,
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