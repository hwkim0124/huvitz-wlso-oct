#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall fetchSystemCalibration(wso_board::HbsCalibration* sys_calib, bool reload);
		bool WSOSYSTEM_DLL_API __stdcall applySystemCalibration(const wso_board::HbsCalibration* sys_calib, bool write);

	}
}