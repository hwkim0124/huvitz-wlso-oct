#include "pch.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include "OctSldLed.h"
#include "SldLaserDriver.h"
#include "ZynqXadcDriver.h"

#include "StepMotor.h"
#include "OctFocusMotor.h"
#include "OctPolarMotor.h"
#include "OctReferMotor.h"
#include "OctRefNdMotor.h"
#include "LsoFocusMotor.h"
#include "RetMirrorMotor.h"
#include "OctAntLensMotor.h"
#include "LsoFilterMotor.h"

#include "SwingMotor.h"
#include "XstageMotor.h"
#include "YstageMotor.h"
#include "ZstageMotor.h"
#include "ChinRestMotor.h"
#include "StageMotor.h"

#include "InfraredCamera.h"
#include "RetinaCamera.h"
#include "CorneaCamera.h"
#include "ColorCamera.h"

#include "LsoWhiteLed.h"
#include "LsoBlueLed.h"
#include "LsoGreenLed.h"
#include "RetinaIrLed.h"
#include "CorneaIrLed.h"

#include "LsoScanner.h"
#include "Galvanometer.h"

#include "FirmwareControl.h"

#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;
using namespace std;


struct MainBoard::MainBoardImpl
{
	unique_ptr<HbsDataProfile> hbsProfile;
	unique_ptr<HbsDataProfile> subProfile;
	UsbComm usbComm;
	UsbComm subComm;
	
	int lcdFixationRow;
	int lcdFixationCol;
	bool lcdFixationOn;

	unique_ptr<LsoScanner> lsoScanner;
	vector<unique_ptr<InfraredCamera>> irCameras;
	unique_ptr<ColorCamera> colorCamera;

	unordered_map<LightType, unique_ptr<LightLed>> lightLeds;
	unordered_map<MotorType, unique_ptr<StepMotor>> stepMotors;
	unordered_map<MotorType, std::unique_ptr<StageMotor>> stageMotors;

	SldLaserDriver sldLaserDriver;
	ZynqXadcDriver zyncXadcDriver;

	unique_ptr< OctSldLed> octSldLed;
	unique_ptr<Galvanometer> octGalvano;
	unique_ptr<FirmwareControl> firmwareControl;

	unsigned int deviceInitFlags;
	bool initiated;

	MainBoardImpl() 
	{
		initMainBoardImpl();
	}

	void initMainBoardImpl() {
		hbsProfile = make_unique<HbsDataProfile>();
		subProfile = make_unique<HbsDataProfile>();

		deviceInitFlags = { DEV_FLAG_NORMAL };
		initiated = false;
		lcdFixationOn = false;
		lcdFixationRow = 0;
		lcdFixationCol = 0;

		stepMotors.clear();
		stageMotors.clear();

		usbComm.setBoardDescript(USB_MAIN_BOARD_DESC);
		subComm.setBoardDescript(USB_SUB_BOARD_DESC);
	}
};


