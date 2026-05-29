#include "pch.h"
#include "InfraredCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

#include <atomic>
#include <functional>
#include <mutex>


using namespace wso_device;
using namespace std;



struct InfraredCamera::InfraredCameraImpl
{
	MainBoard* board;
	UsbComm* usbComm;
	InfraredCameraId camId;
	std::uint8_t epid;
	bool initiated;
	unsigned int errorCount;
	unsigned int frameCount;

	uint8_t buffer[IR_CAMERA_FRAME_DATA_SIZE];
	uint32_t frameWidth;
	uint32_t frameHeight;
	// OctFrameImage descript;

	atomic<bool> playing;
	thread worker;

	unsigned short value;
	unsigned short valueMin = 0;
	unsigned short valueMax = 255;
	unsigned short valueStep = 1;

	float analogGain;
	float digitalGain;
	uint16_t exposureTime;

	CameraType type;
	// InfraredCameraFrameCallback* callback;
	mutex mutexLock;

	InfraredCameraImpl() : board(nullptr), playing(false), initiated(false), errorCount(0), frameCount(0),
		analogGain(IR_CAMERA_AGAIN_VALUE1), value(IR_CAMERA_INTENSITY),
		digitalGain(IR_CAMERA_DGAIN_INIT), exposureTime(IR_CAMERA_EXPOSURE_TIME1),
		buffer{ 0 }, frameWidth(IR_CAMERA_FRAME_WIDTH), frameHeight(IR_CAMERA_FRAME_HEIGHT)
	{
	}
};


InfraredCamera::InfraredCamera() :
	d_ptr(make_unique<InfraredCameraImpl>())
{
}


wso_device::InfraredCamera::InfraredCamera(MainBoard* board, CameraType type, InfraredCameraId CamId, std::uint8_t epid) :
	d_ptr(make_unique<InfraredCameraImpl>())
{
	impl().type = type;
	impl().board = board;
	impl().camId = CamId;
	impl().epid = epid;

	if (type == CameraType::IR_CORNEA_LEFT || type == CameraType::IR_CORNEA_RIGHT) {
		impl().usbComm = &(impl().board->getSubComm());
	}
	else {
		impl().usbComm = &(impl().board->getUsbComm());
	}
}


InfraredCamera::~InfraredCamera()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::InfraredCamera::InfraredCamera(InfraredCamera&& rhs) = default;
InfraredCamera& wso_device::InfraredCamera::operator=(InfraredCamera&& rhs) = default;


bool wso_device::InfraredCamera::initializeInfraredCamera(void)
{
	if (getMainBoard()->isIrCameraNotInUse()) {
		return true;
	}

	if (impl().board == nullptr) {
		return false;
	}

	/*
	// impl().board = board;
	InfraredCameraId camId = (impl().type == CameraType::CORNEA ? InfraredCameraId::CORNEA : InfraredCameraId::RETINA);
	impl().camId = camId;
	impl().epid = (camId == InfraredCameraId::CORNEA ? USB_IR1_PID : USB_IR2_PID);
	// impl().callback = nullptr;
	*/
	impl().initiated = true;

	UsbComm& usbComm = *impl().usbComm;
	bool res = usbComm.IrCameraControl(static_cast<unsigned char>(impl().camId), CTRL_OFF);
	return res;
}


bool wso_device::InfraredCamera::isInitiated(void) const
{
	return impl().initiated;
}

/*
void wso_device::InfraredCamera::setCallback(InfraredCameraFrameCallback* callback)
{
	unique_lock<mutex> lock(impl().mutexLock);
	impl().callback = callback;
	return;
}
*/

bool wso_device::InfraredCamera::isPlaying(void) const
{
	return impl().playing;
}


bool wso_device::InfraredCamera::play(void)
{
	if (!isInitiated()) {
		return false;
	}


	// if (isPlaying()) {
	pause();
	// }


	UsbComm& usbComm = *impl().usbComm;
	bool ret = usbComm.IrCameraControl(static_cast<unsigned char>(impl().camId), CTRL_ON);

	impl().playing = true;
	impl().worker = thread{ &InfraredCamera::acquireCameraData, this };
	impl().errorCount = 0;
	impl().frameCount = 0;
	return ret;
}


