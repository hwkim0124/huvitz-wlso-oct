#include "pch.h"
#include "FirmwareControl.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include <format>
#include <thread>

using namespace wso_device;
using namespace wso_board;

struct FirmwareControl::FirmwareControlImpl
{
	MainBoard* board;
	UsbComm* usbComm;

	thread worker;
	mutex mutexLock;

	FirmwareControlImpl() : board(nullptr), usbComm(nullptr) {
	}
};

FirmwareControl::FirmwareControl() :
	d_ptr(make_unique<FirmwareControlImpl>())
{
}

wso_device::FirmwareControl::FirmwareControl(MainBoard* board, UsbComm* usbComm) :
	d_ptr(make_unique<FirmwareControlImpl>())
{
	impl().board = board;
	impl().usbComm = usbComm;

	return;
}

FirmwareControl::~FirmwareControl()
{
	if (impl().worker.joinable()) {
		impl().worker.join();
	}
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::FirmwareControl::FirmwareControl(FirmwareControl&& rhs) = default;
FirmwareControl& wso_device::FirmwareControl::operator=(FirmwareControl&& rhs) = default;

//bool wso_device::FirmwareControl::writeUpgradeFirmware(unsigned char* binary, int length, uint targetAddress, uint binCrc)
//{
//	if (impl().board == nullptr || impl().usbComm == nullptr)
//	{
//		return false;
//	}
//
//	auto* hbs = impl().board->getHbsDataProfile();
//	auto data = hbs->getHbsBulkBuffer();
//
//	uint zynq_buf_addr = data->TBL_blk_buf_desc[0].buf_addr;
//
//	impl().worker = std::thread([=, this]() {
//
//		unique_lock<mutex> lock(impl().mutexLock);
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Downloading file..", 0, 0, false, false);
//
//		std::vector<uint8_t> OriginalDataArray(binary, binary + length);
//
//		// Step1: Write to device
//		if (!impl().usbComm->writeAddress(zynq_buf_addr, binary, length)) {
//			return false;
//		}
//
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Uploading file..", 0, 0, false, false);
//
//		// Step2: Read back for verification
//		std::vector<uint8_t> ReadDataArray(length);
//		if (!impl().usbComm->readAddress(zynq_buf_addr, ReadDataArray.data(), length)) {
//			return false;
//		}
//
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Verifying file..", 0, 0, false, false);
//
//		if (OriginalDataArray != ReadDataArray) {
//			for (int i = 0; i < length; ++i) {
//				if (OriginalDataArray[i] != ReadDataArray[i]) { 
//					LsoLogError("File verify error at index");
//					return false;
//				}
//			}
//		}
//
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Flash Upgrading..Wait..", 0, 0, false, false);
//
//		bool bFpgaWrite = impl().usbComm->FpgaWrite(zynq_buf_addr, targetAddress, length, binCrc);
//
//		if (!bFpgaWrite) {
//			LsoLogError("FPGA Write failed!");
//			CallbackRegistry::getInstance()->runFirmwareMsgCallback("FPGA Write failed!", 0, 0, false, false);
//			// 실패에 대한 추가 처리 필요시 작성
//			return false;
//		}
//
//		this->waitForState(1, 3000); // Erase State로 넘어가는지 확인 
//
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Erasing sector.", 0, 0, false, false);
//
//		this->waitWhileState(1, 100000, length); // Erase State 동안 표시
//
//		bool bRet = false;
//		bRet = this->isState(2);
//		if (!bRet)
//		{
//			return false;
//		}
//
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Programming FPGA.", 0, 0, false, false);
//
//		this->waitWhileState(2, 100000, length); // Programming State 동안 표시
//
//		bRet = this->isState(3);
//		if (!bRet)
//		{
//			return false;
//		}
//
//		CallbackRegistry::getInstance()->runFirmwareMsgCallback("Verifying FPGA.", 0, 0, false, false);
//
//		this->waitWhileState(3, 50000, length); // verify State 동안 표시
//
//		int nRet = getState();
//
//		switch (nRet)
//		{
//
//		case 1:
//			LsoLogError("FPGA programming done!");
//			impl().board->releaseMainBoard();
//			CallbackRegistry::getInstance()->runFirmwareMsgCallback("FPGA programming done!", 0, 0, true, false);
//			break;
//		case 4:
//			LsoLogError("FPGA programming done!");
//			break;
//		case 5:
//			LsoLogError("FPGA programming error!");
//			break;
//		default:
//			LsoLogError("FPGA programming error : unknown state.");
//			break;
//		}
//
//		return true;
//
//		});
//
//	return true; // 메인 스레드는 곧장 리턴
//}

bool wso_device::FirmwareControl::writeUpgradeFirmware(unsigned char* binary, int length, unsigned int targetAddress, unsigned int binCrc)
{
	if (impl().board == nullptr || impl().usbComm == nullptr)
	{
		return false;
	}

	auto* hbs = impl().board->getHbsDataProfile();
	auto data = hbs->getHbsBufferDescriptor();

	unsigned int zynq_buf_addr = data->entries[0].buf_addr;

	unique_lock<mutex> lock(impl().mutexLock);
	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Downloading file..", 0, 0, false, false);

	std::vector<uint8_t> OriginalDataArray(binary, binary + length);

	// Step1: Write to device
	if (!impl().usbComm->writeAddress(zynq_buf_addr, binary, length)) {
		return false;
	}

	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Uploading file..", 0, 0, false, false);

	// Step2: Read back for verification
	std::vector<uint8_t> ReadDataArray(length);
	if (!impl().usbComm->readAddress(zynq_buf_addr, ReadDataArray.data(), length)) {
		return false;
	}

	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Verifying file..", 0, 0, false, false);

	if (OriginalDataArray != ReadDataArray) {
		for (int i = 0; i < length; ++i) {
			if (OriginalDataArray[i] != ReadDataArray[i]) {
				WsoLogError("File verify error at index");
				return false;
			}
		}
	}

	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Flash Upgrading..Wait..", 0, 0, false, false);

	bool bFpgaWrite = impl().usbComm->FpgaWrite(zynq_buf_addr, targetAddress, length, binCrc);

	if (!bFpgaWrite) {
		WsoLogError("FPGA Write failed!");
		CallbackRegistry::getInstance()->runFirmwareMsgCallback("FPGA Write failed!", 0, 0, false, false);
		// 실패에 대한 추가 처리 필요시 작성
		return false;
	}

	this->waitForState(1, 3000); // Erase State로 넘어가는지 확인 

	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Erasing sector.", 0, 0, false, false);

	this->waitWhileState(1, 100000, length); // Erase State 동안 표시

	bool bRet = false;
	bRet = this->isState(2);
	if (!bRet)
	{
		return false;
	}

	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Programming FPGA.", 0, 0, false, false);

	this->waitWhileState(2, 100000, length); // Programming State 동안 표시

	bRet = this->isState(3);
	if (!bRet)
	{
		return false;
	}

	CallbackRegistry::getInstance()->runFirmwareMsgCallback("Verifying FPGA.", 0, 0, false, false);

	this->waitWhileState(3, 50000, length); // verify State 동안 표시

	int nRet = getState();

	switch (nRet)
	{

	case 1:
		WsoLogError("FPGA programming done!");
		impl().board->releaseMainBoard();
		CallbackRegistry::getInstance()->runFirmwareMsgCallback("FPGA programming done!", 0, 0, true, false);
		break;
	case 4:
		WsoLogError("FPGA programming done!");
		break;
	case 5:
		WsoLogError("FPGA programming error!");
		break;
	default:
		WsoLogError("FPGA programming error : unknown state.");
		break;
	}
	return true; // 메인 스레드는 곧장 리턴
}

/// <summary>
/// 
/// </summary>
/// <param name="state"> 0 : IDLE, 1 : Erase, 2 : Program </param>
/// <param name="timeoutMs"></param>
/// <returns></returns>
int wso_device::FirmwareControl::waitForState(int state, int timeoutMs)
{
	if (impl().board == nullptr || impl().usbComm == nullptr)
	{
		return false;
	}

	auto* hbs = impl().board->getHbsDataProfile();
	auto* hDesc = hbs->getHbsTableDescriptor();
	HbsGpioStatus* status = new HbsGpioStatus();

	int elapsed = 0;
	int intervalMs = 50;
	std::string strStatus;

	while (true)
	{
		if (elapsed >= timeoutMs)
		{
			//LsoLogInfo("is Timeout");
			delete status;
			return -1;
		}

		impl().usbComm->readGpioStatus(status, hDesc);

		if (status->UpgradeStatus == state)
		{
			//strStatus = std::format("start {} STATE", state);
			//LsoLogInfo(strStatus);
			delete status;
			return 0;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
		elapsed += intervalMs;
	}
	return 0;
}

int wso_device::FirmwareControl::waitWhileState(int state, int timeoutMs, int flieSize)
{
	if (impl().board == nullptr || impl().usbComm == nullptr)
	{
		return false;
	}

	auto* hbs = impl().board->getHbsDataProfile();
	auto* hDesc = hbs->getHbsTableDescriptor();
	HbsGpioStatus* status = new HbsGpioStatus();


	int elapsed = 0;
	int intervalMs = 50;
	std::string strStatus = "";

	while (true)
	{
		if (elapsed >= timeoutMs)
		{
			//LsoLogInfo("is Timeout");
			delete status;
			return -1;
		}

		bool bRet = impl().usbComm->readGpioStatus(status, hDesc);
		if (!bRet)
		{
			return -1;
		}

		if (status->UpgradeStatus == state)
		{
			int percent;

			if (state == 1)
			{
				percent = (int)(100 * ((float)status->UpgradeProgressCnt / 68));
				strStatus = std::format("is ERASE STATE : {} %", percent);
				CallbackRegistry::getInstance()->runFirmwareMsgCallback("Erase State", 0, percent, false, false);
			}
			else if (state == 2)
			{
				percent = (int)(100 * ((float)status->UpgradeProgressCnt / (float)flieSize));
				strStatus = std::format("is PROGRAM STATE : {} %", percent);
				CallbackRegistry::getInstance()->runFirmwareMsgCallback("Program State", 0, percent, false, false);
			}
			else if (state == 3)
			{
				percent = (int)(100 * ((float)status->UpgradeProgressCnt / (float)flieSize));
				strStatus = std::format("is VERIFY STATE : {} %", percent);
				CallbackRegistry::getInstance()->runFirmwareMsgCallback("Verify State", 0, percent, false, false);
			}

			//LsoLogInfo(strStatus);
		}
		else
		{
			strStatus = std::format("end {} STATE", state);
			//LsoLogInfo(strStatus);
			delete status;
			return 0;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
		elapsed += intervalMs;
	}
	return 0;
}

bool wso_device::FirmwareControl::isState(int state)
{
	if (impl().board == nullptr || impl().usbComm == nullptr)
	{
		return false;
	}

	auto* hbs = impl().board->getHbsDataProfile();
	auto* hDesc = hbs->getHbsTableDescriptor();
	HbsGpioStatus* status = new HbsGpioStatus();

	impl().usbComm->readGpioStatus(status, hDesc);
	if (status->UpgradeStatus == state)
	{
		std::string strStatus = std::format("is checkState {}", state);
		//LsoLogInfo(strStatus);
		return true;
	}

	std::string strStatus = std::format("is not checkState {}", state);
	//LsoLogInfo(strStatus);
	return false;
}

int wso_device::FirmwareControl::getState()
{
	if (impl().board == nullptr || impl().usbComm == nullptr)
	{
		return false;
	}

	auto* hbs = impl().board->getHbsDataProfile();
	auto* hDesc = hbs->getHbsTableDescriptor();
	HbsGpioStatus* status = new HbsGpioStatus();

	int nRet = impl().usbComm->readGpioStatus(status, hDesc);
	return nRet;
}

void wso_device::FirmwareControl::writeFirmwareThread()
{

}





FirmwareControl::FirmwareControlImpl& wso_device::FirmwareControl::impl(void) const
{
	return *d_ptr;
}