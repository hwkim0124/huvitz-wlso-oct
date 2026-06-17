#include "pch.h"
#include "UsbComm.h"
#include "UsbPort.h"
#include "crc.h"

#include <mutex>
#include <cassert>

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;


struct UsbComm::UsbCommImpl
{
	string boardDescript;

	UsbPort usbPort;
	ReadRequest readReq;
	ReadResponse readResp;
	WriteRequest writeReq;
	WriteResponse writeResp;
	MsgCommand msgCmd;

	bool initiated;
	bool restoring;
	bool cmdAsync;
	mutex mutexRestore;
	mutex mutexControl;
	mutex mutexStream;

	UsbCommImpl() : initiated(false), restoring(false), cmdAsync(false) {
		initializeUsbCommImpl();
	}

	void initializeUsbCommImpl(void) {
		boardDescript = "";
	}
};


UsbComm::UsbComm() :
	d_ptr(make_unique<UsbCommImpl>())
{
	crcInit();
}


UsbComm::~UsbComm()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::UsbComm::UsbComm(UsbComm&& rhs) = default;
UsbComm& wso_device::UsbComm::operator=(UsbComm&& rhs) = default;


void wso_device::UsbComm::setBoardDescript(string str)
{
	impl().boardDescript = str;
	return;
}

std::string wso_device::UsbComm::getBoardDescript(void) const
{
	return impl().boardDescript;
}

bool wso_device::UsbComm::openChannel(bool setNotiCallback)
{
	bool ret = false;
	int retry = 0;

	try {
		if (impl().boardDescript == "") {
			throw new std::exception("");
			ret = false;
		}
		else {
			ret = impl().usbPort.openPort(impl().boardDescript, setNotiCallback);
		}
	}
	catch (const std::exception&) {
	}

	if (ret) {
		impl().restoring = false;
		impl().initiated = true;
	}
	else {
		impl().initiated = false;
	}
	return ret;
}


void wso_device::UsbComm::releaseChannel(void)
{
	impl().usbPort.closePort();
	return;
}


bool wso_device::UsbComm::isInitiated(void) const
{
	return impl().initiated;
}


bool wso_device::UsbComm::isRestoring(void) const
{
	return impl().restoring;
}


void wso_device::UsbComm::setCommandAsyncMode(bool flag)
{
	impl().cmdAsync = flag;
	return;
}


bool wso_device::UsbComm::isCommandAsyncMode(void) const
{
	return impl().cmdAsync;
}


UsbPort* wso_device::UsbComm::getUsbPort(void) const
{
	return &impl().usbPort;
}

bool wso_device::UsbComm::sendMsgCmd(MsgCommand* msgCmd, bool reply)
{
	UsbPort* usbPort = getUsbPort();

	if (usbPort->send((uint8_t*)&msgCmd->packet, msgCmd->size)) {
		if (reply == false /*|| isCommandAsyncMode()*/) {
			return true;
		}
		else {
			MSG_CMD_RES msgRes;
			if (usbPort->receive((uint8_t*)&msgRes, sizeof(MSG_CMD_RES))) {
				// ??
				return true;
			}
		}
	}
	LogDebug() << "sendMsgCmd() failed!";
	// restoreFromUsbError();
	return false;
}


bool wso_device::UsbComm::recvMsgCmdRes(void)
{
	UsbPort* usbPort = getUsbPort();

	MSG_CMD_RES msgRes;
	if (usbPort->receive((uint8_t*)&msgRes, sizeof(MSG_CMD_RES))) {
		// ??
		return true;
	}
	LogDebug() << "recvMsgCmdRes() failed!";
	return false;
}


bool wso_device::UsbComm::readAddress(std::uint32_t addr, std::uint8_t* buff, std::uint32_t size)
{
	std::uint8_t* p_buff = buff;
	std::uint32_t n_read = 0;
	std::uint32_t n_data = size;
	std::uint32_t n_addr = addr;

	if (!buff || size <= 0) {
		return false;
	}

	while (n_data > 0)
	{
		n_read = (n_data > 256 ? 256 : n_data);

		ReadRequest* pReq = getReadRequest(n_addr, n_read);
		ReadResponse* pRes = getReadResponse(n_read);
		UsbPort* usbPort = getUsbPort();

		// The request size to read should be in the range of unsigned char. 
		// It doesn't cosider incomplete send or receive for the requested size. 
		if (usbPort->send((uint8_t*)&pReq->packet, pReq->size, true)) {
			if (usbPort->receive((uint8_t*)&pRes->packet, pRes->size, true)) {
				if (pRes->packet.type == static_cast<uint8_t>(PacketType::NREAD_RESP_OK)) {
					//if (n_read > 0) {
					memcpy(p_buff, pRes->packet.data, n_read);
					n_data -= n_read;
					if (n_data <= 0) {
						return true;
					}
					else {
						p_buff += n_read;
						n_addr += n_read;
						continue;
					}
					//}
				}
				else {
					LogDebug() << "Read response invalid type=" << int(pRes->packet.type);
				}
			}
		}
		break;
	}
	LogDebug() << "readAddress() failed!";
	return false;
}


