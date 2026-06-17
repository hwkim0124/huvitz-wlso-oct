#include "pch.h"
#include "StageMotor.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "BoardComponent.h"

#include <string>
#include <sstream>

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;


struct StageMotor::StageMotorImpl
{
	StageMotorType type;

	bool asyncMode;
	bool stopped;

	int curPos;
	int maxSpeed;
	int minSpeed;
	int accStep;
	int smPosMin;
	int smPosMax;

	int centerPos;
	int targetPos;
	int limitRange[2];
	int limitStatus[2];

	StageMotorImpl()
		: asyncMode(true), stopped(false),
		smPosMin(0), smPosMax(0), curPos(0), maxSpeed(0), minSpeed(0), accStep(0), 
		centerPos(0), targetPos(0), limitRange{ 0 }, limitStatus{ false }
	{
	}
};


StageMotor::StageMotor() :
	d_ptr(make_unique<StageMotorImpl>())
{
}


wso_device::StageMotor::StageMotor(MainBoard* board, StageMotorType type) :
	d_ptr(make_unique<StageMotorImpl>()), BoardComponent(board)
{
	impl().type = type;
}


StageMotor::~StageMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional definition of copy constructor and assignment for the struct to resolve 
// the situation. 
StageMotor::StageMotor::StageMotor(StageMotor&& rhs) = default;
StageMotor& StageMotor::StageMotor::operator=(StageMotor&& rhs) = default;


wso_device::StageMotor::StageMotor(const StageMotor& rhs)
	: d_ptr(make_unique<StageMotorImpl>(*rhs.d_ptr))
{
}


StageMotor& wso_device::StageMotor::operator=(const StageMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::StageMotor::initializeStageMotor(void)
{
	bool ret = updateStatus();
	if (!ret) {
		return false;
	}
	else {
		reportStatus();
		/*
		int value = getInitPosition();
		if (value != getPosition()) {
			controlMove(value);
		}
		*/
	}


	if (getMainBoard()->isMotorsNotInUse()) {
		return true;
	}
	setInitiated(true);
	return ret;
}


bool wso_device::StageMotor::isStepMotor(void) const
{
	return true; //  (getType() == StageMotorType::STAGE_Y);
}


bool wso_device::StageMotor::updatePosition(int pos)
{
	if (!isInitiated()) {
		return false;
	}

	if (controlMove(pos)) {
		/*
		int pos = getPosition();
		float value = getCurrentValueByPosition();
		CallbackRegistry::getInstance()->runStepMotorPositionChanged(getMotorType(), pos, value);
		*/
		return true;
	}
	return false;
}


int wso_device::StageMotor::getSliderStepSize(void) const
{
	int size = 0;
	if (isStepMotor()) {
		size = (getRangeSize() / 100);
	}
	size = (size <= 0 ? 1 : size);
	return size;
}



bool wso_device::StageMotor::updatePositionByOffset(int offset)
{
	// int pos = getPosition() + offset;
	// return updatePosition(pos);
	return controlJogg(offset);
}


bool wso_device::StageMotor::updatePositionToCenter(void)
{
	int offset = getCenterPosition() - getPosition();
	return updatePositionByOffset(offset);
}

bool wso_device::StageMotor::controlMove(int pos)
{
	/*
	if (!isInitiated()) {
		return false;
	}
	*/

	if (isStepMotor() == false) {
		pos = (pos < getRangeMin() ? getRangeMin() : pos);
		pos = (pos > getRangeMax() ? getRangeMax() : pos);
	}
	impl().targetPos = pos;

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorMove(getMotorId(), pos))
	{
		impl().stopped = false;
		if (!isAsyncMode()) {
			return waitForUpdate();
		}
		return true;
	}
	LogDebug() << "StageMotor::controlMove() failed!, name: " << getName() << ", pos: " << pos;
	return false;
}


bool wso_device::StageMotor::controlHome(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorHome(getMotorId()))
	{
		impl().stopped = false;
		if (!isAsyncMode()) {
			return waitForUpdate();
		}
		return true;
	}
	LogDebug() << "StageMotor::controlHome() failed!, name =" << getName();
	return false;
}

