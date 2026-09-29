#include "pch.h"
#include "StepMotor.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include <string>
#include <sstream>
#include <atomic>
#include <mutex>

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct StepMotor::StepMotorImpl
{
	StepMotorType type;

	bool asyncMode;
	int targetPos;
	// Written by the thread requesting a stop, polled by the thread blocked in
	// waitForUpdate(). Atomic for visibility; mutexMoveStop below orders it
	// against the command that clears it.
	std::atomic<bool> isStopped;

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
	// Held only while sending a move/stop command and publishing isStopped.
	// Never held across waitForUpdate() or a CallbackRegistry call.
	mutex mutexMoveStop;

	StepMotorImpl()
		: asyncMode(false), isStopped(true),
		smPosMin(0), smPosMax(0), curPos(0), maxSpeed(0), minSpeed(0), accStep(0), 
		targetPos(0), type(StepMotorType::UNKNOWN), piStatus(0), limitRange{ 0 }, limitStatus{ false },
		motorWait(0), piHitRefPos(0), piHitMargin(0), piHitLastPos(0), piHitPosError(0)
	{
	}

	StepMotorImpl(const StepMotorImpl& other)
		: 
		asyncMode(other.asyncMode),
		isStopped(other.isStopped.load()),
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
	{
	}

	StepMotorImpl& operator=(const StepMotorImpl& other) {
		if (this != &other) {
			asyncMode = other.asyncMode;
			isStopped = other.isStopped.load();

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
		return true;
	}
	return false;
}

/*
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
	return true;
}
*/

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
	if (!isInitiated()) {
		return false;
	}

	pos = (pos < getRangeMin() ? getRangeMin() : pos);
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	impl().targetPos = pos;

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	bool sent = false;
	{
		// Sending the command and clearing isStopped must be one step. A stop
		// arriving in between would set the flag and then be clobbered here,
		// so waitForUpdate() would never see it and would run to full timeout.
		lock_guard<mutex> lock(impl().mutexMoveStop);
		sent = usbComm.MotorMove(getMotorId(), pos);
		if (sent) {
			impl().isStopped = false;
		}
	}

	if (sent)
	{
		async = async || isAsyncMode();
		if (!async) {
			// Outside the lock: a stop must be able to interrupt this.
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
	bool sent = false;
	{
		lock_guard<mutex> lock(impl().mutexMoveStop);
		sent = usbComm.MotorHome(getMotorId());
		if (sent) {
			impl().isStopped = false;
		}
	}

	if (sent)
	{
		if (!isAsyncMode()) {
			// Outside the lock: a stop must be able to interrupt this.
			return waitForUpdate();
		}
		return true;
	}
	LogD() << "StepMotor::controlHome() failed, name: " << getName() << ", async: " << isAsyncMode();
	return false;
}


bool wso_device::StepMotor::controlJogg(int delta)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();
	lock_guard<mutex> lock(impl().mutexMoveStop);
	if (usbComm.MotorJogg(getMotorId(), delta)) {
		impl().isStopped = false;
		return true;
	}
	LogD() << "StepMotor::controlJogg() failed, name: " << getName();
	return false;
}


bool wso_device::StepMotor::controlStop(void)
{
	if (!isInitiated()) {
		return false;
	}
	/*
	if (!impl().isMoving) {
		return true;
	}
	*/

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	lock_guard<mutex> lock(impl().mutexMoveStop);
	if (usbComm.StepMotorStop(getMotorId())) {
		impl().isStopped = true;
		return true;
	}
	LogD() << "StepMotor::controlStop() failed, name: " << getName();
	return false;
}

bool wso_device::StepMotor::moveInVelocityMode(int direction)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	lock_guard<mutex> lock(impl().mutexMoveStop);
	if (usbComm.MotorMoveVelocity(getMotorId(), (uint8_t)direction)) {
		impl().isStopped = false;
		return true;
	}
	LogD() << "StepMotor::moveInVelocityMode() failed, name: " << getName();
	return false;
}

bool wso_device::StepMotor::stopInVelocityMode(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	lock_guard<mutex> lock(impl().mutexMoveStop);
	if (usbComm.MotorStopVelocity(getMotorId())) {
		impl().isStopped = true;
		return true;
	}
	LogD() << "StepMotor::stopInVelocityMode() failed, name: " << getName();
	return false;
}

bool wso_device::StepMotor::setVelocityProfile(int nAccelStop, int nMinSpeed, int nMaxSpeed)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.WriteMotorSpeedVelocity(getMotorId(), (uint32_t)nAccelStop, (uint32_t)nMinSpeed, (uint32_t)nMaxSpeed)) {
		return true;
	}
	LogD() << "StepMotor::setVelocityProfile() failed, name: " << getName() << ", acc_stop: " << nAccelStop << ", min_speed: " << nMinSpeed << ", max_speed: " << nMaxSpeed;
	return false;
}

bool wso_device::StepMotor::setDefaultVelocity(void)
{
	if (!isInitiated()) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.MotorSpeedDefaultVelocity(getMotorId())) {
		return true;
	}
	LogD() << "StepMotor::setDefaultVelocity() failed, name: " << getName();
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
				// Stage motors (X/Y/Z/swing) report their end switches through PI_status
				// (bit0: low PI hit, bit1: high PI hit, see wso_hbs.h StepMotorInfo_st).
				// Mirror them into limitStatus so isAtMin/MaxPosition() and the early
				// exit in waitForUpdate() reflect the hardware.
				if (isStageMotor()) {
					impl().limitStatus[0] = ((info->PI_status & 0x01) != 0) ? 1 : 0;
					impl().limitStatus[1] = ((info->PI_status & 0x02) != 0) ? 1 : 0;
				}
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
	// Polling the current position of moving motor.
	int count = 0;
	auto initPos = getPosition();

	while (!check) {
		std::this_thread::sleep_for(std::chrono::milliseconds(timeDelay));

		if (!updateStatus()) {
			break;
		}
		if (impl().isStopped) {
			check = true;
			break;
		}

		auto currPos = getPosition();
		auto targPos = impl().targetPos;

		if (abs(currPos - impl().targetPos) <= posOffset) {
			check = true;
			break;
		}
		else {
			if (isStageMotor()) {
				if (isAtMinPosition() || isAtMaxPosition()) {
					check = true;
					break;
				}
			}
		}

		if (++count >= countMax) {
			break;
		}
	}

	if (check) {
		int pos = getPosition();
		float value = getCurrentValueByPosition();
		CallbackRegistry::getInstance()->runStepMotorPositionChanged(getMotorType(), pos, value);
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


bool wso_device::StepMotor::isAtMinPosition(void) const
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


bool wso_device::StepMotor::isAtMaxPosition(void) const
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