bool wso_device::UsbComm::writeAddress(std::uint32_t addr, std::uint8_t* buff, std::uint32_t size, bool reply)
{
	// lock_guard<mutex> lock(impl().mutexControl);

	std::uint8_t* p_buff = buff;
	std::uint32_t n_sent = 0;
	std::uint32_t n_data = size;
	std::uint32_t n_addr = addr;

	while (n_data > 0)
	{
		n_sent = (n_data > 256 ? 256 : n_data);

		WriteRequest* pReq = getWriteRequest(n_addr, p_buff, n_sent);
		WriteResponse* pRes = getWriteResponse();
		UsbPort* usbPort = getUsbPort();

		if (usbPort->send((uint8_t*)&pReq->packet, pReq->size, true)) {
			bool resp = false;
			if (reply) {
				if (usbPort->receive((uint8_t*)&pRes->packet, pRes->size)) {
					if (pRes->packet.type == static_cast<uint8_t>(PacketType::NWRITE_RESP) ||
						pRes->packet.type == static_cast<uint8_t>(PacketType::MSG_RESP)) {
						if (pRes->packet.status == static_cast<uint8_t>(PacketType::STATUS_OK)) {
							resp = true;
						}
						else {
							LogDebug() << "Write response invalid status=" << pRes->packet.status;
						}
					}
					else {
						LogDebug() << "Write response invalid type=" << int(pRes->packet.type);
					}
				}
			}
			else {
				resp = true;
			}

			if (resp) {
				n_data -= n_sent;
				if (n_data <= 0) {
					return true;
				}
				else {
					p_buff += n_sent;
					n_addr += n_sent;
					continue;
				}
			}
			break;
		}
	}
	LogDebug() << "writeAddress() failed!";
	return false;
}

bool wso_device::UsbComm::readIrCameraFrame(CameraType type, std::uint8_t* buff, std::uint32_t size)
{
	bool ret = false;
	if (type == CameraType::IR_CORNEA_LEFT) {
		ret = getUsbPort()->receiveFromIr1(buff, size, true);
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		ret = getUsbPort()->receiveFromIr2(buff, size, true);
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		ret = getUsbPort()->receiveFromIr2(buff, size, true);
	}
	else if (type == CameraType::IR_RETINA) {
		ret = getUsbPort()->receiveFromIr1(buff, size, true);
	}

	if (!ret) {
		LogDebug() << "readIrCameraFrame() failed!, camera type = " << static_cast<int>(type);
	}
	return ret;
}

bool wso_device::UsbComm::readSloScanFrame(std::uint8_t* buff, std::uint32_t size)
{
	if (!getUsbPort()->receiveFromSlo(buff, size, true)) {
		LogDebug() << "readSloScanFrame() failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::checkSystemReadyGPIO(void)
{
	const int TIMEOUT_COUNT_MAX = 100;
	const int TIME_PERIOD_MS = 100;

	std::uint32_t status;

	for (int count = 0; count < TIMEOUT_COUNT_MAX; count++) {
		if (!getUsbPort()->readGPIO(&status)) {
			LogD() << "Failed to read GPIO to check system ready, count=" << count;
			return false;
		}
		else {
			if ((status & 0x02) == 0) {
				return true;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PERIOD_MS));
			}
		}
	}
	// Timeout.
	return false;
}

bool wso_device::UsbComm::checkEyeSideGPIO(EyeSide& side)
{
	const int TIMEOUT_COUNT_MAX = 5;
	const int TIME_PERIOD_MS = 50;

	std::uint32_t status;

	for (int count = 0; count < TIMEOUT_COUNT_MAX; count++) {
		if (!getUsbPort()->readGPIO(&status)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(TIME_PERIOD_MS));
		}
		else {
			if ((status & 0x01) == 0) {
				side = EyeSide::OD;
			}
			else {
				side = EyeSide::OS;
			}
			return true;
		}
	}
	// Timeout.
	return false;
}

