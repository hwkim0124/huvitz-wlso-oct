#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		bool WSOSYSTEM_DLL_API _stdcall isSloScannerWorking(void);
		bool WSOSYSTEM_DLL_API _stdcall isSloScannerGrabbing(void);
		bool WSOSYSTEM_DLL_API _stdcall isSloScannerChannelOn(SloColorChannel channel);
		int WSOSYSTEM_DLL_API _stdcall getSloScannerChannelCount(void);

		bool WSOSYSTEM_DLL_API _stdcall turnSloScannerSwitchOn(bool flag);
		bool WSOSYSTEM_DLL_API _stdcall startSloScannerGrabbing(SloScanFrameDataAcquired clb_frame, SloScanFrameImageCaptured clb_image);
		void WSOSYSTEM_DLL_API _stdcall pauseSloScannerGrabbing(void);

		bool WSOSYSTEM_DLL_API _stdcall moveSloGalvanoPositionY(int value);

		bool WSOSYSTEM_DLL_API _stdcall submitSloPixelDataScalerParam(const SloPixelDataScalerParam* param);
		bool WSOSYSTEM_DLL_API _stdcall obtainSloPixelDataScalerParam(SloPixelDataScalerParam* param);
		void WSOSYSTEM_DLL_API _stdcall resetSloPixelDataScalerParam(void);

		bool WSOSYSTEM_DLL_API _stdcall obtainSloScannerControlParam(SloScannerControlParam* param);
		bool WSOSYSTEM_DLL_API _stdcall applySloScannerControlParam(const SloScannerControlParam* param);
		bool WSOSYSTEM_DLL_API _stdcall submitSloScannerAdcGain(SloColorChannel channel, float gain);
		bool WSOSYSTEM_DLL_API _stdcall submitSloScannerAdcOffset(SloColorChannel channel, float offset);
		
		bool WSOSYSTEM_DLL_API _stdcall obtainSloDetectorStatus(SloDetectorStatus* status, bool reload);
		bool WSOSYSTEM_DLL_API _stdcall submitSloDetectorGain(SloColorChannel channel, float gain);
	}

}