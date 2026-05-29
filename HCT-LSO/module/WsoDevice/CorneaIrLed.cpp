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
		//loadCalibParamFromProfile();
		//lightOff();
		return true;
	}
	return false;
}


CorneaIrLed::CorneaIrLedImpl& wso_device::CorneaIrLed::impl(void) const
{
	return *d_ptr;
}