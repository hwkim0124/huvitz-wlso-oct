#include "pch.h"
#include "StepMotor.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include <string>
#include <sstream>

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct StepMotor::StepMotorImpl
{
	StepMotorType type;

	bool asyncMode;

	int curPos;
	int maxSpeed;
	int minSpeed;
	int accStep;
	int smPosMin;
	int smPosMax;

	int targetPos;

	thread worker;
	mutex mutexLock;

	StepMotorImpl()
		: asyncMode(false),
		smPosMin(0), smPosMax(0), curPos(0), maxSpeed(0), minSpeed(0), accStep(0), 
		targetPos(0), type(StepMotorType::UNKNOWN)
	{
	}

	// 복사 생성자
	StepMotorImpl(const StepMotorImpl& other)
		: 
		asyncMode(other.asyncMode),
		type(other.type),
		curPos(other.curPos),
		maxSpeed(other.maxSpeed),
		minSpeed(other.minSpeed),
		accStep(other.accStep),
		smPosMin(other.smPosMin),
		smPosMax(other.smPosMax),
		targetPos(other.targetPos)
		// thread/mutex는 복사 안함(그냥 새로)
	{
	}

	StepMotorImpl& operator=(const StepMotorImpl& other) {
		if (this != &other) {
			asyncMode = other.asyncMode;
			type = other.type;
			curPos = other.curPos;
			maxSpeed = other.maxSpeed;
			minSpeed = other.minSpeed;
			accStep = other.accStep;
			smPosMin = other.smPosMin;
			smPosMax = other.smPosMax;
			targetPos = other.targetPos;
			// worker, mutexLock은 복사하지 않음. (초기화 또는 필요시 상태만 리셋)
		}
		return *this;
	}
};


StepMotor::StepMotor() :
	d_ptr(make_unique<StepMotorImpl>())
{
}


wso_device::StepMotor::StepMotor(MainBoard* board, StepMotorType type) :
	d_ptr(make_unique<StepMotorImpl>()), BoardComponent(board)
{
	impl().type = type;
}


StepMotor::~StepMotor()
{
	if (impl().worker.joinable()) {
		impl().worker.join();
	}
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional definition of copy constructor and assignment for the struct to resolve 
// the situation. 
StepMotor::StepMotor::StepMotor(StepMotor&& rhs) = default;
StepMotor& StepMotor::StepMotor::operator=(StepMotor&& rhs) = default;


wso_device::StepMotor::StepMotor(const StepMotor& rhs)
	: d_ptr(make_unique<StepMotorImpl>(*rhs.d_ptr))
{
}


StepMotor& wso_device::StepMotor::operator=(const StepMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::StepMotor::initializeStepMotor(void)
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


bool wso_device::StepMotor::updatePosition(int pos)
{
	if (!isInitiated()) {
		return false;
	}

	if (controlMove(pos)) {
		int pos = getPosition();
		float value = getCurrentValueByPosition();
		CallbackRegistry::getInstance()->runStepMotorPositionChanged(getMotorType(), pos, value);
		return true;
	}
	return false;
}

bool wso_device::StepMotor::updatePositionUseThread(int pos)
{
	if (!isInitiated()) {
		return false;
	}

	if (impl().worker.joinable()) {
		impl().worker.join();
	}

	impl().worker = std::thread([this, pos]() {
		unique_lock<mutex> lock(impl().mutexLock);
		if (controlMove(pos)) {
			int curPos = getPosition();
			float value = getCurrentValueByPosition();
			CallbackRegistry::getInstance()->runStepMotorPositionChanged(getMotorType(), curPos, value);
		}
	});

	//impl().worker.detach(); // automatically end
	return true;
}

bool wso_device::StepMotor::updatePositionByOffset(int offset)
{
	int pos = getPosition() + offset;
	return updatePosition(pos);
}


bool wso_device::StepMotor::updatePositionByValue(float value)
{
	return false;
}

bool wso_device::StepMotor::updatePositionByValueOffset(float offset)
{
	return false;
}

bool wso_device::StepMotor::updatePositionToOrigin(int mode)
{
	return false;
}

bool wso_device::StepMotor::setCurrentPositionAsOrigin(int mode)
{
	return false;
}

float wso_device::StepMotor::getCurrentValueByPosition(void) const
{
	return 0.0f;
}

int wso_device::StepMotor::getPositionAtValue(float value) const
{
	return 0;
}

float wso_device::StepMotor::getValueAtPosition(int pos) const
{
	return 0.0f;
}


bool wso_device::StepMotor::controlMove(int pos, bool async)
{
	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	impl().targetPos = pos;

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorMove(getMotorId(), pos))
	{
		if (!isAsyncMode() && !async) {
			return waitForUpdate();
		}
		return true;
	}
	LogError() << "Step motor control move failed!, name: " << getName() << ", pos: " << pos;
	return false;
}


bool wso_device::StepMotor::controlHome(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorHome(getMotorId()))
	{
		if (!isAsyncMode()) {
			return waitForUpdate();
		}
		return true;
	}
	LogError() << "Step motor control home failed!, name: " << getName();
	return false;
}

