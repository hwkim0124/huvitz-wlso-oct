#include "pch.h"
#include "ZynqXadcDriver.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;


struct ZynqXadcDriver::ZynqXadcDriverImpl
{
	MainBoard* mainboard;

	ZynqXadcDriverImpl() {
		initializeZynqXadcDriverImpl();
	}

	void initializeZynqXadcDriverImpl() {
		mainboard = nullptr;
	}
};


ZynqXadcDriver::ZynqXadcDriver() : d_ptr(make_unique<ZynqXadcDriverImpl>())
{
}


ZynqXadcDriver::~ZynqXadcDriver()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::ZynqXadcDriver::ZynqXadcDriver(ZynqXadcDriver&& rhs) noexcept = default;
ZynqXadcDriver& wso_device::ZynqXadcDriver::operator=(ZynqXadcDriver&& rhs) noexcept = default;


void wso_device::ZynqXadcDriver::initializeZynqXadcDriver(MainBoard* mainboard)
{
	impl().initializeZynqXadcDriverImpl();

	impl().mainboard = mainboard;
}

bool wso_device::ZynqXadcDriver::fetchZynqXadcStatus(AdcSensorStatus* status)
{
	if (status) {
		status->cpuTempature = getCpuTemperature();
		status->externalMonitorPd = getExternalLdMonPd();
		status->lensHallSensor1 = getLensHallSensor1();
		status->lensHallSensor2 = getLensHallSensor2();
		status->odOs = getOdOs();
		return true;
	}
	return false;
}

float wso_device::ZynqXadcDriver::getCpuTemperature(void) const
{
	if (impl().mainboard) {
		auto* status = getMainBoard()->getHbsDataProfile()->getHbsZyncXADC();
		auto value = status->cpu_temp;
		return value;
	}
	return 0.0f;
}

int wso_device::ZynqXadcDriver::getExternalLdMonPd(void) const
{
	if (impl().mainboard) {
		auto* status = getMainBoard()->getHbsDataProfile()->getHbsZyncXADC();
		auto value = status->ext_ld_mon_pd;
		return value;
	}
	return 0;
}

int wso_device::ZynqXadcDriver::getLensHallSensor1(void) const
{
	if (impl().mainboard) {
		auto* status = getMainBoard()->getHbsDataProfile()->getHbsZyncXADC();
		auto value = status->Hall_Sensor1;
		return value;
	}
	return 0;
}

int wso_device::ZynqXadcDriver::getLensHallSensor2(void) const
{
	if (impl().mainboard) {
		auto* status = getMainBoard()->getHbsDataProfile()->getHbsZyncXADC();
		auto value = status->Hall_sensor2;
		return value;
	}
	return 0;
}

int wso_device::ZynqXadcDriver::getOdOs(void) const
{
	if (impl().mainboard) {
		auto* status = getMainBoard()->getHbsDataProfile()->getHbsZyncXADC();
		auto value = status->odos;
		return value;
	}
	return 0;
}

bool wso_device::ZynqXadcDriver::reloadZynqXadcStatus(void)
{
	if (impl().mainboard) {
		if (auto* hbs = getMainBoard()->getHbsDataProfile(); hbs->loadZynqXADC()) {
			return true;
		}
	}
	return false;
}

MainBoard* wso_device::ZynqXadcDriver::getMainBoard(void) const
{
	return impl().mainboard;
}

ZynqXadcDriver::ZynqXadcDriverImpl& wso_device::ZynqXadcDriver::impl(void) const
{
	return *d_ptr;
}
