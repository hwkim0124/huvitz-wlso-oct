#include "pch.h"
#include "HbsDefines.h"
#include "HbsDataProfile.h"
#include "HbsDataComm.h"

#include <optional>


using namespace wso_board;
using namespace std;


struct HbsDataProfile::HbsDataProfileImpl
{
	optional<HbsDataComm*> dataComm { nullopt };

	HBS_descriptor_st	HBS_descriptor{};
	blk_buf_desc_st 	HBS_BlkBufTbl{};
	SysCal_st 			HBS_SysCal{};
	UserSetup_st 		HBS_UserSetup{};
	MainBoardVerInfo_st HBS_MainBoardVerInfo{};
	SysInitStatus_st 	HBS_SysInitStatus{};
	SysCfg_st 			HBS_SysCfg{};

	GP_Status_st 		HBS_GP_Status{};
	SLD_status_st 		HBS_SLD_status{};
	Zynq_XADC_st		HBS_ZynqXADC{};

	HbsStepMotorStatus 	HBS_OctFocusMotor{};
	HbsStepMotorStatus 	HBS_OctRefMotor{};
	HbsStepMotorStatus 	HBS_OctPolMotor{};
	HbsStepMotorStatus 	HBS_OctRefNDMotor{};
	HbsStepMotorStatus  HBS_OctAntLensMotor{};
	HbsStepMotorStatus 	HBS_LsoFocusMotor{};

	HbsStepMotorStatus	HBS_XstageMotor{};
	HbsStepMotorStatus	HBS_ZstageMotor{};
	HbsStepMotorStatus	HBS_YstageMotor{};

	HbsStepMotorStatus  HBS_SwingMotor{};
	HbsStepMotorStatus  HBS_TiltMotor{};
	HbsStepMotorStatus	HBS_FixMotor{};
	DC_CR_Motor_st 		HBS_CR_Motor{};

	IRCamStatus_st 		HBS_IRCamstatus{};
	LED_Info_st   		HBS_LED_status{};

	LsoScanner_st		HBS_LsoScanner{};
	OctGalvano_st		HBS_Galvanometer{};
	Galvano_Dynamic_st	HBS_GalvanoDynamicParam{};

	HbsDataProfileImpl()
	{ 
	}
};


HbsDataProfile::HbsDataProfile() :
	d_ptr(make_unique<HbsDataProfileImpl>())
{
}


HbsDataProfile::~HbsDataProfile()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_board::HbsDataProfile::HbsDataProfile(HbsDataProfile&& rhs) = default;
HbsDataProfile& wso_board::HbsDataProfile::operator=(HbsDataProfile&& rhs) = default;


wso_board::HbsDataProfile::HbsDataProfile(const HbsDataProfile& rhs)
	: d_ptr(make_unique<HbsDataProfileImpl>(*rhs.d_ptr))
{
}


