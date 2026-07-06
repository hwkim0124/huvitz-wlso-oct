#include "pch.h"
#include "ColorCamera.h"
#include "MainBoard.h"
#include "Spinnaker.h"

#include <format>
#include <chrono> // steady clock


using namespace wso_device;

using namespace Spinnaker;
using namespace std::chrono; // steady clock


class ColorCameraDeviceEventHandler : public Spinnaker::DeviceEventHandler
{
public:
	ColorCameraDeviceEventHandler(ColorCamera* camera) : m_camera(camera) {}
	virtual ~ColorCameraDeviceEventHandler() {}

	// DeviceEventHandler의 순수 가상 함수 구현
	virtual void OnDeviceEvent(Spinnaker::GenICam::gcstring eventName) override {
		if (m_camera) {
			m_camera->OnSpinnakerCameraEvent(eventName.c_str());
		}
	}

private:
	ColorCamera* m_camera;
};


struct ColorCamera::ColorCameraImpl
{
	MainBoard* board;
	//ColorSensor sensor;
	bool initiated;

	std::uint8_t sonyFrameBuffer[FUNDUS_FILR_FRAME_BUFFER_SIZE];

	unsigned int sensorId = 0;
	CvImage image;

	//SensorMode_t sensorMode;
	//CameraROI_t cameraROI;
	//CaptureParam_t captureParam;
	//TrigExposureParam_t trigExposureParam;
	//StrbParam_t strbParam;
	//SensorGain_t sensorGain;
	std::unique_ptr<ColorCameraDeviceEventHandler> m_deviceEventHandler;

	//Uses spinnaker
	Spinnaker::SystemPtr systemPtr;
	Spinnaker::CameraList camList;
	Spinnaker::CameraPtr pCam;
	Spinnaker::ImagePtr rawImage;

	atomic<bool> liveMode;
	atomic<bool> originalMode;
	thread worker;

	int nOffsetFrameCount = 0;

	ColorCameraSettingParam cameraSets; // Color Camera base setting 
	LsoCaptureFrameSeqROIPreset seqRoiPreset; // Use Global Shutter, Sequencer, ROI
	LsoCaptureFrameOffsetROIPreset offsetRoiPreset;
	LsoCaptureFrameRollSwTrigOverlapPreset rollSwTrigPreset;

	mutex mutexLock;
	mutex mutexHandleLock;

	ColorCameraImpl() : initiated(false), liveMode(false), pCam(NULL), rawImage(NULL), systemPtr(NULL), sonyFrameBuffer({}),
		board(nullptr), sensorId(0)
	{
	}

	Spinnaker::CameraPtr getCamera(void) const {
		return pCam;
	}
};

wso_device::ColorCamera::ColorCamera(MainBoard* board) :
	d_ptr(make_unique<ColorCameraImpl>())
{
	impl().board = board;
}

ColorCamera::~ColorCamera()
{
	if (impl().m_deviceEventHandler && impl().getCamera() && impl().getCamera()->IsValid()) {
		try {
			impl().getCamera()->UnregisterEventHandler(*(impl().m_deviceEventHandler));
		}
		catch (const std::exception& e) {
			WsoLogDebug("Exception caught while unregistering event handler in destructor: " + std::string(e.what()));
		}
	}

	impl().rawImage = NULL;
	impl().getCamera() = NULL;
	impl().camList.Clear();
	impl().systemPtr = NULL;
	return;
}

#pragma region Public

bool wso_device::ColorCamera::initialize(void)
{
	impl().systemPtr = System::GetInstance();
	impl().camList = impl().systemPtr->GetCameras();
	unsigned int numCameras = impl().camList.GetSize();

	if (numCameras == 0)
	{
		impl().camList.Clear();
		impl().systemPtr->ReleaseInstance();
		LogE() << "Color camera init failed! ";
		return false;
	}
	impl().pCam = impl().camList.GetByIndex(0);
	impl().pCam->Init();

	//setInitParameters();

	impl().initiated = true;
	return true;
}

bool wso_device::ColorCamera::isInitiated(void) const
{
	return impl().initiated;
}

void wso_device::ColorCamera::uninitialize(void)
{
	// 1. worker 스레드 먼저 종료
	if (impl().liveMode) {
		impl().liveMode = false;
		if (impl().worker.joinable()) {
			impl().worker.join();
		}
	}

	// rawImage grab 해제
	impl().rawImage = nullptr;

	// 2. 카메라가 아직 스트리밍 중이면 중지
	if (impl().pCam && impl().pCam->IsStreaming()) {
		try { impl().pCam->EndAcquisition(); }
		catch (...) {}
	}
	// 3. 이벤트 핸들러 해제
	if (impl().m_deviceEventHandler && impl().pCam && impl().pCam->IsValid()) {
		try { impl().pCam->UnregisterEventHandler(*(impl().m_deviceEventHandler)); }
		catch (...) {}
		impl().m_deviceEventHandler.reset();
	}
	// 4. DeInit 호출 (← 현재 완전 누락)
	if (impl().pCam && impl().pCam->IsValid()) {
		try { impl().pCam->DeInit(); }
		catch (...) {}
	}
	// 5. 리소스 해제
	impl().pCam = nullptr;
	impl().camList.Clear();
	// 6. systemPtr null 체크 후 ReleaseInstance
	if (impl().systemPtr) {
		impl().systemPtr->ReleaseInstance();
		impl().systemPtr = nullptr;
	}
	impl().initiated = false;
}

void wso_device::ColorCamera::setInitParameters()
{
	auto& params = impl().cameraSets;

	for (int i = 0; i < 3; i++)
	{
		try {
			if (!impl().getCamera()->IsValid()) {
				impl().getCamera()->Init();
			}
			if (impl().getCamera()->IsStreaming()) {
				impl().getCamera()->EndAcquisition();
			}

			std::string strModelName = impl().getCamera()->DeviceModelName.GetValue().c_str();
			std::string strModelMessage = std::format("Color Camera Name is {}", strModelName);
			WsoLogDebug(strModelMessage);

			std::string strModelNameSub = (strModelName.size() <= 6)
				? strModelName
				: strModelName.substr(strModelName.size() - 6);

			// Trigger Mode 비활성화
			int nTriggerMode = impl().getCamera()->TriggerMode.GetValue();
			if (nTriggerMode == TriggerMode_On)
				impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

			// Set Indicator LED
			impl().getCamera()->DeviceIndicatorMode.SetValue(DeviceIndicatorMode_Inactive);
			WsoLogDebug("Color Camera Set Indicator LED...ok!");

			// Set Image Size
			impl().getCamera()->OffsetX.SetValue(0);
			impl().getCamera()->OffsetY.SetValue(0);
			
			unsigned int nMaxHeight = getROI_Max_Height();
			unsigned int nMaxWidth = getROI_Max_Width();

			// Set ROI width
			if (params.roi_x_width >= nMaxWidth)
			{
				impl().getCamera()->Width.SetValue(nMaxWidth);
				impl().getCamera()->OffsetX.SetValue(0);
			}
			else
			{
				impl().getCamera()->Width.SetValue(params.roi_x_width);
				impl().getCamera()->OffsetX.SetValue(params.roi_x_offset);
			}

			// Set ROI height
			if (params.roi_y_height >= nMaxHeight)
			{
				impl().getCamera()->Height.SetValue(nMaxHeight);
				impl().getCamera()->OffsetY.SetValue(0);
			}
			else
			{
				impl().getCamera()->Height.SetValue(params.roi_y_height);
				impl().getCamera()->OffsetY.SetValue(params.roi_y_offset);
			}

			WsoLogDebug("Color Camera Set ImageSize...ok!");

			// Set PixelFormat to BayerRG8 as Temporarily
			impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG8);

			// Set AcquisitionMode to Multiframe as Temporarily
			impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame); // Exposure Auto, Gain Auto, BalanceWhite Auto Off 설정을 위해 임시로 

			// Set Exposure time
			impl().getCamera()->ExposureAuto.SetValue(ExposureAuto_Off);
			impl().getCamera()->ExposureTime.SetValue(params.exposure_time);
			//impl().getCamera()->ExposureTime.SetValue(20000);	// Unit = us
			WsoLogDebug("Color Camera Set ExposureTime...ok!");

			// Set Digital gain
			impl().getCamera()->GainAuto.SetValue(GainAuto_Off);
			impl().getCamera()->Gain.SetValue(params.gain);
			impl().getCamera()->AdcBitDepth.SetValue((AdcBitDepthEnums)params.adcDepthIndex);
			WsoLogDebug("Color Camera Set Gain...ok!");

			// Set White Balnce 
			impl().getCamera()->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Off);
			impl().getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
			impl().getCamera()->BalanceRatio.SetValue(1.58);
			impl().getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
			impl().getCamera()->BalanceRatio.SetValue(1.84);
			WsoLogDebug("Color Camera Set WhiteBalnce...ok!");

			// Set ISP Mode
			impl().getCamera()->IspEnable.SetValue(false);
			impl().getCamera()->GammaEnable.SetValue(false);
			WsoLogDebug("Color Camera Set ISP...ok!");

			// Set Strobe Light
			impl().getCamera()->AutoExposureLightingMode.SetIntValue(AutoExposureLightingMode_Frontlight);
			impl().getCamera()->LineSelector.SetValue(LineSelector_Line2);
			impl().getCamera()->LineMode.SetValue(LineMode_Output);
			WsoLogDebug("Color Camera Set StrobeLight...ok!");

			// Set Shutter Mode and Image Grab Test
			if (strModelNameSub == "122S6C")
			{
				impl().getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Global);
				WsoLogDebug("Color Camera Shutter Mode is Global.");
			}
			else if (strModelNameSub == "200S6C")
			{
				impl().getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Rolling);
				WsoLogDebug("Color Camera Shutter Mode is Rolling.");
			}
			else
			{
				impl().getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Rolling);
				WsoLogDebug("Color Camera Shutter Mode is Rolling.");
			}

			// Set Acquisition mode SingleFrame
			//impl().getCamera()->ReverseX.SetValue(false);
			//impl().getCamera()->ReverseY.SetValue(true);	// Horizontal Flip
			//impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG16);
			impl().getCamera()->PixelFormat.SetValue((PixelFormatEnums)params.pixelFormat);
			impl().getCamera()->AcquisitionMode.SetValue((AcquisitionModeEnums)params.acqusitionMode);
			WsoLogDebug("Color Camera Set AcquisitionMode...ok!");

			// Set Acquisition Frame Count
			if ((AcquisitionModeEnums)params.acqusitionMode != AcquisitionMode_SingleFrame)
			{
				setAcquisitionFrameCount(params.acqusitionFrameCount);
				WsoLogDebug("Color Camera Set AcquisitionFrameCount...ok!");
			}

			impl().getCamera()->BeginAcquisition();
			impl().rawImage = impl().getCamera()->GetNextImage();
			impl().getCamera()->EndAcquisition();

			WsoLogDebug("Color Camera Grab Test...ok!");

			WsoLogInfo("Color Camera initialize...ok!");
			break;
		}
		catch (const std::exception& e)
		{
			auto elog = e.what();
			WsoLogError("Exception occurred during ColorCamera init! " + std::string(e.what()));

			unsigned int numCameras = 0;
			impl().getCamera()->DeviceReset.Execute();

			do {
				impl().systemPtr = System::GetInstance();
				impl().camList = impl().systemPtr->GetCameras();
				numCameras = impl().camList.GetSize();
			} while (numCameras == 0);

			impl().pCam = impl().camList.GetByIndex(0);
			impl().pCam->Init();
		}
	}
}

