#pragma once

#include "HbsDataProfile.h"
#include "HbsStructs.h"


namespace wso_board
{
	class WSOBOARD_DLL_API HbsDataComm {
	public:
		virtual bool readDescriptor(const HbsTableDescriptor* data) = 0;

		virtual bool readBulkBuffer(const HbsBufferEntries* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readCalibration(const HbsCalibration* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readConfiguration(const HbsConfiguration* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readMainBoardVersion(const HbsMainBoardVersion* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readSystemInitStatus(const HbsSystemInitStatus* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readSystemConfigure(const HbsSystemConfig* data, const HbsTableDescriptor* desc) = 0;
		
		virtual bool readGpioStatus(const HbsGpioStatus* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readSldStatus(const HbsSldStatus* data, const HbsTableDescriptor* desc) = 0;
	
		virtual bool readZynqXADC(const HbsZyncXADC* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readInfraredCameraStatus(const HbsInfraredCameraStatus* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readStepMotorStatus(const HbsStepMotorStatus* data, StepMotorType type, const HbsTableDescriptor* desc) = 0;
		virtual bool readStageMotorStatus(const HbsStageMotorStatus* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsTableDescriptor* desc) = 0;
		virtual bool readLsoScannerParam(const HbsLsoScanner* data, const HbsTableDescriptor* desc) = 0;

		virtual bool writeCalibration(const HbsCalibration* data, const HbsTableDescriptor* desc) = 0;
		virtual bool writeConfiguration(const HbsConfiguration* data, const HbsTableDescriptor* desc) = 0;
		virtual bool writeSystemConfigure(const HbsSystemConfig* data, const HbsTableDescriptor* desc) = 0;
		virtual bool writeGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsTableDescriptor* desc) = 0;
		virtual bool writeLsoScannerParam(const HbsLsoScanner* data, const HbsTableDescriptor* desc) = 0;

		virtual bool writeTrajectoryParam(std::uint8_t tid, const TrajectoryProfileParam* param, const HbsTableDescriptor* desc) = 0;
		virtual bool writeTrajectoryPositionsX(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsTableDescriptor* desc) = 0;
		virtual bool writeTrajectoryPositionsY(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsTableDescriptor* desc) = 0;
	};
}
