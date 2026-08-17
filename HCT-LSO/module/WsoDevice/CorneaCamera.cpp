#include "pch.h"
#include "CorneaCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

using namespace wso_device;
using namespace std;


struct CorneaCamera::CorneaCameraImpl
{
	float again[4];
	float dgain[4];

	CorneaCameraImpl()
	{
		initializeCorneaCameraImpl();
	}

	void initializeCorneaCameraImpl(void)
	{
		for (int i = 0; i < 3; i++) {
			again[i] = IR_CAMERA_AGAIN_INIT;
			dgain[i] = IR_CAMERA_DGAIN_INIT;
		}
	}
};


CorneaCamera::CorneaCamera() :
	d_ptr(make_unique<CorneaCameraImpl>()), InfraredCamera()
{}


CorneaCamera::CorneaCamera(MainBoard* board, IrCameraType type) :
	d_ptr(make_unique<CorneaCameraImpl>()), InfraredCamera(board, type)
{}


CorneaCamera::~CorneaCamera()
{}


bool wso_device::CorneaCamera::initializeCorneaCamera(void)
{
	if (!InfraredCamera::initializeInfraredCamera()) {
		return false;
	}
	setActiveViewPreset(IrCameraViewPreset::LOWER_LEFT_RIGHT);
	// loadParamsFromProfile();
	return true;
}


bool wso_device::CorneaCamera::handleFrameData(int index, unsigned char* frameData)
{
	static auto image = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);
	auto frame_h = getFrameHeight();

	if (!InfraredCamera::handleFrameData(index, frameData)) {
		LogD() << "Ir anterior camera, handle frame data invalid!";
	}

	for (uint32_t i = 0; i < frame_h; i++) {
		copy(frameData + i * IR_CAMERA_FRAME_WIDTH + IR_CAMERA_FRAME_PADD_SIZE, frameData + (i + 1) * IR_CAMERA_FRAME_WIDTH, image.begin() + i * IR_CAMERA_IMAGE_WIDTH);
	}

	auto preset = getActiveViewPreset();
	if ((preset == IrCameraViewPreset::LOWER_ONLY && index == 0) ||
		(preset == IrCameraViewPreset::LOWER_LEFT && index == 0) || 
		(preset == IrCameraViewPreset::LOWER_RIGHT && index == 0) || 
	    (preset == IrCameraViewPreset::LOWER_LEFT_RIGHT && index == 0)) {
		CallbackRegistry::getInstance()->runCorneaLowerCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
	}
	else if ((preset == IrCameraViewPreset::LEFT_ONLY && index == 0) ||
		(preset == IrCameraViewPreset::LOWER_LEFT && index == 1) ||
		(preset == IrCameraViewPreset::LOWER_LEFT_RIGHT && index == 1)) {
		CallbackRegistry::getInstance()->runCorneaLeftCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
	}
	else if ((preset == IrCameraViewPreset::RIGHT_ONLY && index == 0) ||
		(preset == IrCameraViewPreset::LOWER_RIGHT && index == 1) ||
		(preset == IrCameraViewPreset::LOWER_LEFT_RIGHT && index == 2)) {
		CallbackRegistry::getInstance()->runCorneaRightCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
	}
	return true;
}

bool wso_device::CorneaCamera::loadCalibParamFromProfile(void)
{
	/*
	if (!isInitiated()) {
		return;
	}

	uint8_t data = BoardConfig::CorneaCameraAgain();
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

	data = BoardConfig::CorneaCameraDgain();
	gain = (float)(data >> 5);
	gain += (float)((data & 0x1F) * exp2(-5));

	setDigitalGain(gain, false);
	d_gain = gain;

	// LogDebug() << "Cornea ir camera, a-gain: " << a_gain << ", d-gain: " << d_gain;
	*/
	return true;
}


bool wso_device::CorneaCamera::saveCalibParamToProfile(void)
{
	/*
	if (!isInitiated()) {
		return;
	}

	float gain = getAnalogGain();
	BoardConfig::CorneaCameraAgain(true, getAnalogGainData(gain));

	gain = getDigitalGain();
	BoardConfig::CorneaCameraDgain(true, getDigitalGainData(gain));

	BoardConfig::setModified(true);
	*/
	return true;
}

CorneaCamera::CorneaCameraImpl& wso_device::CorneaCamera::impl(void) const
{
	return *d_ptr;
}
