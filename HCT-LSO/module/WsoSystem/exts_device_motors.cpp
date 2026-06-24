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
		if (motor->updatePositionByOffset(offset)) {
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorByValue(MotorType type, float value)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		switch (type) {
			using enum MotorType;
		case OCT_FOCUS:
		default:
			motor->updatePositionByValue(value);
			break;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorByValueOffset(MotorType type, float offset)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		switch (type) {
			using enum MotorType;
		case OCT_FOCUS:
		default:
			motor->updatePositionByValueOffset(offset);
			break;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveStepMotorToOrigin(MotorType type, int mode)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		switch (type) {
			using enum MotorType;
			case OCT_FOCUS:
			default:
				motor->updatePositionToOrigin(mode);
				break;
		}
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
		auto result = motor->isEndOfUpperPosition();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isStepMotorAtLowerEnd(MotorType type)
{
	if (auto* motor = Hardware::getInstance()->getStepMotor(type); motor) {
		auto result = motor->isEndOfLowerPosition();
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
			default:
				value = ((OctFocusMotor*)motor)->getCurrentDiopter();
				return value;
		}
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
				value = ((OctFocusMotor*)motor)->getValueAtPosition(pos);
				return value;
		}
	}
	return 0.0f;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isSwingMotorAtHighLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isSwingMotorAtHighLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isSwingMotorAtLowLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isSwingMotorAtLowLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctFocusMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isOctFocusMotorAtOrigin();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctPolarMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isOctPolarMotorAtOrigin();
		return result;  
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctReferMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isOctReferMotorAtOrigin();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctRefNdMotorAtOrigin(void)
{
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isLsoFocusMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isLsoFocusMotorAtOrigin();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isRetMirrorMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isRetMirrorMotorAtOrigin();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctAntLensMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isOctAntLensMotorAtOrigin();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isLsoFilterMotorAtOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isLsoFilterMotorAtOrigin();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isXstageMotorAtHighLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isXstageMotorAtHighLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isXstageMotorAtLowLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isXstageMotorAtLowLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isYstageMotorAtHighLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isYstageMotorAtHighLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isYstageMotorAtLowLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isYstageMotorAtLowLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isZstageMotorAtHighLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isZstageMotorAtHighLimit();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isZstageMotorAtLowLimit(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isZstageMotorAtLowLimit();
		return result;
	}
	return false;
}
