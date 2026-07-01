#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall obtainLsoScannerControlParam(int patternId, LsoScannerControlParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitLsoScannerControlParam(int patternId, const LsoScannerControlParam* param);
		bool WSOSYSTEM_DLL_API __stdcall obtainLsoScannerCaptureParam(int patternId, LsoScannerCaptureParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitLsoScannerCaptureParam(int patternId, const LsoScannerCaptureParam* param);

		bool WSOSYSTEM_DLL_API __stdcall moveLsoScannerYposition(int ypos);
		bool WSOSYSTEM_DLL_API __stdcall controlLsoScannerCapture(int patternId, int onOff);
		bool WSOSYSTEM_DLL_API __stdcall setLsoScannerTriggerMode(int onOff);
		bool WSOSYSTEM_DLL_API __stdcall startLsoScannerGrabbing(int patternId);
		bool WSOSYSTEM_DLL_API __stdcall pauseLsoScannerGrabbing(int patternId);
	}
}