bool wso_device::UsbComm::readTableDescriptor(const HbsTableDescriptor* data)
{
	assert(data != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);

	if (!readAddress(HBS_TBL_DESCRIPTOR_ADDR, (unsigned char*)(&data->header), sizeof(hbs_table_header_st))) {
		LogD() << "Failed to read HBS table descriptor, addr: " << HBS_TBL_DESCRIPTOR_ADDR << ", size: " << sizeof(hbs_table_header_st);
		return false;
	}

	if (data->header.ver != HBS_TBL_VERSION) {
		LogD() << "HBS table version not matched, ver: " << data->header.ver << ", expected: " << HBS_TBL_VERSION;
		return false;
	}
	auto checksum = (data->header.ver + data->header.count + data->header.table_chksum);
	if (data->header.header_chksum != checksum) {
		LogD() << "HBS table header checksum invalid, checksum: " << data->header.header_chksum << ", expected: " << checksum;
		return false;
	}
	if (data->header.count != HBS_TBL_ITEM_NUM) {
		LogD() << "HBS table items count invalid, count: " << data->header.count << ", expected: " << HBS_TBL_ITEM_NUM;
		return false;
	}

	auto addrData = HBS_TBL_DESCRIPTOR_ADDR + sizeof(hbs_table_header_st);
	auto wordData = int((data->header.count) * 2);
	auto sizeData = int(wordData * 4);

	if (!readAddress((uint32_t)addrData, (unsigned char*)(&data->entries), sizeData)) {
		LogD() << "Failed to read HBS table descriptor entries, addr: " << addrData << ", size: " << sizeData;
		return false;
	}

	auto checksum2 = getWordChecksum((unsigned int*)(&data->entries[0]), wordData);
	if (checksum2 != data->header.table_chksum) {
		LogD() << "HBS table entries checksum invalid, checksum: " << data->header.table_chksum << ", expected: " << checksum2;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readBufferDescriptor(const HbsBufferDescriptor* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_BLKBUF_ID].buf_addr;
	const auto size = desc->entries[TBL_BLKBUF_ID].buf_size;

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS bulk buffer descriptor, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibsDescriptor(const HbsCalibsDescriptor* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_CALBLKBUF_ID].buf_addr;
	const auto size = desc->entries[TBL_CALBLKBUF_ID].buf_size;

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS calibration block descriptor, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibMotorSets(const HbsCalibMotorSets* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_MOTOR_SETS].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_MOTOR_SETS].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS motor sets calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibOctParams(const HbsCalibOctParams* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_OCT_PARAMS].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_OCT_PARAMS].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS oct params calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibLedSource(const HbsCalibLedSource* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_LED_SOURCE].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_LED_SOURCE].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS led source calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibOctGalvano(const HbsCalibOctGalvano* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_OCT_GALVANO].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_OCT_GALVANO].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS oct galvano calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibDeviceCfg(const HbsCalibDeviceCfg* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_DEVICE_CFG].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_DEVICE_CFG].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS device cfg calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibStepMotors(const HbsCalibStepMotors* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_STEP_MOTORS].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_STEP_MOTORS].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS step motors calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibFactorySet1(const HbsCalibFactorySet1* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_FACTORY_SET1].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_FACTORY_SET1].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS factory set1 calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibFactorySet2(const HbsCalibFactorySet2* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_FACTORY_SET2].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_FACTORY_SET2].RomBlkSize;

	if (size <= 0 || !readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to read HBS factory set2 calibration block, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::pullCalibBlockFromMemory(int region, int blockIdx)
{
	auto dsize = HbsDataUtil::getCalibBlockDataSize(blockIdx);
	if (dsize <= 0 || !FlashCalibBlockLoad(region, blockIdx, dsize)) {
		LogD() << "Failed to pull calibration block from memory, region: " << region << ", block: " << blockIdx << ", size: " << dsize;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::pushCalibBlockToMemory(int region, int blockIdx)
{
	auto dsize = HbsDataUtil::getCalibBlockDataSize(blockIdx);
	if (dsize <= 0 || !FlashCalibBlockProgram(region, blockIdx, dsize)) {
		LogD() << "Failed to push calibration block from memory, region: " << region << ", block: " << blockIdx << ", size: " << dsize;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readConfiguration(const HbsConfiguration* data, const HbsTableDescriptor* desc)
{
	/*
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->TBL_desc[TBL_USERSETUP_ID].buf_addr;
	const auto size = desc->TBL_desc[TBL_USERSETUP_ID].buf_size;

	const auto s1 = sizeof(slo_capture_mode_st[8]);
	const auto s2 = sizeof(slo_imaging_mode_st[6]);

	const auto data_size = sizeof(HbsConfiguration);
	if (data_size != size) {
		LogD() << "User setup data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "User setup data failed to read!";
		return false;
	}
	*/
	return true;
}

bool wso_device::UsbComm::readMainBoardVersion(const HbsMainBoardVersion* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_MB_VER_INFO_ID].buf_addr;
	const auto size = desc->entries[TBL_MB_VER_INFO_ID].buf_size;

	const auto data_size = sizeof(HbsMainBoardVersion);
	if (data_size != size) {
		LogD() << "Mainboard version data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "Mainboard version data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readSystemInitStatus(const HbsSystemInitStatus* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_SYS_INIT_STATUS_ID].buf_addr;
	const auto size = desc->entries[TBL_SYS_INIT_STATUS_ID].buf_size;

	const auto data_size = sizeof(HbsSystemInitStatus);
	if (data_size != size) {
		LogD() << "System init status data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "System init status data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readCalibration(const HbsCalibration* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_SYSCAL_ID].buf_addr;
	const auto size = desc->entries[TBL_SYSCAL_ID].buf_size;

	const auto data_size = sizeof(HbsCalibration);
	if (data_size != size) {
		LogD() << "System calibration data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogD() << "System calibration data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readSystemConfigure(const HbsSystemConfig* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_SYS_CFG_ID].buf_addr;
	const auto size = desc->entries[TBL_SYS_CFG_ID].buf_size;

	const auto data_size = sizeof(HbsSystemConfig);
	if (data_size != size) {
		LogD() << "System configure data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "System configure data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readGpioStatus(const HbsGpioStatus* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_GP_STATUS_ID].buf_addr;
	const auto size = desc->entries[TBL_GP_STATUS_ID].buf_size;

	const auto data_size = sizeof(HbsGpioStatus);
	if (data_size != size) {
		LogD() << "Gpio status data size not matched, " << data_size << ", read_size: " << size;
		// return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "Gpio status data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readLsoScannerParam(const HbsLsoScanner* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_LSO_SCANNER_ID].buf_addr;
	const auto size = desc->entries[TBL_LSO_SCANNER_ID].buf_size;

	const auto data_size = sizeof(HbsLsoScanner);
	if (data_size != size) {
		LogD() << "LSO scanner param data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "LSO scanner param data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readSldStatus(const HbsSldStatus* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_SLD_STATUS_ID].buf_addr;
	const auto size = desc->entries[TBL_SLD_STATUS_ID].buf_size;

	const auto data_size = sizeof(HbsSldStatus);
	if (data_size != size) {
		LogD() << "Sld status data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "Sld status data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readZynqXADC(const HbsZyncXADC* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_ZYNQ_XADC_ID].buf_addr;
	const auto size = desc->entries[TBL_ZYNQ_XADC_ID].buf_size;

	const auto data_size = sizeof(HbsZyncXADC);
	if (data_size != size) {
		LogD() << "Zync XADC data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "Zync XADC data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_OCT_GAVANO_ID].buf_addr;
	const auto size = desc->entries[TBL_OCT_GAVANO_ID].buf_size;

	const auto data_size = sizeof(HbsGalvanoDynamicParam);
	if (data_size != size) {
		LogD() << "Galvano dynamic param data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	auto addr2 = static_cast<uint32_t>(addr + sizeof(HbsOctGalvano::traj_profile1));
	if (!readAddress(addr2, (unsigned char*)data, data_size)) {
		LogDebug() << "Galvano dynamic param data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readInfraredCameraStatus(const HbsInfraredCameraStatus* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_RET_IR_CAM_STATUS_ID].buf_addr;
	const auto size = desc->entries[TBL_RET_IR_CAM_STATUS_ID].buf_size;

	const auto data_size = sizeof(HbsInfraredCameraStatus);
	if (data_size != size) {
		LogD() << "IR camera status data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "IR camera status data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readStepMotorStatus(const HbsStepMotorStatus* data, StepMotorType type, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);

	auto index = HbsDataProfile::getHbsTableIndex(type);
	assert(index >= 0);
	const auto addr = desc->entries[index].buf_addr;
	const auto size = desc->entries[index].buf_size;

	const auto data_size = sizeof(HbsStepMotorStatus);
	if (data_size != size) {
		LogD() << "Step motor status data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!readAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "Step motor status data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::readStageMotorStatus(const HbsStageMotorStatus* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_Y_MOTOR_ID].buf_addr;
	const auto size = desc->entries[TBL_Y_MOTOR_ID].buf_size;

	const auto data_size = sizeof(HbsStageMotorStatus);
	if (data_size != size) {
		LogD() << "Stage motor status data size not matched, " << data_size << ", read_size: " << size;
		return false;
	}

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "Stage motor status data failed to read!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibMotorSets(const HbsCalibMotorSets* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_MOTOR_SETS].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_MOTOR_SETS].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of motor sets, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibOctParams(const HbsCalibOctParams* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_OCT_PARAMS].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_OCT_PARAMS].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of oct params, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibLedSource(const HbsCalibLedSource* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_LED_SOURCE].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_LED_SOURCE].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of led source, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibOctGalvano(const HbsCalibOctGalvano* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_OCT_GALVANO].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_OCT_GALVANO].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of oct galvano, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibDeviceCfg(const HbsCalibDeviceCfg* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_DEVICE_CFG].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_DEVICE_CFG].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of device config, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibStepMotors(const HbsCalibStepMotors* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_STEP_MOTORS].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_STEP_MOTORS].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of step motors, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibFactorySet1(const HbsCalibFactorySet1* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_FACTORY_SET1].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_FACTORY_SET1].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of factory set1, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibFactorySet2(const HbsCalibFactorySet2* data, const HbsCalibsDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->blocks[CALIB_IDX_FACTORY_SET2].HBS_BlkBaseAddr;
	const auto size = desc->blocks[CALIB_IDX_FACTORY_SET2].RomBlkSize;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "Failed to write calibration block of factory set2, addr: " << addr << ", size: " << size;
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeCalibration(const HbsCalibration* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_SYSCAL_ID].buf_addr;
	const auto size = desc->entries[TBL_SYSCAL_ID].buf_size;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "System calibration data write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeConfiguration(const HbsConfiguration* data, const HbsTableDescriptor* desc)
{
	/*
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->TBL_desc[TBL_USERSETUP_ID].buf_addr;
	const auto size = desc->TBL_desc[TBL_USERSETUP_ID].buf_size;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogD() << "System configuration data write failed!";
		return false;
	}
	*/
	return true;
}

bool wso_device::UsbComm::writeSystemConfigure(const HbsSystemConfig* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_SYS_CFG_ID].buf_addr;
	const auto size = desc->entries[TBL_SYS_CFG_ID].buf_size;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "System configure data write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_OCT_GAVANO_ID].buf_addr;
	const auto size = desc->entries[TBL_OCT_GAVANO_ID].buf_size;

	const auto data_size = sizeof(HbsGalvanoDynamicParam);

	auto addr2 = static_cast<uint32_t>(addr + sizeof(HbsOctGalvano::traj_profile1));
	if (!writeAddress(addr2, (unsigned char*)data, data_size)) {
		LogDebug() << "Galvano dynamic param data write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeLsoScannerParam(const HbsLsoScanner* data, const HbsTableDescriptor* desc)
{
	assert(data != nullptr);
	assert(desc != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_LSO_SCANNER_ID].buf_addr;
	const auto size = desc->entries[TBL_LSO_SCANNER_ID].buf_size;

	if (!writeAddress(addr, (unsigned char*)data, size)) {
		LogDebug() << "LSO scanner param data write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeTrajectoryParam(std::uint8_t tid, const TrajectoryProfileParam* param, const HbsTableDescriptor* desc)
{
	assert(desc != nullptr);
	assert(param != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_OCT_GAVANO_ID].buf_addr;
	const auto size = desc->entries[TBL_OCT_GAVANO_ID].buf_size;

	uint32_t addr2 = addr + tid * sizeof(TrajectoryProfile);
	if (!writeAddress(addr2, (uint8_t*)param, sizeof(TrajectoryProfileParam))) {
		LogDebug() << "Galvano trajectory param write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeTrajectoryPositionsX(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsTableDescriptor* desc)
{
	assert(desc != nullptr);
	assert(positions != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_OCT_GAVANO_ID].buf_addr;
	const auto size = desc->entries[TBL_OCT_GAVANO_ID].buf_size;

	uint32_t addr2 = addr + tid * sizeof(TrajectoryProfile) + sizeof(TrajectoryProfileParam) + sizeof(int16_t) * TRAJECT_SAMPLE_SIZE_MAX;
	if (!writeAddress(addr2, (uint8_t*)positions, count * sizeof(int16_t))) {
		LogDebug() << "Galvano trajectory x positions write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::writeTrajectoryPositionsY(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsTableDescriptor* desc)
{
	assert(desc != nullptr);
	assert(positions != nullptr);
	lock_guard<mutex> lock(impl().mutexControl);
	const auto addr = desc->entries[TBL_OCT_GAVANO_ID].buf_addr;
	const auto size = desc->entries[TBL_OCT_GAVANO_ID].buf_size;

	uint32_t addr2 = addr + tid * sizeof(TrajectoryProfile) + sizeof(TrajectoryProfileParam);
	if (!writeAddress(addr2, (uint8_t*)positions, count * sizeof(int16_t))) {
		LogDebug() << "Galvano trajectory y positions write failed!";
		return false;
	}
	return true;
}

bool wso_device::UsbComm::SysCalibLoad(std::uint16_t offset, std::uint16_t size)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LOAD_CALIB, 4);
	msg->packet.ctrl6.s1 = offset;
	msg->packet.ctrl6.s2 = size;
	attachCRC(msg->packet.ctrl6.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::SysCalibSave(std::uint16_t offset, std::uint16_t size)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::SAVE_CALIB, 4);
	msg->packet.ctrl6.s1 = offset;
	msg->packet.ctrl6.s2 = size;
	attachCRC(msg->packet.ctrl6.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::SysCalibLoad2(std::uint16_t offset, std::uint16_t size)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LOAD_CALIB2, 4);
	msg->packet.ctrl6.s1 = offset;
	msg->packet.ctrl6.s2 = size;
	attachCRC(msg->packet.ctrl6.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::SysCalibSave2(std::uint16_t offset, std::uint16_t size)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::SAVE_CALIB2, 4);
	msg->packet.ctrl6.s1 = offset;
	msg->packet.ctrl6.s2 = size;
	attachCRC(msg->packet.ctrl6.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::MotorJogg(std::uint8_t mid, int delta)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::MOTJOG, 5);
	msg->packet.motor.c1 = mid;
	msg->packet.motor.n1 = delta;
	attachCRC(msg->packet.motor.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::MotorMove(std::uint8_t mid, int pos)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::MOTMOVE, 5);
	msg->packet.motor.c1 = mid;
	msg->packet.motor.n1 = pos;
	attachCRC(msg->packet.motor.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::MotorHome(std::uint8_t mid)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::MOTHOME, 1);
	msg->packet.ctrl1.c1 = mid;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::MotorStop(std::uint8_t mid)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::XZ_STOP, 1);
	msg->packet.ctrl1.c1 = mid;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::MotorMoveVelocity(std::uint8_t mid, std::uint8_t dir)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::MOT_VMOVE, 2);
	msg->packet.ctrl2.c1 = mid;
	msg->packet.ctrl2.c2 = dir;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
	return true;
}

bool wso_device::UsbComm::MotorStopVelocity(std::uint8_t mid)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::SMOTSTOP, 1);
	msg->packet.ctrl1.c1 = mid;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::WriteMotorSpeedVelocity(std::uint8_t mid, std::uint32_t accelStep, std::uint32_t minSpeed, std::uint32_t maxSpeed)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::MOT_SPEED, 13);
	msg->packet.ctrl12.c1 = mid;
	msg->packet.ctrl12.n1 = accelStep;
	msg->packet.ctrl12.n2 = minSpeed;
	msg->packet.ctrl12.n3 = maxSpeed;
	attachCRC(msg->packet.ctrl12.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::MotorSpeedDefaultVelocity(std::uint8_t mid)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LOAD_SM_DEFAULT, 1);
	msg->packet.ctrl1.c1 = mid;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::StageMove(StageMotorType type, std::uint8_t dir, std::uint8_t duty)
{
	return false;
}

bool wso_device::UsbComm::StageStop(StageMotorType type)
{
	return false;
}

bool wso_device::UsbComm::ChinrestMove(std::uint8_t dir)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::CR_MOVE, 2);
	msg->packet.ctrl2.c1 = dir;
	msg->packet.ctrl2.c2 = 0;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::ChinrestStop(void)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::CR_STOP, 1);
	msg->packet.ctrl1.c1 = 0;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::IrCameraControl(std::uint8_t cid, std::uint8_t onoff)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::IRCAM, 2);
	msg->packet.ctrl2.c1 = cid;
	msg->packet.ctrl2.c2 = onoff;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::IrCameraDigitalGain(std::uint8_t cid, std::uint8_t gain)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::IRCAM_DGAIN, 2);
	msg->packet.ctrl2.c1 = cid;
	msg->packet.ctrl2.c2 = gain;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::IrCameraAnalogGain(std::uint8_t cid, std::uint8_t gain)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::IRCAM_AGAIN, 2);
	msg->packet.ctrl2.c1 = cid;
	msg->packet.ctrl2.c2 = gain;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::IrCameraExposureTime(std::uint8_t cid, std::uint16_t ints)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::IRCAM_INT, 3);
	msg->packet.ctrl7.c1 = cid;
	msg->packet.ctrl7.s1 = ints;
	attachCRC(msg->packet.ctrl7.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::TestCommand(std::uint8_t cmd)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::TEST_CMD, 1);
	msg->packet.ctrl1.c1 = cmd;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::LsoScannerControl(std::uint8_t patternId, std::uint8_t onOff)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LSO_SCANNER_CTRL, 2);
	msg->packet.ctrl2.c1 = patternId;
	msg->packet.ctrl2.c2 = (onOff > 0 ? 1 : 0);
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LsoGalvanoMoveY(std::int16_t ypos)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LSO_Y_GALVO_MOVE, 2);
	msg->packet.ctrl5.s1 = ypos;
	attachCRC(msg->packet.ctrl5.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LsoScannerTriggerControl(std::uint8_t onOff)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LSO_TRIG_MODE, 1);
	msg->packet.ctrl1.c1 = (onOff > 0 ? 1 : 0);
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::LedSetIntensity(LightType type, std::uint8_t value)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LED_CTL, 4);
	msg->packet.ctrl6.s1 = static_cast<std::uint8_t>(type);
	msg->packet.ctrl6.s2 = value;
	attachCRC(msg->packet.ctrl6.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LedSetMode(LightType type, std::uint8_t value)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LED_MODE, 3);
	msg->packet.ctrl10.s1 = static_cast<std::uint8_t>(type);
	msg->packet.ctrl10.c1 = value;
	attachCRC(msg->packet.ctrl10.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LedSldControl(LaserType type, std::uint8_t onOff)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::SLD_CTRL, 2);
	msg->packet.ctrl2.c1 = static_cast<std::uint8_t>(type);
	msg->packet.ctrl2.c2 = (onOff > 0 ? 1 : 0);
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LedSldPotentiometer(LaserType type, std::uint8_t channel, std::uint16_t data)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::SLD_POTENTIO, 4);
	msg->packet.sctrl1.c1 = static_cast<std::uint8_t>(type);
	msg->packet.sctrl1.c2 = channel;
	msg->packet.sctrl1.s1 = data;
	attachCRC(msg->packet.sctrl1.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LedSldGetParameters(LaserType type, std::uint8_t channel)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::GET_SLD_POT, 2);
	msg->packet.ctrl2.c1 = static_cast<std::uint8_t>(type);
	msg->packet.ctrl2.c2 = channel;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::LedSldUpdateParameters(LaserType type, std::uint8_t channel)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::FLASH_SLD_POT, 2);
	msg->packet.ctrl2.c1 = static_cast<std::uint8_t>(type);
	msg->packet.ctrl2.c2 = channel;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::OctReferencePhaseControl(std::uint8_t mode)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::OCT_PHASE_CTRL, 1);
	msg->packet.ctrl1.c1 = mode;
	attachCRC(msg->packet.ctrl1.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::OctSetReferencePhase(std::int16_t phase)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::OCT_SET_PHASE, 2);
	msg->packet.ctrl5.s1 = phase;
	attachCRC(msg->packet.ctrl5.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::FlashCalibBlockErase(std::uint16_t region, std::uint16_t blockNum)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::ERASE_CALBLOCK, 4);
	msg->packet.ctrl6.s1 = region;
	msg->packet.ctrl6.s2 = blockNum;
	attachCRC(msg->packet.ctrl6.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::FlashCalibBlockProgram(std::uint16_t region, std::uint16_t blockNum, std::uint32_t size)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::FLASH_CALBLOCK, 8);
	msg->packet.ctrl13.s1 = region;
	msg->packet.ctrl13.s2 = blockNum;
	msg->packet.ctrl13.n1 = size;
	attachCRC(msg->packet.ctrl3.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::FlashCalibBlockLoad(std::uint16_t region, std::uint16_t blockNum, std::uint32_t size)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LOAD_FLASHCALIB, 8);
	msg->packet.ctrl13.s1 = region;
	msg->packet.ctrl13.s2 = blockNum;
	msg->packet.ctrl13.n1 = size;
	attachCRC(msg->packet.ctrl3.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::GalvanoMoveX(short x)
{
	lock_guard<mutex> lock(impl().mutexControl);

	// X, Y motor switched. 
	// MsgCommand* msg = getMsgCommand(CommandType::GALVO_MOVEX, 2);
	MsgCommand* msg = getMsgCommand(CommandType::GALVO_MOVEY, 2);
	msg->packet.galv1.s1 = x;
	attachCRC(msg->packet.galv1.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::GalvanoSlewXY(short x, short y)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::GALVO_SLEWXY, 4);

	// X, Y motor switched. 
	// msg->packet.galv2.s1 = x;
	// msg->packet.galv2.s2 = y;
	msg->packet.galv2.s1 = y;
	msg->packet.galv2.s2 = x;

	attachCRC(msg->packet.galv2.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::GalvanoDynamicOffsetWrite(std::int16_t offsetX, std::int16_t offsetY)
{
	return false;
}

bool wso_device::UsbComm::GalvanoDynamicOffsetRead(std::int16_t& offsetX, std::int16_t& offsetY)
{
	return false;
}


bool wso_device::UsbComm::GalvanoScanXY(std::uint16_t sid, std::uint16_t eid)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::START_ACQ, 6);
	msg->packet.scan1.s1 = sid;
	msg->packet.scan1.s2 = eid;
	msg->packet.scan1.s3 = static_cast<uint16_t>(ScanModeType::SCAN_MODE_XY);
	attachCRC(msg->packet.scan1.crc);

	bool reply = (isCommandAsyncMode() ? false : true);
	return sendMsgCmd(msg, reply);
}


bool wso_device::UsbComm::GalvanoRasterX(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::START_ACQR, 10);
	msg->packet.scan2.s1 = tid;
	msg->packet.scan2.s2 = lines;

	msg->packet.scan2.s3 = static_cast<uint16_t>(ScanModeType::SCAN_MODE_Y);
	msg->packet.scan2.s4 = xoffs;
	msg->packet.scan2.s5 = yoffs;
	
	attachCRC(msg->packet.scan2.crc);

	bool reply = (isCommandAsyncMode() ? false : true);
	return sendMsgCmd(msg, reply);
}


bool wso_device::UsbComm::GalvanoRasterY(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::START_ACQR, 10);
	msg->packet.scan2.s1 = tid;
	msg->packet.scan2.s2 = lines;

	msg->packet.scan2.s3 = static_cast<uint16_t>(ScanModeType::SCAN_MODE_X);
	msg->packet.scan2.s4 = xoffs;
	msg->packet.scan2.s5 = yoffs;
	
	attachCRC(msg->packet.scan2.crc);

	bool reply = (isCommandAsyncMode() ? false : true);
	return sendMsgCmd(msg, reply);
}


bool wso_device::UsbComm::GalvanoRasterXY(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::START_ACQR, 10);
	msg->packet.scan2.s1 = tid;
	msg->packet.scan2.s2 = lines;

	msg->packet.scan2.s3 = static_cast<uint16_t>(ScanModeType::SCAN_MODE_XY);
	msg->packet.scan2.s4 = xoffs;
	msg->packet.scan2.s5 = yoffs;
	
	attachCRC(msg->packet.scan2.crc);

	bool reply = (isCommandAsyncMode() ? false : true);
	return sendMsgCmd(msg, reply);
}


bool wso_device::UsbComm::GalvanoRasterFastX(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::START_ACQRF, 10);
	msg->packet.scan2.s1 = tid;
	msg->packet.scan2.s2 = lines;

	msg->packet.scan2.s3 = static_cast<uint16_t>(ScanModeType::SCAN_MODE_Y);
	msg->packet.scan2.s4 = xoffs;
	msg->packet.scan2.s5 = yoffs;
	
	attachCRC(msg->packet.scan2.crc);

	bool reply = (isCommandAsyncMode() ? false : true);
	return sendMsgCmd(msg, reply);
}


bool wso_device::UsbComm::GalvanoRasterFastY(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::START_ACQRF, 10);
	msg->packet.scan2.s1 = tid;
	msg->packet.scan2.s2 = lines;

	msg->packet.scan2.s3 = static_cast<uint16_t>(ScanModeType::SCAN_MODE_X);
	msg->packet.scan2.s4 = xoffs;
	msg->packet.scan2.s5 = yoffs;
	
	attachCRC(msg->packet.scan2.crc);

	bool reply = (isCommandAsyncMode() ? false : true);
	return sendMsgCmd(msg, reply);
}



bool wso_device::UsbComm::LcdFixationControl(std::uint8_t row, std::uint8_t col)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LCDFIXA, 2);
	msg->packet.ctrl2.c1 = row;
	msg->packet.ctrl2.c2 = col;
	attachCRC(msg->packet.ctrl2.crc);
	return sendMsgCmd(msg);
}


