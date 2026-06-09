#include "pch.h"
#include "exts_system.h"

#include "Configuration.h"
#include "exts_configuration.h"

using namespace wso_system;


bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchSystemCalibration(wso_board::HbsCalibration* sys_calib, bool reload)
{
	if (auto* p = Configuration::getInstance(); p && sys_calib) {
		return p->obtainSystemCalibration(sys_calib, reload);
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::applySystemCalibration(const wso_board::HbsCalibration* sys_calib, bool write)
{
	if (auto* p = Configuration::getInstance(); p && sys_calib) {
		return p->submitSystemCalibration(sys_calib, write);
	}
	return false;
}