void wso_device::InfraredCamera::pause(void)
{
	if (!isInitiated()) {
		return;
	}

	// if (isPlaying()) {
	impl().playing = false;
	if (impl().worker.joinable()) {
		impl().worker.join();
	}

	UsbComm& usbComm = *impl().usbComm;
	usbComm.IrCameraControl(static_cast<unsigned char>(impl().camId), CTRL_OFF);
	// }
	return;
}


int wso_device::InfraredCamera::getErrorCount(void)
{
	return impl().errorCount;
}


int wso_device::InfraredCamera::getFrameCount(void)
{
	return impl().frameCount;
}


bool wso_device::InfraredCamera::setAnalogGain(float gain, bool control)
{
	if (!isInitiated()) {
		return false;
	}

	impl().analogGain = gain;

	if (control) {
		unsigned char data = getAnalogGainData(gain);

		UsbComm& usbComm = *impl().usbComm;
		bool ret = usbComm.IrCameraAnalogGain(static_cast<unsigned char>(impl().camId), (uint8_t)data);
		if (ret) {
			LogDebug() << "Ir camera, set analog gain, value: " << gain;
		}
		else {
			LogDebug() << "Ir camera, set analog gain failed!";
		}
		return ret;
	}
	else {
		return true;
	}
}


bool wso_device::InfraredCamera::setDigitalGain(float gain, bool control)
{
	if (!isInitiated()) {
		return false;
	}

	gain = (gain < IR_CAMERA_DGAIN_MIN ? IR_CAMERA_DGAIN_MIN : gain);
	gain = (gain > IR_CAMERA_DGAIN_MAX ? IR_CAMERA_DGAIN_MAX : gain);
	impl().digitalGain = gain;

	if (control) {
		// dgain = x + y * 2^-5
		// bit (7:5) = x
		// bit (4:0) = y
		unsigned char x, y;
		x = static_cast<unsigned char>(std::floor(gain));
		y = static_cast<unsigned char>((gain - x) * 32);
		unsigned char data = (x << 5) + y;

		UsbComm& usbComm = *impl().usbComm;
		bool ret = usbComm.IrCameraDigitalGain(static_cast<unsigned char>(impl().camId), data);
		if (ret) {
			LogDebug() << "Ir camera, set digital gain, value: " << gain;
		}
		else {
			LogDebug() << "Ir camera, set digital gain failed!";
		}
		return ret;
	}
	else {
		return true;
	}
}


float wso_device::InfraredCamera::getAnalogGain(void) const
{
	return impl().analogGain;
}


float wso_device::InfraredCamera::getDigitalGain(void) const
{
	return impl().digitalGain;
}


bool wso_device::InfraredCamera::setExposureTime(bool highSpeed)
{
	impl().exposureTime = (highSpeed ? IR_CAMERA_EXPOSURE_TIME1 : IR_CAMERA_EXPOSURE_TIME2);
	UsbComm& usbComm = *impl().usbComm;
	bool ret = usbComm.IrCameraExposureTime(static_cast<unsigned char>(impl().camId), impl().exposureTime);
	return ret;
}


bool wso_device::InfraredCamera::isExposureTimeForHighSpeed(void)
{
	return (impl().exposureTime == IR_CAMERA_EXPOSURE_TIME1);
}


