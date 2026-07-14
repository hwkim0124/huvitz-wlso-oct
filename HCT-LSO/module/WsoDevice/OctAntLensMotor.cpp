#include "pch.h"
#include "OctAntLensMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct OctAntLensMotor::OctAntLensMotorImpl
{
	int32_t posMirrorIn;
	int32_t posMirrorOut;

	OctAntLensMotorImpl() : posMirrorIn(0), posMirrorOut(0)
	{}
};


OctAntLensMotor::OctAntLensMotor() :
	d_ptr(make_unique<OctAntLensMotorImpl>()), StepMotor()
{}


wso_device::OctAntLensMotor::OctAntLensMotor(MainBoard* board) :
	d_ptr(make_unique<OctAntLensMotorImpl>()), StepMotor(board, StepMotorType::OCT_ANT_LENS)
{}


OctAntLensMotor::~OctAntLensMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctAntLensMotor::OctAntLensMotor::OctAntLensMotor(OctAntLensMotor&& rhs) = default;
OctAntLensMotor& OctAntLensMotor::OctAntLensMotor::operator=(OctAntLensMotor&& rhs) = default;


wso_device::OctAntLensMotor::OctAntLensMotor(const OctAntLensMotor& rhs)
	: d_ptr(make_unique<OctAntLensMotorImpl>(*rhs.d_ptr))
{}


OctAntLensMotor& wso_device::OctAntLensMotor::operator=(const OctAntLensMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::OctAntLensMotor::initializeOctAntLensMotor(void)
{
	if (StepMotor::initializeStepMotor()) {
		loadCalibParamFromProfile();
		updatePositionToMirrorOut();
		return true;
	}
	return false;
}


bool wso_device::OctAntLensMotor::updatePositionToOrigin(int mode)
{
	if (mode == 0) {
		return updatePositionToMirrorIn();
	}
	else {
		return updatePositionToMirrorOut();
	}
}

bool wso_device::OctAntLensMotor::setCurrentPositionAsOrigin(int mode)
{
	if (mode == 0) {
		setCurrentPositionAsMirrorIn();
	}
	else {
		setCurrentPositionAsMirrorOut();
	}
	return true;
}

bool wso_device::OctAntLensMotor::updatePositionToMirrorIn(void)
{
	int pos = getPositionOfMirrorIn();
	return StepMotor::updatePosition(pos);
}

bool wso_device::OctAntLensMotor::updatePositionToMirrorOut(void)
{
	int pos = getPositionOfMirrorOut();
	return StepMotor::updatePosition(pos);
}

void wso_device::OctAntLensMotor::setCurrentPositionAsMirrorIn(void)
{
	int pos = getPosition();
	setPositionOfMirrorIn(pos);
	return;
}

void wso_device::OctAntLensMotor::setCurrentPositionAsMirrorOut(void)
{
	int pos = getPosition();
	setPositionOfMirrorOut(pos);
	return;
}

int wso_device::OctAntLensMotor::getPositionOfMirrorIn(void) const
{
	return impl().posMirrorIn;
}

int wso_device::OctAntLensMotor::getPositionOfMirrorOut(void) const
{
	return impl().posMirrorOut;
}

void wso_device::OctAntLensMotor::setPositionOfMirrorIn(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posMirrorIn = pos;
	return;
}

void wso_device::OctAntLensMotor::setPositionOfMirrorOut(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	impl().posMirrorOut = pos;
	return;
}

bool wso_device::OctAntLensMotor::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets(); p) {
		auto value1 = p->MotorCalPos.OCT_AntLensPos.InPos;
		auto value2 = p->MotorCalPos.OCT_AntLensPos.OutPos;
		setPositionOfMirrorIn(value1);
		setPositionOfMirrorOut(value2);
		LogD() << "OCT anterior lens motor loaded from profile, in_pos: " << value1 << ", out_pos: " << value2; 
		return true;
	}
	return false;
}

bool wso_device::OctAntLensMotor::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibMotorSets*>(getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets()); p) {
		auto value1 = getPositionOfMirrorIn();
		auto value2 = getPositionOfMirrorOut();
		p->MotorCalPos.OCT_AntLensPos.InPos = value1;
		p->MotorCalPos.OCT_AntLensPos.OutPos = value2;
		return true;
	}
	return false;
}


OctAntLensMotor::OctAntLensMotorImpl& wso_device::OctAntLensMotor::impl(void) const
{
	return *d_ptr;
}
