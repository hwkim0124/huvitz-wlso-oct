#pragma once

#include "WsoBoard.h"
#include "HbsDefines.h"

namespace wso_board
{
	class HbsDataComm;

	class WSOBOARD_DLL_API HbsDataProfile
	{
	public:
		HbsDataProfile();
		virtual ~HbsDataProfile();

		HbsDataProfile(HbsDataProfile&& rhs);
		HbsDataProfile& operator=(HbsDataProfile&& rhs);
		HbsDataProfile(const HbsDataProfile& rhs);
		HbsDataProfile& operator=(const HbsDataProfile& rhs);

	public:
		void setHbsDataComm(HbsDataComm* comm);

		bool loadHbsTableHeader(void);
		bool loadHbsTableEntries(void);
		bool loadBufferDescriptor(void);
		bool loadCalibrationBlocks(bool fetch, int region = CALIB_ACTIVE_REGION);

		bool loadCalibBlockMotorSets(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockOctParams(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockLedSource(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockOctGalvano(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockDeviceCfg(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockStepMotors(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockFactorySet1(bool fetch, int region = CALIB_ACTIVE_REGION);
		bool loadCalibBlockFactorySet2(bool fetch, int region = CALIB_ACTIVE_REGION);

		bool loadConfiguration(void);
		bool loadMainBoardVersion(void);
		bool loadSystemInitStatus(void);
		bool loadSystemConfigure(void);
		bool loadLsoScannerParam(void);

		bool loadGpioStatus(void);
		bool loadSldStatus(void);
		bool loadZynqXADC(void);
		bool loadStepMotorStatus(StepMotorType type);
		bool loadStageMotorStatus(StageMotorType type);

		bool loadInfraredCameraStatus(void);
		bool loadGalvanoDynamicParam(void);

		bool saveCalibBlockMotorSets(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockOctParams(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockLedSource(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockOctGalvano(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockDeviceCfg(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockStepMotors(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockFactorySet1(bool write, int region = CALIB_ACTIVE_REGION);
		bool saveCalibBlockFactorySet2(bool write, int region = CALIB_ACTIVE_REGION);

		bool saveCalibration(void);
		bool saveConfiguration(void);
		bool saveSystemConfigure(void);
		bool saveLsoScannerParam(void);
		bool saveGalvanoDynamicParam(void);

		const HbsTableDescriptor* getHbsTableDescriptor(void) const;
		const HbsBufferDescriptor* getHbsBufferDescriptor(void) const;
		const HbsCalibsDescriptor* getHbsCalibsDescriptor(void) const;

		const HbsCalibMotorSets* getHbsCalibMotorSets(void) const;
		const HbsCalibOctParams* getHbsCalibOctParams(void) const;
		const HbsCalibLedSource* getHbsCalibLedSource(void) const;
		const HbsCalibOctGalvano* getHbsCalibOctGalvano(void) const;
		const HbsCalibDeviceCfg* getHbsCalibDeviceCfg(void) const;
		const HbsCalibStepMotors* getHbsCalibStepMotors(void) const;
		const HbsCalibFactorySet1* getHbsCalibFactorySet1(void) const;
		const HbsCalibFactorySet2* getHbsCalibFactorySet2(void) const;

		const HbsCalibration* getHbsCalibration(void) const;
		const HbsConfiguration* getHbsConfiguration(void) const;
		const HbsMainBoardVersion* getHbsMainBoardVersion(void) const;
		const HbsSystemInitStatus* getHbsSystemInitStatus(void) const;
		const HbsSystemConfig* getHbsSystemConfigure(void) const;
		const HbsGpioStatus* getHbsGpioStatus(void) const;
		const HbsSldStatus* getHbsSldStatus(void) const;

		const HbsStepMotorStatus* getHbsOctFocusMotor(void) const;
		const HbsStepMotorStatus* getHbsOctReferMotor(void) const;
		const HbsStepMotorStatus* getHbsOctPolarMotor(void) const;
		const HbsStepMotorStatus* getHbsOctReferNdMotor(void) const;
		const HbsStepMotorStatus* getHbsOctAntLensMotor(void) const;
		const HbsStepMotorStatus* getHbsLsoFocusMotor(void) const;
		const HbsStepMotorStatus* getHbsLsoFilterMotor(void) const;
		const HbsStepMotorStatus* getHbsRetMirrorMotor(void) const;

		const HbsStepMotorStatus* getHbsXstageMotor(void) const;
		const HbsStepMotorStatus* getHbsYstageMotor(void) const;
		const HbsStepMotorStatus* getHbsZstageMotor(void) const;
		const HbsStepMotorStatus* getHbsSwingMotor(void) const;
		const HbsStepMotorStatus* getHbsChinRestMotor(void) const;
		const HbsChinrestMotor* getHbsChinrestMotor(void) const;

		const HbsInfraredCameraStatus* getHbsIrCameraStatus(void) const;
		const HbsLedStatus* getHbsLedStatus(void) const;
		const HbsLsoScanner* getHbsLsoScanner(void) const;
		const HbsOctGalvano* getHbsGalvanometer(void) const;
		const HbsGalvanoDynamicParam* getHbsGalvanoDynamicParam(void);
		const HbsZyncXADC* getHbsZyncXADC(void) const;

		const HbsStepMotorStatus* getHbsStepMotorStatus(StepMotorType type) const;
		const HbsStageMotorStatus* getHbsStageMotorStatus(StageMotorType type) const;

		static int getHbsTableMotorIndex(StepMotorType type);
		static int getHbsTableMotorIndex(StageMotorType type);

	protected:
		HbsDataComm* getDataChannel(void) const ;

	private:
		struct HbsDataProfileImpl;
		std::unique_ptr<HbsDataProfileImpl> d_ptr;
		HbsDataProfileImpl& impl(void) const;
	};
}


