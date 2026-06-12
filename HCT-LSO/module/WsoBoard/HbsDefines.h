#pragma once

#include "wso_hbs.h"


namespace wso_board
{
	using HbsTableDescriptor = hbs_descriptor_st;
	using HbsBufferDescriptor = buffer_descriptor_st;
	using HbsCalibsDescriptor = HBS_CalBlockBuffer_st;

	using HbsMainBoardVersion = mainboard_version_st;
	using HbsSystemInitStatus = SysInitStatus_st;
	using HbsSystemConfig = SysCfg_st;
	using HbsGpioStatus = GP_Status_st;
	using HbsSldStatus = SLD_st; // SLD_status_st;
	using HbsZyncXADC = Zynq_XADC_st;

	using HbsOctFocusMotor = StepMotorInfo_st;
	using HbsOctReferMotor = StepMotorInfo_st;
	using HbsOctPolarMotor = StepMotorInfo_st;
	using HbsLsoFocusMotor = StepMotorInfo_st;
	using HbsReturnMirrorMotor = StepMotorInfo_st;
	using HbsAnteriorLensMotor = StepMotorInfo_st;
	using HbsSwingMotor = StepMotorInfo_st;
	using HbsXstageMotor = StepMotorInfo_st;
	using HbsYstageMotor = StepMotorInfo_st;
	using HbsZstageMotor = StepMotorInfo_st;
	using HbsChinrestMotor = DC_CR_Motor_st;

	using HbsLedStatus = LED_Info_st;
	using HbsLsoScanner = LsoScanner_st;
	using HbsOctGalvano = OctGalvano_st;
	using HbsRetinaIrCamera = IRCamStatus_st;
	using HbsAnteriorCamera1 = IRCamStatus_st;
	using HbsAnteriorCamera2 = IRCamStatus_st;
	using HbsAnteriorCamera3 = IRCamStatus_st;

	using HbsCalibMotorSets = CalBlk0_st;
	using HbsCalibOctParams = CalBlk1_st;
	using HbsCalibOctSource = CalBlk2_st;
	using HbsCalibOctGalvano = CalBlk3_st;
	using HbsCalibDeviceCfg = CalBlk4_st;
	using HbsCalibStepMotors = CalBlk5_st;
	using HbsCalibFactorySet1 = CalBlk6_st;
	using HbsCalibFactorySet2 = CalBlk7_st;


	using HbsStepMotorStatus = StepMotorInfo_st;
	using HbsStageMotorStatus = StepMotorInfo_st;
	using HbsCalibration = SysCal_st;
	using HbsConfiguration = UserSetup_st;
	using HbsInfraredCameraStatus = IRCamStatus_st;
	using HbsGalvanoDynamicParam = Galvano_Dynamic_st;


	enum class HbsBufferIndex : int
	{
		BUFFER_ENTRIES = TBL_BLKBUF_ID,					// 0
		CALIB_ENTRIES = TBL_CALBLKBUF_ID,				// 1
		SYS_CALIBRATION = TBL_SYSCAL_ID,				// 2
		MAINBOARD_VERSION = TBL_MB_VER_INFO_ID,			// 3
		SYS_INIT_STATUS = TBL_SYS_INIT_STATUS_ID,		// 4
		SYS_CONFIG = TBL_SYS_CFG_ID,					// 5
		GPIO_STATUS = TBL_GP_STATUS_ID,					// 6
		SLD_STATUS = TBL_SLD_STATUS_ID,					// 7
		ZYNQ_XADC = TBL_ZYNQ_XADC_ID,					// 8
		OCT_FOCUS_MOTOR = TBL_OCT_FOCUS_MOTOR_ID,		// 9
		OCT_REFER_MOTOR = TBL_OCT_REF_MOTOR_ID,			// 10
		OCT_POLAR_MOTOR = TBL_OCT_POL_MOTOR_ID,			// 11
		REFER_ND_MOTOR = TBL_REF_ND_MOTOR_ID,				// 12
		LSO_FOCUS_MOTOR = TBL_LSO_FOCUS_MOTOR_ID,		// 13
		RETURN_MIRROR_MOTOR = TBL_RM_MOTOR_ID,						// 14
		ANTERIOR_LENS_MOTOR = TBL_ANTLENS_MOTOR_ID,			// 15
		AP1_MOTOR = TBL_AP1_MOTOR_ID,					// 16
		AP2_MOTOR = TBL_AP2_MOTOR_ID,					// 17
		SWING_MOTOR = TBL_SWING_MOTOR_ID,				// 18
		X_STAGE_MOTOR = TBL_X_MOTOR_ID,					// 19
		Z_STAGE_MOTOR = TBL_Z_MOTOR_ID,					// 20
		Y_STAGE_MOTOR = TBL_Y_MOTOR_ID,					// 21
		CHINREST_MOTOR = TBL_CR_MOTOR_ID,				// 22
		LED_STATUS = TBL_LED_STATUS_ID,					// 23
		LSO_SCANNER = TBL_LSO_SCANNER_ID,				// 24
		OCT_GALVANO = TBL_OCT_GAVANO_ID,				// 25
		RETINA_IR_CAMERA = TBL_RET_IR_CAM_STATUS_ID,	// 26
		ANTERIOR_IR_CAMERA1 = TBL_ANT_IR_CAM_0_STATUS_ID, // 27
		ANTERIOR_IR_CAMERA2 = TBL_ANT_IR_CAM_0_STATUS_ID, // 28
		ANTERIOR_IR_CAMERA3 = TBL_ANT_IR_CAM_0_STATUS_ID, // 29
	};

}