#include "pch.h"
#include "InfraredCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

#include <atomic>
#include <mutex>


using namespace wso_device;
using namespace std;



struct InfraredCamera::InfraredCameraImpl
{
	MainBoard* board;
	UsbComm* usbComm;
	std::uint8_t epid;
	bool initiated;

	unsigned int errorCount;
	unsigned int frameCount;

	vector<uint8_t> buffer;
	uint32_t frameWidth;
	uint32_t frameHeight;

	float again[4];
	float dgain[4];

	atomic<bool> playing;
	atomic<bool> workerRunning;
	thread worker;

	IrCameraType type;
	IrCameraViewPreset preset;
	mutex mutexLock;

	InfraredCameraImpl() : board(nullptr), playing(false), workerRunning(false), initiated(false), errorCount(0), frameCount(0),
		frameWidth(IR_CAMERA_FRAME_WIDTH), frameHeight(IR_CAMERA_FRAME_HEIGHT), 
		type(IrCameraType::RETINA), preset(IrCameraViewPreset::DEFAULT), epid(USB_IR1_PID), usbComm(nullptr)
	{
		implementInfraredCameraImpl();
	}

	void implementInfraredCameraImpl(void)
	{
		for (int i = 0; i < 4; i++) {
			again[i] = IR_CAMERA_AGAIN_INIT;
			dgain[i] = IR_CAMERA_DGAIN_INIT;
		}
	}
};


InfraredCamera::InfraredCamera() :
	d_ptr(make_unique<InfraredCameraImpl>())
{
}


wso_device::InfraredCamera::InfraredCamera(MainBoard* board, IrCameraType type) :
	d_ptr(make_unique<InfraredCameraImpl>())
{
	impl().type = type;
	impl().board = board;
	impl().epid = (type == IrCameraType::RETINA ? USB_IR1_PID : USB_IR2_PID);
	impl().usbComm = &(impl().board->getUsbComm());
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
	impl().initiated = true;

	UsbComm& usbComm = *impl().usbComm;
	auto type = getCameraType();
	bool res = usbComm.IrCameraControl(static_cast<unsigned char>(type), CTRL_OFF);
	return res;
}

bool wso_device::InfraredCamera::isInitiated(void) const
{
	return impl().initiated;
}

bool wso_device::InfraredCamera::isPlaying(void) const
{
	return impl().playing;
}


