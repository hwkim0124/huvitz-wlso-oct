#include "pch.h"
#include "ChinRestMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct ChinRestMotor::ChinRestMotorImpl
{
	ChinRestMotorImpl()
	{}
};


ChinRestMotor::ChinRestMotor() :
	d_ptr(make_unique<ChinRestMotorImpl>()), StepMotor()
{}


wso_device::ChinRestMotor::ChinRestMotor(MainBoard* board) :
	d_ptr(make_unique<ChinRestMotorImpl>()), StepMotor(board, StepMotorType::CHIN_REST)
{}


ChinRestMotor::~ChinRestMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
ChinRestMotor::ChinRestMotor::ChinRestMotor(ChinRestMotor&& rhs) = default;
ChinRestMotor& ChinRestMotor::ChinRestMotor::operator=(ChinRestMotor&& rhs) = default;


wso_device::ChinRestMotor::ChinRestMotor(const ChinRestMotor& rhs)
	: d_ptr(make_unique<ChinRestMotorImpl>(*rhs.d_ptr))
{}


ChinRestMotor& wso_device::ChinRestMotor::operator=(const ChinRestMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::ChinRestMotor::initializeChinRestMotor(void)
{
	return StepMotor::initializeStepMotor();
}


ChinRestMotor::ChinRestMotorImpl& wso_device::ChinRestMotor::impl(void) const
{
	return *d_ptr;
}
