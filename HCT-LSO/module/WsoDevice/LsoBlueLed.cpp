#include "pch.h"
#include "LsoBlueLed.h"

#include "MainBoard.h"

using namespace wso_device;
using namespace std;

struct LsoBlueLed::LsoBlueLedImpl
{
	LsoBlueLedImpl()
	{}
};

LsoBlueLed::LsoBlueLed() :
	d_ptr(make_unique<LsoBlueLedImpl>()), LightLed()
{}


wso_device::LsoBlueLed::LsoBlueLed(MainBoard* board) :
	d_ptr(make_unique<LsoBlueLedImpl>()), LightLed(board, LightType::LSO_BLUE_LED)
{}


LsoBlueLed::~LsoBlueLed()
{

}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
LsoBlueLed::LsoBlueLed::LsoBlueLed(LsoBlueLed&& rhs) = default;
LsoBlueLed& LsoBlueLed::LsoBlueLed::operator=(LsoBlueLed&& rhs) = default;


wso_device::LsoBlueLed::LsoBlueLed(const LsoBlueLed& rhs)
	: d_ptr(make_unique<LsoBlueLedImpl>(*rhs.d_ptr))
{}


LsoBlueLed& wso_device::LsoBlueLed::operator=(const LsoBlueLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::LsoBlueLed::initializeLsoBlueLed(void)
{
	if (LightLed::initializeLightLed()) {
		loadCalibParamFromProfile();
		turnLightOff();
		return true;
	}
	return false;
}

bool wso_device::LsoBlueLed::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource(); p) {
		auto value = p->LED_Info.Bled_intensity;
		setIntensity(value);
		LogD() << "LsoBlueLed intensity loaded from profile: " << value;
		return true;
	}
	return false;
}

bool wso_device::LsoBlueLed::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibLedSource*>(getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource()); p) {
		auto value = getIntensity();
		p->LED_Info.Bled_intensity = value;
		return true;
	}
	return false;
}


LsoBlueLed::LsoBlueLedImpl& wso_device::LsoBlueLed::impl(void) const
{
	// TODO: insert return statement here
	return *d_ptr;
}