std::uint8_t wso_device::InfraredCamera::getAnalogGainData(float gain) const
{
	// bit (7:5) : ignore
	// bit (4)   : second stage (0:1x), (1:1.25x)
	// bit (3:0) : first stage (1:1x), (2:2x), (4:4x), (8:8x
	unsigned char first = 1;
	unsigned char second = 0;
	float value = (float)gain;

	if (value == 1.0f) {
		first = 1;  second = 0;
	}
	else if (value == 1.25f) {
		first = 1;  second = 16;
	}
	else if (value == 2.0f) {
		first = 2;  second = 0;
	}
	else if (value == 2.5f) {
		first = 2;  second = 16;
	}
	else if (value == 4.0f) {
		first = 4;  second = 0;
	}
	else if (value == 5.0f) {
		first = 4;  second = 16;
	}
	else if (value == 8.0f) {
		first = 8;  second = 0;
	}
	else if (value == 10.0f) {
		first = 8;  second = 16;
	}
	else {
		// throw std::runtime_error("Invalid analog gain value");
	}

	unsigned char data = first + second;
	return data;
}


std::uint8_t wso_device::InfraredCamera::getDigitalGainData(float gain) const
{
	unsigned char x, y;
	x = static_cast<unsigned char>(std::floor(gain));
	y = static_cast<unsigned char>((gain - x) * 32);
	unsigned char data = (x << 5) + y;

	return data;
}


bool wso_device::InfraredCamera::isCorneaLeftCamera(void) const
{
	return getType() == CameraType::IR_CORNEA_LEFT;
}

bool wso_device::InfraredCamera::isCorneaRightCamera(void) const
{
	return getType() == CameraType::IR_CORNEA_RIGHT;
}

bool wso_device::InfraredCamera::isCorneaLowerCamera(void) const
{
	return getType() == CameraType::IR_CORNEA_LOWER;
}

bool wso_device::InfraredCamera::isRetinaCamera(void) const
{
	return getType() == CameraType::IR_RETINA;
}

CameraType wso_device::InfraredCamera::getType(void) const
{
	return impl().type;
}

std::string wso_device::InfraredCamera::getCameraName(void) const
{
	auto type = getType();
	if (type == CameraType::IR_CORNEA_LEFT) {
		return "Cornea-Left Ir Camera";
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		return "Cornea-Right Ir Camera";
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		return "Cornea-Lower Ir Camera";
	}
	else if (type == CameraType::IR_RETINA) {
		return "Retina Ir Camera";
	}
	return std::string();
}

std::uint8_t* wso_device::InfraredCamera::getBuffer(void) const
{
	return impl().buffer;
}


std::uint32_t wso_device::InfraredCamera::getFrameWidth(void) const
{
	return impl().frameWidth;
}


std::uint32_t wso_device::InfraredCamera::getFrameHeight(void) const
{
	return impl().frameHeight;
}


std::uint32_t wso_device::InfraredCamera::getFrameSize(void) const
{
	return getFrameWidth() * getFrameHeight();
}


bool wso_device::InfraredCamera::captureFrame(std::uint8_t* frame) const
{
	if (!frame) {
		return false;
	}

	unique_lock<mutex> lock(impl().mutexLock);
	memcpy(frame, impl().buffer, getFrameSize());
	return true;
}

/*
bool wso_device::InfraredCamera::captureFrame(CppUtil::CvImage& image) const
{
	uint8_t frame[IR_CAMERA_FRAME_DATA_SIZE];
	if (captureFrame(frame)) {
		image.fromBitsData(frame, getFrameWidth(), getFrameHeight());
		return true;
	}
	return false;
}
*/

bool wso_device::InfraredCamera::captureFrame(OctFrameImageDescript& image) const
{
	static uint8_t frame[IR_CAMERA_FRAME_DATA_SIZE];
	if (captureFrame(frame)) {
		image.setup(frame, getFrameWidth(), getFrameHeight());
		return true;
	}
	return false;
}


/*
bool wso_device::InfraredCamera::loadConfig(SloConfig::CameraSettings * dset)
{
	CameraItem* item = dset->getCameraItem(getType());
	if (item != nullptr) {
		setAnalogGain(item->analogGain());
		setDigitalGain(item->digitalGain());
		// LogDebug() << "retina a-gain: " << item->analogGain() << ", d-gain: " << item->digitalGain();
		return true;
	}
	return false;
}


bool wso_device::InfraredCamera::saveConfig(SloConfig::CameraSettings * dset)
{
	CameraItem* item = dset->getCameraItem(getType());
	if (item != nullptr) {
		item->analogGain() = getAnalogGain();
		item->digitalGain() = getDigitalGain();
		return true;
	}
	return false;
}
*/


