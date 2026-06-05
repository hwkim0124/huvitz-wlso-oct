#pragma once

#include "OctGrab2.h"

#include "UsbLineCamera.h"


namespace oct_grab
{
	class Usb3Grabber;

	class OCTGRAB_DLL_API OctoPlusCamera : public UsbLineCamera
	{
	public:
		OctoPlusCamera();
		virtual ~OctoPlusCamera();

		OctoPlusCamera(OctoPlusCamera&& rhs);
		OctoPlusCamera& operator=(OctoPlusCamera&& rhs);

		// Prevent copy construction and assignment. 
		OctoPlusCamera(const OctoPlusCamera& rhs) = delete;
		OctoPlusCamera& operator=(const OctoPlusCamera& rhs) = delete;

	public:
		bool openLineCamera(Usb3Grabber* grabber) override;
		bool closeLineCamera(void) override;
		bool startAcquisition(bool init) override;
		bool cancelAcquisition(void) override;

		bool updateFrameHeight(int height) override;
		bool setExposureToSlowerSpeed(void) override;
		bool setExposureToNormalSpeed(void) override;
		bool setExposureToFasterSpeed(void) override;

		bool setExposureTime(float expTime) override;
		float getExposureTime(void) override;

		bool acquirePreviewImages(std::vector<int> bufferIds) override;
		bool acquireMeasureImages(std::vector<int> bufferIds) override;
		bool acquireEnfaceImages(std::vector<int> bufferIds) override;
		
	private:
		struct OctoPlusCameraImpl;
		std::unique_ptr<OctoPlusCameraImpl> d_ptr;
		OctoPlusCameraImpl& impl(void) const;
	};
}