MainBoard::MainBoard() :
	d_ptr(make_unique<MainBoardImpl>())
{
	impl().irCameras.emplace_back(new RetinaCamera(this, CameraType::IR_RETINA, USB_IR1_PID)); // use Camera Id : 0
	impl().irCameras.emplace_back(new CorneaCamera(this, CameraType::IR_CORNEA_LOWER, USB_IR2_PID)); // use Camera Id : 1
	impl().irCameras.emplace_back(new CorneaCamera(this, CameraType::IR_CORNEA_LEFT, USB_IR1_PID));
	impl().irCameras.emplace_back(new CorneaCamera(this, CameraType::IR_CORNEA_RIGHT, USB_IR2_PID));

	impl().colorCamera = make_unique<ColorCamera>(this);

	impl().lightLeds.emplace(LightType::LSO_WHITE_LED, make_unique<LsoWhiteLed>(this));
	impl().lightLeds.emplace(LightType::LSO_BLUE_LED, make_unique<LsoBlueLed>(this));
	impl().lightLeds.emplace(LightType::LSO_GREEN_LED, make_unique<LsoGreenLed>(this));
	impl().lightLeds.emplace(LightType::RETINA_IR_LED, make_unique<RetinaIrLed>(this));
	impl().lightLeds.emplace(LightType::CORNEA_IR_LEFT_LED, make_unique<CorneaIrLed>(this, LightType::CORNEA_IR_LEFT_LED));
	impl().lightLeds.emplace(LightType::CORNEA_IR_RIGHT_LED, make_unique<CorneaIrLed>(this, LightType::CORNEA_IR_RIGHT_LED));
		
	impl().stepMotors.emplace(MotorType::OCT_FOCUS, make_unique<OctFocusMotor>(this));
	impl().stepMotors.emplace(MotorType::OCT_POLAR, make_unique<OctPolarMotor>(this));
	impl().stepMotors.emplace(MotorType::OCT_REFER, make_unique<OctReferMotor>(this));
	impl().stepMotors.emplace(MotorType::OCT_REFND, make_unique<OctRefNdMotor>(this));
	impl().stepMotors.emplace(MotorType::LSO_FOCUS, make_unique<LsoFocusMotor>(this));
	impl().stepMotors.emplace(MotorType::RET_MIRROR, make_unique<RetMirrorMotor>(this));
	impl().stepMotors.emplace(MotorType::OCT_ANT_LENS, make_unique<OctAntLensMotor>(this));
	impl().stepMotors.emplace(MotorType::LSO_FILTER, make_unique<LsoFilterMotor>(this));
	impl().stepMotors.emplace(MotorType::SWING, make_unique<SwingMotor>(this));
	impl().stepMotors.emplace(MotorType::STAGE_X, make_unique<XstageMotor>(this));
	impl().stepMotors.emplace(MotorType::STAGE_Y, make_unique<YstageMotor>(this));
	impl().stepMotors.emplace(MotorType::STAGE_Z, make_unique<ZstageMotor>(this));
	impl().stepMotors.emplace(MotorType::CHIN_REST, make_unique<ChinRestMotor>(this));

	// By using emplace() instead of insert() to construct and insert the unique_ptr, 
	// it directly constructs the element within the map, avoiding unnecessary copying or moving. 
	/*
	impl().stageMotors.emplace(MotorType::STAGE_X, make_unique<XstageMotor>(this));
	impl().stageMotors.emplace(MotorType::STAGE_Y, make_unique<YstageMotor>(this));
	impl().stageMotors.emplace(MotorType::STAGE_Z, make_unique<ZstageMotor>(this));
	*/

	impl().lsoScanner = make_unique<LsoScanner>(this);
	impl().octGalvano = make_unique<Galvanometer>(this);
	impl().octSldLed = make_unique<OctSldLed>(this);

	impl().firmwareControl = make_unique<FirmwareControl>(this, &this->getUsbComm());

	impl().sldLaserDriver.initializeSldLaserDriver(this);
	impl().zyncXadcDriver.initializeZynqXadcDriver(this);
}


MainBoard::~MainBoard()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::MainBoard::MainBoard(MainBoard&& rhs) noexcept = default;
MainBoard& wso_device::MainBoard::operator=(MainBoard&& rhs) noexcept = default;

bool wso_device::MainBoard::initializeMainBoard(int* warnings)
{
	int warns = 0;
	*warnings = warns;

	// No device mode is returned with not initiated status.
	if (isMainBoardNotInUse()) {
		WsoLogInfo("Mainboard not in use, skipping initialization") ;
		return true;
	}

	// Usb channel is not opened when no device mode. 
	if (!openFTDIdevices()) {
		WsoLogError("Failed to open USB port channels: main, sub board");
		return false;
	}
	else {
		WsoLogInfo("USB port channels opened: main, sub board");
	}

	if (!waitForSystemReady()) {
		WsoLogError("Target system not in ready state");
		return false;
	}

	if (!loadHostBufferTable()) {
		WsoLogError("Failed to load host buffer table");
		return false;
	}
	else {
		WsoLogInfo("Host buffer table loaded");
	}

	impl().initiated = true;
	return true;
}

void wso_device::MainBoard::releaseMainBoard(void)
{
	try {
		getColorCamera()->uninitialize();

		getUsbComm().releaseChannel();
		getSubComm().releaseChannel();
	}
	catch (...) {
	}
	return;
}

bool wso_device::MainBoard::isInitialized(void)
{
	return impl().initiated;
}

