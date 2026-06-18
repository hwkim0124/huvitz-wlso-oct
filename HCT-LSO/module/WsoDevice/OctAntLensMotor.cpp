#include "pch.h"
#include "OctAntLensMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct OctAntLensMotor::OctAntLensMotorImpl
{
	OctAntLensMotorImpl()
	{}
};


OctAntLensMotor::OctAntLensMotor() :
	d_ptr(make_unique<OctAntLensMotorImpl>()), StepMotor()
{}


wso_device::OctAntLensMotor::OctAntLensMotor(MainBoard* board) :
	d_ptr(make_unique<OctAntLensMotorImpl>()), StepMotor(board, StepMotorType::OCT_ANT_LENS)
{}


OctAntLensMotor::~OctAntLensMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctAntLensMotor::OctAntLensMotor::OctAntLensMotor(OctAntLensMotor&& rhs) = default;
OctAntLensMotor& OctAntLensMotor::OctAntLensMotor::operator=(OctAntLensMotor&& rhs) = default;


wso_device::OctAntLensMotor::OctAntLensMotor(const OctAntLensMotor& rhs)
	: d_ptr(make_unique<OctAntLensMotorImpl>(*rhs.d_ptr))
{}


OctAntLensMotor& wso_device::OctAntLensMotor::operator=(const OctAntLensMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::OctAntLensMotor::initializeOctAntLensMotor(void)
{
	return StepMotor::initializeStepMotor();
}


OctAntLensMotor::OctAntLensMotorImpl& wso_device::OctAntLensMotor::impl(void) const
{
	return *d_ptr;
}
