#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall turnOnInternalFixation(int row, int col);
		bool WSOSYSTEM_DLL_API __stdcall turnOnInternalFixationWithTarget(EyeSide side, FixationTarget target);
		bool WSOSYSTEM_DLL_API __stdcall turnOffInternalFixation(void);
		bool WSOSYSTEM_DLL_API __stdcall getCurrentInternalFixation(int* row, int* col) ;

		bool WSOSYSTEM_DLL_API __stdcall setupInternalFixation(InternalFixationParam param);
	}
}