bool wso_device::MainBoard::initiateBoardComponents(int* numWarns)
{
	int warns = 0;
	*numWarns = warns;

	if (!getLsoWhiteLed()->initializeLsoWhiteLed()) {
		WsoLogWarn("Failed to initialize LSO white led");
		warns += 1;
	}
	if (!getLsoBlueLed()->initializeLsoBlueLed()) {
		WsoLogWarn("Failed to initialize LSO blue led");
		warns += 1;
	}
	if (!getLsoGreenLed()->initializeLsoGreenLed()) {
		WsoLogWarn("Failed to initialize LSO green led");
		warns += 1;
	}
	if (!getLsoWhiteLed()->initializeLsoWhiteLed()) {
		WsoLogWarn("Failed to initialize LSO white led");
		warns += 1;
	}
	if (!getRetinaIrLed()->initializeRetinaIrLed()) {
		WsoLogWarn("Failed to initialize retina IR led");
		warns += 1;
	}
	if (!getCorneaIrLeftLed()->initializeCorneaIrLed()) {
		WsoLogWarn("Failed to initialize cornea IR left led");
		warns += 1;
	}
	if (!getCorneaIrRightLed()->initializeCorneaIrLed()) {
		WsoLogWarn("Failed to initialize cornea IR right led");
		warns += 1;
	}

	if (!getOctFocusMotor()->initializeOctFocusMotor()) {
		WsoLogWarn("Failed to initialize OCT focus motor");
		warns += 1;
	}
	if (!getOctPolarMotor()->initializeOctPolarMotor()) {
		WsoLogWarn("Failed to initialize OCT Polarization motor");
		warns += 1;
	}
	if (!getOctReferMotor()->initializeOctReferMotor()) {
		WsoLogWarn("Failed to initialize OCT Reference motor");
		warns += 1;
	}
	if (!getLsoFocusMotor()->initializeLsoFocusMotor()) {
		WsoLogWarn("Failed to initialize LSO Focus motor");
		warns += 1;
	}
	if (!getRetMirrorMotor()->initializeRetMirrorMotor()) {
		WsoLogWarn("Failed to initialize Return mirror motor");
		warns += 1;
	}
	if (!getOctAntLensMotor()->initializeOctAntLensMotor()) {
		WsoLogWarn("Failed to initialize OCT anterior lens motor");
		warns += 1;
	}

	if (!getSwingMotor()->initializeSwingMotor()) {
		WsoLogWarn("Failed to initialize Swing motor");
		warns += 1;
	}

	if (!getXstageMotor()->initializeXstageMotor()) {
		WsoLogWarn("Failed to initialize X-stage motor");
		warns += 1;
	}
	else {
		ostringstream msg;
		msg << "X-stage motor initialized, pos: " << getXstageMotor()->getPosition();
		WsoLogDebug(msg.str());
	}

	if (!getYstageMotor()->initializeYstageMotor()) {
		WsoLogWarn("Failed to initialize Y-stage motor");
		warns += 1;
	}
	else {
		ostringstream msg;
		msg << "Y-stage motor initialized, pos: " << getYstageMotor()->getPosition();
		WsoLogDebug(msg.str());
	}

	if (!getZstageMotor()->initializeZstageMotor()) {
		WsoLogWarn("Failed to initialize Z-stage motor");
		warns += 1;
	}
	else {
		ostringstream msg;
		msg << "Z-stage motor initialized, pos: " << getZstageMotor()->getPosition();
		WsoLogDebug(msg.str());
	}
	if (!getChinRestMotor()->initializeChinRestMotor()) {
		WsoLogWarn("Failed to initialize Chin Rest motor");
		warns += 1;
	}

	// return true;
	if (!initiateRetinaCamera()) {
		warns += 1;
	}

	if (!initiateCorneaCamera(CameraType::IR_CORNEA_LEFT)) {
		warns += 1;
	}

	if (!initiateCorneaCamera(CameraType::IR_CORNEA_RIGHT)) {
		warns += 1;
	}
	
	if (!initiateCorneaCamera(CameraType::IR_CORNEA_LOWER)) {
		warns += 1;
	}

	if (!initiateColorCamera()) {
		warns += 1;
	}

	if (!getLsoScanner()->initializeLsoScanner()) {
		WsoLogWarn("Failed to initialize LSO scanner");
		warns += 1;
	}
	getGalvanometer()->initializeGalvanometer();
	return true;
}


bool wso_device::MainBoard::initiateRetinaCamera(void)
{
	int warns = 0;

	auto* camera = getRetinaCamera();
	if (!camera->initializeRetinaCamera()) {
		WsoLogWarn("Retina IR camera not initialized");
		warns += 1;
	}
	else {
		WsoLogInfo("Retina IR camera initialized");

		if (camera->play()) {
			bool is_error = false;
			for (int i = 0; i < 20; i++) {
				this_thread::sleep_for(chrono::milliseconds(50));
				if (camera->getErrorCount() > 0) {
					WsoLogWarn("Retina IR camera frame error");
					warns += 1;
					is_error = true;
					break;
				}
			}
			camera->pause();

			if (!is_error) {
				if (camera->getFrameCount() == 0) {
					WsoLogWarn("Retina IR camera frame error");
					warns += 1;
				}
				else {
					WsoLogInfo("Retina IR camera frame tested");
				}
			}
		}
		else {
			WsoLogWarn("Failed to start retina IR camera stream");
			warns += 1;
		}
	}

	if (!warns) {
		LogD() << "Retina IR camera, initial again: " << camera->getAnalogGain() << ", dgain: " << camera->getDigitalGain();
	}
	return (warns == 0);
}