InfraredCamera::InfraredCameraImpl& wso_device::InfraredCamera::impl(void) const
{
	return *d_ptr;
}


MainBoard* wso_device::InfraredCamera::getMainBoard(void) const
{
	return impl().board;
}


void wso_device::InfraredCamera::acquireCameraData(void)
{
	unsigned char* buff = impl().buffer;
	uint32_t width = impl().frameWidth;
	uint32_t height = impl().frameHeight;
	uint32_t size = (width * height);
	uint8_t epid = impl().epid;

	auto image = vector<uint8_t>(IR_CAMERA_IMAGE_WIDTH * IR_CAMERA_IMAGE_HEIGHT);

	UsbComm& usbComm = *impl().usbComm;

	while (true)
	{
		if (!isPlaying()) {
			break;
		}

		bool result = false;
		{
			unique_lock<mutex> lock(impl().mutexLock);
			result = usbComm.readIrCameraFrame(impl().type, buff, size);
		}

		if (result) {
			if (isValidFrameHeader(buff)) {
				impl().errorCount = 0;
				impl().frameCount = 1;
			}
			else {
				if (impl().errorCount == 0) {
					LogError() << getCameraName() << ", fame header invalid!";
					impl().errorCount = 1;
				}
			}

			{
				for (uint32_t i = 0; i < height; i++) {
					copy(buff + i * IR_CAMERA_FRAME_WIDTH + IR_CAMERA_FRAME_PADD_SIZE, buff + (i + 1) * IR_CAMERA_FRAME_WIDTH, image.begin() + i * IR_CAMERA_IMAGE_WIDTH);
				}

				if (isCorneaLeftCamera()) {
					CallbackRegistry::getInstance()->runCorneaLeftCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
				}
				else if (isCorneaRightCamera()) {
					CallbackRegistry::getInstance()->runCorneaRightCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
				}
				else if (isCorneaLowerCamera()) {
					CallbackRegistry::getInstance()->runCorneaLowerCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
				}
				else if (isRetinaCamera()) {
					CallbackRegistry::getInstance()->runRetinaCameraFrameCaptured(image.data(), IR_CAMERA_IMAGE_WIDTH, IR_CAMERA_IMAGE_HEIGHT);
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
	return;
}


bool wso_device::InfraredCamera::isValidFrameHeader(unsigned char* buff)
{
	auto data1 = buff[0];
	auto data2 = buff[1];

	if (data1 == 0xff && (data2 == 0x00 || data2 == 0x02)) {
		unsigned char* p = buff;
		int period = 16;
		for (unsigned int idx = 0; idx < impl().frameHeight; idx += period) {
			auto headTag1 = p[0];
			auto headTag2 = p[1];
			unsigned short* lineId = (unsigned short*)&p[2];
			// LogDebug() << "Line index: " << *lineId << ", head tags: " << (int)headTag1 << ", " << (int)headTag2;
			if (idx != *lineId) {
				return false;
			}
			p += (impl().frameWidth * period);
		}
		return true;
	}
	return false;
}


bool wso_device::InfraredCamera::recoverFromUsbError(void)
{
	if (impl().errorCount == 0) {
		UsbComm& usbComm = *impl().usbComm;
		usbComm.TestCommand(impl().epid);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		usbComm.getUsbPort()->abortPipe(impl().epid);
		usbComm.IrCameraControl(static_cast<unsigned char>(impl().camId), CTRL_ON);
		impl().errorCount++;

		/*
		CString cstr;
		cstr.Format(_T("Recovering from usb error (cam=%x)"), impl().epid);
		AfxMessageBox(cstr);
		*/
		return true;
	}
	return false;
}


