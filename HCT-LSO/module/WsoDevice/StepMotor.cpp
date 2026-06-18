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
	int targetPos;
	bool stopped;

	int curPos;
	int maxSpeed;
	int minSpeed;
	int accStep;
	int smPosMin;
	int smPosMax;

	int motorWait;
	int piStatus;
	int piHitRefPos;
	int piHitMargin;
	int piHitLastPos;
	int piHitPosError;

	int limitRange[2];
	int limitStatus[2];

	thread worker;
	mutex mutexLock;

	StepMotorImpl()
		: asyncMode(false), stopped(true),
		smPosMin(0), smPosMax(0), curPos(0), maxSpeed(0), minSpeed(0), accStep(0), 
		targetPos(0), type(StepMotorType::UNKNOWN), piStatus(0), limitRange{ 0 }, limitStatus{ false },
		motorWait(0), piHitRefPos(0), piHitMargin(0), piHitLastPos(0), piHitPosError(0)
	{
	}

	// 복사 생성자
	StepMotorImpl(const StepMotorImpl& other)
		: 
		asyncMode(other.asyncMode),
		stopped(other.stopped),
		type(other.type),
		curPos(other.curPos),
		maxSpeed(other.maxSpeed),
		minSpeed(other.minSpeed),
		accStep(other.accStep),
		smPosMin(other.smPosMin),
		smPosMax(other.smPosMax),
		motorWait(other.motorWait), 
		piStatus(other.piStatus),
		piHitRefPos(other.piHitRefPos), 
		piHitMargin(other.piHitMargin), 
		piHitLastPos(other.piHitLastPos), 
		piHitPosError(other.piHitPosError),
		targetPos(other.targetPos)
		// thread/mutex는 복사 안함(그냥 새로)
	{
	}

	StepMotorImpl& operator=(const StepMotorImpl& other) {
		if (this != &other) {
			asyncMode = other.asyncMode;
			stopped = other.stopped;

			type = other.type;
			curPos = other.curPos;
			maxSpeed = other.maxSpeed;
			minSpeed = other.minSpeed;
			accStep = other.accStep;
			smPosMin = other.smPosMin;
			smPosMax = other.smPosMax;

			motorWait = other.motorWait;
			piStatus = other.piStatus;
			piHitRefPos = other.piHitRefPos;
			piHitMargin = other.piHitMargin;
			piHitLastPos = other.piHitLastPos;
			piHitPosError = other.piHitPosError;

			limitStatus[0] = other.limitStatus[0];
			limitStatus[1] = other.limitStatus[1];
			limitRange[0] = other.limitRange[0];
			limitRange[1] = other.limitRange[1];

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
		// setLimitRange(impl().smPosMin, impl().smPosMax);
		reportStatus();
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
	/*
	if (isStageMotor()) {
		return controlJogg(offset);
	}
	*/
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
	if (!isInitiated()) {
		return false;
	}

	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	impl().targetPos = pos;

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorMove(getMotorId(), pos))
	{
		impl().stopped = false;
		async = async || isAsyncMode();
		if (!async) {
			return waitForUpdate();
		}
		return true;
	}
	LogD() << "StepMotor::controlMove() failed, name: " << getName() << ", pos: " << pos << ", async: " << async;
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
		impl().stopped = false;
		if (!isAsyncMode()) {
			return waitForUpdate();
		}
		return true;
	}
	LogD() << "StepMotor::controlHome() failed, name: " << getName() << ", async: " << isAsyncMode();
	return false;
}

bool wso_device::StepMotor::controlStop(void)
{
	if (!isInitiated()) {
		return false;
	}
	if (impl().stopped) {
		return true;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorStop(getMotorId()))
	{
		impl().stopped = true;
		return true;
	}
	LogD() << "StepMotor::controlHome() failed, name: " << getName();
	return false;
}

