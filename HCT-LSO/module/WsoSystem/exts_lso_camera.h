#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// Main Camera
		//////////////////////////////////////////////////////////////////////////////////////////
		void WSOSYSTEM_DLL_API __stdcall startColorCameraLive(ColorCameraFrameCaptured clbFrame);
		void WSOSYSTEM_DLL_API __stdcall pauseColorCameraLive(void);
		bool WSOSYSTEM_DLL_API __stdcall isColorCameraLive(void);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraSingleFrameLive(ColorCameraFrameCaptured clbFrame);
		void WSOSYSTEM_DLL_API __stdcall startColorCameraSeqROILive(ColorCameraSeqROIFrameCaptured clbFrame, const LsoCaptureFrameSeqROIPreset* preset);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraSwTriggerOffsetROILive(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset);
		void WSOSYSTEM_DLL_API __stdcall startColorCameraSwTriggerOffsetROICapture(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraRollSwTrigOverlapLive(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset);
		void WSOSYSTEM_DLL_API __stdcall startColorCameraRollSwTrigOverlapCapture(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraSwTriggerLive(ColorCameraFrameCaptured clbFrame);
		void WSOSYSTEM_DLL_API __stdcall stopColorCameraSwTriggerLive(void);
		void WSOSYSTEM_DLL_API __stdcall shootColorCameraSwTriggerLive(void);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraHwTriggerLive(ColorCameraImageCaptured clbImage);
		void WSOSYSTEM_DLL_API __stdcall stopColorCameraHwTriggerLive(void);
		bool WSOSYSTEM_DLL_API __stdcall isColorCameraIsStreaming(void);
		void WSOSYSTEM_DLL_API __stdcall setupColorCameraHwTriggerLive(void);

		void WSOSYSTEM_DLL_API __stdcall startColorCameraOriginal(ColorCameraImageCaptured clbImage);
		void WSOSYSTEM_DLL_API __stdcall pauseColorCameraOriginal(void);

		void WSOSYSTEM_DLL_API __stdcall getColorCameraSettingParam(ColorCameraSettingParam* pParam);
		void WSOSYSTEM_DLL_API __stdcall setColorCameraSettingParam(const ColorCameraSettingParam* pParam);

	}
}