bool wso_device::UsbComm::LcdFixationParameters(std::uint8_t bright, std::uint8_t blink, std::uint16_t period, std::uint16_t ontime, std::uint8_t type)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::LCDFIXPARAM, 7);
	msg->packet.ctrl8.c1 = bright;
	msg->packet.ctrl8.c2 = blink;
	msg->packet.ctrl8.s1 = period;
	msg->packet.ctrl8.s2 = ontime;
	msg->packet.ctrl8.c3 = type;
	attachCRC(msg->packet.ctrl8.crc);
	return sendMsgCmd(msg);
}

bool wso_device::UsbComm::FpgaWrite(std::uint32_t buff_addr, std::uint32_t flash_addr, std::uint32_t size, std::uint32_t calc_crc)
{
	lock_guard<mutex> lock(impl().mutexControl);
	MsgCommand* msg = getMsgCommand(CommandType::FPGA_UP, 16);
	msg->packet.ctrl11.n1 = buff_addr;
	msg->packet.ctrl11.n2 = flash_addr;
	msg->packet.ctrl11.n3 = size;
	msg->packet.ctrl11.n4 = calc_crc;
	attachCRC(msg->packet.ctrl11.crc);
	return sendMsgCmd(msg, false);
}


MsgCommand* wso_device::UsbComm::getMsgCommand(CommandType ctrl, std::uint8_t msg_len)
{
	MsgCommand* msg = &impl().msgCmd;
	msg->packet.hdr.type = static_cast<std::uint8_t>(PacketType::MSG_CMD);
	msg->packet.hdr.pkt_id = getNextPacketID();
	msg->packet.hdr.msg_len = msg_len;
	msg->packet.hdr.ctrl = static_cast<std::uint16_t>(ctrl);
	msg->size = sizeof(MSG_CMD_HEAD) + msg_len + PACKET_CRC_SIZE;
	return msg;
}