bool wso_device::StepMotor::controlStop(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorStop(getMotorId()))
	{
		return true;
	}
	LogError() << "Step motor control stop failed!, name: " << getName();
	return false;
}


bool wso_device::StepMotor::updateStopVelocity()
{
	if (!isInitiated()) {
		return false;
	}

	if (controlStopVelocity()) {
		int pos = getPosition();
		float value = getCurrentValueByPosition();
		CallbackRegistry::getInstance()->runStepMotorPositionChanged(getMotorType(), pos, value);
		return true;
	}
	return false;
}

bool wso_device::StepMotor::controlMoveVelocity(int direction)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorMoveVelocity(getMotorId(), (uint8_t)direction))
	{
		return true;
	}
	LogError() << "Step motor control move velocity failed!, name: " << getName();
	return false;
}

bool wso_device::StepMotor::controlStopVelocity()
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorStopVelocity(getMotorId()))
	{
		if (!updateStatus()) {
			return false;
		}
		return true;
	}
	LogError() << "Step motor control stop failed!, name: " << getName();
	return false;
}

bool wso_device::StepMotor::controlSetVelocity(int nAccelStop, int nMinSpeed, int nMaxSpeed)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.WriteMotorSpeedVelocity(getMotorId(), (uint32_t)nAccelStop, (uint32_t)nMinSpeed, (uint32_t)nMaxSpeed))
	{
		if (!updateStatus()) {
			return false;
		}
		return true;
	}
	LogError() << "Step motor control set velocity failed!, name: " << getName();
	return false;
}

bool wso_device::StepMotor::controlSetDefaultVelocity()
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorSpeedDefaultVelocity(getMotorId()))
	{
		if (!updateStatus()) {
			return false;
		}
		return true;
	}
	LogError() << "Step motor control set default velocity failed!, name: " << getName();
	return false;
}

bool wso_device::StepMotor::updateStatus(void)
{
	// No need to be initialized for update from mainboard.
	/*
	if (!isInitiated()) {
	return false;
	}
	*/

	UsbComm& usbComm = getMainBoard()->getUsbComm();

	if (isYStageMotor()) {
		if (auto* hbs = getMainBoard()->getHbsDataProfile(); hbs->loadStageMotorStatus()) {
			auto* info = hbs->getHbsYStageMotor();
			if (info) {
				impl().curPos = info->CurPos;
				impl().smPosMax = info->sm_pos_max;
				impl().smPosMin = info->sm_pos_min;
				return true;
			}
		}

	}
	else {
		if (auto* hbs = getMainBoard()->getHbsDataProfile(); hbs->loadStepMotorStatus(getType())) {
			auto* info = hbs->getHbsStepMotorStatus(getType());
			if (info) {
				impl().curPos = info->CurPos;
				impl().maxSpeed = info->max_speed;
				impl().minSpeed = info->min_speed;
				impl().smPosMax = info->sm_pos_max;
				impl().smPosMin = info->sm_pos_min;
				impl().accStep = info->acc_step;
				return true;
			}
		}
	}

	LogError() << "Step motor update status failed!, name: " << getName();
	return false;
}