bool wso_device::MainBoard::initiateCorneaCamera(CameraType type)
{
	int warns = 0;
	string strMessage = "";

	auto* camera = getCorneaCamera(type);
	auto name = camera->getCameraName();
	auto camId = camera->getCameraId();
	if (!camera->initializeCorneaCamera()) {
		strMessage = std::format("Cornea IR camera not initialized, name: {}, camId: {}", name, (int)(camId));
		WsoLogWarn(strMessage);
		warns += 1;
	}
	else {
		WsoLogInfo("Cornea IR camera initialized");

		if (camera->play()) {
			bool is_error = false;
			for (int i = 0; i < 20; i++) {
				this_thread::sleep_for(chrono::milliseconds(50));
				if (camera->getErrorCount() > 0) {
					strMessage = std::format("Cornea IR camera frame error, name: {}, camId: {}", name, (int)(camId));
					WsoLogWarn(strMessage);
					warns += 1;
					is_error = true;
					break;
				}
			}
			camera->pause();

			if (!is_error) {
				if (camera->getFrameCount() == 0) {
					strMessage = std::format("Cornea IR camera frame error, name: {}, camId: {}", name, (int)(camId));
					WsoLogWarn(strMessage);
					warns += 1;
				}
				else {
					strMessage = std::format("Cornea IR camera frame tested, name: {}, camId: {}", name, (int)(camId));
					WsoLogInfo(strMessage);
				}
			}
		}
		else {
			strMessage = std::format("Failed to start cornea IR camera stream, name: {}, camId: {}", name, (int)(camId));
			WsoLogWarn(strMessage);
			warns += 1;
		}
	}

	if (!warns) {
		strMessage = std::format("Cornea IR camera, name: {}, camId: {}", name, (int)(camId));
		LogD() << strMessage << camera->getAnalogGain() << ", dgain: " << camera->getDigitalGain();
	}
	return (warns == 0);
}


bool wso_device::MainBoard::initiateColorCamera(void)
{
	bool color_init = false;
	for (int i = 0; i < 3; i++) {
		if (getColorCamera()->initialize()) {
			color_init = true;
			break;
		}
		else {
			// WsoLogDebug("Color camera initialized failed, retrying");
			//if (!openColorCamera(true)) {
			//	GlobalLogger::error("Color camera open failed!");
			//	return false;
			//}
		}
	}

	if (color_init) {
		WsoLogInfo("Color camera initialized");
	}
	else {
		WsoLogWarn("Color camera initialized failed");
	}

	//if (!color_init) {
	//	GlobalLogger::error("Color camera init failed!");
	//	// result = false;
	//	warns += 1;
	//}

	//if (!getFundusFlashLed()->initialize()) {
	//	GlobalLogger::warn("Fundus flash Led init failed!");
	//	warns += 1;
	//}

	return true;
}


void wso_device::MainBoard::setDeviceInitFlags(unsigned int flags)
{
	impl().deviceInitFlags |= flags;
	return;
}

auto wso_device::MainBoard::getDeviceInitFlags(void) const -> unsigned int
{
	return impl().deviceInitFlags;
}


bool wso_device::MainBoard::isMainBoardNotInUse(void) const
{
	return (impl().deviceInitFlags & DEV_FLAG_NOT_MAINBOARD) == DEV_FLAG_NOT_MAINBOARD;
}

bool wso_device::MainBoard::isIrCameraNotInUse(void) const
{
	return (impl().deviceInitFlags & DEV_FLAG_NOT_IR_CAMERA) == DEV_FLAG_NOT_IR_CAMERA;
}

bool wso_device::MainBoard::isSourceLedsNotInUse(void) const
{
	return (impl().deviceInitFlags & DEV_FLAG_NOT_SOURCE_LEDS) == DEV_FLAG_NOT_SOURCE_LEDS;
}

bool wso_device::MainBoard::isMotorsNotInUse(void) const
{
	return (impl().deviceInitFlags & DEV_FLAG_NOT_MOTORS) == DEV_FLAG_NOT_MOTORS;
}

bool wso_device::MainBoard::isOctGrabberNotInUse(void) const
{
	return (impl().deviceInitFlags & DEV_FLAG_NOT_OCT_GRAB) == DEV_FLAG_NOT_OCT_GRAB;
}

bool wso_device::MainBoard::isOpenClNotInUse(void) const
{
	return (impl().deviceInitFlags & DEV_FLAG_NOT_OPENCL) == DEV_FLAG_NOT_OPENCL;
}

