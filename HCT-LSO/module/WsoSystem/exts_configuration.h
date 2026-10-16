#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall obtainInternalFixationPreset(InternalFixationPreset* param);
		bool WSOSYSTEM_DLL_API __stdcall obtainCorneaCameraConfigParam(CorneaCameraConfigParam* param);
		bool WSOSYSTEM_DLL_API __stdcall obtainRetinaCameraConfigParam(RetinaCameraConfigParam* param);

		void WSOSYSTEM_DLL_API __stdcall submitInternalFixationPreset(InternalFixationPreset param);
		void WSOSYSTEM_DLL_API __stdcall submitCorneaCameraConfigParam(CorneaCameraConfigParam param); 
		void WSOSYSTEM_DLL_API __stdcall submitRetinaCameraConfigParam(RetinaCameraConfigParam param);

		bool WSOSYSTEM_DLL_API __stdcall obtainEyeAlignConfigParam(EyeAlignConfigParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitEyeAlignConfigParam(const EyeAlignConfigParam* param);

		bool WSOSYSTEM_DLL_API __stdcall importSystemConfigFile(void);
		bool WSOSYSTEM_DLL_API __stdcall exportSystemConfigFile(void);

		bool WSOSYSTEM_DLL_API __stdcall loadSystemConfiguration(void);
		bool WSOSYSTEM_DLL_API __stdcall saveSystemConfiguration(void);
		bool WSOSYSTEM_DLL_API __stdcall applySystemConfiguration(void);
	}
}