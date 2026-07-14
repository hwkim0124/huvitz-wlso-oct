#include "pch.h"
#include "OctReferMotor.h"

#include "MainBoard.h"
#include "UsbComm.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct OctReferMotor::OctReferMotorImpl
{
	int32_t posOfOrigin;
	int32_t posOfOrigin2;

	OctReferMotorImpl() : posOfOrigin(0), posOfOrigin2(0)
	{
	}
};


OctReferMotor::OctReferMotor() :
	d_ptr(make_unique<OctReferMotorImpl>()), StepMotor()
{
}


wso_device::OctReferMotor::OctReferMotor(MainBoard* board) :
	d_ptr(make_unique<OctReferMotorImpl>()), StepMotor(board, StepMotorType::OCT_REFER)
{
}


OctReferMotor::~OctReferMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
OctReferMotor::OctReferMotor::OctReferMotor(OctReferMotor&& rhs) = default;
OctReferMotor& OctReferMotor::OctReferMotor::operator=(OctReferMotor&& rhs) = default;


wso_device::OctReferMotor::OctReferMotor(const OctReferMotor& rhs)
	: d_ptr(make_unique<OctReferMotorImpl>(*rhs.d_ptr))
{
}


OctReferMotor& wso_device::OctReferMotor::operator=(const OctReferMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::OctReferMotor::initializeOctReferMotor(bool ready)
{
	if (StepMotor::initializeStepMotor()) {
		loadCalibParamFromProfile();
		updatePositionToRetinaOrigin();

		/*
		auto pattern = SystemProfile::getPatternSettings();
		int lower = getPositionOfRetinaOrigin() - pattern->getReferenceRangeLowerSize();
		int upper = getPositionOfRetinaOrigin() + pattern->getReferenceRangeUpperSize();
		setRangeMax(upper);
		setRangeMin(lower);
		LogDebug() << "Reference range, min: " << getRangeMin() << ", max: " << getRangeMax();
		*/
		// updatePosition(getInitPosition());
		return true;
	}
	return false;
}

bool wso_device::OctReferMotor::updatePositionToOrigin(int mode)
{
	if (mode == 0) {
		return updatePositionToRetinaOrigin();
	}
	else {
		return updatePositionToCorneaOrigin();
	}
}

bool wso_device::OctReferMotor::setCurrentPositionAsOrigin(int mode)
{
	if (mode == 0) {
		setCurrentPositionAsRetinaOrigin();
	}
	else {
		setCurrentPositionAsCorneaOrigin();
	}
	return true;
}


int wso_device::OctReferMotor::getPositionOfRetinaOrigin(void) const
{
	return impl().posOfOrigin;
	//return BoardConfig::referencePosToRetina();
}


int wso_device::OctReferMotor::getPositionOfCorneaOrigin(void) const
{
	return impl().posOfOrigin2;
	//return BoardConfig::referencePosToCornea();
}


void wso_device::OctReferMotor::setPositionOfRetinaOrigin(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);

	impl().posOfOrigin = pos;
	// BoardConfig::referencePosToRetina(true, pos);
	return;
}


void wso_device::OctReferMotor::setPositionOfCorneaOrigin(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);

	impl().posOfOrigin2 = pos;
	// BoardConfig::referencePosToCornea(true, pos);
	return;
}


void wso_device::OctReferMotor::setCurrentPositionAsRetinaOrigin(void)
{
	int pos = getPosition();
	setPositionOfRetinaOrigin(pos);
	return;
}


void wso_device::OctReferMotor::setCurrentPositionAsCorneaOrigin(void)
{
	int pos = getPosition();
	setPositionOfCorneaOrigin(pos);
	return;
}


bool wso_device::OctReferMotor::updatePositionToRetinaOrigin(void)
{
	int pos = getPositionOfRetinaOrigin();
	bool result = updatePosition(pos);
	return result;
}


bool wso_device::OctReferMotor::updatePositionToCorneaOrigin(void)
{
	int pos = getPositionOfCorneaOrigin();
	bool result = updatePosition(pos);
	return result;
}


bool wso_device::OctReferMotor::updatePositionToUpperEnd(void)
{
	int pos = getRangeMax();
	bool result = updatePosition(pos);
	return result;
}


bool wso_device::OctReferMotor::updatePositionToLowerEnd(void)
{
	int pos = getRangeMin();
	bool result = updatePosition(pos);
	return result;
}


bool wso_device::OctReferMotor::isEndOfLowerPosition(bool inRange) const
{
	/*
	auto pattern = SystemProfile::getPatternSettings();
	int lowerEnd = getPositionOfRetinaOrigin() - pattern->getReferenceRangeLowerSize();

	if (!inRange) {
		lowerEnd = getRangeMin();
	}

	return (getPosition() <= (lowerEnd + 5) ? true : false);
	*/
	return (fabs(getPosition() - getRangeMin()) <= 5 ? true : false);
}


bool wso_device::OctReferMotor::isEndOfUpperPosition(bool inRange) const
{
	/*
	auto pattern = SystemProfile::getPatternSettings();
	int upperEnd = getPositionOfRetinaOrigin() + pattern->getReferenceRangeUpperSize();

	if (!inRange) {
		upperEnd = getRangeMax();
	}

	return (getPosition() >= (upperEnd - 5) ? true : false);
	*/
	return (fabs(getPosition() - getRangeMax()) <= 5 ? true : false);
}


bool wso_device::OctReferMotor::isAtLowerSideByOrigin(bool isCornea) const
{
	/*
	if (getPosition() < ((getRangeMax() - getRangeMin()) / 2)) {
	return true;
	}
	*/
	int origin = (isCornea ? getPositionOfCorneaOrigin() : getPositionOfRetinaOrigin());
	if (getPosition() < origin) {
		return true;
	}
	return false;
}


bool wso_device::OctReferMotor::isAtUpperSideByOrigin(bool isCornea) const
{
	/*
	if (getPosition() >= ((getRangeMax() - getRangeMin()) / 2)) {
	return true;
	}
	*/
	int origin = (isCornea ? getPositionOfCorneaOrigin() : getPositionOfRetinaOrigin());
	if (getPosition() > origin) {
		return true;
	}
	return false;
}

bool wso_device::OctReferMotor::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets(); p) {
		auto value1 = p->MotorCalPos.REF_RetinaPos;
		auto value2 = p->MotorCalPos.REF_CorneaPos;
		setPositionOfRetinaOrigin(value1);
		setPositionOfCorneaOrigin(value2);
		LogD() << "OctReferMotor origin positions loaded from profile, retina: " << value1 << ", cornea: " << value2;
		return true;
	}
	return false;
}

bool wso_device::OctReferMotor::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibMotorSets*>(getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets()); p) {
		auto value1 = getPositionOfRetinaOrigin();
		auto value2 = getPositionOfCorneaOrigin();
		p->MotorCalPos.REF_RetinaPos = value1;
		p->MotorCalPos.REF_CorneaPos = value2;
		return true;
	}
	return false;
}

OctReferMotor::OctReferMotorImpl& wso_device::OctReferMotor::impl(void) const
{
	return *d_ptr;
}
