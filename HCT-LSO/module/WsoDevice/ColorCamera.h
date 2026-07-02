#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"



namespace wso_device
{
	class MainBoard;

	class SpinnakerCamera
	{
	public:
		virtual void OnSpinnakerCameraEvent(std::string eventName) = 0;
	};

	class WSODEVICE_DLL_API ColorCamera : public BoardComponent, SpinnakerCamera
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
		void startFrameRollSwTrigOverlabLiveMode(void);
		void startFrameRollSwTrigOverlabCaptureMode(void);

		void startSwTriggerLiveMode(void);
		void stopSwTriggerLiveMode(void);
		void setupSoftwareTrigger(int nMode = 0);
		void shootSwTrigger(void);

		void startHwTriggerLiveMode(void);
		void stopHwTriggerLiveMode(void);
		bool isCameraStreaming(void);
		void setupHwTriggerSetting(bool bContinuous = true);
		void setupCameraTriggerOnOffOnly(bool bOn);

		void startOriginalMode(void);
		void pauseOriginalMode(void);

		int getFrameWidth(void) const;
		int getFrameHeight(void) const;
		int getFrameSizeInBytes(void) const;
		int getFrameSizeForBpp(int nBytesPerPixel) const; 

		ColorCameraSettingParam& getCameraSettings(void);
		void setCameraSettings(ColorCameraSettingParam params);

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

		void setCaptureSequencerPreset(LsoCaptureFrameSeqROIPreset preset);
		void setCaptureOffsetRoiPreset(LsoCaptureFrameOffsetROIPreset preset);
		void setRollSwTrigOverlapPreset(LsoCaptureFrameRollSwTrigOverlapPreset preset);

		bool loadConfig();
		bool saveConfig();

		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;
		void OnSpinnakerCameraEvent(std::string eventName) override;

	private:
		struct ColorCameraImpl;
		std::unique_ptr<ColorCameraImpl> d_ptr;
		ColorCameraImpl& impl(void) const;

		void acquireCameraData(void);
		void acquireCameraSingleFrameData(void);
		void acquireCameraSingleSequencerROIData(void);
		void acquireCameraMultiSequencerROIData(void);

		void acquireCameraOffsetROIData(void);
		void acquireCaptureCameraOffsetROIData(void);
		void acquireCameraRollSwTrigOverlapLive(void);
		void acquireCameraRollSwTrigOverlapCapture(void);
		void acquireCameraRollHwTriggerCaptureData(void);

		bool onSequencer(void);
		bool setSequencer(std::vector<LsoCaptureFrameROI> paramList);
		void offSequencer(void);
		
		std::vector<std::vector<LsoCaptureFrameROI>> divideParamVector_(int nFrameCount, LsoCaptureFrameROI* arrayParams);

		void saveUserSet_(int eUserSet);
		void loadUserSet_(int eUserSet);

		void acquireCameraCaptureData(void);
		int getBytesPerPixel();

	private :
		void SetEventMode(bool enableEvents);
	};
}



