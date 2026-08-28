#include "pch.h"
#include "RetinaCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

using namespace wso_device;
using namespace std;


struct RetinaCamera::RetinaCameraImpl
{
	float again;
	float dgain;

	RetinaCameraImpl()
	{
		initializeRetinaCameraImpl();
	}

	void initializeRetinaCameraImpl(void)
	{
		again = IR_CAMERA_AGAIN_INIT;
		dgain = IR_CAMERA_DGAIN_INIT;
	}
};


RetinaCamera::RetinaCamera() :
	d_ptr(make_unique<RetinaCameraImpl>()), InfraredCamera()
{
}


RetinaCamera::RetinaCamera(MainBoard* board, IrCameraType type) :
	d_ptr(make_unique<RetinaCameraImpl>()), InfraredCamera(board, type)
{
}
 

RetinaCamera::~RetinaCamera()
{
}

bool wso_device::RetinaCamera::initializeRetinaCamera(void)
{
	if (!InfraredCamera::initializeInfraredCamera()) {
		return false;
	}
	// loadParamsFromProfile();

	setMirrorMode(IrCameraId::RETINA, IrCameraMirrorMode::MIRROR_OFF, IrCameraMirrorMode::MIRROR_OFF);
	setOutputMode(IrCameraId::RETINA, IrCameraOutputMode::NORMAL, IrCameraTestPattern::STANDARD, 0);
	return true;
}

void wso_device::RetinaCamera::applyConfigParameters(RetinaCameraConfigParam param)
{
	setAnalogGain(param.again);
	setDigitalGain(param.dgain);
	return;
}

bool wso_device::RetinaCamera::applyGainSettings(void)
{
	auto again = getAnalogGain();
	auto dgain = getDigitalGain();
	bool result = setAnalogGain(again, true);
	result = setDigitalGain(dgain, true) && result;
	return result;
}

bool wso_device::RetinaCamera::setAnalogGain(float gain, bool control)
{
	return InfraredCamera::setAnalogGain(IrCameraId::RETINA, gain, control);
}

bool wso_device::RetinaCamera::setDigitalGain(float gain, bool control)
{
	return InfraredCamera::setDigitalGain(IrCameraId::RETINA, gain, control);
}

bool wso_device::RetinaCamera::setExposureTime(unsigned int time, bool control)
{
	return InfraredCamera::setExposureTime(IrCameraId::RETINA, time, control);
}

float wso_device::RetinaCamera::getAnalogGain(void) const
{
	return InfraredCamera::getAnalogGain(IrCameraId::RETINA);
}

float wso_device::RetinaCamera::getDigitalGain(void) const
{
	return InfraredCamera::getDigitalGain(IrCameraId::RETINA);
}

unsigned int wso_device::RetinaCamera::getExposureTime(void) const
{
	return InfraredCamera::getExposureTime(IrCameraId::RETINA);
}

bool wso_device::RetinaCamera::handleFrameData(int index, unsigned char* frameData)
{
	static auto image = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);
	auto frame_h = getFrameHeight();

	if (!InfraredCamera::handleFrameData(index, frameData)) {
		LogD() << "Ir retina camera, handle frame data invalid!";
	}

	for (uint32_t i = 0; i < frame_h; i++) {
		copy(frameData + i * IR_CAMERA_FRAME_WIDTH + IR_CAMERA_FRAME_PADD_SIZE, frameData + (i + 1) * IR_CAMERA_FRAME_WIDTH, image.begin() + i * IR_CAMERA_IMAGE_WIDTH);
	}

	CallbackRegistry::getInstance()->runRetinaCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
	return true;
}

bool wso_device::RetinaCamera::loadCalibParamFromProfile(void)
{
	/*
	if (!isInitiated()) {
		return;
	}

	uint8_t data = BoardConfig::corneaCameraAgain();
	float gain;
	float a_gain, d_gain;

	if (data > 8) {
		gain = (float)(data - 8);
		if (gain == 1.0f) {
			gain += 0.25f;
		}
		else if (gain == 2.0f) {
			gain += 0.5f;
		}
		else if (gain == 4.0f) {
			gain += 1.0f;
		}
		else if (gain == 8.0f) {
			gain += 2.0f;
		}
	}
	else {
		gain = (float)data;
	}

	setAnalogGain(gain, false);
	a_gain = gain;

	data = BoardConfig::corneaCameraDgain();
	gain = (float)(data >> 5);
	gain += (float)((data & 0x1F) * exp2(-5));

	setDigitalGain(gain, false);
	d_gain = gain;

	// LogDebug() << "Cornea ir camera, a-gain: " << a_gain << ", d-gain: " << d_gain;
	*/
	return true;
}


bool wso_device::RetinaCamera::saveCalibParamToProfile(void)
{
	/*
	if (!isInitiated()) {
		return;
	}

	float gain = getAnalogGain();
	BoardConfig::corneaCameraAgain(true, getAnalogGainData(gain));

	gain = getDigitalGain();
	BoardConfig::corneaCameraDgain(true, getDigitalGainData(gain));

	BoardConfig::setModified(true);
	*/
	return true;
}

RetinaCamera::RetinaCameraImpl& wso_device::RetinaCamera::impl(void) const
{
	return *d_ptr;
}
