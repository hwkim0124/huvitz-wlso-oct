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
		InfraredCamera(MainBoard* board, IrCameraType type);
		virtual ~InfraredCamera();

		InfraredCamera(InfraredCamera&& rhs);
		InfraredCamera& operator=(InfraredCamera&& rhs);
		InfraredCamera(const InfraredCamera& rhs) = delete;
		InfraredCamera& operator=(const InfraredCamera& rhs) = delete;

	public:
		virtual bool initializeInfraredCamera(void);
		bool isInitiated(void) const;
		bool isPlaying(void) const;

		bool play(void);
		void pause(void);
		int getErrorCount(void);
		int getFrameCount(void);

		bool setActiveViewPreset(IrCameraViewPreset preset);
		IrCameraViewPreset getActiveViewPreset(void) const;
		bool setMirrorMode(IrCameraId camId, IrCameraMirrorMode hflip, IrCameraMirrorMode vflip);
		bool setOutputMode(IrCameraId camId, IrCameraOutputMode mode, IrCameraTestPattern pattern);

		bool setAnalogGain(IrCameraId camId, float gain, bool control = true);
		bool setDigitalGain(IrCameraId camId, float gain, bool control = true);
		float getAnalogGain(IrCameraId camId) const;
		float getDigitalGain(IrCameraId camId) const;

		std::uint8_t getAnalogGainData(float gain) const;
		std::uint8_t getDigitalGainData(float gain) const;

		bool isRetinaCamera(void) const;
		bool isAnteriorCamera(void) const;

		IrCameraType getCameraType(void) const;
		IrCameraId getCameraId(void) const;
		int getCameraIndex(IrCameraId camId) const;

		std::string getCameraName(void) const;
		std::uint8_t* getBuffer(void) const;
		std::uint32_t getFrameWidth(void) const;
		std::uint32_t getFrameHeight(void) const;
		std::uint32_t getFrameDataSize(void) const;

		std::uint32_t getStreamWidth(void) const;
		std::uint32_t getStreamHeight(void) const;
		std::uint32_t getStreamDataSize(void) const;
		std::uint32_t getNumFramesInStream(void) const;

	protected:
		MainBoard* getMainBoard(void) const;

		void acquireCameraStream(void);
		virtual bool handleFrameData(int index, unsigned char* frameData);
		bool recoverFromUsbError(void);
		bool isValidFrameHeader(unsigned char* buff);

	private:
		struct InfraredCameraImpl;
		std::unique_ptr<InfraredCameraImpl> d_ptr;
		InfraredCameraImpl& impl(void) const;
	};
}