bool wso_device::MainBoard::openFTDIdevices(void)
{
	bool openUsb = false;

	/////////////////////////////////////////////////////////////////////////////
	// Main Board
	// open_retry:
	for (int i = 0; i < USB_RESET_RETRY_MAX; i++) {
		if (!getUsbComm().openChannel(true)) {
			LogD() << "Failed to open USB main channel, retrying ... ";
			resetUsbChannel();
			this_thread::sleep_for(chrono::milliseconds(USB_RESET_RETRY_DELAY));
		}
		else {
			openUsb = true;
			LogD() << "USB main channel opened, desc: " << getUsbComm().getBoardDescript();
			break;
		}
	}
	/////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////
	// Sub Board
	for (int i = 0; i < USB_RESET_RETRY_MAX; i++) {
		if (!getSubComm().openChannel(false)) {
			LogD() << "Failed to open USB sub channel, retrying ... ";
			resetSubChannel();
			this_thread::sleep_for(chrono::milliseconds(USB_RESET_RETRY_DELAY));
		}
		else {
			openUsb = true;
			LogD() << "USB sub channel opened, desc: " << getUsbComm().getBoardDescript();
			break;
		}
	}
	/////////////////////////////////////////////////////////////////////////////
	return openUsb;
}

bool wso_device::MainBoard::resetFTDIdevice(void)
{
	/*
	std::string smsg = UART_CMD_FTDI_RESET;
	std::string rmsg;

	bool result = false;
	if (requestUartCommand(smsg, rmsg)) {
		if (rmsg.find(std::string(UART_CMD_ACK_PREFIX)) != std::string::npos) {
			result = true;
		}
	}
	return result;
	*/
	return true;
}

bool wso_device::MainBoard::resetUsbChannel(void)
{
	/*
	std::string smsg = UART_CMD_USB_RESET;
	std::string rmsg;

	bool result = false;
	if (requestUartCommand(smsg, rmsg)) {
		if (rmsg.find(std::string(UART_CMD_ACK_PREFIX)) != std::string::npos) {
			result = true;
		}
	}
	return result;
	*/
	return true;
}

bool wso_device::MainBoard::resetSubChannel(void)
{
	return true;
}

bool wso_device::MainBoard::waitForSystemReady(void)
{
	bool result = getUsbComm().checkSystemReadyGPIO();
	return result;
}

bool wso_device::MainBoard::prepareDevicesForOctScan(void)
{
	bool res = turnOnOctScanBeam(true);
	return res;
}

bool wso_device::MainBoard::releaseDevicesForOctScan(void)
{
	turnOnOctScanBeam(false);
	return true;
}

bool wso_device::MainBoard::turnOnOctScanBeam(bool flag)
{
	if (auto* p = getOctSldLed(); p) {
		return p->turnLaserOn();
	}
	return false;
}

bool wso_device::MainBoard::loadHostBufferTable(void)
{
	auto* hbs = getHbsDataProfile();
	
	hbs->setHbsDataComm(&impl().usbComm);

	if (hbs->loadHbsTableHeader()) {
		LogD() << "HBS table descriptor loaded from main-board";
	} else {
		LogD() << "Failed to load HBS table descriptor from main-board";
		return false;
	}

	if (hbs->loadHbsTableEntries()) {
		LogD() << "HBS table entries loaded from main-board";
	}
	else {
		LogD() << "Failed to load HBS table entries from main-board";
		return false;
	}

	///////////////////////////////////////////////////////////////
	// SubBoard
	auto* hbsSub = getSubDataProfile();

	hbsSub->setHbsDataComm(&impl().subComm);
	if (hbsSub->loadHbsTableHeader(true)) {
		LogD() << "HBS table descriptor loaded from sub-board";
	}
	else {
		LogD() << "Failed to load HBS table descriptor from sub-board";
		return false;
	}

	if (hbsSub->loadHbsTableEntries(true)) {
		LogD() << "HBS table entries loaded from sub-board";
	}
	else {
		LogD() << "Failed to load HBS table entries from sub-board";
		return false;
	}
	return true;
}

bool wso_device::MainBoard::pullSystemCalibFromMemory(void)
{
	// Send read command to board to get calibration data from EEPROM.
	if (getUsbComm().SysCalibLoad(0, sizeof(SysCal_st))) {
		Sleep(250);
		// Update calibration data to HBS profile.
		if (auto* hbs = getHbsDataProfile(); hbs) {
			if (hbs->loadCalibrationBlocks(true)) {
				return true;
			}
		}
	}
	return false;
}

bool wso_device::MainBoard::pushSystemCalibToMemory(void)
{
	// Save calibration data to EEPROM from board profile.
	if (auto* hbs = getHbsDataProfile(); hbs) {
		if (hbs->saveCalibration()) {
			Sleep(250);
			if (getUsbComm().SysCalibSave(0, sizeof(SysCal_st))) {
				return true;
			}
		}
	}
	return false;
}

bool wso_device::MainBoard::pullSystemConfigFromMemory(void)
{
	if (getUsbComm().SysCalibLoad2(0, sizeof(UserSetup_st))) {
		Sleep(250);
		if (auto* hbs = getHbsDataProfile(); hbs) {
			if (hbs->loadConfiguration()) {
				return true;
			}
		}
	}
	return false;
}

