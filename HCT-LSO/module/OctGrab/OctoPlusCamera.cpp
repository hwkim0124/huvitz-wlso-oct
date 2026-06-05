#include "pch.h"
#include "OctoPlusCamera.h"
#include "OctoPlus.h"


#include <atomic>
#include <functional>
#include <mutex>

using namespace oct_grab;
using namespace std;


struct OctoPlusCamera::OctoPlusCameraImpl
{
};


OctoPlusCamera::OctoPlusCamera() :
	d_ptr(make_unique<OctoPlusCameraImpl>())
{
}


oct_grab::OctoPlusCamera::~OctoPlusCamera()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
oct_grab::OctoPlusCamera::OctoPlusCamera(OctoPlusCamera && rhs) = default;
OctoPlusCamera & oct_grab::OctoPlusCamera::operator=(OctoPlusCamera && rhs) = default;



bool oct_grab::OctoPlusCamera::openLineCamera(Usb3Grabber* grabber)
{
	if (!OctoPlus::isLibraryInitialized()) {
		if (!OctoPlus::initializeLibrary()) {
			LogD() << "Usb cmos camera init failed!";
			return false;
		}
	}

	if (!OctoPlus::openLineCamera()) {
		LogD() << "Usb cmos camera open failed!";
		return false;
	}
	else {
		OctoPlus::setTriggerMode(ETriggerMode::Mode_ExternalLineTrigger_ProgrammableExposure);

		ostringstream msg;
		double exposure;
		OctoPlus::getExposureTime(&exposure);
		msg << "Octo plus exposure time: " << exposure;

		double period;
		OctoPlus::getLinePeriod(&period);
		msg << ", line period: " << period;

		double dgain;
		OctoPlus::getDigitalGain(&dgain);
		msg << ", digital gain: " << dgain;

		double again;
		OctoPlus::getAnalogGain(&again);
		msg << ", analog gain: " << again;
		LogD() << msg.str();

		msg.str("");  msg.clear();
		msg << "Trigger mode: " << OctoPlus::getTriggerMode();
		LogD() << msg.str();

		msg.str(""); msg.clear();
		msg << "Output mode: " << OctoPlus::getOutputMode();
		LogD() << msg.str();

		msg.str(""); msg.clear();
		msg << "Buffer type: " << OctoPlus::getBufferType();
		LogD() << msg.str();

		msg.str(""); msg.clear();
		msg << "Circular buffer: " << OctoPlus::isEnabledCircularBuffer();
		LogD() << msg.str();

		OctoPlus::setupUsb3Grabber(grabber);
		LogD() << "Usb cmos camera init ... ok!";

		setGrabber(grabber);
		return true;
	}
}

bool oct_grab::OctoPlusCamera::closeLineCamera(void)
{
	if (OctoPlus::isLibraryInitialized()) {
		OctoPlus::closeLineCamera();
		OctoPlus::releaseLibrary();
	}
	return true;
}

bool oct_grab::OctoPlusCamera::startAcquisition(bool init)
{
	bool rets = OctoPlus::startAcquisition(init);
	return rets;
}

bool oct_grab::OctoPlusCamera::cancelAcquisition(void)
{
	OctoPlus::cancelAcquisition();
	return true;
}

bool oct_grab::OctoPlusCamera::updateFrameHeight(int height)
{
	if (OctoPlus::getImageHeight() == height) {
		return true;
	}
	if (OctoPlus::isLineCameraGrabbing()) {
		OctoPlus::cancelAcquisition();
	}

	int numOfBuffer = 0;
	if (!OctoPlus::updateImageParameters(height, numOfBuffer)) {
		return false;
	}
	return true;
}

bool oct_grab::OctoPlusCamera::setExposureToSlowerSpeed(void)
{
	if (!OctoPlus::setLinePeriod(TRIGGER_TIME_STEP_AT_SLOWER_SPEED)) {
		return false;
	}

	float exp_time = EXPOSURE_TIME_AT_SLOWER_SPEED_USB3;		// 72.54
	if (!setExposureTime(exp_time)) {
		return false;
	}
	return true;
}

bool oct_grab::OctoPlusCamera::setExposureToNormalSpeed(void)
{
	if (!OctoPlus::setLinePeriod(TRIGGER_TIME_STEP_AT_NORMAL_SPEED)) {
		return false;
	}

	float exp_time = EXPOSURE_TIME_AT_NORMAL_SPEED_USB3;		// 35.898
	if (!setExposureTime(exp_time)) {
		return false;
	}
	return true;
}

bool oct_grab::OctoPlusCamera::setExposureToFasterSpeed(void)
{
	if (!OctoPlus::setLinePeriod(TRIGGER_TIME_STEP_AT_FASTER_SPEED)) {
		return false;
	}

	float exp_time = EXPOSURE_TIME_AT_FASTER_SPEED_USB3; // 0.93f;
	if (!setExposureTime(exp_time)) {
		return false;
	}
	return true;
}

bool oct_grab::OctoPlusCamera::setExposureTime(float expTime)
{
	if (!OctoPlus::setExposureTime(expTime)) {
		return false;
	}
	return true;
}

float oct_grab::OctoPlusCamera::getExposureTime(void)
{
	double expTime;
	if (!OctoPlus::getExposureTime(&expTime)) {
		return 0.0f;
	}
	return (float)expTime;
}

bool oct_grab::OctoPlusCamera::acquirePreviewImages(std::vector<int> bufferIds)
{
	bool retv = OctoPlus::acquirePreviewImages(bufferIds);
	return retv;
}

bool oct_grab::OctoPlusCamera::acquireMeasureImages(std::vector<int> bufferIds)
{
	bool retv = OctoPlus::acquireMeasureImages(bufferIds);
	return retv;
}

bool oct_grab::OctoPlusCamera::acquireEnfaceImages(std::vector<int> bufferIds)
{
	bool retv = OctoPlus::acquireEnfaceImages(bufferIds);
	return retv;
}


OctoPlusCamera::OctoPlusCameraImpl & oct_grab::OctoPlusCamera::impl(void) const
{
	return *d_ptr;
}
