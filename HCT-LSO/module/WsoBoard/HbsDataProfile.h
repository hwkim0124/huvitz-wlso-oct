#pragma once

#include "WsoBoard2.h"
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

		bool loadHbsDescriptor(void);
		bool loadHbsTableData(void);

		bool loadBulkBuffer(void);
		bool loadConfiguration(void);
		bool loadCalibration(void);
		bool loadMainBoardVersion(void);
		bool loadSystemInitStatus(void);
		bool loadSystemConfigure(void);

		bool loadGpioStatus(void);
		bool loadSldStatus(void);
		bool loadZynqXADC(void);
		bool loadStepMotorStatus(StepMotorType type);
		bool loadStageMotorStatus(void);

		bool loadInfraredCameraStatus(void);
		bool loadGalvanoDynamicParam(void);

		bool saveCalibration(void);
		bool saveConfiguration(void);
		bool saveSystemConfigure(void);
		bool saveLsoScannerParam(void);
		bool saveGalvanoDynamicParam(void);

		const HbsDescriptor* getHbsDescriptor(void) const;
		const HbsBulkBuffer* getHbsBulkBuffer(void) const;
		const HbsCalibration* getHbsCalibration(void) const;
		const HbsConfiguration* getHbsConfiguration(void) const;
		const HbsMainBoardVersion* getHbsMainBoardVersion(void) const;
		const HbsSystemInitStatus* getHbsSystemInitStatus(void) const;
		const HbsSystemConfigure* getHbsSystemConfigure(void) const;
		const HbsGpioStatus* getHbsGpioStatus(void) const;
		const HbsSldStatus* getHbsSldStatus(void) const;

		const HbsStepMotorStatus* getHbsOctFocusMotor(void) const;
		const HbsStepMotorStatus* getHbsOctReferMotor(void) const;
		const HbsStepMotorStatus* getHbsOctPolarMotor(void) const;
		const HbsStepMotorStatus* getHbsOctRefNDMotor(void) const;
		const HbsStepMotorStatus* getHbsOctAntLensMotor(void) const;
		const HbsStepMotorStatus* getHbsLsoFocusMotor(void) const;

		const HbsStepMotorStatus* getHbsXStageMotor(void) const;
		const HbsStepMotorStatus* getHbsYStageMotor(void) const;
		const HbsStepMotorStatus* getHbsZStageMotor(void) const;
		const HbsStepMotorStatus* getHbsSwingMotor(void) const;
		const HbsStepMotorStatus* getHbsTiltMotor(void) const;
		const HbsChinrestMotor* getHbsChinrestMotor(void) const;
		const HbsStepMotorStatus* getHbsFixationMotor(void) const;


		const HbsInfraredCameraStatus* getHbsIrCameraStatus(void) const;
		const HbsLedStatus* getHbsLedStatus(void) const;
		const HbsLsoScanner* getHbsLsoScanner(void) const;
		const HbsGalvanometer* getHbsGalvanometer(void) const;
		const HbsGalvanoDynamicParam* getHbsGalvanoDynamicParam(void);
		const HbsZyncXADC* getHbsZyncXADC(void) const;

		const HbsStepMotorStatus* getHbsStepMotorStatus(StepMotorType type) const;
		const HbsStageMotorStatus* getHbsStageMotorStatus(StageMotorType type) const;

		static int getHbsTableIndex(StepMotorType type);

	protected:
		HbsDataComm* getDataChannel(void) const ;

	private:
		struct HbsDataProfileImpl;
		std::unique_ptr<HbsDataProfileImpl> d_ptr;
		HbsDataProfileImpl& impl(void) const;
	};
}


