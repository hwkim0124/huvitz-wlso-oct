#include "pch.h"
#include "LsoFilterMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct LsoFilterMotor::LsoFilterMotorImpl
{
	int32_t posMirrorIn;
	int32_t posMirrorOut;

	LsoFilterMotorImpl() : posMirrorIn(0), posMirrorOut(0)
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
	if (StepMotor::initializeStepMotor()) {
		loadCalibParamFromProfile();
		updatePositionToMirrorOut();
		return true;
	}
	return false;
}


bool wso_device::LsoFilterMotor::updatePositionToOrigin(int mode)
{
	if (mode == 0) {
		return updatePositionToMirrorIn();
	}
	else {
		return updatePositionToMirrorOut();
	}
}

bool wso_device::LsoFilterMotor::setCurrentPositionAsOrigin(int mode)
{
	if (mode == 0) {
		setCurrentPositionAsMirrorIn();
	}
	else {
		setCurrentPositionAsMirrorOut();
	}
	return true;
}

bool wso_device::LsoFilterMotor::updatePositionToMirrorIn(void)
{
	int pos = getPositionOfMirrorIn();
	return StepMotor::updatePosition(pos);
}

bool wso_device::LsoFilterMotor::updatePositionToMirrorOut(void)
{
	int pos = getPositionOfMirrorOut();
	return StepMotor::updatePosition(pos);
}

void wso_device::LsoFilterMotor::setCurrentPositionAsMirrorIn(void)
{
	int pos = getPosition();
	setPositionOfMirrorIn(pos);
	return;
}

void wso_device::LsoFilterMotor::setCurrentPositionAsMirrorOut(void)
{
	int pos = getPosition();
	setPositionOfMirrorOut(pos);
	return;
}

int wso_device::LsoFilterMotor::getPositionOfMirrorIn(void) const
{
	return impl().posMirrorIn;
}

int wso_device::LsoFilterMotor::getPositionOfMirrorOut(void) const
{
	return impl().posMirrorOut;
}

void wso_device::LsoFilterMotor::setPositionOfMirrorIn(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posMirrorIn = pos;
	return;
}

void wso_device::LsoFilterMotor::setPositionOfMirrorOut(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posMirrorOut = pos;
	return;
}

bool wso_device::LsoFilterMotor::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets(); p) {
		/*
		auto value1 = p->MotorCalPos.ReturnMirrorPos.InPos;
		auto value2 = p->MotorCalPos.ReturnMirrorPos.OutPos;
		setPositionOfMirrorIn(value1);
		setPositionOfMirrorOut(value2);
		*/
		return true;
	}
	return false;
}

bool wso_device::LsoFilterMotor::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibMotorSets*>(getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets()); p) {
		/*
		auto value1 = getPositionOfMirrorIn();
		auto value2 = getPositionOfMirrorOut();
		p->MotorCalPos.ReturnMirrorPos.InPos = value1;
		p->MotorCalPos.ReturnMirrorPos.OutPos = value2;
		*/
		return true;
	}
	return false;
}


LsoFilterMotor::LsoFilterMotorImpl& wso_device::LsoFilterMotor::impl(void) const
{
	return *d_ptr;
}
