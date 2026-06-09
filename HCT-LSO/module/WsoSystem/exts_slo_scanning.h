#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	extern "C"
	{
		bool WSOSYSTEM_DLL_API _stdcall isSloScanPrevewing(void);
		bool WSOSYSTEM_DLL_API _stdcall isSloScanMeasuring(void);
		bool WSOSYSTEM_DLL_API _stdcall isSloScanOptimizing(void);

		bool WSOSYSTEM_DLL_API _stdcall setupSloScanCapture(SloCaptureInitParam param);
		bool WSOSYSTEM_DLL_API _stdcall startSloScanPreview(SloScanPreviewImageCaptured clb_preview, SloScanPreviewFrameAcquired clb_frame);
		bool WSOSYSTEM_DLL_API _stdcall enterSloScanMeasure(SloCaptureTakeParam param, SloScanMeasureImageCaptured clb_measure, SloScanMeasureFrameAcquired clb_frame);
		bool WSOSYSTEM_DLL_API _stdcall exitSloScanMeasure(bool to_preview);
		void WSOSYSTEM_DLL_API _stdcall cancelSloScan(void);

		bool WSOSYSTEM_DLL_API _stdcall startSloAutoDiopterFocus(SloAutoFocusOptimized clb_focus, bool with_gain);
		bool WSOSYSTEM_DLL_API _stdcall startSloAutoGainControl(SloAutoGainOptimized clb_focus);
		bool WSOSYSTEM_DLL_API _stdcall cancelSloAutoOptimizing(void);
	}
}