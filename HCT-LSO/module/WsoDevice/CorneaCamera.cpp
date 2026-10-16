#include "pch.h"
#include "CorneaCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

using namespace wso_device;
using namespace std;


struct CorneaCamera::CorneaCameraImpl
{
	CorneaCameraImpl()
	{
		initializeCorneaCameraImpl();
	}

	void initializeCorneaCameraImpl(void)
	{
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
	// The anterior stream is the merged one, so this camera owns the stitch mask.
	// LOWER_LEFT_RIGHT selects all three sensors.
	setActiveViewPreset(IrCameraViewPreset::LOWER_LEFT_RIGHT);
	setMirrorMode(IrCameraId::CORNEA_LEFT, IrCameraMirrorMode::MIRROR_OFF, IrCameraMirrorMode::MIRROR_ON);
	setMirrorMode(IrCameraId::CORNEA_RIGHT, IrCameraMirrorMode::MIRROR_ON, IrCameraMirrorMode::MIRROR_OFF);
	setMirrorMode(IrCameraId::CORNEA_LOWER, IrCameraMirrorMode::MIRROR_ON, IrCameraMirrorMode::MIRROR_OFF);

	setOutputMode(IrCameraId::CORNEA_LEFT, IrCameraOutputMode::NORMAL, IrCameraTestPattern::STANDARD, 0);
	setOutputMode(IrCameraId::CORNEA_RIGHT, IrCameraOutputMode::NORMAL, IrCameraTestPattern::STANDARD, 0);
	setOutputMode(IrCameraId::CORNEA_LOWER, IrCameraOutputMode::NORMAL, IrCameraTestPattern::STANDARD, 0);
	// loadParamsFromProfile();
	return true;
}

void wso_device::CorneaCamera::applyConfigParameters(CorneaCameraConfigParam param)
{
	setAnalogGain(IrCameraId::CORNEA_LEFT, param.again_left, true);
	setAnalogGain(IrCameraId::CORNEA_RIGHT, param.again_right, true);
	setAnalogGain(IrCameraId::CORNEA_LOWER, param.again_lower, true);

	setDigitalGain(IrCameraId::CORNEA_LEFT, param.dgain_left, true);
	setDigitalGain(IrCameraId::CORNEA_RIGHT, param.dgain_right, true);
	setDigitalGain(IrCameraId::CORNEA_LOWER, param.dgain_lower, true);

	setExposureTime(IrCameraId::CORNEA_LEFT, param.exposTime_left, true);	
	setExposureTime(IrCameraId::CORNEA_RIGHT, param.exposTime_right, true);
	setExposureTime(IrCameraId::CORNEA_LOWER, param.exposTime_lower, true);	
	return;
}

bool wso_device::CorneaCamera::setupParameters(void)
{
	constexpr IrCameraId camIds[] = { IrCameraId::CORNEA_LOWER, IrCameraId::CORNEA_LEFT, IrCameraId::CORNEA_RIGHT };

	bool result = true;
	for (auto camId : camIds) {
		auto again = getAnalogGain(camId);
		auto dgain = getDigitalGain(camId);
		auto expos = getExposureTime(camId);
		result = InfraredCamera::setAnalogGain(camId, again, true) && result;
		result = InfraredCamera::setDigitalGain(camId, dgain, true) && result;
		result = InfraredCamera::setExposureTime(camId, expos, true) && result;
	}
	return result;
}

bool wso_device::CorneaCamera::handleFrameData(int index, unsigned char* frameData)
{
	auto frame_h = getFrameHeight();

	if (!InfraredCamera::handleFrameData(index, frameData)) {
		LogD() << "Ir anterior camera, handle frame data invalid!";
	}

	static auto image = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);
	static auto img_lower = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);
	static auto img_left = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);
	static auto img_right = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);

	for (uint32_t i = 0; i < frame_h; i++) {
		copy(frameData + i * IR_CAMERA_FRAME_WIDTH + IR_CAMERA_FRAME_PADD_SIZE, frameData + (i + 1) * IR_CAMERA_FRAME_WIDTH, image.begin() + i * IR_CAMERA_IMAGE_WIDTH);
	}

	auto preset = getActiveViewPreset();
	if ((preset == IrCameraViewPreset::LOWER_ONLY && index == 0) ||
		(preset == IrCameraViewPreset::LOWER_LEFT && index == 0) || 
		(preset == IrCameraViewPreset::LOWER_RIGHT && index == 0) || 
	    (preset == IrCameraViewPreset::LOWER_LEFT_RIGHT && index == 0)) {
		img_lower.assign(image.begin(), image.end());
		notifyFrameSink(IrCameraId::CORNEA_LOWER, img_lower.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
		CallbackRegistry::getInstance()->runCorneaLowerCameraFrameCaptured(img_lower.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
	}
	else if ((preset == IrCameraViewPreset::LEFT_ONLY && index == 0) ||
		(preset == IrCameraViewPreset::LOWER_LEFT && index == 1) ||
		(preset == IrCameraViewPreset::LOWER_LEFT_RIGHT && index == 1)) {
		img_left.assign(image.begin(), image.end());
		notifyFrameSink(IrCameraId::CORNEA_LEFT, img_left.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
		CallbackRegistry::getInstance()->runCorneaLeftCameraFrameCaptured(img_left.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
	}
	else if ((preset == IrCameraViewPreset::RIGHT_ONLY && index == 0) ||
		(preset == IrCameraViewPreset::LOWER_RIGHT && index == 1) ||
		(preset == IrCameraViewPreset::LOWER_LEFT_RIGHT && index == 2)) {
		img_right.assign(image.begin(), image.end());
		notifyFrameSink(IrCameraId::CORNEA_RIGHT, img_right.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
		CallbackRegistry::getInstance()->runCorneaRightCameraFrameCaptured(img_right.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
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
