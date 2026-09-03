#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// Cornea Camera
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall startCorneaCameraPreview(CorneaCameraFrameCaptured clbLeft, CorneaCameraFrameCaptured clbRight, CorneaCameraFrameCaptured clbLower);
		bool WSOSYSTEM_DLL_API __stdcall closeCorneaCameraPreview(void);
		bool WSOSYSTEM_DLL_API __stdcall isCorneaCameraPreviewing(void);
		
		float WSOSYSTEM_DLL_API __stdcall getCorneaCameraAgain(IrCameraId camId);
		float WSOSYSTEM_DLL_API __stdcall getCorneaCameraDgain(IrCameraId camId);
		float WSOSYSTEM_DLL_API __stdcall getCorneaCameraExposureTime(IrCameraId camId);

		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraAgain(IrCameraId camId, float value);
		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraDgain(IrCameraId camId, float value);
		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraExposureTime(IrCameraId camId, float value);

		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraViewPreset(IrCameraViewPreset preset);
		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraMirrorMode(IrCameraId camId, IrCameraMirrorMode hflip, IrCameraMirrorMode vflip);
		bool WSOSYSTEM_DLL_API __stdcall setCorneaCameraOutputMode(IrCameraId camId, IrCameraOutputMode mode, IrCameraTestPattern pattern, int option);
		bool WSOSYSTEM_DLL_API __stdcall getCorneaCameraMirrorMode(IrCameraId camId, IrCameraMirrorMode* hflip, IrCameraMirrorMode* vflip);
		bool WSOSYSTEM_DLL_API __stdcall getCorneaCameraOutputMode(IrCameraId camId, IrCameraOutputMode* mode, IrCameraTestPattern* pattern, int* option);

		// Retina Camera
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall startRetinaCameraPreview(RetinaCameraFrameCaptured clbFrame);
		bool WSOSYSTEM_DLL_API __stdcall closeRetinaCameraPreview(void);
		bool WSOSYSTEM_DLL_API __stdcall isRetinaCameraPreviewing(void);

		float WSOSYSTEM_DLL_API __stdcall getRetinaCameraAgain(void);
		float WSOSYSTEM_DLL_API __stdcall getRetinaCameraDgain(void);
		float WSOSYSTEM_DLL_API __stdcall getRetinaCameraExposureTime(void);

		bool WSOSYSTEM_DLL_API __stdcall setRetinaCameraAgain(float value);
		bool WSOSYSTEM_DLL_API __stdcall setRetinaCameraDgain(float value);
		bool WSOSYSTEM_DLL_API __stdcall setRetinaCameraExposureTime(float value);
	}
}
