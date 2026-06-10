#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall obtainInternalFixationPreset(InternalFixationPreset* param);
		bool WSOSYSTEM_DLL_API __stdcall obtainCorneaCameraConfigParam(CorneaCameraConfigParam* param);
		bool WSOSYSTEM_DLL_API __stdcall obtainOctGalvanoConfigParam(OctGalvanoConfigParam* param);

		void WSOSYSTEM_DLL_API __stdcall submitInternalFixationPreset(InternalFixationPreset param);
		void WSOSYSTEM_DLL_API __stdcall submitCorneaCameraConfigParam(CorneaCameraConfigParam param);
		void WSOSYSTEM_DLL_API __stdcall submitOctGalvanoConfigParam(OctGalvanoConfigParam param);

		bool WSOSYSTEM_DLL_API __stdcall loadSystemConfigFile(void);
		bool WSOSYSTEM_DLL_API __stdcall saveSystemConfigFile(void);

		bool WSOSYSTEM_DLL_API __stdcall loadSystemConfiguration(void);
		bool WSOSYSTEM_DLL_API __stdcall saveSystemConfiguration(void);
		bool WSOSYSTEM_DLL_API __stdcall applySystemConfiguration(void);
	}
}
