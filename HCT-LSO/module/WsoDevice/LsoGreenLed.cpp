#include "pch.h"
#include "LsoGreenLed.h"

#include "MainBoard.h"

using namespace wso_device;
using namespace std;

struct LsoGreenLed::LsoGreenLedImpl
{
	LsoGreenLedImpl()
	{}
};

LsoGreenLed::LsoGreenLed() :
	d_ptr(make_unique<LsoGreenLedImpl>()), LightLed()
{}


wso_device::LsoGreenLed::LsoGreenLed(MainBoard* board) :
	d_ptr(make_unique<LsoGreenLedImpl>()), LightLed(board, LightType::LSO_GREEN_LED)
{}


LsoGreenLed::~LsoGreenLed()
{

}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
LsoGreenLed::LsoGreenLed::LsoGreenLed(LsoGreenLed&& rhs) = default;
LsoGreenLed& LsoGreenLed::LsoGreenLed::operator=(LsoGreenLed&& rhs) = default;


wso_device::LsoGreenLed::LsoGreenLed(const LsoGreenLed& rhs)
	: d_ptr(make_unique<LsoGreenLedImpl>(*rhs.d_ptr))
{}


LsoGreenLed& wso_device::LsoGreenLed::operator=(const LsoGreenLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::LsoGreenLed::initializeLsoGreenLed(void)
{
	if (LightLed::initializeLightLed()) {
		loadCalibParamFromProfile();
		turnLightOff();
		return true;
	}
	return false;
}

bool wso_device::LsoGreenLed::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource(); p) {
		auto value = p->LED_Info.Gled_inentity;
		setIntensity(value);
		LogD() << "LsoGreenLed intensity loaded from profile: " << value;
		return true;
	}
	return false;
}

bool wso_device::LsoGreenLed::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibLedSource*>(getMainBoard()->getHbsDataProfile()->getHbsCalibLedSource()); p) {
		auto value = getIntensity();
		p->LED_Info.Gled_inentity = value;
		return true;
	}
	return false;
}


LsoGreenLed::LsoGreenLedImpl& wso_device::LsoGreenLed::impl(void) const
{
	// TODO: insert return statement here
	return *d_ptr;
}