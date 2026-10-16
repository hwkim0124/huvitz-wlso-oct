#include "pch.h"
#include "exts_device_motors.h"

#include "Hardware.h"


bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchStepMotorStatus(MotorType type, StepMotorStatus* status)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		if (motor->fetchStatus(status)) {
			return true;
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::connectStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb)
{
	Hardware::getInstance()->connectStepMotorPositionChanged(type, clb);
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseStepMotorPositionChanged(MotorType type)
{
	Hardware::getInstance()->disconnectStepMotorPositionChanged(type);
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorPosition(MotorType type, int pos)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		if (motor->updatePosition(pos)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorByPositionOffset(MotorType type, int offset)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		if (motor->moveByStepOffset(offset)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorByValue(MotorType type, float value)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		return motor->updatePositionByValue(value);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorByValueOffset(MotorType type, float offset)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		return motor->updatePositionByValueOffset(offset);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorToOrigin(MotorType type, int mode)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		return motor->updatePositionToOrigin(mode);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorToUpperEnd(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto target = motor->getRangeMax();
		if (motor->updatePosition(target)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorToLowerEnd(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto target = motor->getRangeMin();
		if (motor->updatePosition(target)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorToCenter(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto target = motor->getCenterPosition();
		if (motor->updatePosition(target)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isStepMotorAtUpperEnd(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto result = motor->isAtMaxPosition();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isStepMotorAtLowerEnd(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto result = motor->isAtMinPosition();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isStepMotorAtCenter(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto result = motor->isAtCenterOfPosition();
		return result;
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::stopStepMotor(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		motor->controlStop();
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setStepMotorCurrentPositionAsOrigin(MotorType type, int mode)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		switch (type) {
			using enum MotorType;
			case OCT_FOCUS:
			default:
				motor->setCurrentPositionAsOrigin(mode);
				break;
		}
	}
	return false;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getStepMotorPosition(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		return motor->getPosition();
	}
	return 0;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getStepMotorPositionAtValue(MotorType type, float value)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		int pos = motor->getPositionAtValue(value);
		return pos;
	}
	return 0;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getStepMotorValue(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		float value = 0.0f;
		switch (type) {
			using enum MotorType;
			case OCT_FOCUS:
				value = ((OctFocusMotor*)motor)->getCurrentDiopter();
				break;
			default:
				value = motor->getCurrentValueByPosition();
				break;
		}
		return value;
	}
	return 0.0f;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getStepMotorValueAtPosition(MotorType type, int pos)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		float value = 0.0f;
		switch (type) {
			using enum MotorType;
			case OCT_FOCUS:
			default:
				value = motor->getValueAtPosition(pos);
				break;
		}
		return value;
	}
	return 0.0f;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveHeadLeft(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getXstageMotor(); motor) {
		return motor->moveInVelocityMode(1);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveHeadRight(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getXstageMotor(); motor) {
		return motor->moveInVelocityMode(0);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveHeadUp(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getYstageMotor(); motor) {
		return motor->moveInVelocityMode(0);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveHeadDown(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getYstageMotor(); motor) {
		return motor->moveInVelocityMode(1);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveHeadForward(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getZstageMotor(); motor) {
		return motor->moveInVelocityMode(1);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveHeadBackward(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getZstageMotor(); motor) {
		return motor->moveInVelocityMode(0);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopHeadX(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getXstageMotor(); motor) {
		return motor->stopInVelocityMode();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopHeadY(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getYstageMotor(); motor) {
		return motor->stopInVelocityMode();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopHeadZ(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getZstageMotor(); motor) {
		return motor->stopInVelocityMode();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopHeadXYZ(void)
{
	bool result = true;
	result &= stopHeadX();
	result &= stopHeadY();
	result &= stopHeadZ();
	return result;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isHeadAtLeftEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getXstageMotor(); p) {
		return p->isAtMinPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isHeadAtRightEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getXstageMotor(); p) {
		return p->isAtMaxPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isHeadAtLowerEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getYstageMotor(); p) {
		return p->isAtMinPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isHeadAtUpperEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getYstageMotor(); p) {
		return p->isAtMaxPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isHeadAtForwardEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getZstageMotor(); p) {
		return p->isAtMaxPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isHeadAtBackwardEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getZstageMotor(); p) {
		return p->isAtMinPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveSwingLeft(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getSwingMotor(); motor) {
		return motor->moveInVelocityMode(0);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveSwingRight(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getSwingMotor(); motor) {
		return motor->moveInVelocityMode(1);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopSwingMove(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getSwingMotor(); motor) {
		return motor->stopInVelocityMode();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isSwingAtLeftEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSwingMotor(); p) {
		return p->isAtMinPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isSwingAtRightEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSwingMotor(); p) {
		return p->isAtMaxPosition();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveChinrestUp(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getChinRestMotor(); motor) {
		return motor->startMovingUp();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveChinrestDown(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getChinRestMotor(); motor) {
		return motor->startMovingDown();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopChinrest(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getChinRestMotor(); motor) {
		return motor->stopMoving();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isChinrestAtUpperEnd(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getChinRestMotor(); motor) {
		return motor->isAtUpperLimit();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isChinrestAtLowerEnd(void)
{
	if (auto* motor = Hardware::getInstance()->getMainBoard()->getChinRestMotor(); motor) {
		return motor->isAtLowerLimit();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctRefNdMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctRefNdMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctAntLensMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctAntLensMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isRetMirrorMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getRetMirrorMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isLsoFilterMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getLsoFilterMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isLsoFocusMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getLsoFocusMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctPolarMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctPolarMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctReferMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctReferMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctFocusMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctFocusMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isSwingMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSwingMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isZstageMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getZstageMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isXstageMotorAtPiPosition(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getXstageMotor(); p) {
		return p->isAtPiPostion();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isYstageMotorAtUpperLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getYstageMotor(); p) {
		return p->isAtUpperLimit();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isYstageMotorAtLowerLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getYstageMotor(); p) {
		return p->isAtLowerLimit();
	}
	return false;
}