bool wso_device::StepMotor::controlJogg(int delta)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorJogg(getMotorId(), delta)) {
		impl().stopped = false;
		return true;
	}

	LogD() << "StepMotor::controlJogg() failed, name: " << getName();
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
	LogD() << "StepMotor::updateStopVelocity() failed, name: " << getName();
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
	LogD() << "StepMotor::controlMoveVelocity() failed, name: " << getName();
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
	LogD() << "StepMotor::controlStopVelocity() failed, name: " << getName();
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
	LogD() << "StepMotor::controlSetVelocity() failed, name: " << getName() << ", acc_stop: " << nAccelStop << ", min_speed: " << nMinSpeed << ", max_speed: " << nMaxSpeed;
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
	LogD() << "StepMotor::controlSetDefaultVelocity() failed, name: " << getName();
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
	/*
	if (isYStageMotor()) {
		auto type = static_cast<StageMotorType>(getType());
		if (auto* hbs = getMainBoard()->getHbsDataProfile(); hbs->loadStageMotorStatus(type)) {
			auto* info = hbs->getHbsYstageMotor();
			if (info) {
				impl().curPos = info->CurPos;
				impl().smPosMax = info->sm_pos_max;
				impl().smPosMin = info->sm_pos_min;
				return true;
			}
		}
	}
	else { */
		if (auto* hbs = getMainBoard()->getHbsDataProfile(); hbs->loadStepMotorStatus(getType())) {
			auto* info = hbs->getHbsStepMotorStatus(getType());
			if (info) {
				impl().curPos = info->CurPos;
				impl().maxSpeed = info->max_speed;
				impl().minSpeed = info->min_speed;
				impl().smPosMax = info->sm_pos_max;
				impl().smPosMin = info->sm_pos_min;
				impl().accStep = info->acc_step;

				impl().motorWait = info->MotorWait;
				impl().piStatus = info->PI_status;
				impl().piHitRefPos = info->sm_pi_hit_ref_pos;
				impl().piHitMargin = info->sm_pi_hit_margin;
				impl().piHitLastPos = info->sm_last_pi_hit_pos;
				impl().piHitPosError = info->sm_pi_hit_pos_error;
				return true;
			}
		}
	// }

	LogD() << "StepMotor::updateStatus() failed, name: " << getName();
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

		status->limitRange[0] = impl().limitRange[0];
		status->limitRange[1] = impl().limitRange[1];
		status->limitStatus[0] = impl().limitStatus[0];
		status->limitStatus[1] = impl().limitStatus[1];

		status->sliderPageSize = getSliderStepSize();
		status->sliderStepSize = getSliderStepSize();
		status->rangeMinValue = getValueAtPosition(status->rangeMin);
		status->rangeMaxValue = getValueAtPosition(status->rangeMax);

		status->motorWait = impl().motorWait;
		status->piStatus = impl().piStatus;
		status->piHitRefPos = impl().piHitRefPos;
		status->piHitMargin = impl().piHitMargin;
		status->piHitLastPos = impl().piHitLastPos;
		status->piHitPosError = impl().piHitPosError;
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
	case StepMotorType::OCT_REFND:
	case StepMotorType::LSO_FOCUS:
	case StepMotorType::RET_MIRROR:
	case StepMotorType::OCT_ANT_LENS:
	case StepMotorType::LSO_FILTER:
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
		if (++count >= countMax) {
			break;
		}
		auto currPos = getPosition();
		auto targPos = impl().targetPos;

		if (isStageMotor()) {
			if (abs(currPos - impl().targetPos) <= posOffset) {
				check = true;
				break;
			}
			if (isEndOfLowerPosition() || isEndOfUpperPosition()) {
				check = true;
				break;
			}
		}
		else {
			if ((abs(initPos - currPos) <= posOffset) && count >= MOTOR_MOVE_INIT_COUNT) {
				break;
			}
			if (abs(currPos - impl().targetPos) <= posOffset) {
				check = true;
				break;
			}
		}
	}

	if (check) {
		return true;
	}
	LogD() << "StepMotor::waitForUpdate() timeout, name: " << getName() << ", curPos: " << getPosition() << ", target: " << impl().targetPos << ", count: " << count;
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

