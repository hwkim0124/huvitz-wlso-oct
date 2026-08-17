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

		// Applies camera settings. While a live/capture session is active, structural fields
		// (pixel format, binning, ROI, acq mode/count, ADC) are camera-locked: tunables
		// (exposure, gain) are still applied so live tuning keeps working, structural fields
		// are ignored and false is returned. Stop the session before structural changes.
		bool WSOSYSTEM_DLL_API __stdcall setColorCameraSettingParam(const ColorCameraSettingParam* pParam);

		bool WSOSYSTEM_DLL_API __stdcall squareColorCameraRoi(void);

		// One-call mono square preview: params(Mono8/Bayer) -> squareRoi -> IR LED -> scanner IR grab -> live.
		// mode: 0 = SensorMono8, 1 = BayerSwGray. stop reverses and restores saved params.
		bool WSOSYSTEM_DLL_API __stdcall startColorCameraMonoPreview(int mode, ColorCameraFrameCaptured cb);
		bool WSOSYSTEM_DLL_API __stdcall stopColorCameraMonoPreview(void);

		// One-call color capture: IR off -> preset apply(checked) -> scanner grab ->
		// worker acquisition -> scanner pause -> onCompleted(result, framesAcquired).
		// Returns ColorCaptureStage: CAP_STAGE_OK once the worker started, else the failed
		// pre-acquisition stage (no completion callback follows a non-OK start).
		// stop cancels and joins the worker - never call it from the frame/completed callback.
		// isCapturing() turns false just BEFORE the completion callback runs, so it does not
		// mean the callback finished: a start issued right then joins a worker that is still
		// inside it. Consumers must return from the callback promptly (marshal, do not block).
		int WSOSYSTEM_DLL_API __stdcall startColorCameraColorCapture(const ColorCameraSettingParam* param, int scanPatternId, ColorCameraImageCaptured cb, ColorCaptureCompleted onCompleted);

		// Same device sequence as startColorCameraColorCapture, but the acquired subframes are
		// composed natively and the consumer receives ONE image: subframe j contributes rows
		// [j*segH, (j+1)*segH) of the composite (segH = height / subFrameCount, the last band takes
		// the remainder). cbComposed fires with frameCount=0, totalFrameCount=1 right before
		// onCompleted, and ONLY when every band was written - a cancelled or camera-aborted
		// capture reports through onCompleted alone, with no image.
		// subFrameCount <= 0 derives the band count from the acquired total frame count; frames
		// beyond subFrameCount (extra acquisition frames) are ignored.
		// stop/isCapturing/getLastColorCaptureError are shared with the per-frame capture above.
		int WSOSYSTEM_DLL_API __stdcall startColorCameraColorCaptureComposed(const ColorCameraSettingParam* param, int scanPatternId, int subFrameCount, ColorCameraImageCaptured cbComposed, ColorCaptureCompleted onCompleted);
		bool WSOSYSTEM_DLL_API __stdcall stopColorCameraColorCapture(void);
		bool WSOSYSTEM_DLL_API __stdcall isColorCameraCapturing(void);
		int WSOSYSTEM_DLL_API __stdcall getLastColorCaptureError(char* buf, int len);

	}
}