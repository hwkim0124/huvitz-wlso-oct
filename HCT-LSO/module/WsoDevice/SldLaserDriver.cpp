#include "pch.h"
#include "SldLaserDriver.h"
#include "OctSldLed.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct SldLaserDriver::SldLaserDriverImpl
{
	MainBoard* mainboard;

	SldLaserDriverImpl() {
		initializeSldLaserDriverImpl();
	}

	void initializeSldLaserDriverImpl() {
		mainboard = nullptr;
	}
};


SldLaserDriver::SldLaserDriver() : d_ptr(make_unique<SldLaserDriverImpl>())
{
}


SldLaserDriver::~SldLaserDriver()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::SldLaserDriver::SldLaserDriver(SldLaserDriver&& rhs) noexcept = default;
SldLaserDriver& wso_device::SldLaserDriver::operator=(SldLaserDriver&& rhs) noexcept = default;


void wso_device::SldLaserDriver::initializeSldLaserDriver(MainBoard* mainboard)
{
	impl().initializeSldLaserDriverImpl();

	impl().mainboard = mainboard;
}

bool wso_device::SldLaserDriver::isInitialized(void)
{
	return impl().mainboard != nullptr;
}

bool wso_device::SldLaserDriver::getOctSldStatusParam(OctSldStatusParam* status)
{
	if (status) {
		auto sld = getMainBoard()->getOctSldLed();
		status->calibParam.highCode = sld->getHighCode();
		status->calibParam.lowCode1 = sld->getLowCode1();
		status->calibParam.lowCode2 = sld->getLowCode2();
		status->calibParam.rsiCode = sld->getRsiCode();

		status->sldCurrent = getSldCurrent();
		status->ipdCurrent = getIpdCurrent();
		status->externalPd = getExternalPd();
		status->temperature = getTemperature();
		return true;
	}
	return false;
}

bool wso_device::SldLaserDriver::getOctSldCalibParam(OctSldCalibParam* calib)
{
	if (calib) {
		if (auto* sld = getMainBoard()->getOctSldLed(); sld->loadCalibParamFromProfile()) {
			calib->highCode = sld->getHighCode();
			calib->lowCode1 = sld->getLowCode1();
			calib->lowCode2 = sld->getLowCode2();
			calib->rsiCode = sld->getRsiCode();
			return true;
		}
	}
	return false;
}

float wso_device::SldLaserDriver::getSldCurrent(void)
{
	if (isInitialized()) {
		auto* sld = getMainBoard()->getHbsDataProfile()->getHbsSldStatus();
		auto value = sld->SLD_status.SLD_current;
		return value;
	}
	return 0.0f;
}

float wso_device::SldLaserDriver::getIpdCurrent(void)
{
	if (isInitialized()) {
		auto* sld = getMainBoard()->getHbsDataProfile()->getHbsSldStatus();
		auto value = sld->SLD_status.IPD_current;
		return value;
	}
	return 0.0f;
}

float wso_device::SldLaserDriver::getTemperature(void)
{
	if (isInitialized()) {
		auto* sld = getMainBoard()->getHbsDataProfile()->getHbsSldStatus();
		auto value = sld->SLD_status.temp;
		return value;
	}
	return 0.0f;
}

int wso_device::SldLaserDriver::getExternalPd(void)
{
	if (isInitialized()) {
		auto* sld = getMainBoard()->getHbsDataProfile()->getHbsSldStatus();
		auto value = sld->SLD_status.EPD_DN;
		return value;
	}
	return 0;
}

std::string wso_device::SldLaserDriver::getSerialName(void)
{
	if (isInitialized()) {
		auto sld = getMainBoard()->getOctSldLed();
		auto value = sld->getName();
		return value;
	}
	return std::string();
}

bool wso_device::SldLaserDriver::reloadOctSldStatus(void)
{
	if (isInitialized()) {
		if (auto* hbs = impl().mainboard->getHbsDataProfile(); hbs->loadSldStatus()) {
			return true;
		}
	}
	return false;
}

bool wso_device::SldLaserDriver::reloadSystemProfile(void)
{
	if (isInitialized()) {
		if (auto* hbs = impl().mainboard->getHbsDataProfile(); hbs->loadCalibBlockLedSource(false)) {
			auto sld = getMainBoard()->getOctSldLed();
			return sld->loadCalibParamFromProfile();
		}
	}
	return false;
}

bool wso_device::SldLaserDriver::saveSldCalibration(void)
{
	if (isInitialized()) {
		if (auto sld = getMainBoard()->getOctSldLed(); sld) {
			return sld->writeCalibParam_BoardToMemory();
		}
	}
	return false;
}

bool wso_device::SldLaserDriver::loadSldCalibration(void)
{
	if (isInitialized()) {
		if (auto sld = getMainBoard()->getOctSldLed(); sld) {
			return sld->fetchCalibParam_MemoryToBoard();
		}
	}
	return false;
}

bool wso_device::SldLaserDriver::updateSystemProfile(void)
{
	if (isInitialized()) {
		auto sld = getMainBoard()->getOctSldLed();
		sld->saveCalibParamToProfile();
		if (auto* hbs = impl().mainboard->getHbsDataProfile(); hbs->saveCalibration()) {
			return true;
		}
	}
	return false;
}

MainBoard* wso_device::SldLaserDriver::getMainBoard(void) const
{
	return impl().mainboard;
}


SldLaserDriver::SldLaserDriverImpl& wso_device::SldLaserDriver::impl(void) const
{
	return *d_ptr;
}
