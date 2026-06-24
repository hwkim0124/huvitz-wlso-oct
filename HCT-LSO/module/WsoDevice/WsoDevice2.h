#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)


#ifdef WSODEVICE_EXPORTS
#define WSODEVICE_DLL_API		__declspec(dllexport)
#else
#define WSODEVICE_DLL_API		__declspec(dllimport)
#endif

#include <memory>
#include <string>
#include <cstdint>
#include <cmath>

#include "wso_domain.h"
#include "wso_board.h"
#include "cpp_util.h"

namespace wso_device
{
	using namespace wso_domain;
	using namespace wso_board;
	using namespace cpp_util;
	using namespace std;

	// Device Mode 
	///////////////////////////////////////////////////////////////////////////////////////////
	constexpr int DEV_FLAG_NORMAL = 0x00;
	constexpr int DEV_FLAG_NOT_IR_CAMERA = (1 << 1);
	constexpr int DEV_FLAG_NOT_SOURCE_LEDS = (1 << 2);
	constexpr int DEV_FLAG_NOT_MOTORS = (1 << 3);
	constexpr int DEV_FLAG_NOT_OCT_GRAB = (1 << 4);
	constexpr int DEV_FLAG_NOT_SLO_GRAB = (1 << 5);
	constexpr int DEV_FLAG_NOT_OPENCL = (1 << 6);
	constexpr int DEV_FLAG_NOT_MAINBOARD = (1 << 7);


	constexpr std::uint16_t LED_INTENSITY_MIN = 0;
	constexpr std::uint16_t LED_INTENSITY_MAX = 255;
	constexpr std::uint16_t LED_INTENSITY_INIT = 0;
	constexpr std::uint16_t LED_INTENSITY_STEP = 1;


	// Light LEDs
	//////////////////////////////////////////////////////////////////////////////////////////
	constexpr std::uint16_t LED_LSO_WHITE_INIT_VALUE = 0;
	constexpr std::uint16_t LED_RETINA_IR_INIT_VALUE = 0;
	constexpr std::uint16_t LED_CORNEA_LEFT_IR_INIT_VALUE = 0;
	constexpr std::uint16_t LED_CORNEA_RIGHT_IR_INIT_VALUE = 0;
	constexpr std::uint16_t LED_CORNEA_LOWER_IR_INIT_VALUE = 0;

	constexpr const char* LED_LSO_WHITE_NAME = "LsoWhiteLed";
	constexpr const char* LED_RETINA_IR_NAME = "RetinaIrLed";
	constexpr const char* LED_CORNEA_LEFT_IR_NAME = "CorneaLeftIrLed";
	constexpr const char* LED_CORNEA_RIGHT_IR_NAME = "CorneaRightIrLed";
	constexpr const char* LED_CORNEA_LOWER_IR_NAME = "CorneaLowerIrLed";
	constexpr const char* LED_UNKNOWN_NAME = "UnknownLed";


	// Laser LEDs
	////////////////////////////////////////////////////////////////////////////////////////
	constexpr const char* OCT_SLD_NAME = "OctSld";


	// LCD Fixation
	///////////////////////////////////////////////////////////////////////////////////////
	constexpr int LCD_FIXATION_ROW_MIN = -199;
	constexpr int LCD_FIXATION_ROW_MAX = +199;
	constexpr int LCD_FIXATION_COL_MIN = -199;
	constexpr int LCD_FIXATION_COL_MAX = +199;
	constexpr int LCD_FIXATION_ROW_MID = ((LCD_FIXATION_ROW_MAX - LCD_FIXATION_ROW_MIN) / 2);
	constexpr int LCD_FIXATION_COL_MID = ((LCD_FIXATION_COL_MAX - LCD_FIXATION_COL_MIN) / 2);


	// Step & Stage Motor
	////////////////////////////////////////////////////////////////////////////////////////////
	constexpr const char* MOTOR_OCT_FOCUS_NAME = "OctFocusMotor";
	constexpr const char* MOTOR_OCT_REFER_NAME = "OctReferenceMotor";
	constexpr const char* MOTOR_OCT_POLAR_NAME = "OctPolarizationMotor";
	constexpr const char* MOTOR_OCT_REFND_NAME = "OctRefNdFilterMotor";
	constexpr const char* MOTOR_LSO_FOCUS_NAME = "LsoFocusMotor";
	constexpr const char* MOTOR_RET_MIRROR_NAME = "ReturnMirrorMotor";
	constexpr const char* MOTOR_OCT_ANT_LENS_NAME = "OctAnteriorLensMotor";
	constexpr const char* MOTOR_LSO_FILTER_NAME = "LsoFilterMotor";