void wso_device::ColorCamera::setDefaultParameters(bool update)
{
	for (int i = 0; i < 3; i++)
	{
		try {
			if (!impl().getCamera()->IsValid()) {
				impl().getCamera()->Init();
			}
			if (impl().getCamera()->IsStreaming()) {
				impl().getCamera()->EndAcquisition();
			}

			// Set Indicator LED
			impl().getCamera()->DeviceIndicatorMode.SetValue(DeviceIndicatorMode_Inactive);
			WsoLogDebug("Color Camera Set Indicator LED...ok!");

			// Set Image Size
			impl().getCamera()->OffsetX.SetValue(0);
			impl().getCamera()->OffsetY.SetValue(0);
			impl().getCamera()->Width.SetValue(4000);
			impl().getCamera()->Height.SetValue(3000);
			impl().getCamera()->OffsetX.SetValue(736);
			impl().getCamera()->OffsetY.SetValue(324);
			WsoLogDebug("Color Camera Set ImageSize...ok!");

			// Set Acquisition mode SingleFrame
			//impl().getCamera()->ReverseX.SetValue(false);
			//impl().getCamera()->ReverseY.SetValue(true);	// Horizontal Flip
			//impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG16);
			impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG8);
			impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
			WsoLogDebug("Color Camera Set AcquisitionMode...ok!");

			// Set Exposure time
			impl().getCamera()->ExposureAuto.SetValue(ExposureAuto_Off);
			//impl().getCamera()->ExposureTime.SetValue(20000);	// Unit = us
			WsoLogDebug("Color Camera Set ExposureTime...ok!");

			// Set Digital gain
			impl().getCamera()->GainAuto.SetValue(GainAuto_Off);
			impl().getCamera()->Gain.SetValue(1);
			impl().getCamera()->AdcBitDepth.SetValue(AdcBitDepth_Bit12);
			WsoLogDebug("Color Camera Set Gain...ok!");

			// Set White Balnce 
			impl().getCamera()->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Off);
			impl().getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
			impl().getCamera()->BalanceRatio.SetValue(1.58);
			impl().getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
			impl().getCamera()->BalanceRatio.SetValue(1.84);
			WsoLogDebug("Color Camera Set WhiteBalnce...ok!");

			// Set ISP Mode
			impl().getCamera()->IspEnable.SetValue(false);
			impl().getCamera()->GammaEnable.SetValue(false);
			WsoLogDebug("Color Camera Set ISP...ok!");

			// Set Strobe Light
			//impl().getCamera()->AutoExposureLightingMode.SetIntValue(AutoExposureLightingMode_Frontlight);
			impl().getCamera()->LineSelector.SetValue(LineSelector_Line2);
			impl().getCamera()->LineMode.SetValue(LineMode_Output);
			WsoLogDebug("Color Camera Set StrobeLight...ok!");

			// Set Shutter Mode and Image Grab Test
			impl().getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Rolling);
			impl().getCamera()->BeginAcquisition();
			impl().rawImage = impl().getCamera()->GetNextImage();
			impl().getCamera()->EndAcquisition();
			//impl().getCamera()->SensorShutterMode.SetValue(SensorShutterMode_GlobalReset);
			WsoLogDebug("Color Camera Grab Test...ok!");

			WsoLogInfo("Color Camera initialize...ok!");
			break;
		}
		catch (const std::exception& e)
		{
			WsoLogError("Exception occurred during ColorCamera init!" + std::string(e.what()));

			unsigned int numCameras = 0;
			impl().getCamera()->DeviceReset.Execute();

			do {
				impl().systemPtr = System::GetInstance();
				impl().camList = impl().systemPtr->GetCameras();
				numCameras = impl().camList.GetSize();
			} while (numCameras == 0);

			impl().pCam = impl().camList.GetByIndex(0);
			impl().pCam->Init();
		}
	}
}

void wso_device::ColorCamera::startLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	impl().worker = thread{ &ColorCamera::acquireCameraData, this };
}

void wso_device::ColorCamera::pauseLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}
	if (!impl().liveMode) {
		return;
	}

	impl().liveMode = false;

	if (impl().worker.joinable()) {
		impl().worker.join();
	}
	return;
}

bool wso_device::ColorCamera::isLiveMode(void) const
{
	return impl().liveMode;
}

void wso_device::ColorCamera::startSingleFrameLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	impl().worker = thread{ &ColorCamera::acquireCameraSingleFrameData, this };
}

void wso_device::ColorCamera::startFrameSeqROILiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	int nTotalFrameCount = impl().seqRoiPreset.frameCount;
	if (nTotalFrameCount > 8)
	{
		impl().worker = thread{ &ColorCamera::acquireCameraMultiSequencerROIData, this };
	}
	else
	{
		impl().worker = thread{ &ColorCamera::acquireCameraSingleSequencerROIData, this };
	}
}

void wso_device::ColorCamera::startFrameOffsetROILiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	impl().worker = thread{ &ColorCamera::acquireCameraOffsetROIData, this };
}

void wso_device::ColorCamera::startFrmaeOffsetROICaptureMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	acquireCaptureCameraOffsetROIData();
}

void wso_device::ColorCamera::startFrameRollSwTrigOverlabLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;
	impl().worker = thread{ &ColorCamera::acquireCameraRollSwTrigOverlapLive, this };
	return;
}

void wso_device::ColorCamera::startFrameRollSwTrigOverlabCaptureMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;
	acquireCameraRollSwTrigOverlapCapture();
	return;
}

void wso_device::ColorCamera::startSwTriggerLiveMode(void)
{
	unique_lock<mutex> lock(impl().mutexLock);

	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	if (impl().getCamera()->IsStreaming())
		impl().getCamera()->EndAcquisition();

	// Software Trigger 설정
	setupSoftwareTrigger(1); // Single Frame

	// Acquisition 시작
	impl().getCamera()->BeginAcquisition();
	return;
}

void wso_device::ColorCamera::stopSwTriggerLiveMode(void)
{
	unique_lock<mutex> lock(impl().mutexLock);

	if (impl().getCamera()->IsStreaming())
	{
		impl().getCamera()->EndAcquisition();
	}

	// Trigger Mode 비활성화
	int nTriggerMode = impl().getCamera()->TriggerMode.GetValue();
	if (nTriggerMode == TriggerMode_On)
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

	impl().liveMode = false;
	return;
}

void wso_device::ColorCamera::setupSoftwareTrigger(int nMode)
{
	try {
		// 1. Acquisition 중단
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}

		// 2. Trigger Mode 활성화
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_On);

		// 3. Trigger Source를 Software로 설정
		impl().getCamera()->TriggerSource.SetValue(TriggerSource_Software);

		// 4. Trigger Activation 설정 (Rising Edge)
		//impl().getCamera()->TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// 5. Acquisition Mode 설정 (SingleFrame 또는 MultiFrame)
		switch (nMode)
		{
			case 0 : // Continuous
			{
				impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
			}
			break;
			case 1 : // Single Frame
			{
				impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_SingleFrame);
			}
			break;
			case 2 : // Multi Frame
			{
				impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame);
			}
			break;
		}

		// 6. Trigger Selector 설정 (Frame Start)
		impl().getCamera()->TriggerSelector.SetValue(TriggerSelector_FrameStart);

		WsoLogInfo("Software Trigger setup completed");
	}
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during Software Trigger setup: " + std::string(e.what()));
	}
}

