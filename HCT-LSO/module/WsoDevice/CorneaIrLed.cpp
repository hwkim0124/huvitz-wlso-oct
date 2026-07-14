#include "pch.h"
#include "CorneaIrLed.h"

#include "MainBoard.h"

using namespace wso_device;
using namespace std;

struct CorneaIrLed::CorneaIrLedImpl
{
	CorneaIrLedImpl()
	{

	}
};

CorneaIrLed::CorneaIrLed() :
	d_ptr(make_unique<CorneaIrLedImpl>()), LightLed()
{
}


wso_device::CorneaIrLed::CorneaIrLed(MainBoard* board, LightType LedType):
	d_ptr(make_unique<CorneaIrLedImpl>()), LightLed(board, LedType)
{
}


CorneaIrLed::~CorneaIrLed()
{

}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
CorneaIrLed::CorneaIrLed::CorneaIrLed(CorneaIrLed&& rhs) = default;
CorneaIrLed& CorneaIrLed::CorneaIrLed::operator=(CorneaIrLed&& rhs) = default;


wso_device::CorneaIrLed::CorneaIrLed(const CorneaIrLed& rhs)
	: d_ptr(make_unique<CorneaIrLedImpl>(*rhs.d_ptr))
{
}


CorneaIrLed& wso_device::CorneaIrLed::operator=(const CorneaIrLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::CorneaIrLed::initializeCorneaIrLed(void)
{
	if (LightLed::initializeLightLed()) {
		loadCalibParamFromProfile();
		turnLightOff();
		return true;
	}
	return false;
}

bool wso_device::CorneaIrLed::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource(); p) {
		unsigned short value = 0;
		if (getType() == LightType::CORNEA_IR_LEFT_LED) {
			value = p->LED_Info.AntIR1_intensity;
			LogD() << "CorneaIrLed Left intensity loaded from profile: " << value;
		}
		else if (getType() == LightType::CORNEA_IR_RIGHT_LED) {
			value = p->LED_Info.AntIR2_intensity;
			LogD() << "CorneaIrLed Right intensity loaded from profile: " << value;
		}
		setIntensity(value);
		return true;
	}
	return false;
}

bool wso_device::CorneaIrLed::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibLedSource*>(getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource()); p) {
		auto value = getIntensity();
		if (getType() == LightType::CORNEA_IR_LEFT_LED) {
			p->LED_Info.AntIR1_intensity = value;
		}
		else if (getType() == LightType::CORNEA_IR_RIGHT_LED) {
			p->LED_Info.AntIR2_intensity = value;
		}
		return true;
	}
	return false;
}


CorneaIrLed::CorneaIrLedImpl& wso_device::CorneaIrLed::impl(void) const
{
	return *d_ptr;
}