bool wso_device::MainBoard::pushSystemConfigToMemory(void)
{
	if (auto* hbs = getHbsDataProfile(); hbs) {
		if (hbs->saveConfiguration()) {
			Sleep(250);
			if (getUsbComm().SysCalibSave2(0, sizeof(UserSetup_st))) {
				return true;
			}
		}
	}
	return false;
}

bool wso_device::MainBoard::moveChinrestUp(void)
{
	return getUsbComm().ChinrestMove(1);
}

bool wso_device::MainBoard::moveChinrestDown(void)
{
	return getUsbComm().ChinrestMove(0);
}

bool wso_device::MainBoard::stopChinrest(void)
{
	return getUsbComm().ChinrestStop();
}

bool wso_device::MainBoard::isChinrestAtHighLimit(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 12)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isChinrestAtLowLimit(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 13)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isSwingMotorAtHighLimit(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 9)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isSwingMotorAtLowLimit(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 8)) {
			return true;
		}
	}
	return false;
}


bool wso_device::MainBoard::isXstageMotorAtHighLimit(void)
{
	return false;
}

bool wso_device::MainBoard::isXstageMotorAtLowLimit(void)
{
	return false;
}


bool wso_device::MainBoard::isYstageMotorAtHighLimit(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 5)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isYstageMotorAtLowLimit(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 4)) {
			return true;
		}
	}
	return false;
}


bool wso_device::MainBoard::isZstageMotorAtHighLimit(void)
{
	return false;
}

bool wso_device::MainBoard::isZstageMotorAtLowLimit(void)
{
	return false;
}

bool wso_device::MainBoard::isOctFocusMotorAtOrigin(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 0)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isOctPolarMotorAtOrigin(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 2)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isOctReferMotorAtOrigin(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 1)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isOctRefNdMotorAtOrigin(void)
{
	return false;
}

bool wso_device::MainBoard::isLsoFocusMotorAtOrigin(void)
{
	/*
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 1)) {
			return true;
		}
	}
	*/
	return false;
}

bool wso_device::MainBoard::isRetMirrorMotorAtOrigin(void)
{
	return false;
}

bool wso_device::MainBoard::isOctAntLensMotorAtOrigin(void)
{
	return false;
}

bool wso_device::MainBoard::isLsoFilterMotorAtOrigin(void)
{
	return false;
}

bool wso_device::MainBoard::isOdOsStatusFlagSet(void)
{
	if (getHbsDataProfile()->loadGpioStatus()) {
		auto* gpio = getHbsDataProfile()->getHbsGpioStatus();
		auto status = gpio->PIstatus;
		if (status & (0x01 << 15)) {
			return true;
		}
	}
	return false;
}

bool wso_device::MainBoard::isEyeSideAtOd(void)
{
	auto flag = isOdOsStatusFlagSet();
	return flag;
}

bool wso_device::MainBoard::isEyeSideAtOs(void)
{
	auto flag = isOdOsStatusFlagSet();
	return !flag;
}

EyeSide wso_device::MainBoard::getEyeSide(void)
{
	auto side = isEyeSideAtOd() ? EyeSide::OD : EyeSide::OS;
	return side;
}

bool wso_device::MainBoard::setGalvanoDynamicOffset(float mmPosX, float mmPosY, bool cornea)
{
	std::int16_t offsetX = 0;
	std::int16_t offsetY = 0;

	float mmRevX = mmPosY * -1.0f;
	float mmRevY = mmPosX * -1.0f;

	offsetX = getGalvanometer()->getStepXat(mmRevX, cornea);
	offsetY = getGalvanometer()->getStepYat(mmRevY, cornea);

	if (setGalvanoDynamicOffset(offsetX, offsetY)) {
		LogD() << "Galvano dynamic offset written, mmX: " << mmPosX << ", mmY: " << mmPosY << " => " << offsetX << ", " << offsetY;
		return true;
	}
	return false;
}

bool wso_device::MainBoard::setGalvanoDynamicOffset(std::int16_t offsetX, std::int16_t offsetY)
{
	auto* hbs = getHbsDataProfile();
	auto* param = const_cast<HbsGalvanoDynamicParam*>(hbs->getHbsGalvanoDynamicParam());
	param->offsetX = offsetX;
	param->offsetY = offsetY;

	if (hbs->saveGalvanoDynamicParam()) {
		LogD() << "Galvano dynamic offset written, x: " << offsetX << ", y: " << offsetY;
		return true;
	}
	return false;
}

