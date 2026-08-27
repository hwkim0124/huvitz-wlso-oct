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


