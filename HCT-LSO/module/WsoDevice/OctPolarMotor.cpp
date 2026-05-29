#include "pch.h"
#include "OctPolarMotor.h"

#include "MainBoard.h"
#include "UsbComm.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct OctPolarMotor::OctPolarMotorImpl
{
	int32_t posOfOrigin;
	int32_t posPerDegree;


	OctPolarMotorImpl() : posOfOrigin(0), posPerDegree(0)
	{

	}
};


OctPolarMotor::OctPolarMotor() :
	d_ptr(make_unique<OctPolarMotorImpl>()), StepMotor()
{
}


wso_device::OctPolarMotor::OctPolarMotor(MainBoard* board) :
	d_ptr(make_unique<OctPolarMotorImpl>()), StepMotor(board, StepMotorType::OCT_POLAR)
{
}


OctPolarMotor::~OctPolarMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctPolarMotor::OctPolarMotor::OctPolarMotor(OctPolarMotor&& rhs) = default;
OctPolarMotor& OctPolarMotor::OctPolarMotor::operator=(OctPolarMotor&& rhs) = default;


wso_device::OctPolarMotor::OctPolarMotor(const OctPolarMotor& rhs)
	: d_ptr(make_unique<OctPolarMotorImpl>(*rhs.d_ptr))
{
}


OctPolarMotor& wso_device::OctPolarMotor::operator=(const OctPolarMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}



bool wso_device::OctPolarMotor::initializeOctPolarMotor(bool ready)
{
	if (StepMotor::initializeStepMotor()) {
		if (ready) {
			setPositionsPerDegree(MOTOR_OCT_POLAR_STEPS_PER_DEGREE);
			loadParamsFromProfile();
			updatePositionToZeroDegree();
		}
		return true;
	}
	return false;
}

bool wso_device::OctPolarMotor::updatePositionByValue(float value)
{
	return updatePositionByDegree((int)value);
}


bool wso_device::OctPolarMotor::updatePositionByValueOffset(float offset)
{
	float degree = getCurrentDegree() + offset;
	return updatePositionByDegree((int)degree);
}


bool wso_device::OctPolarMotor::updatePositionToOrigin(int mode)
{
	return updatePositionToZeroDegree();
}

bool wso_device::OctPolarMotor::setCurrentPositionAsOrigin(int mode)
{
	setCurrentPositionAsZeroDegree();
	return true;
}

float wso_device::OctPolarMotor::getCurrentValueByPosition(void) const
{
	return getCurrentDegree();
}

int wso_device::OctPolarMotor::getPositionAtValue(float value) const
{
	int pos = convertDegreeToPosition(value);
	return pos;
}

float wso_device::OctPolarMotor::getValueAtPosition(int pos) const
{
	float value = convertPositionToDegree(pos);
	return value;
}


bool wso_device::OctPolarMotor::updatePositionByDegree(int degree)
{
	int pos = convertDegreeToPosition((float)degree);
	return updatePosition(pos);
}

bool wso_device::OctPolarMotor::updatePositionByDegreeOffset(int offset)
{
	return updatePositionByValueOffset((float)offset);
}


int wso_device::OctPolarMotor::getPositionOfZeroDegree(void) const
{
	return impl().posOfOrigin;
}


void wso_device::OctPolarMotor::setPositionOfZeroDegree(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posOfOrigin = pos;
	return;
}


void wso_device::OctPolarMotor::setCurrentPositionAsZeroDegree(void)
{
	int pos = getPosition();
	setPositionOfZeroDegree(pos);
	return;
}


float wso_device::OctPolarMotor::getCurrentDegree(void) const
{
	float diopt = (float)(getPosition() - getPositionOfZeroDegree()) / (float)getPositionsPerDegree();
	diopt *= getDegreeDirection();
	return diopt;
}


int wso_device::OctPolarMotor::getPositionsPerDegree(void) const
{
	return impl().posPerDegree;
}


void wso_device::OctPolarMotor::setPositionsPerDegree(int pos)
{
	impl().posPerDegree = pos;
	return;
}


bool wso_device::OctPolarMotor::updatePositionToZeroDegree(void)
{
	return updatePositionByDegree((int)0.0f);
}


int wso_device::OctPolarMotor::convertDegreeToPosition(float diopt) const
{
	int pos = (int)(diopt * getPositionsPerDegree() * getDegreeDirection()) + getPositionOfZeroDegree();
	return pos;
}


float wso_device::OctPolarMotor::convertPositionToDegree(int pos) const
{
	float diopt = (float)(pos - getPositionOfZeroDegree()) / (float)getPositionsPerDegree();
	diopt *= getDegreeDirection();
	return diopt;
}


int wso_device::OctPolarMotor::getDegreeDirection(void) const
{
	return 1;
}


void wso_device::OctPolarMotor::loadParamsFromProfile(void)
{
	auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibration();
	setPositionOfZeroDegree(p->PolarizationPos);
	return;
}


void wso_device::OctPolarMotor::saveParamsToProfile(void)
{
	auto p = const_cast<HbsCalibration*>(getMainBoard()->getHbsDataProfile()->getHbsCalibration());
	p->PolarizationPos = getPositionOfZeroDegree();
	return;
}


OctPolarMotor::OctPolarMotorImpl& wso_device::OctPolarMotor::impl(void) const
{
	return *d_ptr;
}
