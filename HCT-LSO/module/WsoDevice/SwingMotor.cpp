#include "pch.h"
#include "SwingMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct SwingMotor::SwingMotorImpl
{
	SwingMotorImpl()
	{
	}
};


SwingMotor::SwingMotor() :
	d_ptr(make_unique<SwingMotorImpl>()), StepMotor()
{
}


wso_device::SwingMotor::SwingMotor(MainBoard* board) :
	d_ptr(make_unique<SwingMotorImpl>()), StepMotor(board, StepMotorType::SWING)
{
}


SwingMotor::~SwingMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
SwingMotor::SwingMotor::SwingMotor(SwingMotor&& rhs) = default;
SwingMotor& SwingMotor::SwingMotor::operator=(SwingMotor&& rhs) = default;


wso_device::SwingMotor::SwingMotor(const SwingMotor& rhs)
	: d_ptr(make_unique<SwingMotorImpl>(*rhs.d_ptr))
{
}


SwingMotor& wso_device::SwingMotor::operator=(const SwingMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::SwingMotor::initializeSwingMotor(void)
{
	return StepMotor::initializeStepMotor();
}


SwingMotor::SwingMotorImpl& wso_device::SwingMotor::impl(void) const
{
	return *d_ptr;
}
