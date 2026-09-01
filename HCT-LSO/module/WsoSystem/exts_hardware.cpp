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

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainSystemInitStatus(SysInitStatus* status, bool reload)
{
	if (auto* p = Hardware::getInstance()->getMainBoard(); p) {
		if (reload) {
			p->reloadSystemInitStatus();
		}
		return p->fetchSystemInitStatus(status);
	}
	return false;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getSysInitStatusStructSize(void)
{
	return (int)sizeof(SysInitStatus);
}

// Copies at most len-1 characters plus a terminator. Returns the number of
// characters written, or 0 when there is nothing to report.
static int copyTextToBuffer(const char* text, char* buf, int len)
{
	if (!buf || len <= 0) {
		return 0;
	}
	buf[0] = '\0';
	if (!text) {
		return 0;
	}

	int i = 0;
	for (; text[i] != '\0' && i < (len - 1); i++) {
		buf[i] = text[i];
	}
	buf[i] = '\0';
	return i;
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getSubsystemName(int bit, char* buf, int len)
{
	if (bit < 0 || bit >= (int)SubsystemBit::NUM_SUBSYSTEMS) {
		return copyTextToBuffer("", buf, len);
	}
	return copyTextToBuffer(SubsystemBitToName(bit), buf, len);
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getSubsystemErrorText(int bit, int code, char* buf, int len)
{
	if (bit < 0 || bit >= (int)SubsystemBit::NUM_SUBSYSTEMS) {
		return copyTextToBuffer("", buf, len);
	}
	return copyTextToBuffer(SubsystemErrorToText(bit, code), buf, len);
}


