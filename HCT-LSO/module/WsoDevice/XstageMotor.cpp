#include "pch.h"
#include "XstageMotor.h"

#include "MainBoard.h"
#include "UsbComm.h"


using namespace wso_device;
using namespace std;



struct XstageMotor::XstageMotorImpl
{
	XstageMotorImpl()
	{
	}
};


XstageMotor::XstageMotor() :
	d_ptr(make_unique<XstageMotorImpl>()), StageMotor()
{
}


wso_device::XstageMotor::XstageMotor(MainBoard* board) :
	d_ptr(make_unique<XstageMotorImpl>()), StageMotor(board, StageMotorType::STAGE_Y)
{
	setLimitRange(STAGE_Y_LIMIT_RANGE_LOW, STAGE_Y_LIMIT_RANGE_HIGH);
}


XstageMotor::~XstageMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
XstageMotor::XstageMotor::XstageMotor(XstageMotor&& rhs) = default;
XstageMotor& XstageMotor::XstageMotor::operator=(XstageMotor&& rhs) = default;


wso_device::XstageMotor::XstageMotor(const XstageMotor& rhs)
	: d_ptr(make_unique<XstageMotorImpl>(*rhs.d_ptr))
{
}


XstageMotor& wso_device::XstageMotor::operator=(const XstageMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


XstageMotor::XstageMotorImpl& wso_device::XstageMotor::impl(void) const
{
	return *d_ptr;
}