bool wso_device::MainBoard::getGalvanoDynamicOffset(std::int16_t& offsetX, std::int16_t& offsetY)
{
	if (auto* hbs = getHbsDataProfile();  hbs->loadGalvanoDynamicParam()) {
		auto* param = hbs->getHbsGalvanoDynamicParam();
		offsetX = param->offsetX;
		offsetY = param->offsetY;
		LogD() << "Galvano dynamic offset read, x: " << offsetX << ", y: " << offsetY;
		return true;
	}
	return false;
}

bool wso_device::MainBoard::setLcdFixationOn(bool flag, int row, int col)
{
	impl().lcdFixationRow = row;
	impl().lcdFixationCol = col;
	impl().lcdFixationOn = flag;

	return getUsbComm().LcdFixationControl(row, col);
}

bool wso_device::MainBoard::getLcdFixationOn(int& row, int& col)
{
	row = impl().lcdFixationRow;
	col = impl().lcdFixationCol;

	if (!impl().lcdFixationOn) {
		return false;
	}
	return true;
}

bool wso_device::MainBoard::updateLcdParameters(const InternalFixationParam param)
{
	return updateLcdParameters((std::uint8_t)param.brightness, (std::uint8_t)param.blinkMode, (std::uint16_t)param.blinkPeriod, (std::uint16_t)param.blinkOnTime, (std::uint8_t)param.fixationType);
}

bool wso_device::MainBoard::updateLcdParameters(std::uint8_t bright, std::uint8_t blink, std::uint16_t period, std::uint16_t ontime, std::uint8_t type)
{
	return getUsbComm().LcdFixationParameters(bright, blink, period, ontime, type);
}

wso_board::HbsDataProfile* wso_device::MainBoard::getHbsDataProfile(void) const
{
	return impl().hbsProfile.get();
}

wso_board::HbsDataProfile* wso_device::MainBoard::getSubDataProfile(void) const
{
	return impl().subProfile.get();
}

UsbComm& wso_device::MainBoard::getUsbComm(void) const
{
	return impl().usbComm;
}

UsbComm& wso_device::MainBoard::getSubComm(void) const
{
	return impl().subComm;
}


SldLaserDriver* wso_device::MainBoard::getSldLaserDriver(void) const
{
	return &impl().sldLaserDriver;
}

ZynqXadcDriver* wso_device::MainBoard::getZyncXadcDriver(void) const
{
	return &impl().zyncXadcDriver;
}


LsoWhiteLed* wso_device::MainBoard::getLsoWhiteLed(void) const
{
	return (LsoWhiteLed*)getLightLed(LightType::LSO_WHITE_LED);
}

LsoBlueLed* wso_device::MainBoard::getLsoBlueLed(void) const
{
	return (LsoBlueLed*)getLightLed(LightType::LSO_BLUE_LED);
}

LsoGreenLed* wso_device::MainBoard::getLsoGreenLed(void) const
{
	return (LsoGreenLed*)getLightLed(LightType::LSO_GREEN_LED);
}

RetinaIrLed* wso_device::MainBoard::getRetinaIrLed(void) const
{
	return (RetinaIrLed*)getLightLed(LightType::RETINA_IR_LED);
}

CorneaIrLed* wso_device::MainBoard::getCorneaIrLeftLed(void) const
{
	return (CorneaIrLed*)getLightLed(LightType::CORNEA_IR_LEFT_LED);
}

CorneaIrLed* wso_device::MainBoard::getCorneaIrRightLed(void) const
{
	return (CorneaIrLed*)getLightLed(LightType::CORNEA_IR_RIGHT_LED);
}

OctFocusMotor* wso_device::MainBoard::getOctFocusMotor(void) const
{
	return (OctFocusMotor*)getStepMotor(StepMotorType::OCT_FOCUS);
}

OctPolarMotor* wso_device::MainBoard::getOctPolarMotor(void) const
{
	return (OctPolarMotor*)getStepMotor(StepMotorType::OCT_POLAR);
}

OctReferMotor* wso_device::MainBoard::getOctReferMotor(void) const
{
	return (OctReferMotor*)getStepMotor(StepMotorType::OCT_REFER);
}

OctRefNdMotor* wso_device::MainBoard::getOctRefNdMotor(void) const
{
	return (OctRefNdMotor*)getStepMotor(StepMotorType::OCT_REFND);
}

LsoFocusMotor* wso_device::MainBoard::getLsoFocusMotor(void) const
{
	return (LsoFocusMotor*)getStepMotor(StepMotorType::LSO_FOCUS);
}

RetMirrorMotor* wso_device::MainBoard::getRetMirrorMotor(void) const
{
	return (RetMirrorMotor*)getStepMotor(StepMotorType::RET_MIRROR);
}

OctAntLensMotor* wso_device::MainBoard::getOctAntLensMotor(void) const
{
	return (OctAntLensMotor*)getStepMotor(StepMotorType::OCT_ANT_LENS);
}

