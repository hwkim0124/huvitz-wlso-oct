#include "pch.h"
#include "RetMirrorMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct RetMirrorMotor::RetMirrorMotorImpl
{
	int32_t posMirrorIn;
	int32_t posMirrorOut;

	RetMirrorMotorImpl() : posMirrorIn(0), posMirrorOut(0)
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
	if (StepMotor::initializeStepMotor()) {
		loadCalibParamFromProfile();
		updatePositionToMirrorOut();
		return true;
	}
	return false;
}

bool wso_device::RetMirrorMotor::updatePositionToOrigin(int mode)
{
	if (mode == 0) {
		return updatePositionToMirrorIn();
	}
	else {
		return updatePositionToMirrorOut();
	}
}

bool wso_device::RetMirrorMotor::setCurrentPositionAsOrigin(int mode)
{
	if (mode == 0) {
		setCurrentPositionAsMirrorIn();
	}
	else {
		setCurrentPositionAsMirrorOut();
	}
	return true;
}

bool wso_device::RetMirrorMotor::updatePositionToMirrorIn(void)
{
	int pos = getPositionOfMirrorIn();
	return StepMotor::updatePosition(pos);
}

bool wso_device::RetMirrorMotor::updatePositionToMirrorOut(void)
{
	int pos = getPositionOfMirrorOut();
	return StepMotor::updatePosition(pos);
}

void wso_device::RetMirrorMotor::setCurrentPositionAsMirrorIn(void)
{
	int pos = getPosition();
	setPositionOfMirrorIn(pos);
	return;
}

void wso_device::RetMirrorMotor::setCurrentPositionAsMirrorOut(void)
{
	int pos = getPosition();
	setPositionOfMirrorOut(pos);
	return;
}

int wso_device::RetMirrorMotor::getPositionOfMirrorIn(void) const
{
	return impl().posMirrorIn;
}

int wso_device::RetMirrorMotor::getPositionOfMirrorOut(void) const
{
	return impl().posMirrorOut;
}

void wso_device::RetMirrorMotor::setPositionOfMirrorIn(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posMirrorIn = pos;
	return;
}

void wso_device::RetMirrorMotor::setPositionOfMirrorOut(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posMirrorOut = pos;
	return;
}

bool wso_device::RetMirrorMotor::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets(); p) {
		auto value1 = p->MotorCalPos.ReturnMirrorPos.InPos;
		auto value2 = p->MotorCalPos.ReturnMirrorPos.OutPos;
		setPositionOfMirrorIn(value1);
		setPositionOfMirrorOut(value2);
		LogD() << "Return mirror motor loaded from profile, in_pos: " << value1 << ", out_pos: " << value2;
		return true;
	}
	return false;
}

bool wso_device::RetMirrorMotor::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibMotorSets*>(getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets()); p) {
		auto value1 = getPositionOfMirrorIn();
		auto value2 = getPositionOfMirrorOut();
		p->MotorCalPos.ReturnMirrorPos.InPos = value1;
		p->MotorCalPos.ReturnMirrorPos.OutPos = value2;
		return true;
	}
	return false;
}


RetMirrorMotor::RetMirrorMotorImpl& wso_device::RetMirrorMotor::impl(void) const
{
	return *d_ptr;
}