	constexpr const char* MOTOR_SWING_NAME = "SwingMotor";
	constexpr const char* MOTOR_STAGE_X_NAME = "XstageMotor";
	constexpr const char* MOTOR_STAGE_Y_NAME = "YstageMotor";
	constexpr const char* MOTOR_STAGE_Z_NAME = "ZstageMotor";
	constexpr const char* MOTOR_CHIN_REST_NAME = "ChinRestMotor";

	constexpr const char* MOTOR_UNKNOWN_NAME = "UnknownMotor";

	constexpr std::int32_t MOTOR_OCT_FOCUS_INIT_POS = 1400;
	constexpr std::int32_t MOTOR_OCT_REFER_INIT_POS = 42000;
	constexpr std::int32_t MOTOR_OCT_POLAR_INIT_POS = 19;
	constexpr std::int32_t MOTOR_OCT_REFND_INIT_POS = 0;
	constexpr std::int32_t MOTOR_LSO_FOCUS_INIT_POS = 1400;
	constexpr std::int32_t MOTOR_RET_MIRROR_INIT_POS = 0;
	constexpr std::int32_t MOTOR_OCT_ANT_LENS_INIT_POS = 0;
	constexpr std::int32_t MOTOR_LSO_FILTER_INIT_POS = 0;

	constexpr std::int32_t MOTOR_SWING_INIT_POS = 0;
	constexpr std::int32_t MOTOR_STAGE_X_INIT_POS = 0;
	constexpr std::int32_t MOTOR_STAGE_Y_INIT_POS = 0;
	constexpr std::int32_t MOTOR_STAGE_Z_INIT_POS = 0;
	constexpr std::int32_t MOTOR_CHIN_REST_INIT_POS = 0;

	constexpr int MOTOR_END_OF_RANGE_OFFSET = 0; // 3;
	constexpr int MOTOR_CENTER_OF_POSITION_OFFSET = 1; // 3;
	constexpr int MOTOR_MOVE_WAIT_DELAY = 25;
	constexpr int MOTOR_MOVE_WAIT_COUNT = 400;
	constexpr int MOTOR_MOVE_INIT_COUNT = 100;

	constexpr int MOTOR_OCT_FOCUS_STEPS_PER_DIOPTER = 210;
	constexpr int MOTOR_LSO_FOCUS_STEPS_PER_DIOPTER = 224;
	constexpr int MOTOR_OCT_POLAR_STEPS_PER_DEGREE = 40; // 4;

	constexpr float MOTOR_LSO_FOCUS_DIOPTER_RANGE = 30.0f;
	constexpr float MOTOR_LSO_FOCUS_DIOPTER_MAX = 15.0f;
	constexpr float MOTOR_LSO_FOCUS_DIOPTER_MIN = -15.0f;

	constexpr float MOTOR_FIXATION_DIOPTER_RANGE = 30.0f;
	constexpr float MOTOR_FIXATION_DIOPTER_MAX = 15.0f;
	constexpr float MOTOR_FIXATION_DIOPTER_MIN = -15.0f;

	constexpr float MOTOR_OCT_FOCUS_DIOPTER_MAX = +20.0f;
	constexpr float MOTOR_OCT_FOCUS_DIOPTER_MIN = -20.0f;


	// Stage Motors 
	//////////////////////////////////////////////////////////////////////////////////////////
	constexpr int STAGE_END_OF_LIMIT_OFFSET = 1;

	constexpr int STAGE_X_LIMIT_RANGE_LOW = -30;
	constexpr int STAGE_X_LIMIT_RANGE_HIGH = +30;
	constexpr int STAGE_Y_LIMIT_RANGE_LOW = 0;
	constexpr int STAGE_Y_LIMIT_RANGE_HIGH = 48000;
	constexpr int STAGE_Z_LIMIT_RANGE_LOW = -30;
	constexpr int STAGE_Z_LIMIT_RANGE_HIGH = +30;


