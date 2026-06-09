#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API _stdcall isOctScanWorking(void);
		bool WSOSYSTEM_DLL_API _stdcall isOctScanPrevewing(void);
		bool WSOSYSTEM_DLL_API _stdcall isOctScanMeasuring(void);
		bool WSOSYSTEM_DLL_API _stdcall isOctScanOptimizing(void);

		bool WSOSYSTEM_DLL_API _stdcall setupOctScanProtocol(OctProtocolInitParam param);
		bool WSOSYSTEM_DLL_API _stdcall startOctScanPreview(OctPreviewImageCaptured clb_image, OctEnfaceImageCaptured clb_enface);
		bool WSOSYSTEM_DLL_API _stdcall closeOctScanPreview(bool to_measure, OctScanProtocolCompleted clb_protocol);
		void WSOSYSTEM_DLL_API _stdcall cancelOctScanning(void);
		
		bool WSOSYSTEM_DLL_API _stdcall startOctScanAutoDiopterFocus(OctAutoFocusOptimized clb_focus);
		bool WSOSYSTEM_DLL_API _stdcall startOctScanAutoReference(OctAutoReferOptimized clb_refer);
		bool WSOSYSTEM_DLL_API _stdcall startOctScanAutoPolarization(OctAutoPolarOptimized clb_polar);
		bool WSOSYSTEM_DLL_API _stdcall startOctScanAutoOptimize(OctAutoScanOptimized clb_scan);
		bool WSOSYSTEM_DLL_API _stdcall cancelOctScanAutoOptimizing(void);

		void WSOSYSTEM_DLL_API _stdcall connectOctSpectrumDataCaptured(OctSpectrumDataCaptured clb_spectrum);
		void WSOSYSTEM_DLL_API _stdcall connectOctResampleDataCaptured(OctResampleDataCaptured clb_resample);
		void WSOSYSTEM_DLL_API _stdcall connectOctIntensityDataCaptured(OctIntensityDataCaptured clb_intensity);
		void WSOSYSTEM_DLL_API _stdcall releaseOctSpectrumDataCaptured(void);
		void WSOSYSTEM_DLL_API _stdcall releaseOctResampleDataCaptured(void);
		void WSOSYSTEM_DLL_API _stdcall releaseOctIntensityDataCaptured(void);
	}
}