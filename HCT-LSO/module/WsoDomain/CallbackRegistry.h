#pragma once

#include "WsoDomain2.h"
#include "CallbackFuncs.h"
#include "DeviceDefines.h"



namespace wso_domain
{
	class WSODOMAIN_DLL_API CallbackRegistry
	{

	public:
		CallbackRegistry();
		virtual ~CallbackRegistry();

		CallbackRegistry(const CallbackRegistry& rhs) = delete;
		CallbackRegistry& operator=(const CallbackRegistry& rhs) = delete;

		static CallbackRegistry* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void setWsoLogMsgCallback(WsoLogMsgCallback clb);
		void setFirmwareMsgCallback(FirmwareMsgCallback clb = nullptr);

		void setJoystickButtonPressed(JoystickButtonPressedCallback clb = nullptr);
		void setOptimizeButtonPressed(OptimizeButtonPressedCallback clb = nullptr);
		void setAdapterLensAttached(AdapterLensAttachedCallback clb = nullptr);

		void setStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb = nullptr);

		void setCorneaLeftCameraFrameCaptured(CorneaCameraFrameCaptured clb = nullptr);
		void setCorneaRightCameraFrameCaptured(CorneaCameraFrameCaptured clb = nullptr);
		void setCorneaLowerCameraFrameCaptured(CorneaCameraFrameCaptured clb = nullptr);
		void setRetinaCameraFrameCaptured(RetinaCameraFrameCaptured clb = nullptr);

		void setColorCameraFrameCaptured(ColorCameraFrameCaptured clb = nullptr);
		void setColorCameraSeqROIFrameCaptured(ColorCameraSeqROIFrameCaptured clb = nullptr);
		void setColorCameraOffsetROIFrameCaptured(ColorCameraOffsetROIFrameCaptured clb = nullptr);
		void setColorCameraRollSWTrigOverlapFrameCaptured(ColorCameraRollSWTrigOverlapFrameCaptured clb = nullptr);
		void setColorCameraImageCaptured(ColorCameraImageCaptured clb = nullptr);
		void setColorCameraSWTriggerFrameCaptured(ColorCameraFrameCaptured clb = nullptr);

		void setOctSpectrumDataCaptured(OctSpectrumDataCaptured clb = nullptr);
		void setOctResampledDataCaptured(OctResampleDataCaptured clb = nullptr);
		void setOctIntensityDataCaptured(OctIntensityDataCaptured clb = nullptr);
		void setOctPreviewImageCaptured(OctPreviewImageCaptured clb = nullptr);
		void setOctEnfaceImageCaptured(OctEnfaceImageCaptured clb = nullptr);

		void setOctScanPatternAcquired(OctScanPatternAcquired clb = nullptr);
		void setOctScanProtocolCompleted(OctScanProtocolCompleted clb = nullptr);

		void setOctAutoFocusOptimized(OctAutoFocusOptimized clb = nullptr);
		void setOctAutoReferOptimized(OctAutoReferOptimized clb = nullptr);
		void setOctAutoPolarOptimized(OctAutoPolarOptimized clb = nullptr);
		void setOctAutoScanOptimized(OctAutoScanOptimized clb = nullptr);


		void runWsoLogMsgCallback(std::string msg, int type);
		void runFirmwareMsgCallback(std::string strMessage, int nCurrentState, int nPercent, bool bReset, bool bError);

		void runJoystickButtonPressed(void);
		void runOptimizeButtonPressed(void);
		void runAdapterLensAttached(int type);
		void runStepMotorPositionChanged(MotorType type, int pos, float value);

		void runCorneaCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height);
		void runCorneaLeftCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height);
		void runCorneaRightCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height);
		void runCorneaLowerCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height);
		void runRetinaCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height);

		void runColorCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel);
		void runColorCameraSeqROIFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t offsetX, int32_t offsetY, int32_t frameCount, int32_t totalframeCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel);
		void runColorCameraOffsetROIFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t offsetX, int32_t offsetY, int32_t frameCount, int32_t totalframeCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel);
		void runColorCameraRollSWTrigOverlapFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t totalframeCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel);
		void runColorCameraImageCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t totalFrameCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel);
		void runColorCameraSWTriggerFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel);

		void runOctSpectrumDataCaptured(uint16_t* data, int width, int height);
		void runOctResampledDataCaptured(float* data, int width, int height);
		void runOctIntensityDataCaptured(float* data, int width, int height);
		void runOctPreviewImageCaptured(uint8_t* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index);
		void runOctEnfaceImageCaptured(unsigned char* bits, int width, int height);

		void runOctScanPatternAcquired(bool result);
		void runOctScanProtocolCompleted(bool result);

		void runOctAutoFocusOptimized(bool result, float quality, float diopter);
		void runOctAutoReferOptimized(bool result, float quality, int ref_point, int position);
		void runOctAutoPolarOptimized(bool result, float quality, float degree);
		void runOctAutoScanOptimized(bool result);


		void resetWsoLogMsgCallback(void);
		void resetFirmwareMsgCallback(void);
		void resetJoystickButtonPressed(void);
		void resetOptimizeButtonPressed(void);

	private:
		struct CallbackRegistryImpl;
		std::unique_ptr<CallbackRegistryImpl> d_ptr;
		CallbackRegistryImpl& impl(void) const;
	};
}



