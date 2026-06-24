#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>


namespace cpp_util {
	class CvImage;
}


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API InfraredCamera : public BoardComponent
	{
	public:
		InfraredCamera();
		InfraredCamera(MainBoard* board, CameraType type, std::uint8_t epid);
		virtual ~InfraredCamera();

		InfraredCamera(InfraredCamera&& rhs);
		InfraredCamera& operator=(InfraredCamera&& rhs);
		InfraredCamera(const InfraredCamera& rhs) = delete;
		InfraredCamera& operator=(const InfraredCamera& rhs) = delete;

	public:
		virtual bool initializeInfraredCamera(void);
		bool isInitiated(void) const;

		// void setCallback(InfraredCameraFrameCallback* callback);
		bool isPlaying(void) const;
		bool play(void);
		void pause(void);
		int getErrorCount(void);
		int getFrameCount(void);

		virtual bool setAnalogGain(float gain, bool control = true);
		virtual bool setDigitalGain(float gain, bool control = true);
		virtual float getAnalogGain(void) const;
		virtual float getDigitalGain(void) const;

		bool setExposureTime(bool highSpeed = false);
		bool isExposureTimeForHighSpeed(void);

		std::uint8_t getAnalogGainData(float gain) const;
		std::uint8_t getDigitalGainData(float gain) const;

		bool isCorneaLeftCamera(void) const;
		bool isCorneaRightCamera(void) const;
		bool isCorneaLowerCamera(void) const;
		bool isRetinaCamera(void) const;

		CameraType getType(void) const;
		IrCameraId getCameraId(void) const;
		std::string getCameraName(void) const;
		std::uint8_t* getBuffer(void) const;
		std::uint32_t getFrameWidth(void) const;
		std::uint32_t getFrameHeight(void) const;
		std::uint32_t getFrameSize(void) const;

		bool captureFrame(std::uint8_t* frame) const;
		// bool captureFrame(cpp_util::CvImage& image) const;
		bool captureFrame(OctFrameImageDescript& image) const;

		/*
		bool loadConfig(SloConfig::CameraSettings* dset);
		bool saveConfig(SloConfig::CameraSettings* dset);
		*/

	protected:
		MainBoard* getMainBoard(void) const;

		void acquireCameraData(void);
		bool recoverFromUsbError(void);
		bool isValidFrameHeader(unsigned char* buff);

	private:
		struct InfraredCameraImpl;
		std::unique_ptr<InfraredCameraImpl> d_ptr;
		InfraredCameraImpl& impl(void) const;
	};
}