void wso_device::StepMotor::reportStatus(void)
{
	ostringstream ss;
	ss << "Step motor name: " << getName();
	ss << ", cur_pos: " << impl().curPos;
	ss << ", max_speed: " << impl().maxSpeed;
	ss << ", min_speed: " << impl().minSpeed;
	ss << ", smPosMax: " << impl().smPosMax;
	ss << ", smPosMin: " << impl().smPosMin;
	ss << ", accStep: " << impl().accStep;
	LogDebug() << ss.str();
	return;
}

bool wso_device::StepMotor::fetchStatus(StepMotorStatus* status)
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
		status->rangeMinValue = getValueAtPosition(status->rangeMin);
		status->rangeMaxValue = getValueAtPosition(status->rangeMax);
	}
	return true;
}


void wso_device::StepMotor::setAsyncMode(bool flag)
{
	impl().asyncMode = flag;
	return;
}


bool wso_device::StepMotor::isAsyncMode(void) const
{
	return impl().asyncMode;
}


bool wso_device::StepMotor::waitForUpdate(int posOffset, int timeDelay, int countMax)
{
	bool check = false;
	switch (impl().type) {
	case StepMotorType::OCT_FOCUS:
	case StepMotorType::OCT_REFER:
	case StepMotorType::OCT_POLAR:
	case StepMotorType::LSO_FOCUS:
	case StepMotorType::OCT_REFND:
		break;
	default:
		break;
	}

	// Polling the current position of moving motor.
	int count = 0;
	auto initPos = getPosition();

	while (!check) {
		std::this_thread::sleep_for(std::chrono::milliseconds(timeDelay));

		if (!updateStatus()) {
			break;
		}

		count++;
		auto currPos = getPosition();
		if (count >= countMax) {
			break;
		}
		else if ((abs(initPos - currPos) <= posOffset) && count >= MOTOR_MOVE_INIT_COUNT) {
			break;
		}
		else {
			if (abs(currPos - impl().targetPos) <= posOffset) {
				check = true;
				break;
			}
		}
	}

	if (check) {
		return true;
	}
	LogDebug() << "StepMotor::waitForUpdate() timeout!, name: " << getName() << ", curPos: " << getPosition() << ", target: " << impl().targetPos << ", count: " << count;
	return false;
}


void wso_device::StepMotor::setPosition(int pos)
{
	impl().curPos = pos;
	return;
}


int wso_device::StepMotor::getPosition(void) const
{
	return impl().curPos;
}


int wso_device::StepMotor::getTargetPosition(void) const
{
	return impl().targetPos;
}


int wso_device::StepMotor::getCenterPosition(void) const
{
	return getRangeMin() + getRangeSize() / 2;
}


int wso_device::StepMotor::getRangeMax(void) const
{
	return impl().smPosMax;
}


int wso_device::StepMotor::getRangeMin(void) const
{
	return impl().smPosMin;
}


int wso_device::StepMotor::getRangeSize(void) const
{
	return abs(getRangeMax() - getRangeMin());
}

void wso_device::StepMotor::setRangeMax(int value)
{
	impl().smPosMax = value;
	return;
}

void wso_device::StepMotor::setRangeMin(int value)
{
	impl().smPosMin = value;
	return;
}


int wso_device::StepMotor::getSliderStepSize(void) const
{
	int size;
	int range = getRangeSize();

	switch (impl().type) {
	case StepMotorType::OCT_FOCUS:
	case StepMotorType::OCT_REFER:
	case StepMotorType::OCT_POLAR:
	case StepMotorType::OCT_REFND:
	case StepMotorType::LSO_FOCUS:
		size = range / 100;
		break;
	default:
		if (range < 100) {
			size = range;
		}
		else {
			size = range / 10;
		}
		break;
	}

	return size;
}


bool wso_device::StepMotor::isEndOfLowerPosition(void) const
{
	auto mpos = getPosition();
	auto rmin = getRangeMin();
	if (mpos <= (rmin + MOTOR_END_OF_RANGE_OFFSET)) {
		return true;
	}
	return false;
}


