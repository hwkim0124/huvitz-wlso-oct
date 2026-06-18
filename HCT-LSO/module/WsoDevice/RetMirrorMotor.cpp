#include "pch.h"
#include "RetMirrorMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct RetMirrorMotor::RetMirrorMotorImpl
{
	RetMirrorMotorImpl()
	{}
};


RetMirrorMotor::RetMirrorMotor() :
	d_ptr(make_unique<RetMirrorMotorImpl>()), StepMotor()
{}


wso_device::RetMirrorMotor::RetMirrorMotor(MainBoard* board) :
	d_ptr(make_unique<RetMirrorMotorImpl>()), StepMotor(board, StepMotorType::RET_MIRROR)
{}


RetMirrorMotor::~RetMirrorMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
RetMirrorMotor::RetMirrorMotor::RetMirrorMotor(RetMirrorMotor&& rhs) = default;
RetMirrorMotor& RetMirrorMotor::RetMirrorMotor::operator=(RetMirrorMotor&& rhs) = default;


wso_device::RetMirrorMotor::RetMirrorMotor(const RetMirrorMotor& rhs)
	: d_ptr(make_unique<RetMirrorMotorImpl>(*rhs.d_ptr))
{}


RetMirrorMotor& wso_device::RetMirrorMotor::operator=(const RetMirrorMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::RetMirrorMotor::initializeRetMirrorMotor(void)
{
	return StepMotor::initializeStepMotor();
}


RetMirrorMotor::RetMirrorMotorImpl& wso_device::RetMirrorMotor::impl(void) const
{
	return *d_ptr;
}