bool wso_device::StageMotor::controlStop(void)
{
	if (!isInitiated()) {
		return false;
	}
	if (impl().stopped) {
		return true;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorStop(getMotorId())) {
		impl().stopped = true;
		return true;
	}
	LogDebug() << "StageMotor::controlHome() failed!, name =" << getName();
	return false;
}


bool wso_device::StageMotor::controlJogg(int delta)
{
	/*
	if (!isInitiated()) {
		return false;
	}
	*/

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorJogg(getMotorId(), delta)) {
		impl().stopped = false;
		return true;
	}

	LogDebug() << "StageMotor::controlJogg() failed!, name: " << getName();
	return false;
}

bool wso_device::StageMotor::updateStatus(void)
{
	/*
	if (!isInitiated()) {
		return false;
	}
	*/

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (isStepMotor()) {
		if (auto* hbs = getMainBoard()->getHbsDataProfile(); hbs->loadStageMotorStatus()) {
			auto* info = hbs->getHbsStageMotorStatus(static_cast<StageMotorType>(getType()));
			if (info) {
				impl().curPos = info->CurPos;
				// impl().limitStatus[0] = info.limit_sensor_state[0];
				// impl().limitStatus[1] = info.limit_sensor_state[1];
				impl().limitRange[0] = info->sm_pos_min;
				impl().limitRange[1] = info->sm_pos_max;
				impl().smPosMax = info->sm_pos_max;
				impl().smPosMin = info->sm_pos_min;
				return true;
			}
		}
	}
	/*
	else {
		StageDcMotorInfo info;
		if (usbComm.StageStatus(getType(), getMainBoard()->getBaseAddressOfStageInfo(), &info)) {
			impl().curPos = info.enc_pos;
			impl().centerPos = info.center_pos;
			return true;
		}
	}
	*/
	LogE() << "Stage motor update status failed!, name: " << getName();
	return false;
}


void wso_device::StageMotor::reportStatus(void)
{
	ostringstream ss;

	if (isStepMotor()) {
		ss << "Stage motor name: " << getName();
		ss << ", cur_pos: " << impl().curPos;
		ss << ", limit[0]: " << impl().limitStatus[0];
		ss << ", limit[1]: " << impl().limitStatus[1];
		ss << ", range[0]: " << impl().limitRange[0];
		ss << ", range[1]: " << impl().limitRange[1];
	}
	else {
		ss << "Stage motor name: " << getName();
		ss << ", cur_pos: " << impl().curPos;
		ss << ", center_pos: " << impl().centerPos;
	}
	LogD() << ss.str();
	return;
}

bool wso_device::StageMotor::fetchStatus(StepMotorStatus* status)
{
	if (status) {
		status->currPos = getPosition();
		status->rangeMin = getRangeMin();
		status->rangeMax = getRangeMax();
		status->maxSpeed = impl().maxSpeed;
		status->minSpeed = impl().minSpeed;
		status->accelStep = impl().accStep;
		status->sliderPageSize = getSliderStepSize();
		status->sliderStepSize = getSliderStepSize();
		// status->rangeMinValue = getValueAtPosition(status->rangeMin);
		// status->rangeMaxValue = getValueAtPosition(status->rangeMax);
	}
	return true;
}


void wso_device::StageMotor::setAsyncMode(bool flag)
{
	impl().asyncMode = flag;
	return;
}


bool wso_device::StageMotor::isAsyncMode(void) const
{
	return impl().asyncMode;
}


bool wso_device::StageMotor::waitForUpdate(int posOffset, int timeDelay, int countMax)
{
	bool check = false;

	// Polling the current position of moving motor.
	int count = 0;
	while (!check) {
		std::this_thread::sleep_for(std::chrono::milliseconds(timeDelay));
		if (!updateStatus() || ++count >= countMax) {
			break;
		}
		else {
			if (abs(getPosition() - impl().targetPos) <= posOffset) {
				check = true;
				break;
			}
			if (isEndOfLowerPosition() || isEndOfUpperPosition()) {
				check = true;
				break;
			}
		}
	}

	if (check) {
		return true;
	}
	LogDebug() << "StageMotor::waitForUpdate() timeout!, name: " << getName() << ", curPos: " << getPosition() << ", target: " << impl().targetPos << ", count: " << count;
	return false;
}


