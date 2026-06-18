#include "pch.h"
#include "OctRefNdMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct OctRefNdMotor::OctRefNdMotorImpl
{
	OctRefNdMotorImpl()
	{}
};


OctRefNdMotor::OctRefNdMotor() :
	d_ptr(make_unique<OctRefNdMotorImpl>()), StepMotor()
{}


wso_device::OctRefNdMotor::OctRefNdMotor(MainBoard* board) :
	d_ptr(make_unique<OctRefNdMotorImpl>()), StepMotor(board, StepMotorType::OCT_REFND)
{}


OctRefNdMotor::~OctRefNdMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctRefNdMotor::OctRefNdMotor::OctRefNdMotor(OctRefNdMotor&& rhs) = default;
OctRefNdMotor& OctRefNdMotor::OctRefNdMotor::operator=(OctRefNdMotor&& rhs) = default;


wso_device::OctRefNdMotor::OctRefNdMotor(const OctRefNdMotor& rhs)
	: d_ptr(make_unique<OctRefNdMotorImpl>(*rhs.d_ptr))
{}


OctRefNdMotor& wso_device::OctRefNdMotor::operator=(const OctRefNdMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::OctRefNdMotor::initializeOctRefNdMotor(void)
{
	return StepMotor::initializeStepMotor();
}


OctRefNdMotor::OctRefNdMotorImpl& wso_device::OctRefNdMotor::impl(void) const
{
	return *d_ptr;
}