ReadRequest* wso_device::UsbComm::getReadRequest(std::uint32_t addr, std::uint16_t rd_size)
{
	ReadRequest* req = &impl().readReq;
	req->packet.nread = static_cast<std::uint8_t>(PacketType::NREAD);
	req->packet.pkt_id = getNextPacketID();
	req->packet.rd_size = (rd_size == 256 ? 0 : rd_size);
	memcpy(req->packet.addr, &addr, sizeof(uint32_t));

	attachCRC(req->packet.crc);
	req->size = sizeof(NREAD_REQ);
	return req;
}


ReadResponse* wso_device::UsbComm::getReadResponse(std::uint16_t rd_size)
{
	ReadResponse* res = &impl().readResp;
	res->packet.type = 0;
	res->packet.pkt_id = 0;
	res->size = sizeof(NREAD_RES) - (PACKET_DATA_SIZE_MAX - rd_size);
	return res;
}


WriteRequest* wso_device::UsbComm::getWriteRequest(std::uint32_t addr, std::uint8_t* data, std::uint16_t size, bool reply)
{
	WriteRequest* req = &impl().writeReq;
	req->packet.pkt_id = getNextPacketID();
	req->packet.wr_size = (size == 256 ? 0 : size);
	memcpy(req->packet.addr, &addr, sizeof(uint32_t));
	memcpy(req->packet.data, data, size);

	if (reply) {
		req->packet.nwrite = static_cast<uint8_t>(PacketType::NWRITE_R);
	}
	else {
		req->packet.nwrite = static_cast<uint8_t>(PacketType::NWRITE);
	}

	attachCRC(req->packet.data, size);
	req->size = (11 + size);
	return req;
}