void wso_device::ColorCamera::shootSwTrigger()
{
	if (!impl().liveMode)
		return;

	unique_lock<mutex> lock(impl().mutexLock);

	auto t1 = steady_clock::now();

	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();
	int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();
	uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);
	unsigned char* buffer = impl().sonyFrameBuffer;

	uint32_t frameCount = 0;

	auto t2 = steady_clock::now();

	impl().getCamera()->TriggerSoftware.Execute();

	auto t3 = steady_clock::now();

	//frameCount++;

	impl().rawImage = impl().getCamera()->GetNextImage();

	auto t4 = steady_clock::now();

	std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

	auto t5 = steady_clock::now();

	CallbackRegistry::getInstance()->runColorCameraSWTriggerFrameCaptured(buffer, width, height, frameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

	impl().rawImage->Release();

	auto t6 = steady_clock::now();

	// print result
	double ms = duration<double, std::milli>(t6 - t1).count();
	std::printf("Total Sequence = %.3f ms\n", ms);
	LogD() << "Total Sequence = " << ms;

	ms = duration<double, std::milli>(t2 - t1).count();
	LogD() << "Get Parameter = " << ms;

	ms = duration<double, std::milli>(t3 - t2).count();
	LogD() << "Trigger Excute = " << ms;

	ms = duration<double, std::milli>(t4 - t3).count();
	LogD() << "Get Next Image = " << ms;

	ms = duration<double, std::milli>(t6 - t5).count();
	LogD() << "Image Callback = " << ms;
}

void wso_device::ColorCamera::startHwTriggerLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().liveMode = true;

	setupHwTriggerSetting(false);

	impl().worker = thread{ &ColorCamera::acquireCameraRollHwTriggerCaptureData, this };
	return;
}

void wso_device::ColorCamera::stopHwTriggerLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}
	if (!impl().liveMode) {
		return;
	}

	impl().liveMode = false;

	if (impl().worker.joinable()) {
		impl().worker.join();
	}
	return;
}

bool wso_device::ColorCamera::isCameraStreaming(void)
{
	if (!isInitiated()) {
		return false;
	}

	if (impl().pCam == nullptr)
	{
		return false;
	}
	
	return impl().pCam->IsStreaming();
}

void wso_device::ColorCamera::setupHwTriggerSetting(bool bContinuous)
{
	try {
		// 1. Acquisition 중단
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}

		// 6. Trigger Selector 설정 (Frame Start)
		impl().getCamera()->TriggerSelector.SetValue(TriggerSelector_FrameStart);

		// 3. Trigger Source를 Software로 설정
		impl().getCamera()->TriggerSource.SetValue(TriggerSource_Line3);

		// 4. Trigger Activation을 ReadOut으로 설정
		impl().getCamera()->TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// 4. Trigger Overlap을 ReadOut으로 설정
		impl().getCamera()->TriggerOverlap.SetValue(TriggerOverlap_ReadOut);

		// 4. LineSelector를 Line2로 설정
		impl().getCamera()->LineSelector.SetValue(LineSelector_Line2);

		// 4. LineMode를 Output로 설정
		impl().getCamera()->LineMode.SetValue(LineMode_Output);

		// 4. LineSource를 Output로 설정
		impl().getCamera()->LineSource.SetValue(LineSource_ExposureActive);

		// 4. LineSelector를 Line2로 설정
		impl().getCamera()->LineSelector.SetValue(LineSelector_Line3);

		// 4. LineMode를 Output로 설정
		impl().getCamera()->LineMode.SetValue(LineMode_Input);

		// 5. Acquisition Mode 설정 (Continuous 또는 MultiFrame)
		//if (bContinuous)
		//	impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		//else
		//	impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame);

		// 2. Trigger Mode 활성화
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_On);

		WsoLogInfo("Hardware trigger setup completed");
	}
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during hardware trigger setup: " + std::string(e.what()));
	}
}

void wso_device::ColorCamera::setupCameraTriggerOnOffOnly(bool bOn)
{
	try
	{
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}

		if (bOn == true)
		{
			impl().getCamera()->TriggerMode.SetValue(TriggerMode_On);
			//WsoLogInfo("Camera trigger mode : On");
		}
		else
		{
			impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);
			//WsoLogInfo("Camera trigger mode : Off");
		}
	}
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during to change camera trigger mode : " + std::string(e.what()));
	}
}

void wso_device::ColorCamera::startOriginalMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (impl().liveMode) {
		return;
	}

	impl().originalMode = true;

	setupCameraTriggerOnOffOnly(false);

	acquireCameraCaptureData();
	return;
}

void wso_device::ColorCamera::pauseOriginalMode(void)
{
	impl().originalMode = false;
}

int wso_device::ColorCamera::getFrameWidth(void) const
{
	int width = -1;
	width = (int)impl().getCamera()->Width.GetValue();

	return width;
}

int wso_device::ColorCamera::getFrameHeight(void) const
{
	int height = -1;
	height = (int)impl().getCamera()->Height.GetValue();

	return height;
}

int wso_device::ColorCamera::getFrameSizeInBytes(void) const
{
	return getFrameWidth() * getFrameHeight() * sizeof(unsigned short);
}

int wso_device::ColorCamera::getFrameSizeForBpp(int nBytesPerPixel) const
{
	return getFrameWidth() * getFrameHeight() * sizeof(char) * nBytesPerPixel;
}


ColorCameraSettingParam& wso_device::ColorCamera::getCameraSettings(void)
{
	auto& camsets = impl().cameraSets;

	try
	{
		camsets.roi_max_width = getROI_Max_Width();
		camsets.roi_max_Height = getROI_Max_Height();
		camsets.roi_x_width = getROI_X_Width();
		camsets.roi_y_height = getROI_Y_Height();
		camsets.roi_x_offset = getROI_X_Offset();
		camsets.roi_y_offset = getROI_Y_Offset();
		camsets.exposure_time = getExposureTime();
		camsets.acqusitionMode = getAcquisitionMode();
		camsets.acqusitionFrameCount = getAcquisitionFrameCount();
		camsets.pixelFormat = getPixelFormat();
		camsets.binningHorizontal = getBinningHorizontal();
		camsets.binningVertical = getBinningVertical();
		camsets.gain = getGain();
		camsets.adcDepthIndex = getAdcBitDepth();
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Get!");
		WsoLogError(elog);
	}
	return camsets;
}

