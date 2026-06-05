#pragma once

#include "OctGrab2.h"



namespace oct_grab
{
	class Usb3Grabber;

	class OCTGRAB_DLL_API UsbLineCamera
	{
	public:
		UsbLineCamera();
		virtual ~UsbLineCamera();

		UsbLineCamera(UsbLineCamera&& rhs);
		UsbLineCamera& operator=(UsbLineCamera&& rhs);

		// Prevent copy construction and assignment. 
		UsbLineCamera(const UsbLineCamera& rhs) = delete;
		UsbLineCamera& operator=(const UsbLineCamera& rhs) = delete;

	public:
		virtual bool openLineCamera(Usb3Grabber* grabber);
		virtual bool closeLineCamera(void);
		virtual bool startAcquisition(bool restart);
		virtual bool cancelAcquisition(void);

		virtual bool isCameraOpened(void);
		virtual bool isCameraGrabbing(void);

		virtual bool updateFrameHeight(int height);
		virtual bool setExposureToSlowerSpeed(void);
		virtual bool setExposureToNormalSpeed(void);
		virtual bool setExposureToFasterSpeed(void);
		virtual bool setExposureTime(float expTime);
		virtual float getExposureTime(void);

		virtual bool acquirePreviewImages(std::vector<int> bufferIds);
		virtual bool acquireMeasureImages(std::vector<int> bufferIds);
		virtual bool acquireEnfaceImages(std::vector<int> bufferIds);

	protected:
		void setGrabber(Usb3Grabber* grabber);
		Usb3Grabber* getGrabber(void) const;

	private:
		struct UsbLineCameraImpl;
		std::unique_ptr<UsbLineCameraImpl> d_ptr;
		UsbLineCameraImpl& impl(void) const;
	};
}