WriteResponse* wso_device::UsbComm::getWriteResponse(void)
{
	WriteResponse* res = &impl().writeResp;
	res->packet.type = 0;
	res->packet.pkt_id = 0;
	res->packet.status = 0;
	res->size = sizeof(NWRITE_RES);
	return res;
}


unsigned char wso_device::UsbComm::getNextPacketID(void)
{
	static unsigned char pktId = PACKET_ID_START;
	unsigned char curId = pktId;

	pktId++;
	if (pktId > PACKET_ID_END) {
		pktId = PACKET_ID_START;
	}
	return curId;
}


void wso_device::UsbComm::attachCRC(std::uint8_t* addr, std::uint32_t offset)
{
	memset(addr + offset, PACKET_CRC_CODE, PACKET_CRC_SIZE);
	return;
}


void wso_device::UsbComm::attachCRC(std::uint8_t* addr)
{
	memset(addr, PACKET_CRC_CODE, PACKET_CRC_SIZE);
	return;
}


unsigned int wso_device::UsbComm::getWordChecksum(std::uint32_t* baseAddr, std::uint32_t wordCount)
{
	auto checksum = 0;
	for (uint32_t i = 0; i < wordCount; i++) {
		checksum += *(baseAddr + i);
	}
	return checksum;
}


UsbComm::UsbCommImpl& wso_device::UsbComm::impl(void) const
{
	return *d_ptr;
}
