#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// External Keys 
		///////////////////////////////////////////////////////////////////////////////////////////
		void WSOSYSTEM_DLL_API __stdcall connectJoystickButtonPressed(JoystickButtonPressedCallback clb);
		void WSOSYSTEM_DLL_API __stdcall connectOptimizeButtonPressed(OptimizeButtonPressedCallback clb);
		void WSOSYSTEM_DLL_API __stdcall connectAdapterLensAttached(AdapterLensAttachedCallback clb);
		void WSOSYSTEM_DLL_API __stdcall releaseJoystickButtonPressed(void);
		void WSOSYSTEM_DLL_API __stdcall releaseOptimizeButtonPressed(void);
		void WSOSYSTEM_DLL_API __stdcall releaseAdapterLensAttached(void);

		// Device Status
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall getDeviceSideOnPatient(EyeSide* side);
		int WSOSYSTEM_DLL_API __stdcall getCurrentEyeSide(void);

		// System Initialization Status
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall obtainSystemInitStatus(SysInitStatus* status, bool reload);
		int WSOSYSTEM_DLL_API __stdcall getSysInitStatusStructSize(void);
		int WSOSYSTEM_DLL_API __stdcall getSubsystemName(int bit, char* buf, int len);
		int WSOSYSTEM_DLL_API __stdcall getSubsystemErrorText(int bit, int code, char* buf, int len);

		// PI Sensor Vaule
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall isOdOsStatusFlagSet(void);
	}
}