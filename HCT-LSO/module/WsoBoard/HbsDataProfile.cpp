#include "pch.h"
#include "WsoBoard2.h"
#include "HbsDefines.h"
#include "HbsDataProfile.h"
#include "HbsDataComm.h"

#include <optional>


using namespace wso_board;
using namespace std;


struct HbsDataProfile::HbsDataProfileImpl
{
	optional<HbsDataComm*> dataComm { nullopt };

	hbs_table_descriptor_st	HBS_descriptor{};
	bulk_buffer_descriptor_st 	HBS_BlkBufTbl{};
	SysCal_st 			HBS_SysCal{};
	UserSetup_st 		HBS_UserSetup{};
	MainBoardVerInfo_st HBS_MainBoardVerInfo{};
	SysInitStatus_st 	HBS_SysInitStatus{};
	SysCfg_st 			HBS_SysCfg{};

	GP_Status_st 		HBS_GP_Status{};
	SLD_st 				HBS_SLD_status{};
	Zynq_XADC_st		HBS_ZynqXADC{};

	HbsCalibsDescriptor hbsCalibsDescriptor{};

	HbsCalibMotorSets hbsCalibMotorSets{};
	HbsCalibOctParams hbsCalibOctParams{};
	HbsCalibLedSource hbsCalibOctSource{};
	HbsCalibOctGalvano hbsCalibOctGalvano{};
	HbsCalibDeviceCfg hbsCalibDeviceCfg{};
	HbsCalibStepMotors hbsCalibStepMotors{};
	HbsCalibFactorySet1 hbsCalibFactorySet1{};
	HbsCalibFactorySet2 hbsCalibFactorySet2{};

	HbsStepMotorStatus 	HBS_OctFocusMotor{};
	HbsStepMotorStatus 	HBS_OctReferMotor{};
	HbsStepMotorStatus 	HBS_OctPolarMotor{};
	HbsStepMotorStatus 	HBS_OctReferNdMotor{};
	HbsStepMotorStatus 	HBS_LsoFocusMotor{};
	HbsStepMotorStatus 	HBS_RetMirrorMotor{};
	HbsStepMotorStatus  HBS_OctAntLensMotor{};
	HbsStepMotorStatus  HBS_LsoFilterMotor{};

	HbsStepMotorStatus  HBS_SwingMotor{};
	HbsStepMotorStatus	HBS_XstageMotor{};
	HbsStepMotorStatus	HBS_ZstageMotor{};
	HbsStepMotorStatus	HBS_YstageMotor{};
	HbsStepMotorStatus	HBS_ChinRestMotor{};

	DC_CR_Motor_st 		HBS_CR_Motor{};

