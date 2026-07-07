#include "pch.h"
#include "Bootstrapper.h"
#include "Hardware.h"
#include "OctScanning.h"
#include "Calibration.h"
#include "Configuration.h"

#include <ostream>


using namespace wso_system;
using namespace std;


std::mutex Bootstrapper::singleMutex_;



struct Bootstrapper::BootstrapperImpl
{
	bool isSystemInitialized;

	bool isMainBoardNotInit; 
	bool isSourceLedsNotInit;
	bool isMotorsNotInit;
	bool isIrCameraNotInit;
	bool isOctGrabberNotInit;

	bool isLoggingSrcLine; 
	bool isKeepingLogFiles;

	Logger::LogLevel logLevel;

	BootstrapperImpl() {
		initializeBootstrapperImpl();
	}

	void initializeBootstrapperImpl(void) {
		isSystemInitialized = false;

		isMainBoardNotInit = false;
		isSourceLedsNotInit = false;
		isMotorsNotInit = false;
		isIrCameraNotInit = false;
		isOctGrabberNotInit = false;

		isLoggingSrcLine = false;
		isKeepingLogFiles = false;

		// logLevel = Logger::LogLevel::LEVEL_INFO;
		logLevel = Logger::LogLevel::LEVEL_DEBUG;
	}
};



wso_system::Bootstrapper::Bootstrapper() :
	d_ptr(std::make_unique<BootstrapperImpl>())
{
}


wso_system::Bootstrapper::~Bootstrapper()
{
}


Bootstrapper* wso_system::Bootstrapper::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static Bootstrapper instance;
	return &instance;
}

bool wso_system::Bootstrapper::initializeWsoSystem(WsoLogMsgCallback clb, bool trace_mode)
{
	// Initialize internal system logger. 
	initSystemLogger();

	// Start with user log callback.
	initLogMsgCallback(clb, trace_mode);

	WsoLogInfo("Starting Huvitz Wide OCT-LSO System");

	establishSystemBootMode();

	if (!initializeHardware()) {
		WsoLogError("Failed to initialize system devices");
		return false;
	}

	if (!implementSystemCalibration()) {
		WsoLogError("System calibration not implemented");
		return false;
	}

	if (!initOctScanning()) {
		// return false;
	}

	if (!implementSystemConfiguration()) {
		WsoLogError("System configuration not implemented.");
		// return false;
	}
	WsoLogInfo("Wso system initialized");
	impl().isSystemInitialized = true;
	return true;
}

bool wso_system::Bootstrapper::isWsoSystemInitialized(void) const
{
	return impl().isSystemInitialized;
}

void wso_system::Bootstrapper::releaseWsoSystem(void)
{
	if (isWsoSystemInitialized()) {
		if (auto board = Hardware::getInstance()->getMainBoard(); board) {
			board->releaseMainBoard();
		}
		if (auto usb3 = Hardware::getInstance()->getUsb3Grabber(); usb3) {
			usb3->releaseUsb3Grabber();
		}
		if (auto oct = OctScanning::getInstance(); oct) {
			oct->releaseOctScanning();
		}
	}

	impl().isSystemInitialized = false;
	WsoLogInfo("Wso system released");
	return;
}

bool wso_system::Bootstrapper::initSystemLogger(void)
{
	auto level = impl().logLevel;
	auto srcLine = impl().isLoggingSrcLine;
	auto keepFiles = impl().isKeepingLogFiles;

	Logger::initLogger(level, srcLine, true, true, keepFiles);
	return true;
}

bool wso_system::Bootstrapper::initLogMsgCallback(WsoLogMsgCallback clb, bool trace_mode)
{
	connectWsoLogMsgCallback(clb);
	if (trace_mode) {
		WsoLogger::getInstance()->setLogMsgLevel(LOG_MSG_TRACE);
	}
	return true;
}

void wso_system::Bootstrapper::connectWsoLogMsgCallback(WsoLogMsgCallback clb)
{
	if (auto* logger = WsoLogger::getInstance(); logger) {
		logger->setLogMsgCallback(clb);
	}
}

