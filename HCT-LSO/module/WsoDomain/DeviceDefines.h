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
		CHIN_REST = 0x0D,
		UNKNOWN = 0xFF,
		NUM_MOTORS = (CHIN_REST + 1)
	};

	enum class LsoFilterColorMode : int {
		Color = 0,
		BlueFAF = 1,
		GreenFAF = 2,
		ICGA = 3
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
		CHIN_REST = 0x0D,
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


	inline int LightTypeToIndex(LightType type) {
		int index = static_cast<int>(type);
		if (index < 0 || index >= static_cast<int>(LightType::NUM_LIGHT_LEDS)) {
			return -1;
		}
		return index;
	}


	// System Initialization Status (HBS Table ID 4)
	/////////////////////////////////////////////////////////////////////////////////////////////

	// Overall mainboard init state, mirrors SYS_INIT_* in WsoBoard/wso_def.h.
	enum class SystemInitState : int
	{
		PROGRESS = 0x00,
		FAIL = 0x01,
		OK = 0x02,
	};

	// Bit index into SysInitStatus::devStatus, and the index into
	// SysInitStatus::devErrCode[] for the same subsystem. Bit set means the
	// subsystem initialized successfully. These are bit indices, not masks:
	// the error code array is indexed by the same number.
	// Motor bits are contiguous: bit = SUBSYS_MOTOR_BIT_BASE + (int)MotorType.
	enum class SubsystemBit : int
	{
		MAIN_BOARD = 0x00,
		CALIB_ROM = 0x01,
		STM_BOARD = 0x02,
		AAM_BOARD = 0x03,
		SLD_BOARD = 0x04,
		FIXATION_LCD = 0x05,
		ANT_CAM_LOWER = 0x06,
		ANT_CAM_LEFT = 0x07,
		ANT_CAM_RIGHT = 0x08,
		RETINA_CAM = 0x09,

		OCT_FOCUS_MOTOR = 0x0A,
		OCT_REFER_MOTOR = 0x0B,
		OCT_POLAR_MOTOR = 0x0C,
		OCT_REFND_MOTOR = 0x0D,
		LSO_FOCUS_MOTOR = 0x0E,
		RET_MIRROR_MOTOR = 0x0F,
		OCT_ANT_LENS_MOTOR = 0x10,
		LSO_FILTER_MOTOR = 0x11,
		AP2_MOTOR = 0x12,
		SWING_MOTOR = 0x13,
		STAGE_X_MOTOR = 0x14,
		STAGE_Y_MOTOR = 0x15,
		STAGE_Z_MOTOR = 0x16,
		CHIN_REST_MOTOR = 0x17,

		OCT_XY_GALVANO = 0x18,
		OCT_REF_PHASE_GALVANO = 0x19,
		LSO_Y_GALVANO = 0x1A,
		LED_CTRL_BOARD = 0x1B,
		FT601 = 0x1C,

		NUM_SUBSYSTEMS = (FT601 + 1)
	};

	// devStatus is 32 bits wide but only the low NUM_SUBSYSTEMS are meaningful.
	// Firmware sets every unused field to 1, so the upper bits must never be
	// evaluated as if they described a device.
	constexpr int SUBSYS_MOTOR_BIT_BASE = static_cast<int>(SubsystemBit::OCT_FOCUS_MOTOR);
	constexpr int SUBSYS_DEV_ERR_CODE_SIZE = 32;

	inline int MotorTypeToSubsystemBit(MotorType type) {
		int index = static_cast<int>(type);
		if (index < 0 || index >= static_cast<int>(MotorType::NUM_MOTORS)) {
			return -1;
		}
		return SUBSYS_MOTOR_BIT_BASE + index;
	}

	// Per-subsystem error codes. The same numeric value means different things
	// for different subsystems, so a code is only meaningful together with its
	// bit index -- decode via describeSubsystemError(), never by value alone.
	enum class StepMotorInitError : int
	{
		SEARCH_PI = 0x01,
		WAIT_STOP = 0x02,
		ESCAPE_PI = 0x03,
		MOVE_INIT_POS = 0x04,
	};

	enum class SldInitError : int
	{
		I2C_SWITCH_SET = 0x01,
		DPOT_READ = 0x02,
		DPOT_WRITE = 0x03,
		UPDATE_PARAM = 0x04,
		INIT_SLD_MONITOR = 0x05,
		GET_MONITOR_DATA = 0x06,
	};

	enum class FixationLcdInitError : int
	{
		INTERFACE_CHECK = 0x01,
	};

	enum class AntCameraInitError : int
	{
		TCA9548_SET = 0x01,
		OG0VA_INIT = 0x02,
		VTC_SYNC_LOST = 0x03,
		PXLCNT = 0x04,
		VBLANK_TIMEOUT = 0x05,
		SAV_TIMEOUT = 0x06,
	};

	enum class RetinaCameraInitError : int
	{
		TCA9548_SET = 0x01,
		SENSOR_INIT = 0x02,
	};

	enum class GalvanoInitError : int
	{
		DRIVER_FAULT = 0x01,
	};

	// This header is consumed by SemtSegm, which builds at /std:c++17 -- keep it
	// C++17-clean. No `using enum`; qualify case labels explicitly.
	inline const char* SubsystemBitToName(int bit) {
		switch (static_cast<SubsystemBit>(bit)) {
			case SubsystemBit::MAIN_BOARD:				return "M/B";
			case SubsystemBit::CALIB_ROM:				return "Calibration ROM";
			case SubsystemBit::STM_BOARD:				return "STM B/D";
			case SubsystemBit::AAM_BOARD:				return "AAM B/D";
			case SubsystemBit::SLD_BOARD:				return "SLD B/D";
			case SubsystemBit::FIXATION_LCD:			return "Fixation LCD";
			case SubsystemBit::ANT_CAM_LOWER:			return "Anterior Camera Lower";
			case SubsystemBit::ANT_CAM_LEFT:			return "Anterior Camera Left";
			case SubsystemBit::ANT_CAM_RIGHT:			return "Anterior Camera Right";
			case SubsystemBit::RETINA_CAM:				return "Retina Camera";
			case SubsystemBit::OCT_FOCUS_MOTOR:			return "OCT Focus Motor";
			case SubsystemBit::OCT_REFER_MOTOR:			return "OCT Ref Motor";
			case SubsystemBit::OCT_POLAR_MOTOR:			return "OCT Pol Motor";
			case SubsystemBit::OCT_REFND_MOTOR:			return "OCT RefND Motor";
			case SubsystemBit::LSO_FOCUS_MOTOR:			return "LSO Focus Motor";
			case SubsystemBit::RET_MIRROR_MOTOR:		return "Return Mirror Motor";
			case SubsystemBit::OCT_ANT_LENS_MOTOR:		return "Anterior Lens Motor";
			case SubsystemBit::LSO_FILTER_MOTOR:		return "LSO Filter Motor";
			case SubsystemBit::AP2_MOTOR:				return "AP2 Motor";
			case SubsystemBit::SWING_MOTOR:				return "Swing Motor";
			case SubsystemBit::STAGE_X_MOTOR:			return "X-Axis Motor";
			case SubsystemBit::STAGE_Y_MOTOR:			return "Y-Axis Motor";
			case SubsystemBit::STAGE_Z_MOTOR:			return "Z-Axis Motor";
			case SubsystemBit::CHIN_REST_MOTOR:			return "CR Step Motor";
			case SubsystemBit::OCT_XY_GALVANO:			return "OCT XY Galvano Scanner";
			case SubsystemBit::OCT_REF_PHASE_GALVANO:	return "OCT Ref.Phase Galvano Scanner";
			case SubsystemBit::LSO_Y_GALVANO:			return "LSO Y Galvano Scanner";
			case SubsystemBit::LED_CTRL_BOARD:			return "LED CTRL B/D";
			case SubsystemBit::FT601:					return "FT601";
			default:									break;
		}
		return "Unknown";
	}

	// A device error code is only meaningful together with its bit index: code 1
	// is a PI search failure for a motor but an interface check failure for the
	// fixation LCD. Returns an empty string when there is nothing to report.
	inline const char* SubsystemErrorToText(int bit, int code) {
		if (code <= 0) {
			return "";
		}

		if (bit >= SUBSYS_MOTOR_BIT_BASE && bit <= static_cast<int>(SubsystemBit::CHIN_REST_MOTOR)) {
			switch (static_cast<StepMotorInitError>(code)) {
				case StepMotorInitError::SEARCH_PI:		return "Search photo-interrupter error";
				case StepMotorInitError::WAIT_STOP:		return "Wait motor stop error";
				case StepMotorInitError::ESCAPE_PI:		return "Escape photo-interrupter error";
				case StepMotorInitError::MOVE_INIT_POS:	return "Move initial position error";
				default:								return "Unknown motor error";
			}
		}

		switch (static_cast<SubsystemBit>(bit)) {
			case SubsystemBit::STM_BOARD:				return "STM B/D interface error";
			case SubsystemBit::AAM_BOARD:				return "AAM B/D interface error";

			case SubsystemBit::SLD_BOARD:
				switch (static_cast<SldInitError>(code)) {
					case SldInitError::I2C_SWITCH_SET:		return "I2C switch set error";
					case SldInitError::DPOT_READ:			return "Digital potentiometer read error";
					case SldInitError::DPOT_WRITE:			return "Digital potentiometer write error";
					case SldInitError::UPDATE_PARAM:		return "Update parameter error";
					case SldInitError::INIT_SLD_MONITOR:	return "SLD monitor init error";
					case SldInitError::GET_MONITOR_DATA:	return "SLD monitor read data error";
					default:								return "Unknown SLD error";
				}

			case SubsystemBit::FIXATION_LCD:
				switch (static_cast<FixationLcdInitError>(code)) {
					case FixationLcdInitError::INTERFACE_CHECK:	return "ST7789P3 interface check failed / disconnected";
					default:									return "Unknown fixation LCD error";
				}

			case SubsystemBit::ANT_CAM_LOWER:
			case SubsystemBit::ANT_CAM_LEFT:
			case SubsystemBit::ANT_CAM_RIGHT:
				switch (static_cast<AntCameraInitError>(code)) {
					case AntCameraInitError::TCA9548_SET:		return "TCA9548 I2C switch error";
					case AntCameraInitError::OG0VA_INIT:		return "OG0VA sensor init error";
					case AntCameraInitError::VTC_SYNC_LOST:		return "VTC sync lost";
					case AntCameraInitError::PXLCNT:			return "Pixel count error";
					case AntCameraInitError::VBLANK_TIMEOUT:	return "VBLANK timeout";
					case AntCameraInitError::SAV_TIMEOUT:		return "SAV timeout";
					default:									return "Unknown anterior camera error";
				}

			case SubsystemBit::RETINA_CAM:
				switch (static_cast<RetinaCameraInitError>(code)) {
					case RetinaCameraInitError::TCA9548_SET:	return "TCA9548 I2C switch error";
					case RetinaCameraInitError::SENSOR_INIT:	return "Sensor init error";
					default:									return "Unknown retina camera error";
				}

			case SubsystemBit::OCT_XY_GALVANO:
			case SubsystemBit::OCT_REF_PHASE_GALVANO:
			case SubsystemBit::LSO_Y_GALVANO:
				switch (static_cast<GalvanoInitError>(code)) {
					case GalvanoInitError::DRIVER_FAULT:	return "Galvano driver hardware fault";
					default:								return "Unknown galvano error";
				}

			default:									break;
		}
		return "Unspecified error";
	}
}