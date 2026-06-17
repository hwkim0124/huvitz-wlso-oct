#include "pch.h"
#include "ZstageMotor.h"

#include "MainBoard.h"
#include "UsbComm.h"


using namespace wso_device;
using namespace std;



struct ZstageMotor::ZstageMotorImpl
{
	ZstageMotorImpl()
	{
	}
};


ZstageMotor::ZstageMotor() :
	d_ptr(make_unique<ZstageMotorImpl>()), StageMotor()
{
}


wso_device::ZstageMotor::ZstageMotor(MainBoard* board) :
	d_ptr(make_unique<ZstageMotorImpl>()), StageMotor(board, StageMotorType::STAGE_Z)
{
	setLimitRange(STAGE_Z_LIMIT_RANGE_LOW, STAGE_Z_LIMIT_RANGE_HIGH);
}


ZstageMotor::~ZstageMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
ZstageMotor::ZstageMotor::ZstageMotor(ZstageMotor&& rhs) = default;
ZstageMotor& ZstageMotor::ZstageMotor::operator=(ZstageMotor&& rhs) = default;


wso_device::ZstageMotor::ZstageMotor(const ZstageMotor& rhs)
	: d_ptr(make_unique<ZstageMotorImpl>(*rhs.d_ptr))
{
}


ZstageMotor& wso_device::ZstageMotor::operator=(const ZstageMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


ZstageMotor::ZstageMotorImpl& wso_device::ZstageMotor::impl(void) const
{
	return *d_ptr;
}