void wso_device::StepMotor::setLimitRange(int lmin, int lmax)
{
	impl().limitRange[0] = lmin;
	impl().limitRange[1] = lmax;
	return;
}

int wso_device::StepMotor::getLimitMin(void) const
{
	return impl().limitRange[0];
}

int wso_device::StepMotor::getLimitMax(void) const
{
	return impl().limitRange[1];
}

int wso_device::StepMotor::getLimitSize(void) const
{
	auto size = impl().limitRange[1] - impl().limitRange[0];
	return size;
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
	if (isStageMotor()) {
		if (impl().limitStatus[0]) {
			return true;
		}
	}
	else {
		auto mpos = getPosition();
		auto rmin = getRangeMin();
		if (mpos <= (rmin + MOTOR_END_OF_RANGE_OFFSET)) {
			return true;
		}
	}
	return false;
}


bool wso_device::StepMotor::isEndOfUpperPosition(void) const
{
	if (isStageMotor()) {
		if (impl().limitStatus[1]) {
			return true;
		}
	}
	else {
		auto mpos = getPosition();
		auto rmax = getRangeMax();
		if (mpos >= (rmax - MOTOR_END_OF_RANGE_OFFSET)) {
			return true;
		}
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
	auto flag = (abs(mpos - cent) <= MOTOR_CENTER_OF_POSITION_OFFSET ? true : false);
	return flag;
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
	case StepMotorType::LSO_FOCUS:
		return MOTOR_LSO_FOCUS_NAME;
	case StepMotorType::RET_MIRROR:
		return MOTOR_RET_MIRROR_NAME;
	case StepMotorType::OCT_ANT_LENS:
		return MOTOR_OCT_ANT_LENS_NAME;
	case StepMotorType::LSO_FILTER:
		return MOTOR_LSO_FILTER_NAME;
	case StepMotorType::SWING:
		return MOTOR_SWING_NAME;
	case StepMotorType::STAGE_X:
		return MOTOR_STAGE_Y_NAME;
	case StepMotorType::STAGE_Y:
		return MOTOR_STAGE_X_NAME;
	case StepMotorType::STAGE_Z:
		return MOTOR_STAGE_Z_NAME;
	case StepMotorType::CHIN_REST:
		return MOTOR_CHIN_REST_NAME;
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

bool wso_device::StepMotor::isSwingMotor(void) const
{
	return (getType() == StepMotorType::SWING);
}

bool wso_device::StepMotor::isStageMotor(void) const
{
	switch (getType()) {
	case StepMotorType::STAGE_X:
	case StepMotorType::STAGE_Y:
	case StepMotorType::STAGE_Z:
	case StepMotorType::SWING:
		return true;
	}
	return false;
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
	case StepMotorType::OCT_REFND:
		return MOTOR_OCT_REFND_INIT_POS;
	case StepMotorType::LSO_FOCUS:
		return MOTOR_LSO_FOCUS_INIT_POS;
	case StepMotorType::RET_MIRROR:
		return MOTOR_RET_MIRROR_INIT_POS;
	case StepMotorType::OCT_ANT_LENS:
		return MOTOR_OCT_ANT_LENS_INIT_POS;
	case StepMotorType::LSO_FILTER:
		return MOTOR_LSO_FILTER_INIT_POS;
	case StepMotorType::SWING:
		return MOTOR_SWING_INIT_POS;
	case StepMotorType::STAGE_X:
		return MOTOR_STAGE_X_INIT_POS;
	case StepMotorType::STAGE_Y:
		return MOTOR_STAGE_Y_INIT_POS;
	case StepMotorType::STAGE_Z:
		return MOTOR_STAGE_Z_INIT_POS;
	case StepMotorType::CHIN_REST:
		return MOTOR_CHIN_REST_INIT_POS;
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

