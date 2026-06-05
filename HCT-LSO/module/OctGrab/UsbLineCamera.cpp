#include "pch.h"
#include "UsbLineCamera.h"

#include "Usb3Grabber.h"

#include <atomic>
#include <functional>
#include <mutex>

using namespace oct_grab;
using namespace cpp_util;
using namespace std;


struct UsbLineCamera::UsbLineCameraImpl
{
	Usb3Grabber* grabber = NULL;

	UsbLineCameraImpl() {

	}
};


UsbLineCamera::UsbLineCamera() :
	d_ptr(make_unique<UsbLineCameraImpl>())
{
}


oct_grab::UsbLineCamera::~UsbLineCamera()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
oct_grab::UsbLineCamera::UsbLineCamera(UsbLineCamera && rhs) = default;
UsbLineCamera & oct_grab::UsbLineCamera::operator=(UsbLineCamera && rhs) = default;


bool oct_grab::UsbLineCamera::openLineCamera(Usb3Grabber* grabber)
{
	return false;
}

bool oct_grab::UsbLineCamera::closeLineCamera(void)
{
	return false;
}

bool oct_grab::UsbLineCamera::startAcquisition(bool restart)
{
	return false;
}

bool oct_grab::UsbLineCamera::cancelAcquisition(void)
{
	return false;
}

void oct_grab::UsbLineCamera::setGrabber(Usb3Grabber * grabber)
{
	impl().grabber = grabber;
	return;
}

bool oct_grab::UsbLineCamera::isCameraOpened(void)
{
	return false;
}

bool oct_grab::UsbLineCamera::isCameraGrabbing(void)
{
	return false;
}

bool oct_grab::UsbLineCamera::updateFrameHeight(int height)
{
	return false;
}

bool oct_grab::UsbLineCamera::setExposureToSlowerSpeed(void)
{
	return false;
}

bool oct_grab::UsbLineCamera::setExposureToNormalSpeed(void)
{
	return false;
}

bool oct_grab::UsbLineCamera::setExposureToFasterSpeed(void)
{
	return false;
}

float oct_grab::UsbLineCamera::getExposureTime(void)
{
	return 0.0f;
}

bool oct_grab::UsbLineCamera::setExposureTime(float expTime)
{
	return false;
}

bool oct_grab::UsbLineCamera::acquirePreviewImages(std::vector<int> bufferIds)
{
	return false;
}

bool oct_grab::UsbLineCamera::acquireMeasureImages(std::vector<int> bufferIds)
{
	return false;
}

bool oct_grab::UsbLineCamera::acquireEnfaceImages(std::vector<int> bufferIds)
{
	return false;
}

Usb3Grabber * oct_grab::UsbLineCamera::getGrabber(void) const
{
	return impl().grabber;
}


UsbLineCamera::UsbLineCameraImpl & oct_grab::UsbLineCamera::impl(void) const
{
	return *d_ptr;
}
