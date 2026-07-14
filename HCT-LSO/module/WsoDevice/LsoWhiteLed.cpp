#include "pch.h"
#include "LsoWhiteLed.h"

#include "MainBoard.h"

using namespace wso_device;
using namespace std;

struct LsoWhiteLed::LsoWhiteLedImpl
{
	LsoWhiteLedImpl()
	{
	}
};

LsoWhiteLed::LsoWhiteLed() :
	d_ptr(make_unique<LsoWhiteLedImpl>()), LightLed()
{
}


wso_device::LsoWhiteLed::LsoWhiteLed(MainBoard* board) :
	d_ptr(make_unique<LsoWhiteLedImpl>()), LightLed(board, LightType::LSO_WHITE_LED)
{
}


LsoWhiteLed::~LsoWhiteLed()
{

}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
LsoWhiteLed::LsoWhiteLed::LsoWhiteLed(LsoWhiteLed&& rhs) = default;
LsoWhiteLed& LsoWhiteLed::LsoWhiteLed::operator=(LsoWhiteLed&& rhs) = default;


wso_device::LsoWhiteLed::LsoWhiteLed(const LsoWhiteLed& rhs)
	: d_ptr(make_unique<LsoWhiteLedImpl>(*rhs.d_ptr))
{
}


LsoWhiteLed& wso_device::LsoWhiteLed::operator=(const LsoWhiteLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::LsoWhiteLed::initializeLsoWhiteLed(void)
{
	if (LightLed::initializeLightLed()) {
		loadCalibParamFromProfile();
		turnLightOff();
		return true;
	}
	return false;
}

bool wso_device::LsoWhiteLed::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource(); p) {
		auto value = p->LED_Info.WLED_intensity;
		setIntensity(value);
		LogD() << "LsoWhiteLed intensity loaded from profile: " << value;
		return true;
	}
	return false;
}

bool wso_device::LsoWhiteLed::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibLedSource*>(getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource()); p) {
		auto value = getIntensity();
		p->LED_Info.WLED_intensity = value;
		return true;
	}
	return false;
}


LsoWhiteLed::LsoWhiteLedImpl& wso_device::LsoWhiteLed::impl(void) const
{
	// TODO: insert return statement here
	return *d_ptr;
}