#include "pch.h"
#include "YstageMotor.h"

#include "MainBoard.h"
#include "UsbComm.h"


using namespace wso_device;
using namespace std;



struct YstageMotor::YstageMotorImpl
{
	YstageMotorImpl()
	{
	}
};


YstageMotor::YstageMotor() :
	d_ptr(make_unique<YstageMotorImpl>()), StageMotor()
{
}


wso_device::YstageMotor::YstageMotor(MainBoard* board) :
	d_ptr(make_unique<YstageMotorImpl>()), StageMotor(board, StageMotorType::STAGE_Y)
{
	setLimitRange(STAGE_Y_LIMIT_RANGE_LOW, STAGE_Y_LIMIT_RANGE_HIGH);
}


YstageMotor::~YstageMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
YstageMotor::YstageMotor::YstageMotor(YstageMotor&& rhs) = default;
YstageMotor& YstageMotor::YstageMotor::operator=(YstageMotor&& rhs) = default;


wso_device::YstageMotor::YstageMotor(const YstageMotor& rhs)
	: d_ptr(make_unique<YstageMotorImpl>(*rhs.d_ptr))
{
}


YstageMotor& wso_device::YstageMotor::operator=(const YstageMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


YstageMotor::YstageMotorImpl& wso_device::YstageMotor::impl(void) const
{
	return *d_ptr;
}
