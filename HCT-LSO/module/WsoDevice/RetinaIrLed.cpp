#include "pch.h"
#include "RetinaIrLed.h"

#include "MainBoard.h"

using namespace wso_device;
using namespace std;

struct RetinaIrLed::RetinaIrLedImpl
{
	RetinaIrLedImpl()
	{
	}
};

RetinaIrLed::RetinaIrLed() :
	d_ptr(make_unique<RetinaIrLedImpl>()), LightLed()
{
}


wso_device::RetinaIrLed::RetinaIrLed(MainBoard* board) :
	d_ptr(make_unique<RetinaIrLedImpl>()), LightLed(board, LightType::RETINA_IR_LED)
{
}


RetinaIrLed::~RetinaIrLed()
{

}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
RetinaIrLed::RetinaIrLed::RetinaIrLed(RetinaIrLed&& rhs) = default;
RetinaIrLed& RetinaIrLed::RetinaIrLed::operator=(RetinaIrLed&& rhs) = default;


wso_device::RetinaIrLed::RetinaIrLed(const RetinaIrLed& rhs)
	: d_ptr(make_unique<RetinaIrLedImpl>(*rhs.d_ptr))
{
}


RetinaIrLed& wso_device::RetinaIrLed::operator=(const RetinaIrLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::RetinaIrLed::initializeRetinaIrLed(void)
{
	if (LightLed::initializeLightLed()) {
		//loadCalibParamFromProfile();
		//lightOff();
		return true;
	}
	return false;
}


RetinaIrLed::RetinaIrLedImpl& wso_device::RetinaIrLed::impl(void) const
{
	// TODO: insert return statement here
	return *d_ptr;
}