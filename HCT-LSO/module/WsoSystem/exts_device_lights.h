#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// SLO sources 
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall obtainQldStatusParam(LightType type, QldStatusParam* param, bool reload);
		bool WSOSYSTEM_DLL_API __stdcall obtainQldChannelStatus(LightType type, QldStatusParam* param, bool reload);
		bool WSOSYSTEM_DLL_API __stdcall obtainQldChannelHeader(LightType type, QldStatusParam* param, bool reload);
		bool WSOSYSTEM_DLL_API __stdcall obtainQldChannelProfile(LightType type, int profile, QldStatusParam* param, bool reload);
		
		bool WSOSYSTEM_DLL_API __stdcall submitQldChannelHeader(LightType type, const QldStatusParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitQldChannelProfile(LightType type, int profile, const QldStatusParam* param);

		bool WSOSYSTEM_DLL_API __stdcall commitQldChannelHeaderToMemory(LightType type);
		bool WSOSYSTEM_DLL_API __stdcall commitQldChannelProfileToMemory(LightType type, int profile);
		bool WSOSYSTEM_DLL_API __stdcall fetchQldChannelHeaderFromMemory(LightType type);
		bool WSOSYSTEM_DLL_API __stdcall fetchQldChannelProfileFromMemory(LightType type, int profile);

		bool WSOSYSTEM_DLL_API __stdcall turnOnLight(LightType type);
		bool WSOSYSTEM_DLL_API __stdcall turnOffLight(LightType type);
		bool WSOSYSTEM_DLL_API __stdcall isLightOn(LightType type);

		bool WSOSYSTEM_DLL_API __stdcall setQldApcAccMode(LightType type, int mode);
		bool WSOSYSTEM_DLL_API __stdcall setQldCurrentLimitLevel(LightType type, int limit);
		bool WSOSYSTEM_DLL_API __stdcall setQldReferenceIntensity(LightType type, int profile, int normal, int value);
		bool WSOSYSTEM_DLL_API __stdcall setQldApcGainResistorDcode(LightType type, int profile, int dcode);
		bool WSOSYSTEM_DLL_API __stdcall setQldActiveProfile(LightType type, int profile, int normal);

		bool WSOSYSTEM_DLL_API __stdcall obtainQldControlResult(LightType type, int* result, bool reload);
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