LsoFilterMotor* wso_device::MainBoard::getLsoFilterMotor(void) const
{
	return (LsoFilterMotor*)getStepMotor(StepMotorType::LSO_FILTER);
}

XstageMotor* wso_device::MainBoard::getXstageMotor(void) const
{
	return (XstageMotor*)getStepMotor(StepMotorType::STAGE_X);
}

YstageMotor* wso_device::MainBoard::getYstageMotor(void) const
{
	return (YstageMotor*)getStepMotor(StepMotorType::STAGE_Y);
}

ZstageMotor* wso_device::MainBoard::getZstageMotor(void) const
{
	return (ZstageMotor*)getStepMotor(StepMotorType::STAGE_Z);
}

ChinRestMotor* wso_device::MainBoard::getChinRestMotor(void) const
{
	return (ChinRestMotor*)getStepMotor(StepMotorType::CHIN_REST);
}

SwingMotor* wso_device::MainBoard::getSwingMotor(void) const
{
	return (SwingMotor*)getStepMotor(StepMotorType::SWING);
}

RetinaCamera* wso_device::MainBoard::getRetinaCamera(void) const
{
	return (RetinaCamera*)getInfraredCamera(CameraType::IR_RETINA);
}

CorneaCamera* wso_device::MainBoard::getCorneaLeftCamera(void) const
{
	return (CorneaCamera*)getInfraredCamera(CameraType::IR_CORNEA_LEFT);
}

CorneaCamera* wso_device::MainBoard::getCorneaRightCamera(void) const
{
	return (CorneaCamera*)getInfraredCamera(CameraType::IR_CORNEA_RIGHT);
}

CorneaCamera* wso_device::MainBoard::getCorneaLowerCamera(void) const
{
	return (CorneaCamera*)getInfraredCamera(CameraType::IR_CORNEA_LOWER);
}

CorneaCamera* wso_device::MainBoard::getCorneaCamera(CameraType type) const
{
	CorneaCamera* pCamera = nullptr;

	switch (type)
	{
	case CameraType::IR_CORNEA_LEFT:
		pCamera = (CorneaCamera*)getInfraredCamera(CameraType::IR_CORNEA_LEFT);
		break;
	case CameraType::IR_CORNEA_RIGHT:
		pCamera = (CorneaCamera*)getInfraredCamera(CameraType::IR_CORNEA_RIGHT);
		break;
	case CameraType::IR_CORNEA_LOWER:
		pCamera = (CorneaCamera*)getInfraredCamera(CameraType::IR_CORNEA_LOWER);
		break;
	case CameraType::IR_RETINA:
		pCamera = (CorneaCamera*)getInfraredCamera(CameraType::IR_RETINA);
		break;
	}
	return pCamera;
}

ColorCamera* wso_device::MainBoard::getColorCamera(void)
{
	return (ColorCamera*)impl().colorCamera.get();
}

LsoScanner* wso_device::MainBoard::getLsoScanner(void) const
{
	return impl().lsoScanner.get();
}

OctSldLed* wso_device::MainBoard::getOctSldLed(void) const
{
	return impl().octSldLed.get();
}

Galvanometer* wso_device::MainBoard::getGalvanometer(void) const
{
	return impl().octGalvano.get();
}

LightLed* wso_device::MainBoard::getLightLed(LightType type) const
{
	auto p = impl().lightLeds.find(type);
	if (p != impl().lightLeds.end()) {
		LightLed* led = p->second.get();
		return led;
	}
	return nullptr;
}

StepMotor* wso_device::MainBoard::getStepMotor(StepMotorType type) const
{
	auto motor_type = static_cast<MotorType>(type);
	auto p = impl().stepMotors.find(motor_type);
	if (p != impl().stepMotors.end()) {
		StepMotor* motor = p->second.get();
		return motor;
	}
	return nullptr;
}


StageMotor* wso_device::MainBoard::getStageMotor(StageMotorType type) const
{
	auto motor_type = static_cast<MotorType>(type);
	auto p = impl().stageMotors.find(motor_type);
	if (p != impl().stageMotors.end()) {
		StageMotor* motor = p->second.get();
		return motor;
	}
	return nullptr;
}

InfraredCamera* wso_device::MainBoard::getInfraredCamera(CameraType type) const
{
	for (int i = 0; i < impl().irCameras.size(); i++) {
		InfraredCamera* irCam = impl().irCameras[i].get();
		if (irCam->getType() == type) {
			return irCam;
		}
	}
	return nullptr;
}

FirmwareControl* wso_device::MainBoard::getFirmwareControl(void) const
{
	return impl().firmwareControl.get();
}

MainBoard::MainBoardImpl& wso_device::MainBoard::impl(void) const
{
	return *d_ptr;
}
