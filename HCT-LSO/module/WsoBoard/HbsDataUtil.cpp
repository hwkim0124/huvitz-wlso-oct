#include "pch.h"
#include "WsoBoard2.h"
#include "HbsDataUtil.h"

using namespace wso_board;


unsigned int wso_board::HbsDataUtil::getCalibBlockDataSize(int blockIdx)
{
	unsigned int size = 0;

	if (blockIdx == CALIB_IDX_MOTOR_SETS) {
		size = sizeof(HbsCalibMotorSets);
	}
	else if (blockIdx == CALIB_IDX_OCT_PARAMS) {
		size = sizeof(HbsCalibOctParams);
	}
	else if (blockIdx == CALIB_IDX_LED_SOURCE) {
		size = sizeof(HbsCalibLedSource);
	}
	else if (blockIdx == CALIB_IDX_OCT_GALVANO) {
		size = sizeof(HbsCalibOctGalvano);
	}
	else if (blockIdx == CALIB_IDX_DEVICE_CFG) {
		size = sizeof(HbsCalibDeviceCfg);
	}
	else if (blockIdx == CALIB_IDX_STEP_MOTORS) {
		size = sizeof(HbsCalibStepMotors);
	}
	else if (blockIdx == CALIB_IDX_FACTORY_SET1) {
		size = sizeof(HbsCalibFactorySet1);
	}
	else if (blockIdx == CALIB_IDX_FACTORY_SET2) {
		size = sizeof(HbsCalibFactorySet2);
	}

	return size;
}