	// Infrared Camera 
	/////////////////////////////////////////////////////////////////////////////////////////
	constexpr int IR_CAMERA_FRAME_WIDTH = 644;
	constexpr int IR_CAMERA_FRAME_HEIGHT = 480;
	constexpr int IR_CAMERA_FRAME_DATA_SIZE = (IR_CAMERA_FRAME_WIDTH * IR_CAMERA_FRAME_HEIGHT);
	constexpr int IR_CAMERA_FRAME_PADD_SIZE = 4;

	constexpr int IR_CAMERA_IMAGE_WIDTH = (IR_CAMERA_FRAME_WIDTH - IR_CAMERA_FRAME_PADD_SIZE);
	constexpr int IR_CAMERA_IMAGE_HEIGHT = IR_CAMERA_FRAME_HEIGHT;
	constexpr uint16_t IR_CAMERA_INTENSITY = 20;

	constexpr std::uint8_t IR_CAMERA_AGAIN_VALUE1 = 1;
	constexpr std::uint8_t IR_CAMERA_AGAIN_VALUE2 = 2;
	constexpr std::uint8_t IR_CAMERA_AGAIN_VALUE3 = 4;
	constexpr std::uint8_t IR_CAMERA_AGAIN_VALUE4 = 8;
	constexpr std::float_t IR_CAMERA_DGAIN_MIN = 0.0f;
	constexpr std::float_t IR_CAMERA_DGAIN_MAX = 8.0f;
	constexpr std::float_t IR_CAMERA_DGAIN_INIT = 1.0f;

	constexpr std::uint16_t IR_CAMERA_EXPOSURE_TIME1 = 0x03BC;
	constexpr std::uint16_t IR_CAMERA_EXPOSURE_TIME2 = 0x0706;


	// Control Value
	////////////////////////////////////////////////////////////////////////////////////////////
	constexpr unsigned char CTRL_OFF = 0x00;
	constexpr unsigned char CTRL_ON = 0x01;
	constexpr unsigned char TURN_OFF = 0x00;
	constexpr unsigned char TURN_ON = 0x01;

	constexpr unsigned char MIRROR_OUT = 0x00;
	constexpr unsigned char MIRROR_IN = 0x01;


	// USB FTDI Descriptor 
	//////////////////////////////////////////////////////////////////////////////////////////
	constexpr unsigned char USB_READ_PID = 0x82;
	constexpr unsigned char USB_WRITE_PID = 0x02;
	constexpr unsigned char USB_IR1_PID = 0x83;
	constexpr unsigned char USB_IR2_PID = 0x84;
	constexpr unsigned char USB_EMPTY_PID = 0x00;
	constexpr unsigned char USB_NOTI_PID = 0x85;
	constexpr unsigned long USB_TIMEOUT = 12000; // 3000; // 15000;

	constexpr const char* USB_MAIN_BOARD_DESC = "SLO_CTRL";
	constexpr const char* USB_SUB_BOARD_DESC = "RTCAM_CTRL";
	constexpr std::uint32_t USB_BOARD_ADDR = 0x80000000;


	// USB Port Control
	////////////////////////////////////////////////////////////////////////////////////////////
	constexpr std::uint32_t FTDI_RESET_WAIT_TIME = 10000;
	constexpr std::uint32_t USB_RESET_WAIT_TIME = 1000;
	constexpr std::uint32_t USB_RESET_RETRY_DELAY = 1000;
	constexpr std::uint32_t USB_RESET_RETRY_MAX = 3;

	constexpr std::uint32_t USB_RESET_COLOR_WAIT_TIME = 10000;


			
	enum class SldControlChannel : unsigned char {
		RMON_HIGH_CODE = 0,
		RMON_LOW_CODE1 = 1,
		RMON_LOW_CODE2 = 2,
		RMON_RSI_CODE = 3
	};

	enum class IrCameraId : unsigned char
	{
		CORNEA_LEFT = 0x00,
		CORNEA_RIGHT = 0x01,
		RETINA = 0x02,
		CORNEA_LOWER = 0x03,
		UNKNOWN = 0x00
	};

	enum class PacketType : unsigned char
	{
		NREAD = 0x5A,
		NREAD_RESP_OK = 0x75,
		NREAD_RESP_ERR = 0x7A,
		NWRITE = 0x33,
		NWRITE_R = 0x3C,
		NWRITE_RESP = 0x7C,
		MSG_CMD = 0xC5,
		MSG_RESP = 0xCC,
		STATUS_OK = 0x01,
		STATUS_ERR = 0x02, 
		STATUS_RETRY = 0x03,
	};

