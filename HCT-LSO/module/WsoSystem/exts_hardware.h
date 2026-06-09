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

		// Cornea Camera
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall startCorneaCameraPreview(CorneaCameraFrameCaptured clb);
		bool WSOSYSTEM_DLL_API __stdcall closeCorneaCameraPreview(void);
		bool WSOSYSTEM_DLL_API __stdcall isCorneaCameraPreviewing(void);
		float WSOSYSTEM_DLL_API __stdcall getCorneaCameraAgain(void);
		float WSOSYSTEM_DLL_API __stdcall getCorneaCameraDgain(void);
		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraAgain(float value);
		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraDgain(float value);

		int WSOSYSTEM_DLL_API __stdcall getWorkingDotIntensity(int index);
		bool WSOSYSTEM_DLL_API __stdcall setWorkingDotIntensity(int index, int value);


		// Chinrest
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall moveChinrestUp(void);
		bool WSOSYSTEM_DLL_API __stdcall moveChinrestDown(void);
		bool WSOSYSTEM_DLL_API __stdcall stopChinrestMove(void);
		bool WSOSYSTEM_DLL_API __stdcall isChinrestAtUpperEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isChinrestAtLowerEnd(void);


		// Device Status
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall getDeviceSideOnPatient(EyeSide* side);
		int WSOSYSTEM_DLL_API __stdcall getCurrentEyeSide(void);

		// PI Sensor Vaule
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall isOdOsStatusFlagSet(void);
	}
}