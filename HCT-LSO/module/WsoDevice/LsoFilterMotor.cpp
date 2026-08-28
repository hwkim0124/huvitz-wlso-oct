#include "pch.h"
#include "LsoFilterMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


namespace
{
	int clampToMotorRange(const LsoFilterMotor& motor, int pos)
	{
		pos = (pos > motor.getRangeMax() ? motor.getRangeMax() : pos);
		pos = (pos < motor.getRangeMin() ? motor.getRangeMin() : pos);
		return pos;
	}
}


struct LsoFilterMotor::LsoFilterMotorImpl
{
	int posModeColor;
	int posModeBlueFaf;
	int posModeGreenFaf;
	int posModeIcga;

	LsoFilterMotorImpl()
	{
		initializeLsoFilterMotorImpl();
	}

	void initializeLsoFilterMotorImpl()
	{
		posModeColor = 0;
		posModeBlueFaf = 0;
		posModeGreenFaf = 0;
		posModeIcga = 0;
	}
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
		updatePositionToColor();
		return true;
	}
	return false;
}


bool wso_device::LsoFilterMotor::updatePositionToOrigin(int mode)
{
	switch (static_cast<LsoFilterColorMode>(getColorModeIndex(mode))) {
		using enum LsoFilterColorMode;
	case Color:
		return updatePositionToColor();
	case BlueFAF:
		return updatePositionToBlueFaf();
	case GreenFAF:
		return updatePositionToGreenFaf();
	case ICGA:
		return updatePositionToICGA();
	default:
		break;
	}
	LogE() << "LSO filter motor: unknown color mode, mode: " << mode;
	return false;
}

bool wso_device::LsoFilterMotor::setCurrentPositionAsOrigin(int mode)
{
	switch (static_cast<LsoFilterColorMode>(getColorModeIndex(mode))) {
		using enum LsoFilterColorMode;
	case Color:
		setCurrentPositionAsColor();
		return true;
	case BlueFAF:
		setCurrentPositionAsBlueFaf();
		return true;
	case GreenFAF:
		setCurrentPositionAsGreenFaf();
		return true;
	case ICGA:
		setCurrentPositionAsICGA();
		return true;
	default:
		break;
	}
	LogE() << "LSO filter motor: unknown color mode, mode: " << mode;
	return false;
}

bool wso_device::LsoFilterMotor::updatePositionToColor(void)
{
	int pos = getPositionOfColor();
	return StepMotor::updatePosition(pos);
}

bool wso_device::LsoFilterMotor::updatePositionToBlueFaf(void)
{
	int pos = getPositionOfBlueFaf();
	return StepMotor::updatePosition(pos);
}

bool wso_device::LsoFilterMotor::updatePositionToGreenFaf(void)
{
	int pos = getPositionOfGreenFaf();
	return StepMotor::updatePosition(pos);
}

bool wso_device::LsoFilterMotor::updatePositionToICGA(void)
{
	int pos = getPositionOfICGA();
	return StepMotor::updatePosition(pos);
}

void wso_device::LsoFilterMotor::setCurrentPositionAsColor(void)
{
	int pos = getPosition();
	setPositionOfColor(pos);
	return;
}

void wso_device::LsoFilterMotor::setCurrentPositionAsBlueFaf(void)
{
	int pos = getPosition();
	setPositionOfBlueFaf(pos);
	return;
}

void wso_device::LsoFilterMotor::setCurrentPositionAsGreenFaf(void)
{
	int pos = getPosition();
	setPositionOfGreenFaf(pos);
	return;
}

void wso_device::LsoFilterMotor::setCurrentPositionAsICGA(void)
{
	int pos = getPosition();
	setPositionOfICGA(pos);
	return;
}

int wso_device::LsoFilterMotor::getPositionOfColor(void)
{
	return impl().posModeColor;
}

int wso_device::LsoFilterMotor::getPositionOfBlueFaf(void)
{
	return impl().posModeBlueFaf;
}

int wso_device::LsoFilterMotor::getPositionOfGreenFaf(void)
{
	return impl().posModeGreenFaf;
}

int wso_device::LsoFilterMotor::getPositionOfICGA(void)
{
	return impl().posModeIcga;
}

void wso_device::LsoFilterMotor::setPositionOfColor(int pos)
{
	impl().posModeColor = clampToMotorRange(*this, pos);
	return;
}

void wso_device::LsoFilterMotor::setPositionOfBlueFaf(int pos)
{
	impl().posModeBlueFaf = clampToMotorRange(*this, pos);
	return;
}

void wso_device::LsoFilterMotor::setPositionOfGreenFaf(int pos)
{
	impl().posModeGreenFaf = clampToMotorRange(*this, pos);
	return;
}

void wso_device::LsoFilterMotor::setPositionOfICGA(int pos)
{
	impl().posModeIcga = clampToMotorRange(*this, pos);
	return;
}

// The mode value arrives unvalidated from the managed side through
// moveStepMotorToOrigin(MotorType, int), so this is the one guard point.
// Returns -1 for any value outside LsoFilterColorMode.
int wso_device::LsoFilterMotor::getColorModeIndex(int mode)
{
	switch (static_cast<LsoFilterColorMode>(mode)) {
		using enum LsoFilterColorMode;
	case Color:
	case BlueFAF:
	case GreenFAF:
	case ICGA:
		return mode;
	default:
		break;
	}
	return -1;
}


bool wso_device::LsoFilterMotor::loadCalibParamFromProfile(void)
{
	if (auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets(); p) {
		const auto& cal = p->MotorCalPos.LsoFilter_CalPos;
		setPositionOfColor(cal.ColorModePos);
		setPositionOfBlueFaf(cal.BlueFAF_Pos);
		setPositionOfGreenFaf(cal.GreenFAF_Pos);
		setPositionOfICGA(cal.ICGA_Pos);
		LogD() << "Lso filter motor loaded from profile"
			<< ", color: " << cal.ColorModePos
			<< ", blue: " << cal.BlueFAF_Pos
			<< ", green: " << cal.GreenFAF_Pos
			<< ", icga: " << cal.ICGA_Pos;
		return true;
	}
	return false;
}

bool wso_device::LsoFilterMotor::saveCalibParamToProfile(void)
{
	if (auto p = const_cast<HbsCalibMotorSets*>(getMainBoard()->getHbsDataProfile()->getHbsCalibMotorSets()); p) {
		auto& cal = p->MotorCalPos.LsoFilter_CalPos;
		cal.ColorModePos = getPositionOfColor();
		cal.BlueFAF_Pos = getPositionOfBlueFaf();
		cal.GreenFAF_Pos = getPositionOfGreenFaf();
		cal.ICGA_Pos = getPositionOfICGA();
		return true;
	}
	return false;
}

bool wso_device::LsoFilterMotor::isAtPiPostion(void)
{
	if (auto* board = getMainBoard(); board) {
		if (auto* p = board->getHbsDataProfile(); p) {
			if (p->loadGpioStatus()) {
				auto* gpio = p->getHbsGpioStatus();
				auto status = gpio->PIstatus;
				if (status & (0x01 << 10)) {
					return true;
				}
			}
		}
	}
	return false;
}


LsoFilterMotor::LsoFilterMotorImpl& wso_device::LsoFilterMotor::impl(void) const
{
	return *d_ptr;
}