void wso_device::ColorCamera::setCameraSettings(ColorCameraSettingParam params)
{
	try
	{
		/////현재의 Width, Height 값에 안맞는 offset 값을 넣으면 문제가 생기므로, 처음에 0,0으로 초기화를 먼저 시켜준다.
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		///
		setROI_X_Width(params.roi_x_width);
		setROI_Y_Height(params.roi_y_height);
		setROI_X_Offset(params.roi_x_offset);
		setROI_Y_Offset(params.roi_y_offset);
		setExposureTime(params.exposure_time);
		setAcquisitionMode(params.acqusitionMode);
		setAcquisitionFrameCount(params.acqusitionFrameCount);
		setPixelFormat(params.pixelFormat);
		setBinningHorizontal(params.binningHorizontal);
		setBinningVertical(params.binningVertical);
		setGain(params.gain);
		setAdcBitDepth(params.adcDepthIndex);

		auto& camsets = impl().cameraSets;
		camsets.roi_x_width = params.roi_x_width;
		camsets.roi_y_height = params.roi_y_height;
		camsets.roi_x_offset = params.roi_x_offset;
		camsets.roi_y_offset = params.roi_y_offset;
		camsets.exposure_time = params.exposure_time;
		camsets.acqusitionMode = params.acqusitionMode;
		camsets.acqusitionFrameCount = params.acqusitionFrameCount;
		camsets.pixelFormat = params.pixelFormat;
		camsets.binningHorizontal = params.binningHorizontal;
		camsets.binningVertical = params.binningVertical;
		camsets.gain = params.gain;
		camsets.adcDepthIndex = params.adcDepthIndex;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::setCaptureSequencerPreset(LsoCaptureFrameSeqROIPreset preset)
{
	try
	{
		impl().seqRoiPreset = preset;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::setCaptureOffsetRoiPreset(LsoCaptureFrameOffsetROIPreset preset)
{
	try
	{
		impl().offsetRoiPreset = preset;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::setRollSwTrigOverlapPreset(LsoCaptureFrameRollSwTrigOverlapPreset preset)
{
	try
	{
		impl().rollSwTrigPreset = preset;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

unsigned int wso_device::ColorCamera::getROI_Max_Width()
{
	return (unsigned int)impl().getCamera()->WidthMax.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_Max_Height()
{
	return (unsigned int)impl().getCamera()->HeightMax.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_X_Width()
{
	return (unsigned int)impl().getCamera()->Width.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_Y_Height()
{
	return (unsigned int)impl().getCamera()->Height.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_X_Offset()
{
	return (unsigned int)impl().getCamera()->OffsetX.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_Y_Offset()
{
	return (unsigned int)impl().getCamera()->OffsetY.GetValue();
}

unsigned int wso_device::ColorCamera::getExposureTime()
{
	return (unsigned int)impl().getCamera()->ExposureTime.GetValue();
}

unsigned int wso_device::ColorCamera::getAcquisitionMode()
{
	return (unsigned int)impl().getCamera()->AcquisitionMode.GetValue();
}

unsigned int wso_device::ColorCamera::getAcquisitionFrameCount()
{
	return (unsigned int)impl().getCamera()->AcquisitionFrameCount.GetValue();
}

unsigned int wso_device::ColorCamera::getPixelFormat()
{
	return (unsigned int)impl().getCamera()->PixelFormat.GetValue();
}

unsigned int wso_device::ColorCamera::getBinningHorizontal()
{
	return (unsigned int)impl().getCamera()->BinningHorizontal.GetValue();
}

unsigned int wso_device::ColorCamera::getBinningVertical()
{
	return (unsigned int)impl().getCamera()->BinningVertical.GetValue();
}

float wso_device::ColorCamera::getGain()
{
	return (float)impl().getCamera()->Gain.GetValue();
}

unsigned int wso_device::ColorCamera::getAdcBitDepth()
{
	return (unsigned int)impl().getCamera()->AdcBitDepth.GetValue();
}

void wso_device::ColorCamera::setROI_X_Width(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->Width.SetValue(val);
	return;
}

void wso_device::ColorCamera::setROI_Y_Height(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->Height.SetValue(val);
	return;
}

void wso_device::ColorCamera::setROI_X_Offset(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->OffsetX.SetValue(val);
	return;
}

void wso_device::ColorCamera::setROI_Y_Offset(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->OffsetY.SetValue(val);
	return;
}

void wso_device::ColorCamera::setExposureTime(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->ExposureTime.SetValue(val);
	return;
}

void wso_device::ColorCamera::setAcquisitionMode(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->AcquisitionMode.SetValue((AcquisitionModeEnums)val);
	return;
}

void wso_device::ColorCamera::setAcquisitionFrameCount(unsigned int val)
{
	val = (val < 2 ? 2 : val);
	impl().getCamera()->AcquisitionFrameCount.SetValue(val);
	return;
}

void wso_device::ColorCamera::setPixelFormat(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->PixelFormat.SetValue((PixelFormatEnums)val);
	return;
}

void wso_device::ColorCamera::setBinningHorizontal(unsigned int val)
{
	val = (val < 1 ? 1 : val);
	impl().getCamera()->BinningHorizontal.SetValue(val);
	return;
}

void wso_device::ColorCamera::setBinningVertical(unsigned int val)
{
	val = (val < 1 ? 1 : val);
	impl().getCamera()->BinningVertical.SetValue(val);
	return;
}

void wso_device::ColorCamera::setGain(float val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->Gain.SetValue(val);
	return;
}

void wso_device::ColorCamera::setAdcBitDepth(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	impl().getCamera()->AdcBitDepth.SetValue((AdcBitDepthEnums)val);
	return;
}

unsigned int wso_device::ColorCamera::getFlipMode()
{
	bool bX = false;
	bool bY = false;

	unsigned int ReverseX = impl().getCamera()->ReverseX.GetValue();
	unsigned int ReverseY = impl().getCamera()->ReverseY.GetValue();

	bX = static_cast<bool>(ReverseX);
	bY = static_cast<bool>(ReverseY);

	if (bX == true && bY == false)
		return 1;
	else if (bX == false && bY == true)
		return 2;
	else if (bX == true && bY == true)
		return 3;
	else
		return 0;
}

bool wso_device::ColorCamera::loadConfig()
{
	bool bRet = false;

	IniFile ini(L".//myConfig.ini");

	int nRet = 0;
	nRet = ini.ReadInt(L"ColorCameraSettings", L"Roi_X_Width");
	return bRet;
}

bool wso_device::ColorCamera::loadConfigFromIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		auto& camsets = impl().cameraSets;
		camsets.roi_x_width = ini->ReadUInt(L"COLOR_CAMERA", L"RoiXWidth");
		camsets.roi_y_height = ini->ReadUInt(L"COLOR_CAMERA", L"RoiYHeight");
		camsets.roi_x_offset = ini->ReadUInt(L"COLOR_CAMERA", L"RoiXOffset");
		camsets.roi_y_offset = ini->ReadUInt(L"COLOR_CAMERA", L"RoiYOffset");
		camsets.exposure_time = ini->ReadUInt(L"COLOR_CAMERA", L"ExposureTime");
		camsets.acqusitionMode = ini->ReadUInt(L"COLOR_CAMERA", L"AcqusitionMode");
		camsets.acqusitionFrameCount = ini->ReadUInt(L"COLOR_CAMERA", L"AcqusitionFrameCount");
		camsets.pixelFormat = ini->ReadUInt(L"COLOR_CAMERA", L"PixelFormat");
		camsets.binningHorizontal = ini->ReadUInt(L"COLOR_CAMERA", L"BinningHorizontal", 1);
		camsets.binningVertical = ini->ReadUInt(L"COLOR_CAMERA", L"BinningVertical", 1);
		camsets.gain = ini->ReadFloat(L"COLOR_CAMERA", L"Gain");
		camsets.adcDepthIndex = ini->ReadUInt(L"COLOR_CAMERA", L"AdcDepth");

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}
	return bRet;
}

bool wso_device::ColorCamera::saveConfigToIniFile()
{
	bool bRet = false;
	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		auto& camsets = impl().cameraSets;
		ini->WriteInt(L"COLOR_CAMERA", L"RoiXWidth", camsets.roi_x_width);
		ini->WriteInt(L"COLOR_CAMERA", L"RoiYHeight", camsets.roi_y_height);
		ini->WriteInt(L"COLOR_CAMERA", L"RoiXOffset", camsets.roi_x_offset);
		ini->WriteInt(L"COLOR_CAMERA", L"RoiYOffset", camsets.roi_y_offset);
		ini->WriteInt(L"COLOR_CAMERA", L"ExposureTime", camsets.exposure_time);
		ini->WriteInt(L"COLOR_CAMERA", L"AcqusitionMode", camsets.acqusitionMode);
		ini->WriteInt(L"COLOR_CAMERA", L"AcqusitionFrameCount", camsets.acqusitionFrameCount);
		ini->WriteInt(L"COLOR_CAMERA", L"PixelFormat", camsets.pixelFormat);
		ini->WriteInt(L"COLOR_CAMERA", L"BinningHorizontal", camsets.binningHorizontal);
		ini->WriteInt(L"COLOR_CAMERA", L"BinningVertical", camsets.binningVertical);
		ini->WriteFloat(L"COLOR_CAMERA", L"Gain", camsets.gain);
		ini->WriteInt(L"COLOR_CAMERA", L"AdcDepth", camsets.adcDepthIndex);

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}
	return bRet;
}


#pragma endregion

#pragma region Private

ColorCamera::ColorCameraImpl& wso_device::ColorCamera::impl(void) const
{
	return *d_ptr;
}

void wso_device::ColorCamera::acquireCameraData(void)
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();
	uint32_t frameSize = getFrameSizeInBytes();

	uint32_t frameCount;

	unsigned char* buffer = impl().sonyFrameBuffer;

	if (!impl().liveMode)
	{
		impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame);
		 impl().getCamera()->AcquisitionFrameCount.GetValue();

		impl().getCamera()->BeginAcquisition();
		impl().rawImage = impl().getCamera()->GetNextImage();
		
		ImageProcessor processor;
		processor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_DIRECTIONAL_FILTER);
		impl().rawImage = processor.Convert(impl().rawImage, PixelFormat_BayerGB16);
		
		
		std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

		impl().getCamera()->EndAcquisition();
	}
	else
	{
		impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		//impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerGB16);
		//impl().getCamera()->ReverseX.SetValue(false);
		//impl().getCamera()->ReverseY.SetValue(false);
		//impl().getCamera()->BinningHorizontalMode.SetValue(BinningHorizontalMode_Average);
		//impl().getCamera()->BinningVerticalMode.SetIntValue(BinningVerticalMode_Average);
		//impl().getCamera()->BinningHorizontal.SetValue(4);
		//impl().getCamera()->BinningVertical.SetValue(4);
		//impl().getCamera()->Width.SetValue(4000);
		//impl().getCamera()->Height.SetValue(3000);
		//impl().getCamera()->OffsetX.SetValue(736);
		//impl().getCamera()->OffsetY.SetValue(324);
		//int nMaxWidth = (int)impl().getCamera()->WidthMax.GetValue();
		//int nMaxHeight = (int)impl().getCamera()->HeightMax.GetValue();
		//impl().getCamera()->OffsetX.SetValue(0);
		//impl().getCamera()->OffsetY.SetValue(0);
		//impl().getCamera()->Width.SetValue(4000);
		//impl().getCamera()->Height.SetValue(3000);
		//impl().getCamera()->OffsetX.SetValue(736);
		//impl().getCamera()->OffsetY.SetValue(324);

		width = getFrameWidth();
		height = getFrameHeight();

		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();
		//double nFrameRateMin = impl().getCamera()->AcquisitionFrameRate.GetMin();
		//double nFrameRateMax = impl().getCamera()->AcquisitionFrameRate.GetMax();
		//double nFrameRate = impl().getCamera()->AcquisitionFrameRate.GetValue();

		frameSize = getFrameSizeForBpp(nBytesPerPixel);
		frameCount = 0;

		impl().getCamera()->BeginAcquisition();

		do {
			frameCount++;

			impl().rawImage = impl().getCamera()->GetNextImage();
			std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

			unique_lock<mutex> lock(impl().mutexLock);
			//CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, getFlipMode());
			CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

		} while (impl().liveMode);

		impl().getCamera()->EndAcquisition();
		//impl().getCamera()->ReverseX.SetValue(false);
		//impl().getCamera()->ReverseY.SetValue(true);
		//impl().getCamera()->BinningHorizontal.SetValue(1);
		//impl().getCamera()->BinningVertical.SetValue(1);
		//impl().getCamera()->Width.SetValue(4000);
		//impl().getCamera()->Height.SetValue(3000);
		//impl().getCamera()->OffsetX.SetValue(400);
	}
	return;
}

void wso_device::ColorCamera::acquireCameraSingleFrameData(void) // Single Frame Live
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();
	uint32_t frameSize;

	uint32_t frameCount;

	unsigned char* buffer = impl().sonyFrameBuffer;

	int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_SingleFrame);

	frameCount = 0;
	
	do {
		auto t0 = steady_clock::now();

		frameCount++;
		//setROI_Y_Offset(0);
		//setROI_Y_Height(3000);
		frameSize = getFrameSizeForBpp(nBytesPerPixel);
	
		auto t1 = steady_clock::now();

		impl().getCamera()->BeginAcquisition();

		auto t2 = steady_clock::now();

		impl().rawImage = impl().getCamera()->GetNextImage();

		auto t3 = steady_clock::now();

		std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

		auto t4 = steady_clock::now();

		unique_lock<mutex> lock(impl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

		auto t5 = steady_clock::now();

		impl().rawImage->Release();

		auto t6 = steady_clock::now();

		impl().getCamera()->EndAcquisition();

		auto t7 = steady_clock::now();

		double ms = duration<double, std::milli>(t7 - t0).count();
		std::printf("Total Sequence = %.3f ms\n", ms);
		LogD() << "Total Sequence = " << ms;

		ms = duration<double, std::milli>(t1 - t0).count();
		LogD() << "Set ROI Height = " << ms;

		ms = duration<double, std::milli>(t2 - t1).count();
		LogD() << "BeginAcquisition = " << ms;

		ms = duration<double, std::milli>(t3 - t2).count();
		LogD() << "GetNextImage() = " << ms;

		ms = duration<double, std::milli>(t4 - t3).count();
		LogD() << "memcpy = " << ms;

		ms = duration<double, std::milli>(t5 - t4).count();
		LogD() << "callback = " << ms;

		ms = duration<double, std::milli>(t6 - t5).count();
		LogD() << "rawImage->Release() = " << ms;

		ms = duration<double, std::milli>(t7 - t6).count();
		LogD() << "EndAcquisition() = " << ms;

	} while (impl().liveMode);

	return;
}

void wso_device::ColorCamera::acquireCameraSingleSequencerROIData(void)
{
	auto& preset = impl().seqRoiPreset;
	uint32_t totalFrameCount = preset.frameCount;
	LsoCaptureFrameROI* rois = preset.frameRois;

	if (totalFrameCount <= 0)
		return;
	if (rois == nullptr)
		return;

	uint32_t width;
	uint32_t height;
	uint32_t offsetX;
	uint32_t offsetY;

	uint32_t frameSize;
	uint32_t frameCount;

	unsigned char* buffer = impl().sonyFrameBuffer;

	int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

	if (onSequencer() == false)
	{
		return;
	}

	frameCount = 0;

	impl().getCamera()->BeginAcquisition();

	do {

		impl().rawImage = impl().getCamera()->GetNextImage();

		width = rois[frameCount].width;
		height = rois[frameCount].height;
		offsetX = rois[frameCount].offsetX;
		offsetY = rois[frameCount].offsetY;
		
		frameSize = width * height * sizeof(char) * nBytesPerPixel;

		std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

		unique_lock<mutex> lock(impl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraSeqROIFrameCaptured(buffer, width, height, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

		impl().rawImage->Release();

		frameCount++;
		if (frameCount == totalFrameCount)
		{
			frameCount = 0;
		}

	} while (impl().liveMode);

	impl().getCamera()->EndAcquisition();

	offSequencer();

	return;
}

void wso_device::ColorCamera::acquireCameraMultiSequencerROIData(void)
{
	try
	{
		auto& preset = impl().seqRoiPreset;
		uint32_t totalFrameCount = preset.frameCount;
		LsoCaptureFrameROI* rois = preset.frameRois;
		
		std::vector<std::vector<LsoCaptureFrameROI>> roiList = divideParamVector_((int)totalFrameCount, rois);

		if (totalFrameCount <= 0)
			return;
		if (rois == nullptr)
			return;

		uint32_t width;
		uint32_t height;
		uint32_t offsetX;
		uint32_t offsetY;

		uint32_t frameSize;
		uint32_t frameCount;

		unsigned char* buffer = impl().sonyFrameBuffer;

		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		frameCount = 0;

		impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

		do {

			for (int i = 0; i < roiList.size(); ++i)
			{
				{
					unique_lock<mutex> lock(impl().mutexLock);
					if (!impl().liveMode)
						break;
				}

				auto t1 = steady_clock::now();

				setSequencer(roiList[i]);

				auto t2 = steady_clock::now();

				impl().getCamera()->BeginAcquisition();

				auto t3 = steady_clock::now();

				for (int j = 0; j < roiList[i].size(); ++j)
				{
					impl().rawImage = impl().getCamera()->GetNextImage();

					width = roiList[i][j].width;
					height = roiList[i][j].height;
					offsetX = roiList[i][j].offsetX;
					offsetY = roiList[i][j].offsetY;

					frameSize = width * height * sizeof(char) * nBytesPerPixel;
					std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

					CallbackRegistry::getInstance()->runColorCameraSeqROIFrameCaptured(buffer, width, height, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

					impl().rawImage->Release();

					frameCount++;
					if (frameCount == totalFrameCount)
					{
						frameCount = 0;
					}
				}

				auto t6 = steady_clock::now();

				impl().getCamera()->EndAcquisition();

				auto t7 = steady_clock::now();

				impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);
				
				auto t8 = steady_clock::now();

				double ms = duration<double, std::milli>(t2 - t1).count();
				LogD() << "Set setSequencer = " << ms;

				ms = duration<double, std::milli>(t3 - t2).count();
				LogD() << "BeginAcquisition = " << ms;

				ms = duration<double, std::milli>(t7 - t6).count();
				LogD() << "EndAcquisition() = " << ms;

				ms = duration<double, std::milli>(t8 - t7).count();
				LogD() << "SequencerMode_Off() = " << ms;
			}

		} while (impl().liveMode);

		offSequencer();

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraMultiSequencerROIData!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraOffsetROIData(void)
{
	try
	{
		auto& preset = impl().offsetRoiPreset;
		uint32_t totalFrameCount = preset.frameCount;
		uint32_t nRoiWidth = preset.roiWidth;
		uint32_t nRoiHeight = preset.roiHeight;

		LsoCaptureFrameROI* rois = preset.frameRois;

		if (totalFrameCount <= 0)
			return;
		if (rois == nullptr)
			return;

		uint32_t frameSize;
		uint32_t frameCount;

		unsigned char* buffer = impl().sonyFrameBuffer;

		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		frameCount = 0;

		if (impl().getCamera()->IsStreaming())
			impl().getCamera()->EndAcquisition();

		// Width, Height 설정
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);

		//SetEventMode(true);
		// Software Trigger 설정
		setupSoftwareTrigger();

		impl().getCamera()->BeginAcquisition();

		do {

			impl().nOffsetFrameCount = 0;

			auto t0 = steady_clock::now();

			for (int i = 0; i < totalFrameCount; ++i)
			{
				// liveMode 체크를 뮤텍스로 보호
				{
					unique_lock<mutex> lock(impl().mutexLock);
					if (!impl().liveMode)
						break;
				}

				//LogD() << "================================";
				//LogD() << "ROI Frame : " << i;

				uint32_t offsetX = rois[i].offsetX;
				uint32_t offsetY = rois[i].offsetY;

				//auto t1 = steady_clock::now();
				//setROI_X_Offset(Params[i].OffsetX);
				setROI_Y_Offset(offsetY);

				//auto t2 = steady_clock::now();

				//Sleep(1);

				impl().getCamera()->TriggerSoftware.Execute();

				//auto t3 = steady_clock::now();

				impl().rawImage = impl().getCamera()->GetNextImage();

				//auto t4 = steady_clock::now();
				std::memcpy(buffer, impl().rawImage->GetData(), frameSize);


				//auto t5 = steady_clock::now();
				//unique_lock<mutex> lock(impl().mutexLock);
				CallbackRegistry::getInstance()->runColorCameraOffsetROIFrameCaptured(buffer, nRoiWidth, nRoiHeight, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

				//auto t6 = steady_clock::now();

				impl().rawImage->Release();

				//auto t7 = steady_clock::now();

				frameCount++;
				if (frameCount == totalFrameCount)
				{
					frameCount = 0;
				}

				//Sleep(50);

				//impl().nOffsetFrameCount++;
				//if (impl().nOffsetFrameCount == totalFrameCount)
				//{
				//	impl().nOffsetFrameCount = 0;
				//}

				//auto m0 = steady_clock::now();

				//double ms = duration<double, std::milli>(t2 - t1).count();
				//LogD() << "setROI_Y_Offset = " << ms;

				//ms = duration<double, std::milli>(t3 - t2).count();
				//LogD() << "impl().getCamera()->TriggerSoftware.Execute() = " << ms;

				//ms = duration<double, std::milli>(t4 - t3).count();
				//LogD() << "impl().rawImage = impl().getCamera()->GetNextImage() = " << ms;

				//ms = duration<double, std::milli>(t5 - t4).count();
				//LogD() << "std::memcpy() = " << ms;

				//ms = duration<double, std::milli>(t6 - t5).count();
				//LogD() << "Callback = " << ms;

				//ms = duration<double, std::milli>(t7 - t6).count();
				//LogD() << "impl().rawImage->Release() = " << ms;

				//ms = duration<double, std::milli>(t7 - t1).count();
				//LogD() << "ROI Frame = " << ms;

				//LogD() << "================================";

				//auto m1 = steady_clock::now();

				//double messgeMs = duration<double, std::milli>(m1 - m0).count();
				//LogD() << "Message Ms = " << messgeMs;
			}

			auto t8 = steady_clock::now();

			double totalms = duration<double, std::milli>(t8 - t0).count();
			LogD() << "Total 1 Frame = " << totalms;

		} while (impl().liveMode);

		impl().getCamera()->EndAcquisition();

		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		//SetEventMode(false);

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraOffsetROIData!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCaptureCameraOffsetROIData(void)
{
	try
	{
		auto& preset = impl().offsetRoiPreset;
		uint32_t totalFrameCount = preset.frameCount;
		uint32_t nRoiWidth = preset.roiWidth;
		uint32_t nRoiHeight = preset.roiHeight;

		LsoCaptureFrameROI* rois = preset.frameRois;

		if (totalFrameCount <= 0)
			return;
		if (rois == nullptr)
			return;

		uint32_t frameSize;
		uint32_t frameCount;

		unsigned char* buffer = impl().sonyFrameBuffer;

		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		frameCount = 0;

		if (impl().getCamera()->IsStreaming())
			impl().getCamera()->EndAcquisition();

		// Width, Height 설정
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);

		//SetEventMode(true);
		// Software Trigger 설정
		setupSoftwareTrigger(2); // Multiframe
		setAcquisitionFrameCount(totalFrameCount);

		impl().getCamera()->BeginAcquisition();

		auto t0 = steady_clock::now();

		for (int i = 0; i < totalFrameCount; ++i)
		{
			// liveMode 체크를 뮤텍스로 보호
			//{
			//	unique_lock<mutex> lock(impl().mutexLock);
			//	if (!impl().liveMode)
			//		break;
			//}

			//LogD() << "================================";
			//LogD() << "ROI Frame : " << i;

			uint32_t offsetX = rois[i].offsetX;
			uint32_t offsetY = rois[i].offsetY;

			//auto t1 = steady_clock::now();
			//setROI_X_Offset(Params[i].OffsetX);
			setROI_Y_Offset(offsetY);

			//auto t2 = steady_clock::now();

			//Sleep(1);

			impl().getCamera()->TriggerSoftware.Execute();

			//auto t3 = steady_clock::now();

			impl().rawImage = impl().getCamera()->GetNextImage(3000);

			//auto t4 = steady_clock::now();
			std::memcpy(buffer, impl().rawImage->GetData(), frameSize);


			//auto t5 = steady_clock::now();
			unique_lock<mutex> lock(impl().mutexLock);
			CallbackRegistry::getInstance()->runColorCameraOffsetROIFrameCaptured(buffer, nRoiWidth, nRoiHeight, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

			//auto t6 = steady_clock::now();

			impl().rawImage->Release();

			//auto t7 = steady_clock::now();

			frameCount++;
			if (frameCount == totalFrameCount)
			{
				frameCount = 0;
			}

			//impl().nOffsetFrameCount++;
			//if (impl().nOffsetFrameCount == totalFrameCount)
			//{
			//	impl().nOffsetFrameCount = 0;
			//}

			//auto m0 = steady_clock::now();

			//double ms = duration<double, std::milli>(t2 - t1).count();
			//LogD() << "setROI_Y_Offset = " << ms;

			//ms = duration<double, std::milli>(t3 - t2).count();
			//LogD() << "impl().getCamera()->TriggerSoftware.Execute() = " << ms;

			//ms = duration<double, std::milli>(t4 - t3).count();
			//LogD() << "impl().rawImage = impl().getCamera()->GetNextImage() = " << ms;

			//ms = duration<double, std::milli>(t5 - t4).count();
			//LogD() << "std::memcpy() = " << ms;

			//ms = duration<double, std::milli>(t6 - t5).count();
			//LogD() << "Callback = " << ms;

			//ms = duration<double, std::milli>(t7 - t6).count();
			//LogD() << "impl().rawImage->Release() = " << ms;

			//ms = duration<double, std::milli>(t7 - t1).count();
			//LogD() << "ROI Frame = " << ms;

			//LogD() << "================================";

			//auto m1 = steady_clock::now();

			//double messgeMs = duration<double, std::milli>(m1 - m0).count();
			//LogD() << "Message Ms = " << messgeMs;
		}

		auto t8 = steady_clock::now();

		double totalms = duration<double, std::milli>(t8 - t0).count();
		LogD() << "Total 1 Frame = " << totalms;

		impl().getCamera()->EndAcquisition();

		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		impl().liveMode = false;

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraOffsetROIData!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraRollSwTrigOverlapLive(void)
{
	try
	{
		auto& preset = impl().rollSwTrigPreset;
		uint32_t totalFrameCount = preset.overlap.overlapFrameCount;
		uint32_t nRoiWidth = preset.overlap.frameWidth;
		uint32_t nRoiHeight = preset.overlap.frameHeight;
		uint32_t nInterval = preset.overlap.triggerIntervalMs;
		uint32_t nOffsetX = preset.overlap.offsetX;
		uint32_t nOffsetY = preset.overlap.offsetY;

		if (totalFrameCount <= 0)
			return;

		unsigned char* buffer = impl().sonyFrameBuffer;

		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		uint32_t frameCount = 0;
		uint32_t frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		if (impl().getCamera()->IsStreaming())
			impl().getCamera()->EndAcquisition();

		int nMaxWidth = getROI_Max_Width();
		int nMaxHeight = getROI_Max_Height();

		// Width, Height 설정
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nMaxWidth);
		setROI_Y_Height(nMaxHeight);

		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);
		setROI_X_Offset(nOffsetX);
		setROI_Y_Offset(nOffsetY);

		//SetEventMode(true);
		// Software Trigger 설정
		setupSoftwareTrigger();

		impl().getCamera()->BeginAcquisition();

		do {
			//auto t0 = steady_clock::now();

			for (int i = 0; i < totalFrameCount; ++i)
			{
				// liveMode 체크를 뮤텍스로 보호
				{
					unique_lock<mutex> lock(impl().mutexLock);
					if (!impl().liveMode)
						break;
				}

				//auto t1 = steady_clock::now();

				impl().getCamera()->TriggerSoftware.Execute();

				//auto t2 = steady_clock::now();

				impl().rawImage = impl().getCamera()->GetNextImage();

				//auto t3 = steady_clock::now();

				std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

				//auto t4 = steady_clock::now();

				CallbackRegistry::getInstance()->runColorCameraRollSWTrigOverlapFrameCaptured(buffer, nRoiWidth, nRoiHeight, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

				//auto t5 = steady_clock::now();

				impl().rawImage->Release();

				//auto t6 = steady_clock::now();

				frameCount++;
				if (frameCount == totalFrameCount)
				{
					frameCount = 0;
				}

				Sleep(nInterval);

				//auto t7 = steady_clock::now();

				//double ms = duration<double, std::milli>(t2 - t1).count();
				//LogD() << "TriggerSoftware.Execute = " << ms;

				//ms = duration<double, std::milli>(t3 - t2).count();
				//LogD() << "impl().getCamera()->GetNextImage = " << ms;

				//ms = duration<double, std::milli>(t4 - t3).count();
				//LogD() << "memcpy = " << ms;

				//ms = duration<double, std::milli>(t5 - t4).count();
				//LogD() << "Callback = " << ms;

				//ms = duration<double, std::milli>(t6 - t5).count();
				//LogD() << "Release = " << ms;

				//double ROIms = duration<double, std::milli>(t7 - t1).count();
				//LogD() << "ROI Frame = " << ROIms;
			}

			//auto t8 = steady_clock::now();

			//double totalms = duration<double, std::milli>(t8 - t0).count();
			//LogD() << "Total 1 Frame = " << totalms;

		} while (impl().liveMode);

		impl().getCamera()->EndAcquisition();

		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraRollSWTrigOverlapLive!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraRollSwTrigOverlapCapture(void)
{
	try
	{
		auto& preset = impl().rollSwTrigPreset;
		uint32_t totalFrameCount = preset.overlap.overlapFrameCount;
		uint32_t nRoiWidth = preset.overlap.frameWidth;
		uint32_t nRoiHeight = preset.overlap.frameHeight;
		uint32_t nInterval = preset.overlap.triggerIntervalMs;
		uint32_t nOffsetX = preset.overlap.offsetX;
		uint32_t nOffsetY = preset.overlap.offsetY;

		if (totalFrameCount <= 0)
			return;

		unsigned char* buffer = impl().sonyFrameBuffer;

		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		uint32_t frameCount = 0;
		uint32_t frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		if (impl().getCamera()->IsStreaming())
			impl().getCamera()->EndAcquisition();

		// Width, Height 설정
		int nMaxWidth = getROI_Max_Width();
		int nMaxHeight = getROI_Max_Height();

		// Width, Height 설정
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nMaxWidth);
		setROI_Y_Height(nMaxHeight);

		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);
		setROI_X_Offset(nOffsetX);
		setROI_Y_Offset(nOffsetY);

		// Software Trigger 설정
		setupSoftwareTrigger(2); // Multiframe
		setAcquisitionFrameCount(totalFrameCount);

		impl().getCamera()->BeginAcquisition();

		for (int i = 0; i < totalFrameCount; ++i)
		{
			impl().getCamera()->TriggerSoftware.Execute();

			impl().rawImage = impl().getCamera()->GetNextImage();

			std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

			CallbackRegistry::getInstance()->runColorCameraRollSWTrigOverlapFrameCaptured(buffer, nRoiWidth, nRoiHeight, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

			impl().rawImage->Release();

			frameCount++;
			if (frameCount == totalFrameCount)
			{
				frameCount = 0;
			}

			Sleep(nInterval);
		}

		impl().getCamera()->EndAcquisition();

		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		impl().liveMode = false;

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraRollSWTrigOverlapCapture!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraRollHwTriggerCaptureData(void)
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();

	int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);

	uint32_t frameCount = 0;

	unsigned char* buffer = impl().sonyFrameBuffer;

	uint32_t nTotalFrameCount;
	if (getAcquisitionMode() == AcquisitionMode_SingleFrame)
	{
		nTotalFrameCount = 1;
	}
	else
	{
		nTotalFrameCount = (int)impl().getCamera()->AcquisitionFrameCount.GetValue();
	}

	impl().getCamera()->BeginAcquisition();

	for (uint32_t frameCount = 0; frameCount < nTotalFrameCount; ++frameCount)
	{
		try
		{
			impl().rawImage = impl().getCamera()->GetNextImage(3000);

			// 정상적으로 이미지를 받은 경우
			std::memcpy(buffer, impl().rawImage->GetData(), frameSize);
		}
		catch (const Spinnaker::Exception& e)
		{
			// 타임아웃 또는 기타 오류 발생 시
			LogD() << "Failed to get image at frame " << frameCount << ": " << e.what();

			// buffer를 채워서 회색 이미지 생성
			std::memset(buffer, 0x80, frameSize);
		}

		unique_lock<mutex> lock(impl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraImageCaptured(buffer, width, height, frameCount, nTotalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);
	}

	impl().getCamera()->EndAcquisition();
}

bool wso_device::ColorCamera::onSequencer(void)
{
	bool bRet = false;

	try
	{
		auto& preset = impl().seqRoiPreset;
		int nIndexNextSet = 0;
		int nSeqSetCount = preset.frameCount;
		auto* rois = preset.frameRois;

		// Sequencer Mode Off
		impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);
		// Sequencer Configuration Mode > On
		impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_On);

		////////////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < nSeqSetCount; ++i)
		{
			impl().getCamera()->SequencerSetSelector.SetValue(i);

			// Set i의 카메라 파라미터 설정
			//impl().getCamera()->ExposureTime.SetValue(2000);  // 20ms 노출 시간
			//impl().getCamera()->Gain.SetValue(2.0);            // 낮은 게인
			impl().getCamera()->OffsetX.SetValue(0);         
			impl().getCamera()->OffsetY.SetValue(0);

			impl().getCamera()->Width.SetValue(rois[i].width);          // ROI 폭
			impl().getCamera()->Height.SetValue(rois[i].height);         // ROI 높이
			impl().getCamera()->OffsetX.SetValue(rois[i].offsetX);         // X 오프셋
			impl().getCamera()->OffsetY.SetValue(rois[i].offsetY);         // Y 오프셋

			// Set 0의 Sequencer 트리거 설정
			impl().getCamera()->SequencerTriggerSource.SetValue(SequencerTriggerSource_FrameStart);
			impl().getCamera()->SequencerTriggerActivation.SetValue(SequencerTriggerActivation_RisingEdge);

			
			// Set Next Sequence Set 
			++nIndexNextSet;
			if (nIndexNextSet >= nSeqSetCount) nIndexNextSet = 0;

			impl().getCamera()->SequencerSetNext.SetValue(nIndexNextSet);

			// Set i 저장
			impl().getCamera()->SequencerSetSave.Execute();

			LogD() << "Sequencer Set " << i << " configured and saved";
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		// 6. 시작 세트 지정
		impl().getCamera()->SequencerSetStart.SetValue(0);

		// 7. Sequencer Configuration 모드를 Off로 설정 (설정 완료)
		impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);

		// 8. Sequencer 설정 유효성 검사
		if (impl().getCamera()->SequencerConfigurationValid.GetValue() == SequencerConfigurationValid_Yes) {
			LogD() << "Sequencer configuration is valid";

			// 9. Sequencer 모드 활성화
			impl().getCamera()->SequencerMode.SetValue(SequencerMode_On);
			LogD() << "Sequencer mode activated for BFS-U3-122S6C";

			bRet = true;
		}
		else {
			LogD() << "Sequencer configuration is invalid";
			impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);

			bRet = false;
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred during Sequencer setup!";
		LogD() << elog;

		// 예외 발생 시 Sequencer 비활성화
		try {
			impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);
			impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);
		}
		catch (...) {
			LogD() << "Failed to cleanup Sequencer after exception";
		}

		bRet = false;
	}

	return bRet;
}

bool wso_device::ColorCamera::setSequencer(std::vector<LsoCaptureFrameROI> paramList)
{
	bool bRet = false;

	try
	{
		int nIndexNextSet = 0;
		int nSeqSetCount = (int)paramList.size();

		// Sequencer Mode Off
		int nMode = impl().getCamera()->SequencerMode.GetValue();
		if (nMode == SequencerMode_On)
		{
			impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);
		}
		// Sequencer Configuration Mode > On
		impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_On);

		////////////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < nSeqSetCount; ++i)
		{
			impl().getCamera()->SequencerSetSelector.SetValue(i);

			// Set i의 카메라 파라미터 설정
			//impl().getCamera()->ExposureTime.SetValue(2000);  // 20ms 노출 시간
			//impl().getCamera()->Gain.SetValue(2.0);            // 낮은 게인
			impl().getCamera()->OffsetX.SetValue(0);
			impl().getCamera()->OffsetY.SetValue(0);

			impl().getCamera()->Width.SetValue(paramList[i].width);          // ROI 폭
			impl().getCamera()->Height.SetValue(paramList[i].height);         // ROI 높이
			impl().getCamera()->OffsetX.SetValue(paramList[i].offsetX);         // X 오프셋
			impl().getCamera()->OffsetY.SetValue(paramList[i].offsetY);         // Y 오프셋

			// Set 0의 Sequencer 트리거 설정
			impl().getCamera()->SequencerTriggerSource.SetValue(SequencerTriggerSource_FrameStart);
			impl().getCamera()->SequencerTriggerActivation.SetValue(SequencerTriggerActivation_RisingEdge);


			// Set Next Sequence Set 
			++nIndexNextSet;
			if (nIndexNextSet >= nSeqSetCount) nIndexNextSet = 0;

			impl().getCamera()->SequencerSetNext.SetValue(nIndexNextSet);

			// Set i 저장
			impl().getCamera()->SequencerSetSave.Execute();

			LogD() << "Sequencer Set " << i << " configured and saved";
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		// 6. 시작 세트 지정
		impl().getCamera()->SequencerSetStart.SetValue(0);

		// 7. Sequencer Configuration 모드를 Off로 설정 (설정 완료)
		impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);

		// 8. Sequencer 설정 유효성 검사
		if (impl().getCamera()->SequencerConfigurationValid.GetValue() == SequencerConfigurationValid_Yes) {
			LogD() << "Sequencer configuration is valid";

			// 9. Sequencer 모드 활성화
			impl().getCamera()->SequencerMode.SetValue(SequencerMode_On);
			LogD() << "Sequencer mode activated for BFS-U3-122S6C";

			bRet = true;
		}
		else {
			LogD() << "Sequencer configuration is invalid";
			//impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);

			bRet = false;
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred during Sequencer setup!";
		LogD() << elog;

		// 예외 발생 시 Sequencer 비활성화
		try {
			impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);
			impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);
		}
		catch (...) {
			LogD() << "Failed to cleanup Sequencer after exception";
		}

		bRet = false;
	}

	return bRet;
}

void wso_device::ColorCamera::offSequencer(void)
{
	impl().getCamera()->SequencerMode.SetValue(SequencerMode_Off);

	//loadUserSet_(1);
}

std::vector<std::vector<LsoCaptureFrameROI>> wso_device::ColorCamera::divideParamVector_(int nFrameCount, LsoCaptureFrameROI* arrayParams)
{
	std::vector<std::vector<LsoCaptureFrameROI>> result;

	if (arrayParams == nullptr || nFrameCount <= 0) {
		return result;
	}

	const int chunkSize = 8;
	int currentIndex = 0;

	while (currentIndex < nFrameCount) {
		std::vector<LsoCaptureFrameROI> chunk;

		// 현재 청크에 최대 8개 또는 남은 개수만큼 추가
		int endIndex = std::min((int)(currentIndex + chunkSize), nFrameCount);

		for (int i = currentIndex; i < endIndex; ++i) {
			chunk.push_back(arrayParams[i]);
		}

		result.push_back(chunk);
		currentIndex = endIndex;
	}

	return result;
}

void wso_device::ColorCamera::saveUserSet_(int eUserSet)
{
	switch (eUserSet)
	{
		case 1:
			{
				impl().getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet0);
			}
			break;
		case 2:
			{
				impl().getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet1);
			}
			break;
		default:
			{
				impl().getCamera()->UserSetSelector.SetValue(UserSetSelector_Default);
			}
			break;
	}
	impl().getCamera()->UserSetSave.Execute();
}

void wso_device::ColorCamera::loadUserSet_(int eUserSet)
{
	switch (eUserSet)
	{
	case 1:
	{
		impl().getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet0);
	}
	break;
	case 2:
	{
		impl().getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet1);
	}
	break;
	default:
	{
		impl().getCamera()->UserSetSelector.SetValue(UserSetSelector_Default);
	}
	break;
	}

	impl().getCamera()->UserSetLoad.Execute();
}

void wso_device::ColorCamera::acquireCameraCaptureData(void)
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();

	int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);

	uint32_t frameCount = 0;

	unsigned char* buffer = impl().sonyFrameBuffer;

	uint32_t nTotalFrameCount;
	if (getAcquisitionMode() == AcquisitionMode_SingleFrame)
	{
		nTotalFrameCount = 1;
	}
	else
	{
		nTotalFrameCount = (int)impl().getCamera()->AcquisitionFrameCount.GetValue();
	}

	impl().getCamera()->BeginAcquisition();

	for (uint32_t frameCount = 0; frameCount < nTotalFrameCount; ++frameCount)
	{
		impl().rawImage = impl().getCamera()->GetNextImage();

		//ImageProcessor processor;
		//ImagePtr convertedImage = processor.Convert(impl().rawImage, PixelFormat_RGB8);

		//// Create a unique filename
		//ostringstream filename;

		//filename << "Acquisition-";
		//filename << nIndex << ".jpg";
		//convertedImage->Save(filename.str().c_str());

		std::memcpy(buffer, impl().rawImage->GetData(), frameSize);
		unique_lock<mutex> lock(impl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraImageCaptured(buffer, width, height, frameCount, nTotalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);
	}

	impl().getCamera()->EndAcquisition();
}

int wso_device::ColorCamera::getBytesPerPixel()
{
	int nRet = -1;

	try
	{
		unsigned int nPixelFormat = getPixelFormat();
		PixelFormatEnums ePixelFormat = (PixelFormatEnums)nPixelFormat;

		switch (ePixelFormat)
		{
			case PixelFormat_Mono8:
				nRet = 1;
				break;
			case PixelFormat_Mono16:
				nRet = 2;
				break;
			case PixelFormat_BayerRG8:
				nRet = 1;
				break;
			case PixelFormat_BayerRG16:
				nRet = 2;
				break;
			case PixelFormat_RGB8Packed:
				nRet = 3;
				break;
			case PixelFormat_BGR8:
				nRet = 3;
				break;
			default:
				nRet = -1;
				break;
		}
	}
	catch (Exception ex)
	{
		nRet = -1;
	}
	return nRet;
}

void wso_device::ColorCamera::OnSpinnakerCameraEvent(std::string eventName)
{
	try
	{
		std::string strEventName = eventName.c_str();

		if (strEventName == "EventExposureEnd")
		{
			// ExposureEnd 이벤트 처리
			try {

				auto t0 = steady_clock::now();

				unique_lock<mutex> lock(impl().mutexLock);

				uint64_t frameID = impl().getCamera()->EventExposureEndFrameID.GetValue();
				uint64_t timestamp = impl().getCamera()->EventExposureEndTimestamp.GetValue();

				LogD() << "ExposureEnd Event - FrameID: " + std::to_string(frameID) << ", Timestamp: " + std::to_string(timestamp);

				auto& preset = impl().offsetRoiPreset;
				uint32_t frameCount = impl().nOffsetFrameCount;
				uint32_t totalFrameCount = preset.frameCount;
				uint32_t nRoiWidth = preset.roiWidth;
				uint32_t nRoiHeight = preset.roiHeight;

				unsigned char* buffer = impl().sonyFrameBuffer;

				int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
				int nBytesPerPixel = getBytesPerPixel();

				uint32_t frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

				impl().rawImage = impl().getCamera()->GetNextImage();

				std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

				//unique_lock<mutex> lock(impl().mutexHandleLock);
				CallbackRegistry::getInstance()->runColorCameraOffsetROIFrameCaptured(buffer, nRoiWidth, nRoiHeight, 0, 0, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

				impl().rawImage->Release();

				auto t8 = steady_clock::now();

				double totalms = duration<double, std::milli>(t8 - t0).count();
				LogD() << "Event Total 1 Frame = " << totalms;

				// 콜백 실행 (필요시 CallbackRegistry에 추가)
				//CallbackRegistry::getInstance()->runColorCameraExposureEndEvent(frameID, timestamp);
			}
			catch (const std::exception& e) {
				LogD() << "Error processing ExposureEnd event: " << std::string(e.what());
			}
		}
		//else if (strEventName == "EventError")
		//{
		//	// Error 이벤트 처리
		//	try {
		//		uint64_t errorCode = impl().getCamera()->EventErrorCode.GetValue();
		//		uint64_t timestamp = impl().getCamera()->EventErrorTimestamp.GetValue();

		//		LogD() << "Error Event - Code: " + std::to_string(errorCode) + ", Timestamp: " + std::to_string(timestamp);
		//	}
		//	catch (const std::exception& e) {
		//		LogD() << "Error processing Error event: " + std::string(e.what());
		//	}
		//}
		//else if (strEventName == "EventSerialPortReceive")
		//{
		//	// SerialPortReceive 이벤트 처리
		//	try {
		//		uint64_t timestamp = impl().getCamera()->EventSerialPortReceiveTimestamp.GetValue();
		//		std::string serialData = impl().getCamera()->EventSerialData.GetValue().c_str();

		//		LogD() << "SerialPortReceive Event - Data: " + serialData + ", Timestamp: " + std::to_string(timestamp);
		//	}
		//	catch (const std::exception& e) {
		//		LogD() << "Error processing SerialPortReceive event: " + std::string(e.what());
		//	}
		//}
		else
		{
			LogD() << "Unhandled Device Event: " + strEventName;
		}
	}
	catch (const std::exception& e)
	{
		LogD() << "Exception in HandleDeviceEvent: " + std::string(e.what());
	}
}

void wso_device::ColorCamera::SetEventMode(bool enableEvents)
{
	try
	{
		if (!isInitiated()) {
			LogD() << "ColorCamera is not initialized!";
			return;
		}

		//if (!impl().getCamera()->IsValid()) {
		//	WsoLogError("Camera is not valid!");
		//	return;
		//}

		//if (impl().getCamera()->IsStreaming()) {
		//	impl().getCamera()->EndAcquisition();
		//	WsoLogDebug("Acquisition stopped for event mode setup");
		//}

		if (enableEvents)
		{
			// DeviceEventHandler 생성 (아직 생성되지 않은 경우)
			if (!impl().m_deviceEventHandler) {
				impl().m_deviceEventHandler = std::make_unique<ColorCameraDeviceEventHandler>(this);
			}

			// ExposureEnd 이벤트 설정 및 핸들러 등록
			impl().getCamera()->EventSelector.SetValue(EventSelector_ExposureEnd);
			impl().getCamera()->EventNotification.SetValue(EventNotification_On);
			//impl().getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventExposureEnd");
			impl().getCamera()->RegisterEventHandler(*(impl().m_deviceEventHandler));
			LogD() << "ExposureEnd event enabled and handler registered";

			//// Error 이벤트 설정 및 핸들러 등록
			//impl().getCamera()->EventSelector.SetValue(EventSelector_Error);
			//impl().getCamera()->EventNotification.SetValue(EventNotification_On);
			//impl().getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventError");
			//WsoLogInfo("Error event enabled and handler registered");

			//// SerialPortReceive 이벤트 설정 및 핸들러 등록 (필요한 경우)
			//impl().getCamera()->EventSelector.SetValue(EventSelector_SerialPortReceive);
			//impl().getCamera()->EventNotification.SetValue(EventNotification_On);
			//impl().getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventSerialPortReceive");
			//WsoLogInfo("SerialPortReceive event enabled and handler registered");

			LogD() << "Event Control Mode activated for BFS-U3-122S6";
		}
		else
		{
			// EventHandler 등록 해제
			if (impl().m_deviceEventHandler) {
				try {
					impl().getCamera()->UnregisterEventHandler(*(impl().m_deviceEventHandler));
					LogD() << "Event handlers unregistered";
				}
				catch (const std::exception& e) {
					LogD() << "Error unregistering event handlers: " << std::string(e.what());
				}
			}

			// 모든 이벤트 비활성화
			impl().getCamera()->EventSelector.SetValue(EventSelector_ExposureEnd);
			impl().getCamera()->EventNotification.SetValue(EventNotification_Off);

			//impl().getCamera()->EventSelector.SetValue(EventSelector_Error);
			//impl().getCamera()->EventNotification.SetValue(EventNotification_Off);

			//impl().getCamera()->EventSelector.SetValue(EventSelector_SerialPortReceive);
			//impl().getCamera()->EventNotification.SetValue(EventNotification_Off);

			LogD() << "Event Control Mode deactivated";
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred during Event Mode setup!";
		LogD() << elog;
	}
}


#pragma endregion