	enum class ScanModeType : unsigned short
	{
		SCAN_MODE_X = 0x0000,
		SCAN_MODE_Y = 0x0001,
		SCAN_MODE_XY = 0x0003
	};

	enum class CommandType : unsigned short
	{
		LED_CTL = 0x0230,
		LED_MODE = 0x0231,

		GALVO_MOVEX = 0x0001,
		GALVO_MOVEY = 0x0002,
		GALVO_MOVEXY = 0x0003,
		GALVO_SLEWX = 0x0004,
		GALVO_SLEWY = 0x0005,
		GALVO_SLEWXY = 0x0006,
		GALVO_DMOVEX = 0x0007,
		GALVO_DMOVEY = 0x0008,
		GALVO_DMOVEXY = 0x0009,
		START_ACQ = 0x000E,
		START_ACQR = 0x0010,
		START_ACQRF = 0x0012,
		GALVO_DSLEWXY = 0x000F,
		FLASH = 0xF000,
		MOTJOG = 0x0100,
		MOTMOVE = 0x0101,
		MOTHOME = 0x0102,
		MOT_SPEED = 0x0103,
		FLEDSOL = 0x0104,
		LOAD_SM_DEFAULT = 0x0105,
		MOT_VMOVE = 0x0106,
		SMOTSTOP = 0x0107,
		IRCAM = 0x0205,
		IRLED = 0x0206,
		INTFIXA = 0x0207,
		EXTFIXA = 0x0208,
		RNGELED = 0x0209,
		RNGFLED = 0x020A,
		PNL_LED = 0x020B,
		SLD_CTRL = 0x020C,
		LCDFIXPARAM = 0x020E,
		LCDFIXA = 0x020F,
		SLD_POTENTIO = 0x0210,
		FLASH_SLD_POT = 0x0211,
		GET_SLD_POT = 0x0212,

		FPGA_UP = 0xF00F,
		UPDATE_CAL_ROM = 0xF033,
		RESET_CPU = 0xF355,
		UPDATE_DEV_STATUS = 0x0303,
		TEST_CMD = 0x03FF,
		FLSH_WR = 0x03FF,
		QRTNM = 0x4000,
		SPLIT_FM = 0x4001,
		FDCS_LENS = 0x4002,
		PUPIL_MASK = 0x4003,
		COR740_LED = 0x4004,
		COR940_LED = 0x4005,
		FLASH_LED = 0x4006,
		LSO_TRIG_MODE = 0x4007,
		SPFOCUS_LED = 0x4008,
		WDOT_LED = 0x4009,
		WDOT_LED2 = 0x400A,
		IRCAM_AGAIN = 0x4010,
		IRCAM_DGAIN = 0x4011,
		XZ_STOP = 0x4012,
		AF_XZMOVE = 0x4020,
		AF_XZPOS = 0x4021,
		CR_MOVE = 0x4022,
		CR_STOP = 0x4023,
		IRCAM_INT = 0x5000,
		UNKNOWN = 0xFFEF,

		LOAD_CALIB = 0x4100,
		SAVE_CALIB = 0x4101,
		LOAD_CALIB2 = 0x4102,
		SAVE_CALIB2 = 0x4103, 

		ERASE_CALBLOCK = 0x4104, 
		FLASH_CALBLOCK = 0x4105,
		LOAD_FLASHCALIB = 0x4106,

		LSO_SCANNER_CTRL = 0x6000,
		LSO_Y_GALVO_MOVE = 0x6001,
		OCT_PHASE_CTRL = 0x7000,
		OCT_SET_PHASE = 0x7001,
		/*
		SLO_CAPTURE_CMD = 0x6002,
		SLO_AGAIN = 0x6003,
		SLO_AOFFSET = 0x6004,
		SLO_DET_GAIN = 0x6005,

		QLD_SET_MODE = 0x7000,
		QLD_SET_REF = 0x7001,
		QLD_SET_LIMIT = 0x7002,
		QLD_SET_RMD = 0x7003,
		QLD_WRITE_EEPROM = 0x700F,
		QLD_READ_EEPROM = 0x701F, 
		QLD_WRITE_LD_INFO = 0x7004, 
		QLD_READ_LD_INFO = 0x7005,
		QLD_SET_PROFILE = 0x7006,
		*/

		SET_LSRC_CFG = 0x7700,
	};
}