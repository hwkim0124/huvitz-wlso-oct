#include "pch.h"
#include "LsoFilterMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct LsoFilterMotor::LsoFilterMotorImpl
{
	LsoFilterMotorImpl()
	{}
};


LsoFilterMotor::LsoFilterMotor() :
	d_ptr(make_unique<LsoFilterMotorImpl>()), StepMotor()
{}


wso_device::LsoFilterMotor::LsoFilterMotor(MainBoard* board) :
	d_ptr(make_unique<LsoFilterMotorImpl>()), StepMotor(board, StepMotorType::LSO_FILTER)
{}


LsoFilterMotor::~LsoFilterMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
LsoFilterMotor::LsoFilterMotor::LsoFilterMotor(LsoFilterMotor&& rhs) = default;
LsoFilterMotor& LsoFilterMotor::LsoFilterMotor::operator=(LsoFilterMotor&& rhs) = default;


wso_device::LsoFilterMotor::LsoFilterMotor(const LsoFilterMotor& rhs)
	: d_ptr(make_unique<LsoFilterMotorImpl>(*rhs.d_ptr))
{}


LsoFilterMotor& wso_device::LsoFilterMotor::operator=(const LsoFilterMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::LsoFilterMotor::initializeLsoFilterMotor(void)
{
	return StepMotor::initializeStepMotor();
}


LsoFilterMotor::LsoFilterMotorImpl& wso_device::LsoFilterMotor::impl(void) const
{
	return *d_ptr;
}
