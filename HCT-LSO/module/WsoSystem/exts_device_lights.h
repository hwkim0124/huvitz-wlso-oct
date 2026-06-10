#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// SLO sources 
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall turnOnLight(LightType type);
		bool WSOSYSTEM_DLL_API __stdcall turnOffLight(LightType type);
		bool WSOSYSTEM_DLL_API __stdcall isLightOn(LightType type);

		bool WSOSYSTEM_DLL_API __stdcall obtainAdcSensorStatus(AdcSensorStatus* status, bool reload);


		// OCT sources 
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall obtainOctSldStatusParam(OctSldStatusParam* status, bool reload);
		bool WSOSYSTEM_DLL_API __stdcall obtainOctSldCalibParam(OctSldCalibParam* calib, bool reload);

		bool WSOSYSTEM_DLL_API __stdcall setOctSldHighCode(int value);
		bool WSOSYSTEM_DLL_API __stdcall setOctSldLowCode1(int value);
		bool WSOSYSTEM_DLL_API __stdcall setOctSldLowCode2(int value);
		bool WSOSYSTEM_DLL_API __stdcall setOctSldRsiCode(int value);

		bool WSOSYSTEM_DLL_API __stdcall saveOctSldCalibration(void);
		bool WSOSYSTEM_DLL_API __stdcall loadOctSldCalibration(void);


	}
}