#include "pch.h"
#include "AxisController.h"
#include "StepMotor.h"


using namespace eye_align;


AxisController::AxisController() :
	motor_(nullptr), type_(MotorType::UNKNOWN), name_("none"),
	jogging_(false), direction_(-1), position_(0), atMin_(false), atMax_(false), commandFailed_(false)
{}


AxisController::AxisController(wso_device::StepMotor* motor, MotorType type, const char* name) :
	motor_(motor), type_(type), name_(name ? name : "axis"),
	jogging_(false), direction_(-1), position_(0), atMin_(false), atMax_(false), commandFailed_(false)
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

MotorType eye_align::AxisController::getType(void) const
{
	return type_;
}

const char* eye_align::AxisController::getName(void) const
{
	return name_.c_str();
}

bool eye_align::AxisController::jog(int dir)
{
	if (motor_ == nullptr) {
		return false;
	}
	dir = (dir != 0) ? 1 : 0;
	if (jogging_ && direction_ == dir) {
		return true;
	}
	if (jogging_) {
		if (!stop()) {
			return false;
		}
	}
	const bool ok = motor_->moveInVelocityMode(dir);
	commandFailed_ = !ok;
	if (ok) {
		jogging_ = true;
		direction_ = dir;
	}
	else {
		LogW() << "EyeAlign: " << name_ << " moveInVelocityMode(" << dir << ") rejected";
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
	atMin_ = motor_->isAtMinPosition();
	atMax_ = motor_->isAtMaxPosition();
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
	const int dir = (delta > 0) ? POSITIVE_STEP_DIR : (1 - POSITIVE_STEP_DIR);
	jog(dir);
	return false;
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

bool eye_align::AxisController::isAtMin(void) const
{
	return atMin_;
}

bool eye_align::AxisController::isAtMax(void) const
{
	return atMax_;
}

bool eye_align::AxisController::isAtLimitInDirection(void) const
{
	if (!jogging_) {
		return false;
	}
	return (direction_ == 0) ? atMin_ : atMax_;
}

bool eye_align::AxisController::hasCommandFailed(void) const
{
	return commandFailed_;
}
