#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"
#include "Spinnaker.h"

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API ColorCamera : public BoardComponent
	{
	public:
		ColorCamera(MainBoard* board);
		virtual ~ColorCamera();

	public:
		bool initialize(void);
		bool isInitiated(void) const;
		void uninitialize(void);

		void setInitParameters();
		void setDefaultParameters(bool update = true);

		void startLiveMode(void);
		void pauseLiveMode(void);
		bool isLiveMode(void) const;

		void startSingleFrameLiveMode(void);

		void startFrameSeqROILiveMode(void);
		void startFrameOffsetROILiveMode(void);
		void startFrmaeOffsetROICaptureMode(void);

		void startFrameRollSWTrigOverlabLiveMode(void);
		void startFrameRollSWTrigOverlabCaptureMode(void);

		void startSWTriggerLiveMode(void);
		void stopSWTriggerLiveMode(void);
		void setupSoftwareTrigger(int nMode = 0);
		void ShootSWTrigger(void);

		void startHWTriggerLiveMode(void);
		void stopHWTriggerLiveMode(void);
		bool isCameraStreaming(void);
		void setupHWTriggerSetting(bool bContinuous = true);

		void setupCameraTriggerOnOffOnly(bool bOn);

		void startOriginalMode(void);
		void pauseOriginalMode(void);

		int getFrameWidth(void) const;
		int getFrameHeight(void) const;
		int getFrameSizeInBytes(void) const;
		int getFrameSizeForBpp(int nBytesPerPixel) const; 

		void getParameters(ColorCameraSettingParam* pParam);
		void setParameters(ColorCameraSettingParam* pParam);

		unsigned int getROI_Max_Width();
		unsigned int getROI_Max_Height();
		unsigned int getROI_X_Width();
		unsigned int getROI_Y_Height();
		unsigned int getROI_X_Offset();
		unsigned int getROI_Y_Offset();
		void setROI_X_Width(unsigned int val);
		void setROI_Y_Height(unsigned int val);
		void setROI_X_Offset(unsigned int val);
		void setROI_Y_Offset(unsigned int val);

		unsigned int getExposureTime();
		void setExposureTime(unsigned int val);

		unsigned int getAcquisitionMode();
		void setAcquisitionMode(unsigned int val);

		unsigned int getAcquisitionFrameCount();
		void setAcquisitionFrameCount(unsigned int val);

		unsigned int getPixelFormat();
		void setPixelFormat(unsigned int val);

		unsigned int getBinningHorizontal();
		void setBinningHorizontal(unsigned int val);

		unsigned int getBinningVertical();
		void setBinningVertical(unsigned int val);

		float getGain();
		void setGain(float val);

		unsigned int getAdcBitDepth();
		void setAdcBitDepth(unsigned int val);

		unsigned int getFlipMode();

		void setSeqParameters(CaptureFrameSeqROIPreset* pParam);
		void setOffsetRoiParameters(CaptureFrameOffsetROIPreset* pParam);
		void setRollSWTrigOverlapParameters(CaptureFrameRollSWTrigOverlapPreset* pParam);

		bool loadConfig();
		bool saveConfig();

		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;

	private:
		struct ColorCameraImpl;
		std::unique_ptr<ColorCameraImpl> d_ptr;
		ColorCameraImpl& getImpl(void) const;

		Spinnaker::CameraPtr getCamera(void) const;

		void acquireCameraData(void);
		void acquireCameraSingleFrameData(void);

		void acquireCameraSingleSequencerROIData(void);
		void acquireCameraMultiSequencerROIData(void);

		void acquireCameraOffsetROIData(void);
		void acquireCaptureCameraOffsetROIData(void);

		void acquireCameraRollSWTrigOverlapLive(void);
		void acquireCameraRollSWTrigOverlapCapture(void);

		void acquireCameraRollHWTriggerCaptureData(void);

		bool onSequencer(void);
		bool setSequencer(std::vector<FrameSeqROIParam> paramList);
		void offSequencer(void);
		
		std::vector<std::vector<FrameSeqROIParam>> divideParamVector_(int nFrameCount, FrameSeqROIParam* arrayParams);

		void saveUserSet_(int eUserSet);
		void loadUserSet_(int eUserSet);

		void acquireCameraCaptureData(void);

		int getBytesPerPixel();

	private :

		class ColorCameraDeviceEventHandler : public Spinnaker::DeviceEventHandler
		{
		public:
			ColorCameraDeviceEventHandler(ColorCamera* camera) : m_camera(camera) {}
			virtual ~ColorCameraDeviceEventHandler() {}

			// DeviceEventHandler의 순수 가상 함수 구현
			virtual void OnDeviceEvent(Spinnaker::GenICam::gcstring eventName) override;

		private:
			ColorCamera* m_camera;
		};

		void HandleDeviceEvent(const Spinnaker::GenICam::gcstring& eventName);
		std::unique_ptr<ColorCameraDeviceEventHandler> m_deviceEventHandler;

		void SetEventMode(bool enableEvents);
	};
}



