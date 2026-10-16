#include "pch.h"
#include "AxisController.h"
#include "StepMotor.h"


using namespace eye_align;


AxisController::AxisController() :
	motor_(nullptr), type_(MotorType::UNKNOWN), name_("none"),
	jogging_(false), direction_(-1), position_(0), atMinimumEnd_(false), atMaximumEnd_(false), commandFailed_(false)
{}


AxisController::AxisController(wso_device::StepMotor* motor, MotorType type, const char* name) :
	motor_(motor), type_(type), name_(name ? name : "axis"),
	jogging_(false), direction_(-1), position_(0), atMinimumEnd_(false), atMaximumEnd_(false), commandFailed_(false)
{}


AxisController::~AxisController()
{}


bool eye_align::AxisController::isValid(void) const
{
	return motor_ != nullptr;
}

bool eye_align::AxisController::isReady(void) const
{
	return motor_ != nullptr && motor_->isInitiated();
}

MotorType eye_align::AxisController::getMotorType(void) const
{
	return type_;
}

const char* eye_align::AxisController::getName(void) const
{
	return name_.c_str();
}

bool eye_align::AxisController::jog(int direction)
{
	if (motor_ == nullptr) {
		return false;
	}
	direction = (direction != 0) ? 1 : 0;
	if (jogging_ && direction_ == direction) {
		return true;
	}
	if (jogging_) {
		if (!stop()) {
			return false;
		}
	}
	const bool ok = motor_->moveInVelocityMode(direction);
	commandFailed_ = !ok;
	if (ok) {
		jogging_ = true;
		direction_ = direction;
	}
	else {
		LogW() << "EyeAlign: " << name_ << " moveInVelocityMode(" << direction << ") rejected";
	}
	return ok;
}

bool eye_align::AxisController::stop(void)
{
	if (motor_ == nullptr || !jogging_) {
		return true;
	}
	const bool ok = motor_->stopInVelocityMode();
	commandFailed_ = !ok;
	// Consider the jog ended even when the stop was rejected: a retry from the caller
	// (stopAllAxes) must not be skipped because we believe we are still jogging.
	jogging_ = false;
	direction_ = -1;
	if (!ok) {
		LogW() << "EyeAlign: " << name_ << " stopInVelocityMode() rejected";
	}
	return ok;
}

bool eye_align::AxisController::poll(void)
{
	if (motor_ == nullptr) {
		return false;
	}
	const bool ok = motor_->updateStatus();
	position_ = motor_->getPosition();
	atMinimumEnd_ = motor_->isAtMinPosition();
	atMaximumEnd_ = motor_->isAtMaxPosition();
	return ok;
}

bool eye_align::AxisController::jogToward(int target, int tolerance)
{
	if (motor_ == nullptr) {
		return false;
	}
	if (tolerance < 0) tolerance = 0;

	const int delta = target - position_;
	if (std::abs(delta) <= tolerance) {
		stop();
		return true;
	}
	const int direction = (delta > 0) ? POSITIVE_STEP_DIRECTION : (1 - POSITIVE_STEP_DIRECTION);
	jog(direction);
	return false;
}

bool eye_align::AxisController::moveTo(int target)
{
	if (motor_ == nullptr) {
		return false;
	}
	if (jogging_ && !stop()) {
		return false;
	}
	const bool ok = motor_->moveToPosition(target, false);
	commandFailed_ = !ok;
	if (!ok) {
		LogW() << "EyeAlign: " << name_ << " moveToPosition(" << target << ") failed";
	}
	else {
		LogD() << "EyeAlign: " << name_ << " moved to " << target;
	}
	poll();
	return ok;
}

bool eye_align::AxisController::moveBy(int offset)
{
	if (motor_ == nullptr) {
		return false;
	}
	if (jogging_ && !stop()) {
		return false;
	}
	const bool ok = motor_->moveByStepOffset(offset, true);
	commandFailed_ = !ok;
	if (!ok) {
		LogW() << "EyeAlign: " << name_ << " moveByStepOffset(" << offset << ") failed";
	}
	else {
		LogD() << "EyeAlign: " << name_ << " moving by " << offset << " from " << position_ << " to " << (position_ + offset);
	}
	poll();
	return ok;
}

bool eye_align::AxisController::abortMove(void)
{
	if (motor_ == nullptr) {
		return false;
	}
	return motor_->controlStop();
}

bool eye_align::AxisController::isJogging(void) const
{
	return jogging_;
}

int eye_align::AxisController::getDirection(void) const
{
	return jogging_ ? direction_ : -1;
}

int eye_align::AxisController::getPosition(void) const
{
	return position_;
}

bool eye_align::AxisController::isAtMinimumEnd(void) const
{
	return atMinimumEnd_;
}

bool eye_align::AxisController::isAtMaximumEnd(void) const
{
	return atMaximumEnd_;
}

bool eye_align::AxisController::isJoggingIntoEndSwitch(void) const
{
	if (!jogging_) {
		return false;
	}
	return (direction_ == 0) ? atMinimumEnd_ : atMaximumEnd_;
}

bool eye_align::AxisController::hasCommandFailed(void) const
{
	return commandFailed_;
}