const char* wso_device::StageMotor::getName(void) const
{
	switch (getType()) {
	case MotorType::STAGE_X:
		return MOTOR_STAGE_X_NAME;
	case MotorType::STAGE_Y:
		return MOTOR_STAGE_Y_NAME;
	case MotorType::STAGE_Z:
		return MOTOR_STAGE_Z_NAME;
	}

	return MOTOR_UNKNOWN_NAME;
}


MotorType wso_device::StageMotor::getType(void) const
{
	return static_cast<MotorType>(impl().type);
}

bool wso_device::StageMotor::isXStageMotor(void) const
{
	return (getType() == MotorType::STAGE_X);
}

bool wso_device::StageMotor::isYStageMotor(void) const
{
	return (getType() == MotorType::STAGE_Y);
}

bool wso_device::StageMotor::isZStageMotor(void) const
{
	return (getType() == MotorType::STAGE_Z);
}


bool wso_device::StageMotor::isEndOfLowerPosition(void) const
{
	if (isStepMotor()) {
		if (impl().limitStatus[0]) {
			return true;
		}
	}
	else {
		if (getPosition() <= (getRangeMin() + STAGE_END_OF_LIMIT_OFFSET)) {
			return true;
		}
	}
	return false;
}


bool wso_device::StageMotor::isEndOfUpperPosition(void) const
{
	if (isStepMotor()) {
		if (impl().limitStatus[1]) {
			return true;
		}
	}
	else {
		if (getPosition() >= (getRangeMax() - STAGE_END_OF_LIMIT_OFFSET)) {
			return true;
		}
	}
	return false;
}


bool wso_device::StageMotor::isAtLowerSideOfPosition(void) const
{
	return (getPosition() < getCenterPosition() ? true : false);
}


bool wso_device::StageMotor::isAtUpperSideOfPosition(void) const
{
	return (getPosition() >= getCenterPosition() ? true : false);
}


bool wso_device::StageMotor::isAtCenterOfPosition(void) const
{
	return (abs(getPosition() - getCenterPosition()) <= STAGE_END_OF_LIMIT_OFFSET ? true : false);
}

/*
bool wso_device::StageMotor::loadConfig(OctConfig::DeviceSettings * dset)
{
	return true;
}


bool wso_device::StageMotor::saveConfig(OctConfig::DeviceSettings * dset)
{
	return true;
}
*/

StageMotor::StageMotorImpl& wso_device::StageMotor::impl(void) const
{
	return *d_ptr;
}

std::uint8_t wso_device::StageMotor::getMotorId(void) const
{
	return static_cast<std::uint8_t>(impl().type);
}


std::int32_t wso_device::StageMotor::getInitPosition(StageMotorType type) 
{
	switch (type) {
	case StageMotorType::STAGE_X:
		return MOTOR_STAGE_X_INIT_POS;
	case StageMotorType::STAGE_Y:
		return MOTOR_STAGE_Y_INIT_POS;
	case StageMotorType::STAGE_Z:
		return MOTOR_STAGE_Z_INIT_POS;
	}
	return 0;
}


void wso_device::StageMotor::setPosition(int pos)
{
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	impl().curPos = pos;
	return;
}


int wso_device::StageMotor::getPosition(void) const
{
	return impl().curPos;
}


int wso_device::StageMotor::getTargetPosition(void) const
{
	return impl().targetPos;
}


int wso_device::StageMotor::getCenterPosition(void) const
{
	// return getRangeMin() + getRangeSize() / 2;
	return impl().centerPos;
}


void wso_device::StageMotor::setLimitRange(int low, int high)
{
	impl().limitRange[0] = low;
	impl().limitRange[1] = high;
	return;
}


int wso_device::StageMotor::getRangeMax(void) const
{
	if (isStepMotor()) {
		return impl().limitRange[1];
	}
	else {
		return impl().centerPos + impl().limitRange[1];
	}
}


int wso_device::StageMotor::getRangeMin(void) const
{
	if (isStepMotor()) {
		return impl().limitRange[0];
	}
	else {
		return impl().centerPos + impl().limitRange[0];
	}
}


int wso_device::StageMotor::getRangeSize(void) const
{
	return abs(getRangeMax() - getRangeMin());
}
