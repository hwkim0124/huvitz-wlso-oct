#include "pch.h"
#include "exts_hardware.h"

#include "Hardware.h"




void WSOSYSTEM_DLL_API __stdcall wso_system::connectJoystickButtonPressed(JoystickButtonPressedCallback clb)
{
	Hardware::getInstance()->connectJoystickButtonPressed(clb);
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::connectOptimizeButtonPressed(OptimizeButtonPressedCallback clb)
{
	Hardware::getInstance()->connectOptimizeButtonPressed(clb);
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::connectAdapterLensAttached(AdapterLensAttachedCallback clb)
{
	Hardware::getInstance()->connectAdapterLensAttached(clb);
	return ;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseJoystickButtonPressed(void)
{
	Hardware::getInstance()->releaseJoystickButtonPressed();
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseOptimizeButtonPressed(void)
{
	Hardware::getInstance()->releaseOptimizeButtonPressed();
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseAdapterLensAttached(void)
{
	Hardware::getInstance()->releaseAdapterLensAttached();
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::startCorneaCameraPreview(CorneaCameraFrameCaptured clb)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getCorneaCamera(); p) {
			inst->connectCorneaCameraImageCaptured(clb);
			return p->play();
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::closeCorneaCameraPreview(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getCorneaCamera(); p) {
			p->pause();
			inst->disconnectCorneaCameraImageCaptured();
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isCorneaCameraPreviewing(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getCorneaCamera(); p) {
			auto result = p->isPlaying();
			return result;
		}
	}
	return false;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getCorneaCameraAgain(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->getAnalogGain();
	}
	return 0.0f;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getCorneaCameraDgain(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->getDigitalGain();
	}
	return 0.0f;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraAgain(float value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setAnalogGain(value, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraDgain(float value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setDigitalGain(value, true);
	}
	return false;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getWorkingDotIntensity(int index)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		auto value = p->getWdotIntensity(index);
		return value;
	}
	return 0;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setWorkingDotIntensity(int index, int value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		auto result = p->setWdotIntensity(index, value);
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveChinrestUp(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->moveChinrestUp();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveChinrestDown(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->moveChinrestDown();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::stopChinrestMove(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->stopChinrest();
		return result;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isChinrestAtUpperEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto b = p->isChinrestAtHighLimit();
		return b;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isChinrestAtLowerEnd(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto b = p->isChinrestAtLowLimit();
		return b;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getDeviceSideOnPatient(EyeSide* side)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto b = p->getUsbComm().checkEyeSideGPIO(*side);
		return b;
	}
	return false;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getCurrentEyeSide(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto b = p->getEyeSide();
		return (int)b;
	}
	return 0;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOdOsStatusFlagSet(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		auto result = p->isOdOsStatusFlagSet();
		return result;
	}
	return false;
}