bool wso_device::InfraredCamera::play(void)
{
	if (!isInitiated()) {
		return false;
	}

	pause();
	
	UsbComm& usbComm = *impl().usbComm;
	auto type = getCameraType();
	bool ret = usbComm.IrCameraControl(static_cast<unsigned char>(type), CTRL_ON);

	// Set before spawning so a pause() that races right after play() reliably
	// observes the worker as alive and waits for it (see pause()).
	impl().playing = true;
	impl().workerRunning = true;
	impl().errorCount = 0;
	impl().frameCount = 0;
	impl().worker = thread{ &InfraredCamera::acquireCameraStream, this };
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
		// The acquisition worker may be blocked inside a synchronous FT_ReadPipe
		// that never returns when the camera is not streaming (the read path does
		// not honor the pipe timeout). Clearing 'playing' is not enough: the worker
		// only re-checks it between reads. Abort the read pipe so the pending
		// transfer fails fast, letting the worker observe playing==false and exit.
		// Once the first abort makes a read fail, the pipe is flagged unavailable
		// and any subsequent read fast-fails, so the worker exits within an
		// iteration or two. The loop is BOUNDED on purpose: if the driver does not
		// unblock the in-flight synchronous read, this degrades to a plain join()
		// (same as before) rather than spinning on abort forever.
		constexpr int IR_CAMERA_PAUSE_ABORT_MAX = 250;  // ~5s ceiling (250 x 20ms)
		UsbPort* port = impl().usbComm->getUsbPort();
		for (int i = 0; i < IR_CAMERA_PAUSE_ABORT_MAX && impl().workerRunning; i++) {
			port->abortPipe(impl().epid);
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
		impl().worker.join();

		// The abort above makes the pending read fail, which flags the pipe as
		// errored (UsbPort::receive). That failure was intentional, not an I/O
		// fault, so clear the flag ? otherwise the pipe would report unavailable
		// and the next play() could never read a frame again.
		port->setPipeError(impl().epid, false);
	}

	UsbComm& usbComm = *impl().usbComm;
	auto type = getCameraType();
	usbComm.IrCameraControl(static_cast<unsigned char>(type), CTRL_OFF);
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

bool wso_device::InfraredCamera::setActiveViewPreset(IrCameraViewPreset preset)
{
	UsbComm& usbComm = *impl().usbComm;
	auto format = static_cast<unsigned char>(preset);
	bool result = usbComm.IrCameraStreamFormat(format);

	if (result) {
		impl().preset = preset;
		auto name = getCameraName();
		LogD() << name << ", set active view preset : " << static_cast<int>(preset);
	}
	else {
		auto name = getCameraName();
		LogD() << name << ", set active view preset failed!";
	}
	return result;
}

IrCameraViewPreset wso_device::InfraredCamera::getActiveViewPreset(void) const
{
	return impl().preset;
}

bool wso_device::InfraredCamera::setMirrorMode(IrCameraId camId, IrCameraMirrorMode hflip, IrCameraMirrorMode vflip)
{
	UsbComm& usbComm = *impl().usbComm;
	auto cid = static_cast<unsigned char>(camId);
	auto h = static_cast<unsigned char>(hflip);
	auto v = static_cast<unsigned char>(vflip);
	auto result = usbComm.IrCameraMirrorMode(cid, h, v);
	return result;
}

bool wso_device::InfraredCamera::setOutputMode(IrCameraId camId, IrCameraOutputMode mode, IrCameraTestPattern pattern)
{
	UsbComm& usbComm = *impl().usbComm;
	auto cid = static_cast<unsigned char>(camId);
	auto m = static_cast<unsigned char>(mode);
	auto p = static_cast<unsigned char>(pattern);
	auto result = usbComm.IrCameraOutputMode(cid, m, p);
	return result;
}

bool wso_device::InfraredCamera::setAnalogGain(IrCameraId camId, float gain, bool control)
{
	if (!isInitiated()) {
		return false;
	}

	auto index = getCameraIndex(camId);
	impl().again[index] = gain;

	if (control) {
		unsigned char data = getAnalogGainData(gain);

		UsbComm& usbComm = *impl().usbComm;
		bool ret = usbComm.IrCameraAnalogGain(static_cast<unsigned char>(camId), (uint8_t)data);
		if (ret) {
			LogDebug() << "Ir camera id: " << static_cast<int>(camId) << ", set analog gain, value: " << gain;
		}
		else {
			LogDebug() << "Ir camera id: " << static_cast<int>(camId) << ", set analog gain failed!";
		}
		return ret;
	}
	else {
		return true;
	}
}

bool wso_device::InfraredCamera::setDigitalGain(IrCameraId camId, float gain, bool control)
{
	if (!isInitiated()) {
		return false;
	}

	gain = (gain < IR_CAMERA_DGAIN_MIN ? IR_CAMERA_DGAIN_MIN : gain);
	gain = (gain > IR_CAMERA_DGAIN_MAX ? IR_CAMERA_DGAIN_MAX : gain);

	auto index = getCameraIndex(camId);
	impl().dgain[index] = gain;	

	if (control) {
		// dgain = x + y * 2^-5
		// bit (7:5) = x
		// bit (4:0) = y
		unsigned char x, y;
		x = static_cast<unsigned char>(std::floor(gain));
		y = static_cast<unsigned char>((gain - x) * 32);
		unsigned char data = (x << 5) + y;

		UsbComm& usbComm = *impl().usbComm;
		bool ret = usbComm.IrCameraDigitalGain(static_cast<unsigned char>(camId), data);
		if (ret) {
			LogDebug() << "Ir camera id: " << static_cast<int>(camId) << ", set digital gain, value: " << gain;
		}
		else {
			LogDebug() << "Ir camera id: " << static_cast<int>(camId) << ", set digital gain failed!";
		}
		return ret;
	}
	else {
		return true;
	}
}

float wso_device::InfraredCamera::getAnalogGain(IrCameraId camId) const
{
	auto index = getCameraIndex(camId);
	auto gain = impl().again[index];
	return gain;
}

float wso_device::InfraredCamera::getDigitalGain(IrCameraId camId) const
{
	auto index = getCameraIndex(camId);
	auto gain = impl().dgain[index];
	return gain;
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

bool wso_device::InfraredCamera::isRetinaCamera(void) const
{
	return getCameraType() == IrCameraType::RETINA;
}

bool wso_device::InfraredCamera::isAnteriorCamera(void) const
{
	return getCameraType() == IrCameraType::ANTERIOR;
}

IrCameraType wso_device::InfraredCamera::getCameraType(void) const
{
	return impl().type;
}

int wso_device::InfraredCamera::getCameraIndex(IrCameraId camId) const
{
	int index = static_cast<int>(camId);
	if (index < 0 || index >= static_cast<int>(IrCameraId::NUMBER_OF_ITEMS)) {
		return 0;
	}
	return index;
}

std::string wso_device::InfraredCamera::getCameraName(void) const
{
	auto type = getCameraType();
	if (type == IrCameraType::RETINA) {
		return "Retina Ir Camera";
	}
	else if (type == IrCameraType::ANTERIOR) {
		return "Anterior Ir Camera";
	}
	return std::string();
}

std::uint8_t* wso_device::InfraredCamera::getBuffer(void) const
{
	return impl().buffer.data();
}


std::uint32_t wso_device::InfraredCamera::getFrameWidth(void) const
{
	return impl().frameWidth;
}


std::uint32_t wso_device::InfraredCamera::getFrameHeight(void) const
{
	return impl().frameHeight;
}


std::uint32_t wso_device::InfraredCamera::getFrameDataSize(void) const
{
	return getFrameWidth() * getFrameHeight();
}

std::uint32_t wso_device::InfraredCamera::getStreamWidth(void) const
{
	return getFrameWidth() * getNumFramesInStream();
}

std::uint32_t wso_device::InfraredCamera::getStreamHeight(void) const
{
	return getFrameHeight();
}

std::uint32_t wso_device::InfraredCamera::getStreamDataSize(void) const
{
	return getStreamWidth() * getStreamHeight();
}

std::uint32_t wso_device::InfraredCamera::getNumFramesInStream(void) const
{
	if (impl().preset == IrCameraViewPreset::LOWER_LEFT ||
		impl().preset == IrCameraViewPreset::LOWER_RIGHT || 
		impl().preset == IrCameraViewPreset::LEFT_RIGHT) {
		return 2;
	}
	else if (impl().preset == IrCameraViewPreset::LOWER_LEFT_RIGHT) {
		return 3;
	}
	return 1;
}

InfraredCamera::InfraredCameraImpl& wso_device::InfraredCamera::impl(void) const
{
	return *d_ptr;
}


MainBoard* wso_device::InfraredCamera::getMainBoard(void) const
{
	return impl().board;
}

void wso_device::InfraredCamera::acquireCameraStream(void)
{
	auto width = getStreamWidth();
	auto height = getStreamHeight();
	auto ssize = getStreamDataSize();
	auto fsize = getFrameDataSize();
	auto frame_w = getFrameWidth();
	auto frame_h = getFrameHeight();

	auto type = getCameraType();
	auto nframes = getNumFramesInStream();

	impl().buffer.assign(ssize, 0);
	auto fdata = vector<uint8_t>(fsize);
	auto pbuff = getBuffer();

	UsbComm& usbComm = *impl().usbComm;

	while (true)
	{
		if (!isPlaying()) {
			break;
		}

		bool result = false;
		{
			unique_lock<mutex> lock(impl().mutexLock);
			result = usbComm.readIrCameraFrame(type, pbuff, ssize);
		}

		if (result) {
			for (int n = 0; n < nframes; n++) {
				for (int i = 0; i < frame_h; i++) {
					auto* p1 = pbuff + width * i + frame_w * n;
					copy(p1, p1 + frame_w, fdata.begin() + i * frame_w);
				}
				handleFrameData(n, fdata.data());
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	// Signal to pause() that the worker has fully exited so it can stop aborting
	// the read pipe and join without blocking.
	impl().workerRunning = false;
	return;
}

bool wso_device::InfraredCamera::handleFrameData(int index, unsigned char* frameData)
{
	if (isValidFrameHeader(frameData)) {
		impl().errorCount = 0;
		impl().frameCount = 1;
		return true;
	}
	else {
		if (impl().errorCount == 0) {
			impl().errorCount = 1;
		}
	}
	return false;
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
		auto type = getCameraType();
		usbComm.IrCameraControl(static_cast<unsigned char>(type), CTRL_ON);
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