	IRCamInfo_st 		HBS_IRCamstatus{};
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

bool wso_board::HbsDataProfile::loadHbsTableHeader(void)
{
	auto* channel = getDataChannel();
	if (auto data = getHbsTableDescriptor(); channel) {
		if (channel->readTableDescriptor(data)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadHbsTableEntries(void)
{
	if (!loadBufferDescriptor()) {
		return false;
	}
	if (!loadCalibrationBlocks(true)) {
		return false;
	}
	/*
	if (!loadConfiguration()) {
		return false;
	}
	*/
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

bool wso_board::HbsDataProfile::loadBufferDescriptor(void)
{
	auto* channel = getDataChannel();

	if (auto desc = getHbsTableDescriptor();  channel) {
		auto data = getHbsBufferDescriptor();
		if (channel->readBufferDescriptor(data, desc)) {
			auto checksum1 = data->header.header_chksum;
			auto checksum2 = data->header.count + data->header.table_chksum;
			if (checksum1 != checksum2) {
				LogD() << "HBS buffers header checksum not matched, header checksum: " << data->header.header_chksum << ", count: " << data->header.count << ", table checksum: " << data->header.table_chksum;
				return false;
			}

			auto func_word_checksum = [](std::uint32_t* baseAddr, std::uint32_t wordCount) -> int {
				auto checksum = 0;
				for (uint32_t i = 0; i < wordCount; i++) {
					checksum += *(baseAddr + i);
				}
				return checksum;
			};

			auto wordAddr = (unsigned int*)(&data->entries[0]);
			auto wordSize = 2 * data->header.count;
			auto wordsum1 = data->header.table_chksum;
			auto wordsum2 = func_word_checksum(wordAddr, wordSize);
			if (wordsum1 != wordsum2) {
				LogD() << "HBS buffers table checksum not matched, table checksum: " << data->header.table_chksum << ", calculated: " << wordsum2;
				return false;
			}
			return true;
		}
	}
	return false;
}


bool wso_board::HbsDataProfile::loadCalibrationBlocks(bool fetch, int region)
{
	auto* channel = getDataChannel();

	if (auto desc = getHbsTableDescriptor(); channel) {
		if (auto data = getHbsCalibsDescriptor(); data) {
			if (!channel->readCalibsDescriptor(data, desc)) {
				return false;
			}
			else {
				for (int i = 0; i < CALIB_ENTRIES_MAX_NUM; i++) {
					LogD() << "Block index: " << i << ", ROM base addr: " << data->blocks[i].RomBlkBaseAddr << ", size: " << data->blocks[i].RomBlkSize << ", HBS base addr: " << data->blocks[i].HBS_BlkBaseAddr;
				}
			}
		}
	}

	if (!loadCalibBlockMotorSets(fetch, region)) {
		return false;
	}
	if (!loadCalibBlockOctParams(fetch, region)) {
		return false;
	}
	if (!loadCalibBlockLedSource(fetch, region)) {
		return false;
	}
	/*
	if (!loadCalibBlockOctGalvano(fetch, region)) {
		return false;
	}
	*/
	if (!loadCalibBlockDeviceCfg(fetch, region)) {
		return false;
	}
	if (!loadCalibBlockStepMotors(fetch, region)) {
		return false;
	}
	/*
	if (!loadCalibBlockFactorySet1(fetch, region)) {
		return false;
	}
	if (!loadCalibBlockFactorySet2(fetch, region)) {
		return false;
	}
	*/
	return true;
}

bool wso_board::HbsDataProfile::loadCalibBlockMotorSets(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibMotorSets(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_MOTOR_SETS)) {
					return false;
				}
			}
			if (!channel->readCalibMotorSets(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockOctParams(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibOctParams(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_OCT_PARAMS)) {
					return false;
				}
			}
			if (!channel->readCalibOctParams(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockLedSource(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibLedSource(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_LED_SOURCE)) {
					return false;
				}
			}
			if (!channel->readCalibLedSource(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockOctGalvano(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibOctGalvano(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_OCT_GALVANO)) {
					return false;
				}
			}
			if (!channel->readCalibOctGalvano(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockDeviceCfg(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibDeviceCfg(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_DEVICE_CFG)) {
					return false;
				}
			}
			if (!channel->readCalibDeviceCfg(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockStepMotors(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibStepMotors(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_STEP_MOTORS)) {
					return false;
				}
			}
			if (!channel->readCalibStepMotors(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockFactorySet1(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibFactorySet1(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_FACTORY_SET1)) {
					return false;
				}
			}
			if (!channel->readCalibFactorySet1(data, desc)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadCalibBlockFactorySet2(bool fetch, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibFactorySet2(); data) {
			if (fetch) {
				if (!channel->pullCalibBlockFromMemory(region, CALIB_IDX_FACTORY_SET2)) {
					return false;
				}
			}
			if (!channel->readCalibFactorySet2(data, desc)) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
		auto data = getHbsSystemInitStatus();
		if (channel->readSystemInitStatus(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadSystemConfigure(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
		auto data = getHbsGalvanoDynamicParam();
		if (channel->readGalvanoDynamicParam(data, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockMotorSets(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibMotorSets(); data) {
			if (!channel->writeCalibMotorSets(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_MOTOR_SETS)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockOctParams(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibOctParams(); data) {
			if (!channel->writeCalibOctParams(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_OCT_PARAMS)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockLedSource(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibLedSource(); data) {
			if (!channel->writeCalibLedSource(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_LED_SOURCE)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockOctGalvano(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibOctGalvano(); data) {
			if (!channel->writeCalibOctGalvano(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_OCT_GALVANO)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockDeviceCfg(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibDeviceCfg(); data) {
			if (!channel->writeCalibDeviceCfg(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_DEVICE_CFG)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockStepMotors(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibStepMotors(); data) {
			if (!channel->writeCalibStepMotors(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_STEP_MOTORS)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockFactorySet1(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibFactorySet1(); data) {
			if (!channel->writeCalibFactorySet1(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_FACTORY_SET1)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::saveCalibBlockFactorySet2(bool write, int region)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsCalibsDescriptor(); channel) {
		if (auto data = getHbsCalibFactorySet2(); data) {
			if (!channel->writeCalibFactorySet2(data, desc)) {
				return false;
			}
			if (write) {
				if (!channel->pushCalibBlockToMemory(region, CALIB_IDX_FACTORY_SET2)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadStepMotorStatus(StepMotorType type)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsTableDescriptor(); channel) {
		auto data = getHbsStepMotorStatus(type);
		if (channel->readStepMotorStatus(data, type, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadStageMotorStatus(StageMotorType type)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsTableDescriptor(); channel) {
		auto data = getHbsStageMotorStatus(type);
		if (channel->readStageMotorStatus(data, type, desc)) {
			return true;
		}
	}
	return false;
}

bool wso_board::HbsDataProfile::loadInfraredCameraStatus(void)
{
	auto* channel = getDataChannel();
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
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
	if (auto desc = getHbsTableDescriptor(); channel) {
		auto data = getHbsGalvanoDynamicParam();
		if (channel->writeGalvanoDynamicParam(data, desc)) {
			return true;
		}
	}
	return false;
}

const hbs_table_descriptor_st* wso_board::HbsDataProfile::getHbsTableDescriptor(void) const
{
	return &impl().HBS_descriptor;
}

const HbsBufferDescriptor* wso_board::HbsDataProfile::getHbsBufferDescriptor(void) const
{
	return &impl().HBS_BlkBufTbl;
}

const HbsCalibsDescriptor* wso_board::HbsDataProfile::getHbsCalibsDescriptor(void) const
{
	return &impl().hbsCalibsDescriptor;
}

const HbsCalibMotorSets* wso_board::HbsDataProfile::getHbsCalibMotorSets(void) const
{
	return &impl().hbsCalibMotorSets;
}

const HbsCalibOctParams* wso_board::HbsDataProfile::getHbsCalibOctParams(void) const
{
	return &impl().hbsCalibOctParams;
}

const HbsCalibLedSource* wso_board::HbsDataProfile::getHbsCalibLedSource(void) const
{
	return &impl().hbsCalibOctSource;
}

const HbsCalibOctGalvano* wso_board::HbsDataProfile::getHbsCalibOctGalvano(void) const
{
	return &impl().hbsCalibOctGalvano;
}

const HbsCalibDeviceCfg* wso_board::HbsDataProfile::getHbsCalibDeviceCfg(void) const
{
	return &impl().hbsCalibDeviceCfg;
}

const HbsCalibStepMotors* wso_board::HbsDataProfile::getHbsCalibStepMotors(void) const
{
	return &impl().hbsCalibStepMotors;
}

const HbsCalibFactorySet1* wso_board::HbsDataProfile::getHbsCalibFactorySet1(void) const
{
	return &impl().hbsCalibFactorySet1;
}

const HbsCalibFactorySet2* wso_board::HbsDataProfile::getHbsCalibFactorySet2(void) const
{
	return &impl().hbsCalibFactorySet2;
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

const HbsSystemConfig* wso_board::HbsDataProfile::getHbsSystemConfigure(void) const
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

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsLsoFilterMotor(void) const
{
	return &impl().HBS_LsoFilterMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsRetMirrorMotor(void) const
{
	return &impl().HBS_RetMirrorMotor;
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
	return &impl().HBS_OctReferMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctPolarMotor(void) const
{
	return &impl().HBS_OctPolarMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsOctReferNdMotor(void) const
{
	return &impl().HBS_OctReferNdMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsXstageMotor(void) const
{
	return &impl().HBS_XstageMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsYstageMotor(void) const
{
	return &impl().HBS_YstageMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsZstageMotor(void) const
{
	return &impl().HBS_ZstageMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsSwingMotor(void) const
{
	return &impl().HBS_SwingMotor;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsChinRestMotor(void) const
{
	return &impl().HBS_ChinRestMotor;
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

const HbsOctGalvano* wso_board::HbsDataProfile::getHbsGalvanometer(void) const
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
			return getHbsOctReferNdMotor();
		case LSO_FOCUS:
			return getHbsLsoFocusMotor();
		case RET_MIRROR:
			return getHbsRetMirrorMotor();
		case OCT_ANT_LENS:
			return getHbsOctAntLensMotor();
		case LSO_FILTER:
			return getHbsLsoFilterMotor();
		case STAGE_X:
			return getHbsXstageMotor();
		case STAGE_Y:
			return getHbsYstageMotor();
		case STAGE_Z:
			return getHbsZstageMotor();
		case SWING:
			return getHbsSwingMotor();
		case CHIN_REST:
			return getHbsChinRestMotor();
	}
	return nullptr;
}

const HbsStepMotorStatus* wso_board::HbsDataProfile::getHbsStageMotorStatus(StageMotorType type) const
{
	switch (type) {
		using enum StageMotorType;
		case STAGE_X:
			return getHbsXstageMotor();
		case STAGE_Y:
			return getHbsYstageMotor();
		case STAGE_Z:
			return getHbsZstageMotor();
		case SWING:
			return getHbsSwingMotor();
	}
	return nullptr;
}


int wso_board::HbsDataProfile::getHbsTableMotorIndex(StepMotorType type)
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
		case RET_MIRROR:
			return TBL_RM_MOTOR_ID;
		case OCT_ANT_LENS:
			return TBL_ANTLENS_MOTOR_ID;
		case LSO_FILTER:
			return TBL_AP1_MOTOR_ID;
		case SWING:
			return TBL_SWING_MOTOR_ID;
		case STAGE_X:
			return TBL_X_MOTOR_ID;
		case STAGE_Y:
			return TBL_Y_MOTOR_ID;
		case STAGE_Z:
			return TBL_Z_MOTOR_ID;
		case CHIN_REST:
			return TBL_CR_MOTOR_ID;
	}
	return -1;
}

int wso_board::HbsDataProfile::getHbsTableMotorIndex(StageMotorType type)
{
	switch (type) {
		using enum StageMotorType;
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
