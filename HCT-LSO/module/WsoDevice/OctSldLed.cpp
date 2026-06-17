#include "pch.h"
#include "OctSldLed.h"

#include "MainBoard.h"
#include "UsbComm.h"


#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct OctSldLed::OctSldLedImpl
{
	OctSldCalibParam param;
	LaserType type;

	int value;

	OctSldLedImpl() 
	{
		type = LaserType::OCT_SLD;
		value = 0;
	}
};


OctSldLed::OctSldLed() :
	d_ptr(make_unique<OctSldLedImpl>())
{
}


wso_device::OctSldLed::OctSldLed(MainBoard* board) :
	d_ptr(make_unique<OctSldLedImpl>()), BoardComponent(board)
{
}


OctSldLed::~OctSldLed()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctSldLed::OctSldLed::OctSldLed(OctSldLed&& rhs) = default;
OctSldLed& OctSldLed::OctSldLed::operator=(OctSldLed&& rhs) = default;

/*
wso_device::OctSldLed::OctSldLed(const OctSldLed& rhs)
	: d_ptr(make_unique<OctSldLedImpl>(*rhs.d_ptr))
{
}


OctSldLed& wso_device::OctSldLed::operator=(const OctSldLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}
*/

bool wso_device::OctSldLed::initializeOctSldLed(void)
{
	setInitiated(true);
	loadCalibParamFromProfile();
	turnLaserOff();
	return true;
}


bool wso_device::OctSldLed::writeCalibParam_BoardToMemory(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LedSldUpdateParameters(getType(), 0)) {
		LogDebug() << "OctSldLed::updateParameters(0) failed!";
		return false;
	}
	if (!usbComm.LedSldUpdateParameters(getType(), 1)) {
		LogDebug() << "OctSldLed::updateParameters(1) failed!";
		return false;
	}
	if (!usbComm.LedSldUpdateParameters(getType(), 2)) {
		LogDebug() << "OctSldLed::updateParameters(2) failed!";
		return false;
	}
	if (!usbComm.LedSldUpdateParameters(getType(), 3)) {
		LogDebug() << "OctSldLed::updateParameters(3) failed!";
		return false;
	}
	return true;
}

bool wso_device::OctSldLed::fetchCalibParam_MemoryToBoard(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LedSldGetParameters(getType(), 0)) {
		LogDebug() << "OctSldLed::fetchParameters(0) failed!";
		return false;
	}
	if (!usbComm.LedSldGetParameters(getType(), 1)) {
		LogDebug() << "OctSldLed::fetchParameters(1) failed!";
		return false;
	}
	if (!usbComm.LedSldGetParameters(getType(), 2)) {
		LogDebug() << "OctSldLed::fetchParameters(2) failed!";
		return false;
	}
	if (!usbComm.LedSldGetParameters(getType(), 3)) {
		LogDebug() << "OctSldLed::fetchParameters(3) failed!";
		return false;
	}
	return true;
}


bool wso_device::OctSldLed::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource(); p) {
		impl().param.highCode = p->SLD_Param.RmonHighCode;
		impl().param.lowCode1 = p->SLD_Param.RmonLowCode1;
		impl().param.lowCode2 = p->SLD_Param.RmonLowCode2;
		impl().param.rsiCode = p->SLD_Param.RsiCode;
		return true;
	}
	return false;
}

bool wso_device::OctSldLed::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibLedSource*>(getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource()); p) {
		p->SLD_Param.RmonHighCode = impl().param.highCode;
		p->SLD_Param.RmonLowCode1 = impl().param.lowCode1;
		p->SLD_Param.RmonLowCode2 = impl().param.lowCode2;
		p->SLD_Param.RsiCode = impl().param.rsiCode;
		return true;
	}
	return false;
}

bool wso_device::OctSldLed::isLaserOn(void)
{
	auto flag = (impl().value > 0);
	return flag;
}

bool wso_device::OctSldLed::turnLaserOn(void)
{
	auto res = control(1);
	return res;
}

bool wso_device::OctSldLed::turnLaserOff(void)
{
	auto res = control(0);
	return res;
}

bool wso_device::OctSldLed::control(int value)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSldControl(impl().type, (uint8_t)value)) {
		impl().value = value;
		return true;
	}
	return false;
}

const char* wso_device::OctSldLed::getName(void) const
{
	return OCT_SLD_NAME;
}

LaserType wso_device::OctSldLed::getType(void) const
{
	return impl().type;
}


std::uint16_t wso_device::OctSldLed::getHighCode(void)
{
	return impl().param.highCode;
}


std::uint16_t wso_device::OctSldLed::getLowCode1(void)
{
	return impl().param.lowCode1;
}


std::uint16_t wso_device::OctSldLed::getLowCode2(void)
{
	return impl().param.lowCode2;
}


std::uint16_t wso_device::OctSldLed::getRsiCode(void)
{
	return impl().param.rsiCode;
}


bool wso_device::OctSldLed::setHighCode(std::uint16_t code)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSldPotentiometer(getType(), static_cast<uint8_t>(SldControlChannel::RMON_HIGH_CODE), code)) {
		impl().param.highCode = code;
		saveCalibParamToProfile();
		return true;
	}
	LogDebug() << "OctSldLed::setHighCode() failed!";
	return false;
}


bool wso_device::OctSldLed::setLowCode1(std::uint16_t code)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSldPotentiometer(getType(), static_cast<uint8_t>(SldControlChannel::RMON_LOW_CODE1), code)) {
		impl().param.lowCode1 = code;
		saveCalibParamToProfile();
		return true;
	}
	LogDebug() << "OctSldLed::setLowCode1() failed!";
	return false;
}


bool wso_device::OctSldLed::setLowCode2(std::uint16_t code)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSldPotentiometer(getType(), static_cast<uint8_t>(SldControlChannel::RMON_LOW_CODE2), code)) {
		impl().param.lowCode2 = code;
		saveCalibParamToProfile();
		return true;
	}
	LogDebug() << "OctSldLed::setLowCode2() failed!";
	return false;
}


bool wso_device::OctSldLed::setRsiCode(std::uint16_t code)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSldPotentiometer(getType(), static_cast<uint8_t>(SldControlChannel::RMON_RSI_CODE), code)) {
		impl().param.rsiCode = code;
		saveCalibParamToProfile();
		return true;
	}
	LogDebug() << "OctSldLed::setRsiCode() failed!";
	return false;
}


OctSldLed::OctSldLedImpl& wso_device::OctSldLed::impl(void) const
{
	// TODO: insert return statement here
	return *d_ptr;
}