HbsDataProfile& wso_board::HbsDataProfile::operator=(const HbsDataProfile& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

void wso_board::HbsDataProfile::setHbsDataComm(HbsDataComm* comm)
{
	impl().dataComm = comm;
	return;
}

bool wso_board::HbsDataProfile::loadHbsDescriptor(void)
{
	auto* channel = getDataChannel();
	if (auto data = getHbsDescriptor(); channel) {
		if (channel->readDescriptor(data)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadHbsTableData(void)
{
	if (!loadBulkBuffer()) {
		return false;
	}
	if (!loadCalibration()) {
		return false;
	}
	if (!loadConfiguration()) {
		return false;
	}
	if (!loadMainBoardVersion()) {
		return false;
	}
	if (!loadSystemInitStatus()) {
		return false;
	}
	if (!loadSystemConfigure()) {
		return false;
	}
	if (!loadGpioStatus()) {
		return false;
	}
	if (!loadSldStatus()) {
		return false;
	}
	return true;
}

bool wso_board::HbsDataProfile::loadBulkBuffer(void)
{
	auto* channel = getDataChannel();

	if (auto desc = getHbsDescriptor();  channel) {
		auto data = getHbsBulkBuffer();
		if (channel->readBulkBuffer(data, desc)) {
			auto checksum1 = data->blk_buf_desc_header.header_chksum;
			auto checksum2 = data->blk_buf_desc_header.count + data->blk_buf_desc_header.table_chksum;
			if (checksum1 != checksum2) {
				LogDebug() << "HBS data header checksum not matched! " << checksum1 << ", " << checksum2;
				return false;
			}

			auto func_word_checksum = [](std::uint32_t* baseAddr, std::uint32_t wordCount) -> int {
				auto checksum = 0;
				for (uint32_t i = 0; i < wordCount; i++) {
					checksum += *(baseAddr + i);
				}
				return checksum;
			};

			auto wordAddr = (unsigned int*)(&data->TBL_blk_buf_desc[0]);
			auto wordSize = 2 * data->blk_buf_desc_header.count;
			auto wordsum1 = data->blk_buf_desc_header.table_chksum;
			auto wordsum2 = func_word_checksum(wordAddr, wordSize);
			if (wordsum1 != wordsum2) {
				LogDebug() << "HBS data word checksum not matched! " << wordsum1 << ", " << wordsum2;
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadConfiguration(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsConfiguration();
		if (channel->readConfiguration(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadMainBoardVersion(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsMainBoardVersion();
		if (channel->readMainBoardVersion(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadSystemInitStatus(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsSystemInitStatus();
		if (channel->readSystemInitStatus(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibration(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		if (auto data = getHbsCalibration(); data) {
			if (channel->readCalibration(data, desc)) {
				return true;
			}
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadSystemConfigure(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsSystemConfigure();
		if (channel->readSystemConfigure(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadGpioStatus(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsGpioStatus();
		if (channel->readGpioStatus(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadSldStatus(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsSldStatus();
		if (channel->readSldStatus(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadZynqXADC(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsZyncXADC();
		if (channel->readZynqXADC(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadGalvanoDynamicParam(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsGalvanoDynamicParam();
		if (channel->readGalvanoDynamicParam(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadStepMotorStatus(StepMotorType type)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsStepMotorStatus(type);
		if (channel->readStepMotorStatus(data, type, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadStageMotorStatus(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsYStageMotor();
		if (channel->readStageMotorStatus(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadInfraredCameraStatus(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsIrCameraStatus();
		if (channel->readInfraredCameraStatus(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibration(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsCalibration();
		if (channel->writeCalibration(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveConfiguration(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsConfiguration();
		
		auto* p = const_cast<HbsConfiguration*>(data);
		p->chksum = sizeof(HbsConfiguration);

		if (channel->writeConfiguration(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveSystemConfigure(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsSystemConfigure();
		if (channel->writeSystemConfigure(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveLsoScannerParam(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsLsoScanner();
		if (channel->writeLsoScannerParam(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveGalvanoDynamicParam(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsDescriptor(); channel) {
		auto data = getHbsGalvanoDynamicParam();
		if (channel->writeGalvanoDynamicParam(data, desc)) {
			return true;
		}
	}
	return false;
}

const HBS_descriptor_st* wso_board::HbsDataProfile::getHbsDescriptor(void) const
{
	return &impl().HBS_descriptor;
}

const HbsBulkBuffer* wso_board::HbsDataProfile::getHbsBulkBuffer(void) const
{
	return &impl().HBS_BlkBufTbl;
}

const HbsCalibration* wso_board::HbsDataProfile::getHbsCalibration(void) const
{
	return &impl().HBS_SysCal;
}

const HbsConfiguration* wso_board::HbsDataProfile::getHbsConfiguration(void) const
{
	return &impl().HBS_UserSetup;
}

const HbsMainBoardVersion* wso_board::HbsDataProfile::getHbsMainBoardVersion(void) const
{
	return &impl().HBS_MainBoardVerInfo;
}

const HbsSystemInitStatus* wso_board::HbsDataProfile::getHbsSystemInitStatus(void) const
{
	return &impl().HBS_SysInitStatus;
}

const HbsSystemConfigure* wso_board::HbsDataProfile::getHbsSystemConfigure(void) const
{
	return &impl().HBS_SysCfg;
}

const HbsGpioStatus* wso_board::HbsDataProfile::getHbsGpioStatus(void) const
{
	return &impl().HBS_GP_Status;
}

const HbsSldStatus* wso_board::HbsDataProfile::getHbsSldStatus(void) const
{
	return &impl().HBS_SLD_status;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsLsoFocusMotor(void) const
{
	return &impl().HBS_LsoFocusMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctAntLensMotor(void) const
{
	return &impl().HBS_OctAntLensMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctFocusMotor(void) const
{
	return &impl().HBS_OctFocusMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctReferMotor(void) const
{
	return &impl().HBS_OctRefMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctPolarMotor(void) const
{
	return &impl().HBS_OctPolMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctRefNDMotor(void) const
{
	return &impl().HBS_OctRefNDMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsXStageMotor(void) const
{
	return &impl().HBS_XstageMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsYStageMotor(void) const
{
	return &impl().HBS_YstageMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsZStageMotor(void) const
{
	return &impl().HBS_ZstageMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsSwingMotor(void) const
{
	return &impl().HBS_SwingMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsTiltMotor(void) const
{
	return &impl().HBS_TiltMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsFixationMotor(void) const
{
	return &impl().HBS_FixMotor;
}

const HbsChinrestMotor* wso_board::HbsDataProfile::getHbsChinrestMotor(void) const
{
	return &impl().HBS_CR_Motor;
}

const HbsInfraredCameraStatus* wso_board::HbsDataProfile::getHbsIrCameraStatus(void) const
{
	return &impl().HBS_IRCamstatus;
}

const HbsLedStatus* wso_board::HbsDataProfile::getHbsLedStatus(void) const
{
	return &impl().HBS_LED_status;
}

const HbsLsoScanner* wso_board::HbsDataProfile::getHbsLsoScanner(void) const
{
	return &impl().HBS_LsoScanner;
}

const HbsGalvanometer* wso_board::HbsDataProfile::getHbsGalvanometer(void) const
{
	return &impl().HBS_Galvanometer;
}

const HbsGalvanoDynamicParam* wso_board::HbsDataProfile::getHbsGalvanoDynamicParam(void)
{
	return &impl().HBS_GalvanoDynamicParam;
}

const HbsZyncXADC* wso_board::HbsDataProfile::getHbsZyncXADC(void) const
{
	return &impl().HBS_ZynqXADC;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsStepMotorStatus(StepMotorType type) const
{
	switch (type) {
		using enum StepMotorType;
		case OCT_FOCUS:
			return getHbsOctFocusMotor();
		case OCT_REFER:
			return getHbsOctReferMotor();
		case OCT_POLAR:
			return getHbsOctPolarMotor();
		case OCT_REFND:
			return getHbsOctRefNDMotor();
		case LSO_FOCUS:
			return getHbsLsoFocusMotor();
		case OCT_ANT_LENS:
			return getHbsOctAntLensMotor();
		case STAGE_X:
			return getHbsXStageMotor();
		case STAGE_Y:
			return getHbsYStageMotor();
		case STAGE_Z:
			return getHbsZStageMotor();
		case SWING:
			return getHbsSwingMotor();
	}
	return nullptr;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsStageMotorStatus(StageMotorType type) const
{
	switch (type) {
		using enum StageMotorType;
		case STAGE_Y:
			return getHbsYStageMotor();
	}
	return nullptr;
}


int wso_board::HbsDataProfile::getHbsTableIndex(StepMotorType type)
{
	switch (type) {
		using enum StepMotorType;
		case OCT_FOCUS:
			return TBL_OCT_FOCUS_MOTOR_ID;
		case OCT_REFER:
			return TBL_OCT_REF_MOTOR_ID;
		case OCT_POLAR:
			return TBL_OCT_POL_MOTOR_ID;
		case OCT_REFND:
			return TBL_REF_ND_MOTOR_ID;
		case LSO_FOCUS:
			return TBL_LSO_FOCUS_MOTOR_ID;
		case SWING:
			return TBL_SWING_MOTOR_ID;
		case STAGE_X:
			return TBL_X_MOTOR_ID;
		case STAGE_Y:
			return TBL_Y_MOTOR_ID;
		case STAGE_Z:
			return TBL_Z_MOTOR_ID;
	}
	return -1;
}

HbsDataComm* wso_board::HbsDataProfile::getDataChannel(void) const
{
	if (impl().dataComm.has_value()) {
		return *impl().dataComm;
	}
	return nullptr;
}


HbsDataProfile::HbsDataProfileImpl& wso_board::HbsDataProfile::impl(void) const
{
	return *d_ptr;
}
