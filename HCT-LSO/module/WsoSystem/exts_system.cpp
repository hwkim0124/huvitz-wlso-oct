#include "pch.h"
#include "exts_system.h"

#include "Bootstrapper.h"
#include "Hardware.h"

using namespace wso_system;


bool WSOSYSTEM_DLL_API __stdcall wso_system::initializeWsoSystem(wso_domain::WsoLogMsgCallback clb, bool trace_mode)
{
	if (auto* bsp = Bootstrapper::getInstance(); bsp) {
		bsp->initializeWsoSystem(clb, trace_mode);
	}
	return true;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseWsoSystem(void)
{
	if (auto* bsp = Bootstrapper::getInstance(); bsp) {
		bsp->releaseWsoSystem();
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isWsoSystemInitialized(void)
{
	bool flag = Bootstrapper::getInstance()->isWsoSystemInitialized();
	return flag;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::connectWsoLogMsgCallback(wso_domain::WsoLogMsgCallback clb)
{
	if (auto* bsp = Bootstrapper::getInstance(); bsp) {
		bsp->connectWsoLogMsgCallback(clb);
	}
}

void WSOSYSTEM_DLL_API __stdcall wso_system::releaseWsoLogMsgCallback(void)
{
	if (auto* inst = Bootstrapper::getInstance(); inst) {
		inst->releaseWsoLogMsgCallback();
	}
}
