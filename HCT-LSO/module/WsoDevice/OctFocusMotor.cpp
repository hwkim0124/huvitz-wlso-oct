#include "pch.h"
#include "OctFocusMotor.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;
using namespace std;



struct OctFocusMotor::OctFocusMotorImpl
{
	int32_t posOfOrigin;
	int32_t posPerDiopt;

	CEdit* pEditDiopt;

	OctFocusMotorImpl() : posOfOrigin(0), posPerDiopt(0), pEditDiopt(NULL)
	{

	}
};


OctFocusMotor::OctFocusMotor() :
	d_ptr(make_unique<OctFocusMotorImpl>()), StepMotor()
{
}


wso_device::OctFocusMotor::OctFocusMotor(MainBoard* board) :
	d_ptr(make_unique<OctFocusMotorImpl>()), StepMotor(board, StepMotorType::OCT_FOCUS)
{
}


OctFocusMotor::~OctFocusMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctFocusMotor::OctFocusMotor::OctFocusMotor(OctFocusMotor&& rhs) = default;
OctFocusMotor& OctFocusMotor::OctFocusMotor::operator=(OctFocusMotor&& rhs) = default;


wso_device::OctFocusMotor::OctFocusMotor(const OctFocusMotor& rhs)
	: d_ptr(make_unique<OctFocusMotorImpl>(*rhs.d_ptr))
{
}


OctFocusMotor& wso_device::OctFocusMotor::operator=(const OctFocusMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::OctFocusMotor::initializeOctFocusMotor(bool ready)
{
	if (StepMotor::initializeStepMotor()) {
		if (ready) {
			setPositionsPerDiopter(MOTOR_OCT_FOCUS_STEPS_PER_DIOPTER);
			loadCalibParamFromProfile();
			updatePositionToZeroDiopter();
		}
		return true;
	}
	return false;
}


bool wso_device::OctFocusMotor::updatePositionByValue(float value)
{
	return updatePositionByDiopter(value);
}


bool wso_device::OctFocusMotor::updatePositionByValueOffset(float offset)
{
	float diopt = getCurrentDiopter() + offset;
	return updatePositionByDiopter(diopt);
}

bool wso_device::OctFocusMotor::updatePositionToOrigin(int mode)
{
	return updatePositionToZeroDiopter();
}


bool wso_device::OctFocusMotor::setCurrentPositionAsOrigin(int mode)
{
	setCurrentPositionAsZeroDiopter();
	return true;
}

float wso_device::OctFocusMotor::getCurrentValueByPosition(void) const
{
	return getCurrentDiopter();
}


int wso_device::OctFocusMotor::getZeroDiopterPosition(void) const
{
	return impl().posOfOrigin;
}


void wso_device::OctFocusMotor::setZeroDiopterPosition(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);

	impl().posOfOrigin = pos;
	return;
}

int wso_device::OctFocusMotor::getPositionAtValue(float value) const
{
	int pos = convertDiopterToPosition(value);
	return pos;
}

float wso_device::OctFocusMotor::getValueAtPosition(int pos) const
{
	float value = convertPositionToDiopter(pos);
	return value;
}


void wso_device::OctFocusMotor::setCurrentPositionAsZeroDiopter(void)
{
	int pos = getPosition();
	setZeroDiopterPosition(pos);
	return;
}


float wso_device::OctFocusMotor::getCurrentDiopter(void) const
{
	int currPos = getPosition();
	int zeroPos = getZeroDiopterPosition();
	int posPerDiopt = getPositionsPerDiopter();

	float diopt = (float)(currPos - zeroPos) / (float)posPerDiopt;
	diopt *= getDiopterDirection();
	return diopt;
}


int wso_device::OctFocusMotor::getPositionsPerDiopter(void) const
{
	return impl().posPerDiopt;
}


void wso_device::OctFocusMotor::setPositionsPerDiopter(int pos)
{
	impl().posPerDiopt = pos;
	return;
}


bool wso_device::OctFocusMotor::updatePositionByDiopterOffset(float offset)
{
	float diopt = getCurrentDiopter() + offset;
	return updatePositionByDiopter(diopt);
}


bool wso_device::OctFocusMotor::updatePositionByDiopter(float diopt)
{
	int pos = convertDiopterToPosition(diopt);
	return updatePosition(pos);
}


bool wso_device::OctFocusMotor::updatePositionToZeroDiopter(void)
{
	return updatePositionByDiopter(0.0f);
}


int wso_device::OctFocusMotor::convertDiopterToPosition(float diopt) const
{
	int zeroPos = getZeroDiopterPosition();
	int posPerDiopt = getPositionsPerDiopter();
	int pos = (int)(diopt * posPerDiopt * getDiopterDirection()) + zeroPos;
	return pos;
}


float wso_device::OctFocusMotor::convertPositionToDiopter(int pos) const
{
	int zeroPos = getZeroDiopterPosition();
	int posPerDiopt = getPositionsPerDiopter();

	float diopt = (float)(pos - zeroPos) / (float)posPerDiopt;
	diopt *= getDiopterDirection();
	return diopt;
}


int wso_device::OctFocusMotor::getDiopterDirection(void) const
{
	return +1; // -1;
}


bool wso_device::OctFocusMotor::isEndOfPlusDiopter(void) const
{
	// return (getCurrentDiopter() >= MOTOR_OCT_FOCUS_DIOPTER_MAX ? true : false) ;
	// return isEndOfUpperPosition();
	return isEndOfLowerPosition();
}


bool wso_device::OctFocusMotor::isEndOfMinusDiopter(void) const
{
	// return (getCurrentDiopter() <= MOTOR_OCT_FOCUS_DIOPTER_MIN ? true : false) ;
	// return isEndOfLowerPosition();
	return isEndOfUpperPosition();
}

bool wso_device::OctFocusMotor::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets(); p) {
		auto value = p->MotorCalPos.oct_focus_motor_0D_pos;
		setZeroDiopterPosition(value);
		LogD() << "OctFocusMotor loaded from profile, zero-diopt pos: " << value; 
		return true;
	}
	return false;
}

bool wso_device::OctFocusMotor::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibMotorSets*>(getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets()); p) {
		auto value = getZeroDiopterPosition();
		p->MotorCalPos.oct_focus_motor_0D_pos = value;
		return true;
	}
	return false;
}

OctFocusMotor::OctFocusMotorImpl& wso_device::OctFocusMotor::impl(void) const
{
	return *d_ptr;
}