void wso_system::Bootstrapper::releaseWsoLogMsgCallback(void)
{
	if (auto* logger = WsoLogger::getInstance(); logger) {
		logger->setLogMsgCallback(nullptr);
	}
}

bool wso_system::Bootstrapper::initializeHardware(void)
{
	auto* hardware = Hardware::getInstance();
	
	if (!hardware->initializeBoardDevices()) {
		return false;
	}

	if (!hardware->initializeOctScanner()) {
		return false;
	}
	return true;
}

bool wso_system::Bootstrapper::initOctScanning(void)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (board->isOctGrabberNotInUse()) {
			return true;
		}
		if (auto* scan = OctScanning::getInstance(); scan) {
			if (!scan->initializeOctScanning()) {
				WsoLogError("OCT scanning initialization failed!");
				return false;
			}
		}
		WsoLogInfo("OCT scanning initialized => ok");
		return true;
	}
	return false;
}

void wso_system::Bootstrapper::establishSystemBootMode(void)
{
	parseCommandLineArguments();

	auto* hardware = Hardware::getInstance();
	hardware->excludeDeviceFromInit(
		impl().isMainBoardNotInit,
		impl().isSourceLedsNotInit,
		impl().isMotorsNotInit,
		impl().isIrCameraNotInit
	);

	hardware->excludeGrabberFromInit(
		impl().isOctGrabberNotInit
	);

	return;
}

void wso_system::Bootstrapper::parseCommandLineArguments(void)
{
	ostringstream ss;

	if (__argc > 1)
	{
		for (int i = 1; i < __argc; i++)
		{
			string option = __argv[i];

			if (option.compare("-mainboard_off")) {
				ss << "-mainboard_off" << " ";
				impl().isMainBoardNotInit = true;
				impl().isSourceLedsNotInit = true;
				impl().isMotorsNotInit = true;
				impl().isIrCameraNotInit = true;
				impl().isOctGrabberNotInit = true;
			}
			else if (option.compare("-motor_off")) {
				ss << "-motor_off" << " ";
				impl().isMotorsNotInit = true;
			}
			else if (option.compare("-source_off")) {
				ss << "-source_off" << " ";
				impl().isSourceLedsNotInit = true;
			}
			else if (option.compare("-camera_off")) {
				ss << "-camera_off" << " ";
				impl().isIrCameraNotInit = true;
			}
			else if (option.compare("-octgrab_off")) {
				ss << "-octgrab_off" << " ";
				impl().isOctGrabberNotInit = true;
			}
			else if (option.compare("-debug")) {
				ss << "-debug" << " ";
				impl().logLevel = Logger::LogLevel::LEVEL_DEBUG;
			}
			else if (option.compare("-srcline")) {
				ss << "-srcline" << " ";
				impl().isLoggingSrcLine = true;
			}
			else if (option.compare("-logkeep")) {
				ss << "-logkeep" << " ";
				impl().isKeepingLogFiles = true;
			}
		}
	}

	if (!ss.str().empty()) {
		ostringstream msg;
		msg << "Command options: " << ss.str();
		WsoLogInfo(msg.str());
	}
	return;
}


bool wso_system::Bootstrapper::implementSystemCalibration(void)
{
	if (auto* calib = Calibration::getInstance(); calib) {
		if (!calib->loadSystemCalibration(true)) {
			WsoLogError("Failed to load system calibration.");
			return false;
		}
		if (!calib->applySystemCalibration()) {
			WsoLogError("Failed to apply system calibration.");
			return false;
		}
		return true;
	}
	return false;
}


bool wso_system::Bootstrapper::implementSystemConfiguration(void)
{
	if (auto* config = Configuration::getInstance(); config) {
		if (!config->loadSystemConfiguration(true)) {
			WsoLogWarn("Failed to load system configuration.");
			if (!config->saveSystemConfiguration(true)) {
				WsoLogError("Failed to save system configuration.");
				return false;
			}
		}
		if (!config->applySystemConfiguration()) {
			WsoLogError("Failed to apply system configuration.");
			return false;
		}
		return true;
	}
	return false;
}


Bootstrapper::BootstrapperImpl& wso_system::Bootstrapper::impl(void) const
{
	return *d_ptr;
}
