#include "pch.h"
#include "exts_system.h"
#include "exts_calibration.h"

#include "wso_board.h"
#include "Calibration.h"

using namespace wso_system;
using namespace wso_board;

bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchSystemCalibration(SystemCalibration* sys_calib, bool reload)
{
	if (auto* p = Calibration::getInstance(); p && sys_calib) {
		return p->obtainSystemCalibration(sys_calib, reload);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::applySystemCalibration(const SystemCalibration* sys_calib, bool write)
{
	if (auto* p = Calibration::getInstance(); p && sys_calib) {
		return p->submitSystemCalibration(sys_calib, write);
	}
	return false;
}