bool wso_device::StepMotor::isEndOfUpperPosition(void) const
{
	auto mpos = getPosition();
	auto rmax = getRangeMax();
	if (mpos >= (rmax - MOTOR_END_OF_RANGE_OFFSET)) {
		return true;
	}
	return false;
}


bool wso_device::StepMotor::isAtLowerSideOfPosition(void) const
{
	auto mpos = getPosition();
	auto cent = getCenterPosition();
	return (mpos < cent ? true : false);
}


bool wso_device::StepMotor::isAtUpperSideOfPosition(void) const
{
	auto mpos = getPosition();
	auto cent = getCenterPosition();
	return (mpos >= cent ? true : false);
}


bool wso_device::StepMotor::isAtCenterOfPosition(void) const
{
	auto mpos = getPosition();
	auto cent = getCenterPosition();
	return (abs(mpos - cent) <= MOTOR_CENTER_OF_POSITION_OFFSET ? true : false);
}


const char* wso_device::StepMotor::getName(void) const
{
	switch (impl().type) {
	case StepMotorType::OCT_FOCUS:
		return MOTOR_OCT_FOCUS_NAME;
	case StepMotorType::OCT_REFER:
		return MOTOR_OCT_REFER_NAME;
	case StepMotorType::OCT_POLAR:
		return MOTOR_OCT_POLAR_NAME;
	case StepMotorType::OCT_REFND:
		return MOTOR_OCT_REFND_NAME;
	case StepMotorType::OCT_ANT_LENS:
		return MOTOR_OCT_ANT_LENS_NAME;
	case StepMotorType::LSO_FOCUS:
		return MOTOR_LSO_FOCUS_NAME;
	case StepMotorType::STAGE_Y:
		return MOTOR_STAGE_X_NAME;
	case StepMotorType::STAGE_X:
		return MOTOR_STAGE_Y_NAME;
	case StepMotorType::STAGE_Z:
		return MOTOR_STAGE_Z_NAME;
	case StepMotorType::SWING:
		return MOTOR_SWING_NAME;
	}
	return MOTOR_UNKNOWN_NAME;
}


StepMotorType wso_device::StepMotor::getType(void) const
{
	return impl().type;
}

void wso_device::StepMotor::setType(StepMotorType type)
{
	impl().type = type;
	return;
}

MotorType wso_device::StepMotor::getMotorType(void) const
{
	return static_cast<MotorType>(impl().type);
}

bool wso_device::StepMotor::isXStageMotor(void) const
{
	return (getType() == StepMotorType::STAGE_X);
}

bool wso_device::StepMotor::isYStageMotor(void) const
{
	return (getType() == StepMotorType::STAGE_Y);
}

bool wso_device::StepMotor::isZStageMotor(void) const
{
	return (getType() == StepMotorType::STAGE_Z);
}


std::int32_t wso_device::StepMotor::getInitPosition(StepMotorType type)
{
	switch (type) {
	case StepMotorType::OCT_FOCUS:
		return MOTOR_OCT_FOCUS_INIT_POS;
	case StepMotorType::OCT_REFER:
		return MOTOR_OCT_REFER_INIT_POS;
	case StepMotorType::OCT_POLAR:
		return MOTOR_OCT_POLAR_INIT_POS;
	case StepMotorType::LSO_FOCUS:
		return MOTOR_LSO_FOCUS_INIT_POS;
	case StepMotorType::STAGE_X:
		return MOTOR_STAGE_X_INIT_POS;
	case StepMotorType::STAGE_Y:
		return MOTOR_STAGE_Y_INIT_POS;
	case StepMotorType::STAGE_Z:
		return MOTOR_STAGE_Z_INIT_POS;
	case StepMotorType::SWING:
		return MOTOR_SWING_INIT_POS;
	}
	return 0;
}

StepMotor::StepMotorImpl& wso_device::StepMotor::impl(void) const
{
	return *d_ptr;
}

std::uint8_t wso_device::StepMotor::getMotorId(void) const
{
	return static_cast<std::uint8_t>(impl().type);
}

