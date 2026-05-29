#include "pch.h"
#include "ColorCamera.h"

#include "MainBoard.h"
#include <format>
#include <chrono> // steady clock

using namespace wso_device;
using namespace wso_domain;

using namespace Spinnaker;
using namespace std::chrono; // steady clock


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

	//Uses spinnaker
	Spinnaker::SystemPtr systemPtr;
	Spinnaker::CameraList camList;
	Spinnaker::CameraPtr pCam;
	Spinnaker::ImagePtr rawImage;

	atomic<bool> liveMode;
	atomic<bool> originalMode;
	thread worker;

	int nOffsetFrameCount = 0;

	ColorCameraSettingParam* cameraParams = new ColorCameraSettingParam(); // Color Camera base setting 
	CaptureFrameSeqROIPreset* cameraSeqParams = new CaptureFrameSeqROIPreset(); // Use Global Shutter, Sequencer, ROI
	CaptureFrameOffsetROIPreset* cameraOffsetRoiParams = new CaptureFrameOffsetROIPreset();

	CaptureFrameRollSWTrigOverlapPreset* cameraRollSWTrigOverlapParams = new CaptureFrameRollSWTrigOverlapPreset();

	mutex mutexLock;
	mutex mutexHandleLock;

	ColorCameraImpl() : initiated(false), liveMode(false), pCam(NULL), rawImage(NULL), systemPtr(NULL), sonyFrameBuffer({}),
		board(nullptr), sensorId(0)
	{
	}
};

wso_device::ColorCamera::ColorCamera(MainBoard* board) :
	d_ptr(make_unique<ColorCameraImpl>())
{
	getImpl().board = board;
}

ColorCamera::~ColorCamera()
{
	if (m_deviceEventHandler && getCamera() && getCamera()->IsValid()) {
		try {
			getCamera()->UnregisterEventHandler(*m_deviceEventHandler);
		}
		catch (const std::exception& e) {
			WsoLogError("Error unregistering event handler in destructor: " + std::string(e.what()));
		}
	}

	getImpl().rawImage = NULL;
	getCamera() = NULL;
	getImpl().camList.Clear();
	getImpl().systemPtr = NULL;
}

#pragma region Public

bool wso_device::ColorCamera::initialize(void)
{
	getImpl().systemPtr = System::GetInstance();
	getImpl().camList = getImpl().systemPtr->GetCameras();
	unsigned int numCameras = getImpl().camList.GetSize();

	if (numCameras == 0)
	{
		getImpl().camList.Clear();
		getImpl().systemPtr->ReleaseInstance();
		LogE() << "Color camera init failed! ";
		return false;
	}
	getImpl().pCam = getImpl().camList.GetByIndex(0);
	getImpl().pCam->Init();

	loadConfigFromIniFile();

	setInitParameters();

	//saveUserSet_(1);

	getImpl().initiated = true;

	return true;
}

bool wso_device::ColorCamera::isInitiated(void) const
{
	return getImpl().initiated;
}

void wso_device::ColorCamera::uninitialize(void)
{
	// 1. worker 스레드 먼저 종료
	if (getImpl().liveMode) {
		getImpl().liveMode = false;
		if (getImpl().worker.joinable()) {
			getImpl().worker.join();
		}
	}

	// rawImage grab 해제
	getImpl().rawImage = nullptr;

	// 2. 카메라가 아직 스트리밍 중이면 중지
	if (getImpl().pCam && getImpl().pCam->IsStreaming()) {
		try { getImpl().pCam->EndAcquisition(); }
		catch (...) {}
	}
	// 3. 이벤트 핸들러 해제
	if (m_deviceEventHandler && getImpl().pCam && getImpl().pCam->IsValid()) {
		try { getImpl().pCam->UnregisterEventHandler(*m_deviceEventHandler); }
		catch (...) {}
		m_deviceEventHandler.reset();
	}
	// 4. DeInit 호출 (← 현재 완전 누락)
	if (getImpl().pCam && getImpl().pCam->IsValid()) {
		try { getImpl().pCam->DeInit(); }
		catch (...) {}
	}
	// 5. 리소스 해제
	getImpl().pCam = nullptr;
	getImpl().camList.Clear();
	// 6. systemPtr null 체크 후 ReleaseInstance
	if (getImpl().systemPtr) {
		getImpl().systemPtr->ReleaseInstance();
		getImpl().systemPtr = nullptr;
	}
	getImpl().initiated = false;
}

void wso_device::ColorCamera::setInitParameters()
{
	for (int i = 0; i < 3; i++)
	{
		try {
			if (!getCamera()->IsValid()) {
				getCamera()->Init();
			}
			if (getCamera()->IsStreaming()) {
				getCamera()->EndAcquisition();
			}

			std::string strModelName = getCamera()->DeviceModelName.GetValue().c_str();
			std::string strModelMessage = std::format("Color Camera Name is {}", strModelName);
			WsoLogDebug(strModelMessage);

			std::string strModelNameSub = (strModelName.size() <= 6)
				? strModelName
				: strModelName.substr(strModelName.size() - 6);

			// Trigger Mode 비활성화
			int nTriggerMode = getCamera()->TriggerMode.GetValue();
			if (nTriggerMode == TriggerMode_On)
				getCamera()->TriggerMode.SetValue(TriggerMode_Off);

			// Set Indicator LED
			getCamera()->DeviceIndicatorMode.SetValue(DeviceIndicatorMode_Inactive);
			WsoLogDebug("Color Camera Set Indicator LED...ok!");

			// Set Image Size
			getCamera()->OffsetX.SetValue(0);
			getCamera()->OffsetY.SetValue(0);
			
			unsigned int nMaxHeight = getROI_Max_Height();
			unsigned int nMaxWidth = getROI_Max_Width();

			// Set ROI width
			if (getImpl().cameraParams->roi_x_width >= nMaxWidth)
			{
				getCamera()->Width.SetValue(nMaxWidth);
				getCamera()->OffsetX.SetValue(0);
			}
			else
			{
				getCamera()->Width.SetValue(getImpl().cameraParams->roi_x_width);
				getCamera()->OffsetX.SetValue(getImpl().cameraParams->roi_x_offset);
			}

			// Set ROI height
			if (getImpl().cameraParams->roi_y_height >= nMaxHeight)
			{
				getCamera()->Height.SetValue(nMaxHeight);
				getCamera()->OffsetY.SetValue(0);
			}
			else
			{
				getCamera()->Height.SetValue(getImpl().cameraParams->roi_y_height);
				getCamera()->OffsetY.SetValue(getImpl().cameraParams->roi_y_offset);
			}

			WsoLogDebug("Color Camera Set ImageSize...ok!");

			// Set PixelFormat to BayerRG8 as Temporarily
			getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG8);

			// Set AcquisitionMode to Multiframe as Temporarily
			getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame); // Exposure Auto, Gain Auto, BalanceWhite Auto Off 설정을 위해 임시로 

			// Set Exposure time
			getCamera()->ExposureAuto.SetValue(ExposureAuto_Off);
			getCamera()->ExposureTime.SetValue(getImpl().cameraParams->exposure_time);
			//getCamera()->ExposureTime.SetValue(20000);	// Unit = us
			WsoLogDebug("Color Camera Set ExposureTime...ok!");

			// Set Digital gain
			getCamera()->GainAuto.SetValue(GainAuto_Off);
			getCamera()->Gain.SetValue(getImpl().cameraParams->gain);
			getCamera()->AdcBitDepth.SetValue((AdcBitDepthEnums)getImpl().cameraParams->adcDepthIndex);
			WsoLogDebug("Color Camera Set Gain...ok!");

			// Set White Balnce 
			getCamera()->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Off);
			getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
			getCamera()->BalanceRatio.SetValue(1.58);
			getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
			getCamera()->BalanceRatio.SetValue(1.84);
			WsoLogDebug("Color Camera Set WhiteBalnce...ok!");

			// Set ISP Mode
			getCamera()->IspEnable.SetValue(false);
			getCamera()->GammaEnable.SetValue(false);
			WsoLogDebug("Color Camera Set ISP...ok!");

			// Set Strobe Light
			getCamera()->AutoExposureLightingMode.SetIntValue(AutoExposureLightingMode_Frontlight);
			getCamera()->LineSelector.SetValue(LineSelector_Line2);
			getCamera()->LineMode.SetValue(LineMode_Output);
			WsoLogDebug("Color Camera Set StrobeLight...ok!");

			// Set Shutter Mode and Image Grab Test
			if (strModelNameSub == "122S6C")
			{
				getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Global);
				WsoLogDebug("Color Camera Shutter Mode is Global.");
			}
			else if (strModelNameSub == "200S6C")
			{
				getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Rolling);
				WsoLogDebug("Color Camera Shutter Mode is Rolling.");
			}
			else
			{
				getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Rolling);
				WsoLogDebug("Color Camera Shutter Mode is Rolling.");
			}

			// Set Acquisition mode SingleFrame
			//getCamera()->ReverseX.SetValue(false);
			//getCamera()->ReverseY.SetValue(true);	// Horizontal Flip
			//getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG16);
			getCamera()->PixelFormat.SetValue((PixelFormatEnums)getImpl().cameraParams->pixelFormat);
			getCamera()->AcquisitionMode.SetValue((AcquisitionModeEnums)getImpl().cameraParams->acqusitionMode);
			WsoLogDebug("Color Camera Set AcquisitionMode...ok!");

			// Set Acquisition Frame Count
			if ((AcquisitionModeEnums)getImpl().cameraParams->acqusitionMode != AcquisitionMode_SingleFrame)
			{
				setAcquisitionFrameCount(getImpl().cameraParams->acqusitionFrameCount);
				WsoLogDebug("Color Camera Set AcquisitionFrameCount...ok!");
			}

			getCamera()->BeginAcquisition();
			getImpl().rawImage = getCamera()->GetNextImage();
			getCamera()->EndAcquisition();

			WsoLogDebug("Color Camera Grab Test...ok!");

			WsoLogInfo("Color Camera initialize...ok!");
			break;
		}
		catch (const std::exception& e)
		{
			auto elog = e.what();
			WsoLogError("Exception occurred during ColorCamera init!");
			WsoLogError(elog);

			unsigned int numCameras = 0;
			getCamera()->DeviceReset.Execute();

			do {
				getImpl().systemPtr = System::GetInstance();
				getImpl().camList = getImpl().systemPtr->GetCameras();
				numCameras = getImpl().camList.GetSize();
			} while (numCameras == 0);

			getImpl().pCam = getImpl().camList.GetByIndex(0);
			getImpl().pCam->Init();
		}
	}
}

void wso_device::ColorCamera::setDefaultParameters(bool update)
{
	for (int i = 0; i < 3; i++)
	{
		try {
			if (!getCamera()->IsValid()) {
				getCamera()->Init();
			}
			if (getCamera()->IsStreaming()) {
				getCamera()->EndAcquisition();
			}

			// Set Indicator LED
			getCamera()->DeviceIndicatorMode.SetValue(DeviceIndicatorMode_Inactive);
			WsoLogDebug("Color Camera Set Indicator LED...ok!");

			// Set Image Size
			getCamera()->OffsetX.SetValue(0);
			getCamera()->OffsetY.SetValue(0);
			getCamera()->Width.SetValue(4000);
			getCamera()->Height.SetValue(3000);
			getCamera()->OffsetX.SetValue(736);
			getCamera()->OffsetY.SetValue(324);
			WsoLogDebug("Color Camera Set ImageSize...ok!");

			// Set Acquisition mode SingleFrame
			//getCamera()->ReverseX.SetValue(false);
			//getCamera()->ReverseY.SetValue(true);	// Horizontal Flip
			//getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG16);
			getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG8);
			getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
			WsoLogDebug("Color Camera Set AcquisitionMode...ok!");

			// Set Exposure time
			getCamera()->ExposureAuto.SetValue(ExposureAuto_Off);
			//getCamera()->ExposureTime.SetValue(20000);	// Unit = us
			WsoLogDebug("Color Camera Set ExposureTime...ok!");

			// Set Digital gain
			getCamera()->GainAuto.SetValue(GainAuto_Off);
			getCamera()->Gain.SetValue(1);
			getCamera()->AdcBitDepth.SetValue(AdcBitDepth_Bit12);
			WsoLogDebug("Color Camera Set Gain...ok!");

			// Set White Balnce 
			getCamera()->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Off);
			getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
			getCamera()->BalanceRatio.SetValue(1.58);
			getCamera()->BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
			getCamera()->BalanceRatio.SetValue(1.84);
			WsoLogDebug("Color Camera Set WhiteBalnce...ok!");

			// Set ISP Mode
			getCamera()->IspEnable.SetValue(false);
			getCamera()->GammaEnable.SetValue(false);
			WsoLogDebug("Color Camera Set ISP...ok!");

			// Set Strobe Light
			//getCamera()->AutoExposureLightingMode.SetIntValue(AutoExposureLightingMode_Frontlight);
			getCamera()->LineSelector.SetValue(LineSelector_Line2);
			getCamera()->LineMode.SetValue(LineMode_Output);
			WsoLogDebug("Color Camera Set StrobeLight...ok!");

			// Set Shutter Mode and Image Grab Test
			getCamera()->SensorShutterMode.SetValue(SensorShutterMode_Rolling);
			getCamera()->BeginAcquisition();
			getImpl().rawImage = getCamera()->GetNextImage();
			getCamera()->EndAcquisition();
			//getCamera()->SensorShutterMode.SetValue(SensorShutterMode_GlobalReset);
			WsoLogDebug("Color Camera Grab Test...ok!");

			WsoLogInfo("Color Camera initialize...ok!");
			break;
		}
		catch (const std::exception& e)
		{
			auto elog = e.what();
			WsoLogError("Exception occurred during ColorCamera init!");
			WsoLogError(elog);

			unsigned int numCameras = 0;
			getCamera()->DeviceReset.Execute();

			do {
				getImpl().systemPtr = System::GetInstance();
				getImpl().camList = getImpl().systemPtr->GetCameras();
				numCameras = getImpl().camList.GetSize();
			} while (numCameras == 0);

			getImpl().pCam = getImpl().camList.GetByIndex(0);
			getImpl().pCam->Init();
		}
	}
}

void wso_device::ColorCamera::startLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	getImpl().worker = thread{ &ColorCamera::acquireCameraData, this };
}

void wso_device::ColorCamera::pauseLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}
	if (!getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = false;

	if (getImpl().worker.joinable()) {
		getImpl().worker.join();
	}
	return;
}

bool wso_device::ColorCamera::isLiveMode(void) const
{
	return getImpl().liveMode;
}

void wso_device::ColorCamera::startSingleFrameLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	getImpl().worker = thread{ &ColorCamera::acquireCameraSingleFrameData, this };
}

void wso_device::ColorCamera::startFrameSeqROILiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	int nTotalFrameCount = getImpl().cameraSeqParams->FrameCount;
	if (nTotalFrameCount > 8)
	{
		getImpl().worker = thread{ &ColorCamera::acquireCameraMultiSequencerROIData, this };
	}
	else
	{
		getImpl().worker = thread{ &ColorCamera::acquireCameraSingleSequencerROIData, this };
	}
}

void wso_device::ColorCamera::startFrameOffsetROILiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	getImpl().worker = thread{ &ColorCamera::acquireCameraOffsetROIData, this };
}

void wso_device::ColorCamera::startFrmaeOffsetROICaptureMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	setupCameraTriggerOnOffOnly(false);

	acquireCaptureCameraOffsetROIData();
}

void wso_device::ColorCamera::startFrameRollSWTrigOverlabLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;
	getImpl().worker = thread{ &ColorCamera::acquireCameraRollSWTrigOverlapLive, this };
}

void wso_device::ColorCamera::startFrameRollSWTrigOverlabCaptureMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;
	acquireCameraRollSWTrigOverlapCapture();
}

void wso_device::ColorCamera::startSWTriggerLiveMode(void)
{
	unique_lock<mutex> lock(getImpl().mutexLock);

	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	if (getCamera()->IsStreaming())
		getCamera()->EndAcquisition();

	// Software Trigger 설정
	setupSoftwareTrigger(1); // Single Frame

	// Acquisition 시작
	getCamera()->BeginAcquisition();
}

void wso_device::ColorCamera::stopSWTriggerLiveMode(void)
{
	unique_lock<mutex> lock(getImpl().mutexLock);

	if (getCamera()->IsStreaming())
	{
		getCamera()->EndAcquisition();
	}

	// Trigger Mode 비활성화
	int nTriggerMode = getCamera()->TriggerMode.GetValue();
	if (nTriggerMode == TriggerMode_On)
		getCamera()->TriggerMode.SetValue(TriggerMode_Off);

	getImpl().liveMode = false;
}

void wso_device::ColorCamera::setupSoftwareTrigger(int nMode)
{
	try {
		// 1. Acquisition 중단
		if (getCamera()->IsStreaming()) {
			getCamera()->EndAcquisition();
		}

		// 2. Trigger Mode 활성화
		getCamera()->TriggerMode.SetValue(TriggerMode_On);

		// 3. Trigger Source를 Software로 설정
		getCamera()->TriggerSource.SetValue(TriggerSource_Software);

		// 4. Trigger Activation 설정 (Rising Edge)
		//getCamera()->TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// 5. Acquisition Mode 설정 (SingleFrame 또는 MultiFrame)
		switch (nMode)
		{
			case 0 : // Continuous
			{
				getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
			}
			break;
			case 1 : // Single Frame
			{
				getCamera()->AcquisitionMode.SetValue(AcquisitionMode_SingleFrame);
			}
			break;
			case 2 : // Multi Frame
			{
				getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame);
			}
			break;
		}

		// 6. Trigger Selector 설정 (Frame Start)
		getCamera()->TriggerSelector.SetValue(TriggerSelector_FrameStart);

		WsoLogInfo("Software Trigger setup completed");
	}
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during Software Trigger setup: " + std::string(e.what()));
	}
}

void wso_device::ColorCamera::ShootSWTrigger()
{
	if (!getImpl().liveMode)
		return;

	unique_lock<mutex> lock(getImpl().mutexLock);

	auto t1 = steady_clock::now();

	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();
	int nPixelFormat = getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();
	uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);
	unsigned char* buffer = getImpl().sonyFrameBuffer;

	uint32_t frameCount = 0;

	auto t2 = steady_clock::now();

	getCamera()->TriggerSoftware.Execute();

	auto t3 = steady_clock::now();

	//frameCount++;

	getImpl().rawImage = getCamera()->GetNextImage();

	auto t4 = steady_clock::now();

	std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

	auto t5 = steady_clock::now();

	CallbackRegistry::getInstance()->runColorCameraSWTriggerFrameCaptured(buffer, width, height, frameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

	getImpl().rawImage->Release();

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

void wso_device::ColorCamera::startHWTriggerLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}

	if (getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = true;

	setupHWTriggerSetting(false);

	getImpl().worker = thread{ &ColorCamera::acquireCameraRollHWTriggerCaptureData, this };
}

void wso_device::ColorCamera::stopHWTriggerLiveMode(void)
{
	if (!isInitiated()) {
		return;
	}
	if (!getImpl().liveMode) {
		return;
	}

	getImpl().liveMode = false;

	if (getImpl().worker.joinable()) {
		getImpl().worker.join();
	}
	return;
}

bool wso_device::ColorCamera::isCameraStreaming(void)
{
	if (!isInitiated()) {
		return false;
	}

	if (getImpl().pCam == nullptr)
	{
		return false;
	}
	
	return getImpl().pCam->IsStreaming();
}

void wso_device::ColorCamera::setupHWTriggerSetting(bool bContinuous)
{
	try {
		// 1. Acquisition 중단
		if (getCamera()->IsStreaming()) {
			getCamera()->EndAcquisition();
		}

		// 6. Trigger Selector 설정 (Frame Start)
		getCamera()->TriggerSelector.SetValue(TriggerSelector_FrameStart);

		// 3. Trigger Source를 Software로 설정
		getCamera()->TriggerSource.SetValue(TriggerSource_Line3);

		// 4. Trigger Activation을 ReadOut으로 설정
		getCamera()->TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// 4. Trigger Overlap을 ReadOut으로 설정
		getCamera()->TriggerOverlap.SetValue(TriggerOverlap_ReadOut);

		// 4. LineSelector를 Line2로 설정
		getCamera()->LineSelector.SetValue(LineSelector_Line2);

		// 4. LineMode를 Output로 설정
		getCamera()->LineMode.SetValue(LineMode_Output);

		// 4. LineSource를 Output로 설정
		getCamera()->LineSource.SetValue(LineSource_ExposureActive);

		// 4. LineSelector를 Line2로 설정
		getCamera()->LineSelector.SetValue(LineSelector_Line3);

		// 4. LineMode를 Output로 설정
		getCamera()->LineMode.SetValue(LineMode_Input);

		// 5. Acquisition Mode 설정 (Continuous 또는 MultiFrame)
		//if (bContinuous)
		//	getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		//else
		//	getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame);

		// 2. Trigger Mode 활성화
		getCamera()->TriggerMode.SetValue(TriggerMode_On);

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
		if (getCamera()->IsStreaming()) {
			getCamera()->EndAcquisition();
		}

		if (bOn == true)
		{
			getCamera()->TriggerMode.SetValue(TriggerMode_On);
			//WsoLogInfo("Camera trigger mode : On");
		}
		else
		{
			getCamera()->TriggerMode.SetValue(TriggerMode_Off);
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

	if (getImpl().liveMode) {
		return;
	}

	getImpl().originalMode = true;

	setupCameraTriggerOnOffOnly(false);

	acquireCameraCaptureData();
}

void wso_device::ColorCamera::pauseOriginalMode(void)
{
	getImpl().originalMode = false;
}

int wso_device::ColorCamera::getFrameWidth(void) const
{
	int width = -1;
	width = (int)getCamera()->Width.GetValue();

	return width;
}

int wso_device::ColorCamera::getFrameHeight(void) const
{
	int height = -1;
	height = (int)getCamera()->Height.GetValue();

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

void wso_device::ColorCamera::setParameters(ColorCameraSettingParam* pParam)
{
	try
	{
		if (pParam)
		{
			/////현재의 Width, Height 값에 안맞는 offset 값을 넣으면 문제가 생기므로, 처음에 0,0으로 초기화를 먼저 시켜준다.
			setROI_X_Offset(0);
			setROI_Y_Offset(0);
			///
			setROI_X_Width(pParam->roi_x_width);
			setROI_Y_Height(pParam->roi_y_height);
			setROI_X_Offset(pParam->roi_x_offset);
			setROI_Y_Offset(pParam->roi_y_offset);
			setExposureTime(pParam->exposure_time);
			setAcquisitionMode(pParam->acqusitionMode);
			setAcquisitionFrameCount(pParam->acqusitionFrameCount);
			setPixelFormat(pParam->pixelFormat);
			setBinningHorizontal(pParam->binningHorizontal);
			setBinningVertical(pParam->binningVertical);
			setGain(pParam->gain);
			setAdcBitDepth(pParam->adcDepthIndex);

			getImpl().cameraParams->roi_x_width = pParam->roi_x_width;
			getImpl().cameraParams->roi_y_height = pParam->roi_y_height;
			getImpl().cameraParams->roi_x_offset = pParam->roi_x_offset;
			getImpl().cameraParams->roi_y_offset = pParam->roi_y_offset;
			getImpl().cameraParams->exposure_time = pParam->exposure_time;
			getImpl().cameraParams->acqusitionMode = pParam->acqusitionMode;
			getImpl().cameraParams->acqusitionFrameCount = pParam->acqusitionFrameCount;
			getImpl().cameraParams->pixelFormat = pParam->pixelFormat;
			getImpl().cameraParams->binningHorizontal = pParam->binningHorizontal;
			getImpl().cameraParams->binningVertical = pParam->binningVertical;
			getImpl().cameraParams->gain = pParam->gain;
			getImpl().cameraParams->adcDepthIndex = pParam->adcDepthIndex;

			saveConfigToIniFile();
			//saveUserSet_(1);
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::setSeqParameters(CaptureFrameSeqROIPreset* pParam)
{
	try
	{
		if (pParam)
		{
			getImpl().cameraSeqParams->FrameCount = pParam->FrameCount;

			if (pParam->FrameCount > 0)
			{
				getImpl().cameraSeqParams->pParams = new FrameSeqROIParam[getImpl().cameraSeqParams->FrameCount];
				// 복사
				memcpy(getImpl().cameraSeqParams->pParams, pParam->pParams,
					sizeof(FrameSeqROIParam) * pParam->FrameCount);
			}
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::setOffsetRoiParameters(CaptureFrameOffsetROIPreset* pParam)
{
	try
	{
		if (pParam)
		{
			getImpl().cameraOffsetRoiParams->RoiWidth = pParam->RoiWidth;
			getImpl().cameraOffsetRoiParams->RoiHeight = pParam->RoiHeight;
			getImpl().cameraOffsetRoiParams->FrameCount = pParam->FrameCount;

			if (pParam->FrameCount > 0)
			{
				getImpl().cameraOffsetRoiParams->pParams = new FrameOffsetROIParam[getImpl().cameraOffsetRoiParams->FrameCount];
				// 복사
				memcpy(getImpl().cameraOffsetRoiParams->pParams, pParam->pParams,
					sizeof(FrameOffsetROIParam) * pParam->FrameCount);
			}
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::setRollSWTrigOverlapParameters(CaptureFrameRollSWTrigOverlapPreset* pParam)
{
	try
	{
		if (pParam)
		{
			getImpl().cameraRollSWTrigOverlapParams->FrameWidth = pParam->FrameWidth;
			getImpl().cameraRollSWTrigOverlapParams->FrameHeight = pParam->FrameHeight;
			getImpl().cameraRollSWTrigOverlapParams->OverlapFrameCount = pParam->OverlapFrameCount;
			getImpl().cameraRollSWTrigOverlapParams->OffsetX = pParam->OffsetX;
			getImpl().cameraRollSWTrigOverlapParams->OffsetY = pParam->OffsetY;
			getImpl().cameraRollSWTrigOverlapParams->TriggerIntervalMs = pParam->TriggerIntervalMs;
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Set!");
		WsoLogError(elog);
	}
}

void wso_device::ColorCamera::getParameters(ColorCameraSettingParam* pParam)
{
	try
	{
		if (pParam)
		{
			pParam->roi_max_width = getROI_Max_Width();
			pParam->roi_max_Height = getROI_Max_Height();
			pParam->roi_x_width = getROI_X_Width();
			pParam->roi_y_height = getROI_Y_Height();
			pParam->roi_x_offset = getROI_X_Offset();
			pParam->roi_y_offset = getROI_Y_Offset();
			pParam->exposure_time = getExposureTime();
			pParam->acqusitionMode = getAcquisitionMode();
			pParam->acqusitionFrameCount = getAcquisitionFrameCount();
			pParam->pixelFormat = getPixelFormat();
			pParam->binningHorizontal = getBinningHorizontal();
			pParam->binningVertical = getBinningVertical();
			pParam->gain = getGain();
			pParam->adcDepthIndex = getAdcBitDepth();

			getImpl().cameraParams->roi_x_width = pParam->roi_x_width;
			getImpl().cameraParams->roi_y_height = pParam->roi_y_height;
			getImpl().cameraParams->roi_x_offset = pParam->roi_x_offset;
			getImpl().cameraParams->roi_y_offset = pParam->roi_y_offset;
			getImpl().cameraParams->exposure_time = pParam->exposure_time;
			getImpl().cameraParams->acqusitionMode = pParam->acqusitionMode;
			getImpl().cameraParams->acqusitionFrameCount = pParam->acqusitionFrameCount;
			getImpl().cameraParams->pixelFormat = pParam->pixelFormat;
			getImpl().cameraParams->binningHorizontal = pParam->binningHorizontal;
			getImpl().cameraParams->binningVertical = pParam->binningVertical;
			getImpl().cameraParams->gain = pParam->gain;
			getImpl().cameraParams->adcDepthIndex = pParam->adcDepthIndex;
		}
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		WsoLogError("Exception occurred during ColorCamera Get!");
		WsoLogError(elog);
	}
	return;
}

unsigned int wso_device::ColorCamera::getROI_Max_Width()
{
	return (unsigned int)getCamera()->WidthMax.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_Max_Height()
{
	return (unsigned int)getCamera()->HeightMax.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_X_Width()
{
	return (unsigned int)getCamera()->Width.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_Y_Height()
{
	return (unsigned int)getCamera()->Height.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_X_Offset()
{
	return (unsigned int)getCamera()->OffsetX.GetValue();
}

unsigned int wso_device::ColorCamera::getROI_Y_Offset()
{
	return (unsigned int)getCamera()->OffsetY.GetValue();
}

unsigned int wso_device::ColorCamera::getExposureTime()
{
	return (unsigned int)getCamera()->ExposureTime.GetValue();
}

unsigned int wso_device::ColorCamera::getAcquisitionMode()
{
	return (unsigned int)getCamera()->AcquisitionMode.GetValue();
}

unsigned int wso_device::ColorCamera::getAcquisitionFrameCount()
{
	return (unsigned int)getCamera()->AcquisitionFrameCount.GetValue();
}

unsigned int wso_device::ColorCamera::getPixelFormat()
{
	return (unsigned int)getCamera()->PixelFormat.GetValue();
}

unsigned int wso_device::ColorCamera::getBinningHorizontal()
{
	return (unsigned int)getCamera()->BinningHorizontal.GetValue();
}

unsigned int wso_device::ColorCamera::getBinningVertical()
{
	return (unsigned int)getCamera()->BinningVertical.GetValue();
}

float wso_device::ColorCamera::getGain()
{
	return (float)getCamera()->Gain.GetValue();
}

unsigned int wso_device::ColorCamera::getAdcBitDepth()
{
	return (unsigned int)getCamera()->AdcBitDepth.GetValue();
}

void wso_device::ColorCamera::setROI_X_Width(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->Width.SetValue(val);
	return;
}

void wso_device::ColorCamera::setROI_Y_Height(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->Height.SetValue(val);
	return;
}

void wso_device::ColorCamera::setROI_X_Offset(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->OffsetX.SetValue(val);
	return;
}

void wso_device::ColorCamera::setROI_Y_Offset(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->OffsetY.SetValue(val);
	return;
}

void wso_device::ColorCamera::setExposureTime(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->ExposureTime.SetValue(val);
	return;
}

void wso_device::ColorCamera::setAcquisitionMode(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->AcquisitionMode.SetValue((AcquisitionModeEnums)val);
	return;
}

void wso_device::ColorCamera::setAcquisitionFrameCount(unsigned int val)
{
	val = (val < 2 ? 2 : val);
	getCamera()->AcquisitionFrameCount.SetValue(val);
	return;
}

void wso_device::ColorCamera::setPixelFormat(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->PixelFormat.SetValue((PixelFormatEnums)val);
	return;
}

void wso_device::ColorCamera::setBinningHorizontal(unsigned int val)
{
	val = (val < 1 ? 1 : val);
	getCamera()->BinningHorizontal.SetValue(val);
	return;
}

void wso_device::ColorCamera::setBinningVertical(unsigned int val)
{
	val = (val < 1 ? 1 : val);
	getCamera()->BinningVertical.SetValue(val);
	return;
}

void wso_device::ColorCamera::setGain(float val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->Gain.SetValue(val);
	return;
}

void wso_device::ColorCamera::setAdcBitDepth(unsigned int val)
{
	val = (val < 0 ? 0 : val);
	getCamera()->AdcBitDepth.SetValue((AdcBitDepthEnums)val);
	return;
}

unsigned int wso_device::ColorCamera::getFlipMode()
{
	bool bX = false;
	bool bY = false;

	unsigned int ReverseX = getCamera()->ReverseX.GetValue();
	unsigned int ReverseY = getCamera()->ReverseY.GetValue();

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

		getImpl().cameraParams->roi_x_width = ini->ReadUInt(L"COLOR_CAMERA", L"RoiXWidth");
		getImpl().cameraParams->roi_y_height = ini->ReadUInt(L"COLOR_CAMERA", L"RoiYHeight");
		getImpl().cameraParams->roi_x_offset = ini->ReadUInt(L"COLOR_CAMERA", L"RoiXOffset");
		getImpl().cameraParams->roi_y_offset = ini->ReadUInt(L"COLOR_CAMERA", L"RoiYOffset");
		getImpl().cameraParams->exposure_time = ini->ReadUInt(L"COLOR_CAMERA", L"ExposureTime");
		getImpl().cameraParams->acqusitionMode = ini->ReadUInt(L"COLOR_CAMERA", L"AcqusitionMode");
		getImpl().cameraParams->acqusitionFrameCount = ini->ReadUInt(L"COLOR_CAMERA", L"AcqusitionFrameCount");
		getImpl().cameraParams->pixelFormat = ini->ReadUInt(L"COLOR_CAMERA", L"PixelFormat");
		getImpl().cameraParams->gain = ini->ReadFloat(L"COLOR_CAMERA", L"Gain");
		getImpl().cameraParams->adcDepthIndex = ini->ReadUInt(L"COLOR_CAMERA", L"AdcDepth");

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

		ini->WriteInt(L"COLOR_CAMERA", L"RoiXWidth", getImpl().cameraParams->roi_x_width);
		ini->WriteInt(L"COLOR_CAMERA", L"RoiYHeight", getImpl().cameraParams->roi_y_height);
		ini->WriteInt(L"COLOR_CAMERA", L"RoiXOffset", getImpl().cameraParams->roi_x_offset);
		ini->WriteInt(L"COLOR_CAMERA", L"RoiYOffset", getImpl().cameraParams->roi_y_offset);
		ini->WriteInt(L"COLOR_CAMERA", L"ExposureTime", getImpl().cameraParams->exposure_time);
		ini->WriteInt(L"COLOR_CAMERA", L"AcqusitionMode", getImpl().cameraParams->acqusitionMode);
		ini->WriteInt(L"COLOR_CAMERA", L"AcqusitionFrameCount", getImpl().cameraParams->acqusitionFrameCount);
		ini->WriteInt(L"COLOR_CAMERA", L"PixelFormat", getImpl().cameraParams->pixelFormat);
		ini->WriteFloat(L"COLOR_CAMERA", L"Gain", getImpl().cameraParams->gain);
		ini->WriteInt(L"COLOR_CAMERA", L"AdcDepth", getImpl().cameraParams->adcDepthIndex);

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

ColorCamera::ColorCameraImpl& wso_device::ColorCamera::getImpl(void) const
{
	return *d_ptr;
}

CameraPtr wso_device::ColorCamera::getCamera(void) const
{
	return getImpl().pCam;
}

void wso_device::ColorCamera::acquireCameraData(void)
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();
	uint32_t frameSize = getFrameSizeInBytes();

	uint32_t frameCount;

	unsigned char* buffer = getImpl().sonyFrameBuffer;

	if (!getImpl().liveMode)
	{
		getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame);
		 getCamera()->AcquisitionFrameCount.GetValue();

		getCamera()->BeginAcquisition();
		getImpl().rawImage = getCamera()->GetNextImage();
		
		ImageProcessor processor;
		processor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_DIRECTIONAL_FILTER);
		getImpl().rawImage = processor.Convert(getImpl().rawImage, PixelFormat_BayerGB16);
		
		
		std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

		getCamera()->EndAcquisition();
	}
	else
	{
		getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		//getCamera()->PixelFormat.SetValue(PixelFormat_BayerGB16);
		//getCamera()->ReverseX.SetValue(false);
		//getCamera()->ReverseY.SetValue(false);
		//getCamera()->BinningHorizontalMode.SetValue(BinningHorizontalMode_Average);
		//getCamera()->BinningVerticalMode.SetIntValue(BinningVerticalMode_Average);
		//getCamera()->BinningHorizontal.SetValue(4);
		//getCamera()->BinningVertical.SetValue(4);
		//getCamera()->Width.SetValue(4000);
		//getCamera()->Height.SetValue(3000);
		//getCamera()->OffsetX.SetValue(736);
		//getCamera()->OffsetY.SetValue(324);
		//int nMaxWidth = (int)getCamera()->WidthMax.GetValue();
		//int nMaxHeight = (int)getCamera()->HeightMax.GetValue();
		//getCamera()->OffsetX.SetValue(0);
		//getCamera()->OffsetY.SetValue(0);
		//getCamera()->Width.SetValue(4000);
		//getCamera()->Height.SetValue(3000);
		//getCamera()->OffsetX.SetValue(736);
		//getCamera()->OffsetY.SetValue(324);

		width = getFrameWidth();
		height = getFrameHeight();

		int nPixelFormat = getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();
		//double nFrameRateMin = getCamera()->AcquisitionFrameRate.GetMin();
		//double nFrameRateMax = getCamera()->AcquisitionFrameRate.GetMax();
		//double nFrameRate = getCamera()->AcquisitionFrameRate.GetValue();

		frameSize = getFrameSizeForBpp(nBytesPerPixel);
		frameCount = 0;

		getCamera()->BeginAcquisition();

		do {
			frameCount++;

			getImpl().rawImage = getCamera()->GetNextImage();
			std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

			unique_lock<mutex> lock(getImpl().mutexLock);
			//CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, getFlipMode());
			CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

		} while (getImpl().liveMode);

		getCamera()->EndAcquisition();
		//getCamera()->ReverseX.SetValue(false);
		//getCamera()->ReverseY.SetValue(true);
		//getCamera()->BinningHorizontal.SetValue(1);
		//getCamera()->BinningVertical.SetValue(1);
		//getCamera()->Width.SetValue(4000);
		//getCamera()->Height.SetValue(3000);
		//getCamera()->OffsetX.SetValue(400);
	}
	return;
}

void wso_device::ColorCamera::acquireCameraSingleFrameData(void) // Single Frame Live
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();
	uint32_t frameSize;

	uint32_t frameCount;

	unsigned char* buffer = getImpl().sonyFrameBuffer;

	int nPixelFormat = getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	getCamera()->AcquisitionMode.SetValue(AcquisitionMode_SingleFrame);

	frameCount = 0;
	
	do {
		auto t0 = steady_clock::now();

		frameCount++;
		//setROI_Y_Offset(0);
		//setROI_Y_Height(3000);
		frameSize = getFrameSizeForBpp(nBytesPerPixel);
	
		auto t1 = steady_clock::now();

		getCamera()->BeginAcquisition();

		auto t2 = steady_clock::now();

		getImpl().rawImage = getCamera()->GetNextImage();

		auto t3 = steady_clock::now();

		std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

		auto t4 = steady_clock::now();

		unique_lock<mutex> lock(getImpl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

		auto t5 = steady_clock::now();

		getImpl().rawImage->Release();

		auto t6 = steady_clock::now();

		getCamera()->EndAcquisition();

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

	} while (getImpl().liveMode);

	return;
}

void wso_device::ColorCamera::acquireCameraSingleSequencerROIData(void)
{
	uint32_t totalFrameCount = getImpl().cameraSeqParams->FrameCount;
	FrameSeqROIParam* Params = getImpl().cameraSeqParams->pParams;

	if (totalFrameCount <= 0)
		return;
	if (Params == nullptr)
		return;

	uint32_t width;
	uint32_t height;
	uint32_t offsetX;
	uint32_t offsetY;

	uint32_t frameSize;
	uint32_t frameCount;

	unsigned char* buffer = getImpl().sonyFrameBuffer;

	int nPixelFormat = getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

	if (onSequencer() == false)
	{
		return;
	}

	frameCount = 0;

	getCamera()->BeginAcquisition();

	do {

		getImpl().rawImage = getCamera()->GetNextImage();

		width = Params[frameCount].Width;
		height = Params[frameCount].Height;
		offsetX = Params[frameCount].OffsetX;
		offsetY = Params[frameCount].OffsetY;
		
		frameSize = width * height * sizeof(char) * nBytesPerPixel;

		std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

		unique_lock<mutex> lock(getImpl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraSeqROIFrameCaptured(buffer, width, height, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

		getImpl().rawImage->Release();

		frameCount++;
		if (frameCount == totalFrameCount)
		{
			frameCount = 0;
		}

	} while (getImpl().liveMode);

	getCamera()->EndAcquisition();

	offSequencer();

	return;
}

void wso_device::ColorCamera::acquireCameraMultiSequencerROIData(void)
{
	try
	{
		uint32_t totalFrameCount = getImpl().cameraSeqParams->FrameCount;
		FrameSeqROIParam* Params = getImpl().cameraSeqParams->pParams;
		
		std::vector<std::vector<FrameSeqROIParam>> ParamList = divideParamVector_((int)totalFrameCount, Params);

		if (totalFrameCount <= 0)
			return;
		if (Params == nullptr)
			return;

		uint32_t width;
		uint32_t height;
		uint32_t offsetX;
		uint32_t offsetY;

		uint32_t frameSize;
		uint32_t frameCount;

		unsigned char* buffer = getImpl().sonyFrameBuffer;

		int nPixelFormat = getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		frameCount = 0;

		getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

		do {

			for (int i = 0; i < ParamList.size(); ++i)
			{
				{
					unique_lock<mutex> lock(getImpl().mutexLock);
					if (!getImpl().liveMode)
						break;
				}

				auto t1 = steady_clock::now();

				setSequencer(ParamList[i]);

				auto t2 = steady_clock::now();

				getCamera()->BeginAcquisition();

				auto t3 = steady_clock::now();

				for (int j = 0; j < ParamList[i].size(); ++j)
				{
					getImpl().rawImage = getCamera()->GetNextImage();

					width = ParamList[i][j].Width;
					height = ParamList[i][j].Height;
					offsetX = ParamList[i][j].OffsetX;
					offsetY = ParamList[i][j].OffsetY;

					frameSize = width * height * sizeof(char) * nBytesPerPixel;
					std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

					CallbackRegistry::getInstance()->runColorCameraSeqROIFrameCaptured(buffer, width, height, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

					getImpl().rawImage->Release();

					frameCount++;
					if (frameCount == totalFrameCount)
					{
						frameCount = 0;
					}
				}

				auto t6 = steady_clock::now();

				getCamera()->EndAcquisition();

				auto t7 = steady_clock::now();

				getCamera()->SequencerMode.SetValue(SequencerMode_Off);
				
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

		} while (getImpl().liveMode);

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
		uint32_t totalFrameCount = getImpl().cameraOffsetRoiParams->FrameCount;
		uint32_t nRoiWidth = getImpl().cameraOffsetRoiParams->RoiWidth;
		uint32_t nRoiHeight = getImpl().cameraOffsetRoiParams->RoiHeight;
		uint32_t offsetX;
		uint32_t offsetY;

		FrameOffsetROIParam* Params = getImpl().cameraOffsetRoiParams->pParams;

		if (totalFrameCount <= 0)
			return;
		if (Params == nullptr)
			return;

		uint32_t frameSize;
		uint32_t frameCount;

		unsigned char* buffer = getImpl().sonyFrameBuffer;

		int nPixelFormat = getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		frameCount = 0;

		if (getCamera()->IsStreaming())
			getCamera()->EndAcquisition();

		// Width, Height 설정
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);

		//SetEventMode(true);
		// Software Trigger 설정
		setupSoftwareTrigger();

		getCamera()->BeginAcquisition();

		do {

			getImpl().nOffsetFrameCount = 0;

			auto t0 = steady_clock::now();

			for (int i = 0; i < totalFrameCount; ++i)
			{
				// liveMode 체크를 뮤텍스로 보호
				{
					unique_lock<mutex> lock(getImpl().mutexLock);
					if (!getImpl().liveMode)
						break;
				}

				//LogD() << "================================";
				//LogD() << "ROI Frame : " << i;

				uint32_t offsetX = Params[i].OffsetX;
				uint32_t offsetY = Params[i].OffsetY;

				//auto t1 = steady_clock::now();
				//setROI_X_Offset(Params[i].OffsetX);
				setROI_Y_Offset(offsetY);

				//auto t2 = steady_clock::now();

				//Sleep(1);

				getCamera()->TriggerSoftware.Execute();

				//auto t3 = steady_clock::now();

				getImpl().rawImage = getCamera()->GetNextImage();

				//auto t4 = steady_clock::now();
				std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);


				//auto t5 = steady_clock::now();
				//unique_lock<mutex> lock(getImpl().mutexLock);
				CallbackRegistry::getInstance()->runColorCameraOffsetROIFrameCaptured(buffer, nRoiWidth, nRoiHeight, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

				//auto t6 = steady_clock::now();

				getImpl().rawImage->Release();

				//auto t7 = steady_clock::now();

				frameCount++;
				if (frameCount == totalFrameCount)
				{
					frameCount = 0;
				}

				//Sleep(50);

				//getImpl().nOffsetFrameCount++;
				//if (getImpl().nOffsetFrameCount == totalFrameCount)
				//{
				//	getImpl().nOffsetFrameCount = 0;
				//}

				//auto m0 = steady_clock::now();

				//double ms = duration<double, std::milli>(t2 - t1).count();
				//LogD() << "setROI_Y_Offset = " << ms;

				//ms = duration<double, std::milli>(t3 - t2).count();
				//LogD() << "getCamera()->TriggerSoftware.Execute() = " << ms;

				//ms = duration<double, std::milli>(t4 - t3).count();
				//LogD() << "getImpl().rawImage = getCamera()->GetNextImage() = " << ms;

				//ms = duration<double, std::milli>(t5 - t4).count();
				//LogD() << "std::memcpy() = " << ms;

				//ms = duration<double, std::milli>(t6 - t5).count();
				//LogD() << "Callback = " << ms;

				//ms = duration<double, std::milli>(t7 - t6).count();
				//LogD() << "getImpl().rawImage->Release() = " << ms;

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

		} while (getImpl().liveMode);

		getCamera()->EndAcquisition();

		getCamera()->TriggerMode.SetValue(TriggerMode_Off);

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
		uint32_t totalFrameCount = getImpl().cameraOffsetRoiParams->FrameCount;
		uint32_t nRoiWidth = getImpl().cameraOffsetRoiParams->RoiWidth;
		uint32_t nRoiHeight = getImpl().cameraOffsetRoiParams->RoiHeight;
		uint32_t offsetX;
		uint32_t offsetY;

		FrameOffsetROIParam* Params = getImpl().cameraOffsetRoiParams->pParams;

		if (totalFrameCount <= 0)
			return;
		if (Params == nullptr)
			return;

		uint32_t frameSize;
		uint32_t frameCount;

		unsigned char* buffer = getImpl().sonyFrameBuffer;

		int nPixelFormat = getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		frameCount = 0;

		if (getCamera()->IsStreaming())
			getCamera()->EndAcquisition();

		// Width, Height 설정
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);

		//SetEventMode(true);
		// Software Trigger 설정
		setupSoftwareTrigger(2); // Multiframe
		setAcquisitionFrameCount(totalFrameCount);

		getCamera()->BeginAcquisition();

		auto t0 = steady_clock::now();

		for (int i = 0; i < totalFrameCount; ++i)
		{
			// liveMode 체크를 뮤텍스로 보호
			//{
			//	unique_lock<mutex> lock(getImpl().mutexLock);
			//	if (!getImpl().liveMode)
			//		break;
			//}

			//LogD() << "================================";
			//LogD() << "ROI Frame : " << i;

			uint32_t offsetX = Params[i].OffsetX;
			uint32_t offsetY = Params[i].OffsetY;

			//auto t1 = steady_clock::now();
			//setROI_X_Offset(Params[i].OffsetX);
			setROI_Y_Offset(offsetY);

			//auto t2 = steady_clock::now();

			//Sleep(1);

			getCamera()->TriggerSoftware.Execute();

			//auto t3 = steady_clock::now();

			getImpl().rawImage = getCamera()->GetNextImage(3000);

			//auto t4 = steady_clock::now();
			std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);


			//auto t5 = steady_clock::now();
			unique_lock<mutex> lock(getImpl().mutexLock);
			CallbackRegistry::getInstance()->runColorCameraOffsetROIFrameCaptured(buffer, nRoiWidth, nRoiHeight, offsetX, offsetY, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

			//auto t6 = steady_clock::now();

			getImpl().rawImage->Release();

			//auto t7 = steady_clock::now();

			frameCount++;
			if (frameCount == totalFrameCount)
			{
				frameCount = 0;
			}

			//getImpl().nOffsetFrameCount++;
			//if (getImpl().nOffsetFrameCount == totalFrameCount)
			//{
			//	getImpl().nOffsetFrameCount = 0;
			//}

			//auto m0 = steady_clock::now();

			//double ms = duration<double, std::milli>(t2 - t1).count();
			//LogD() << "setROI_Y_Offset = " << ms;

			//ms = duration<double, std::milli>(t3 - t2).count();
			//LogD() << "getCamera()->TriggerSoftware.Execute() = " << ms;

			//ms = duration<double, std::milli>(t4 - t3).count();
			//LogD() << "getImpl().rawImage = getCamera()->GetNextImage() = " << ms;

			//ms = duration<double, std::milli>(t5 - t4).count();
			//LogD() << "std::memcpy() = " << ms;

			//ms = duration<double, std::milli>(t6 - t5).count();
			//LogD() << "Callback = " << ms;

			//ms = duration<double, std::milli>(t7 - t6).count();
			//LogD() << "getImpl().rawImage->Release() = " << ms;

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

		getCamera()->EndAcquisition();

		getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		getImpl().liveMode = false;

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraOffsetROIData!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraRollSWTrigOverlapLive(void)
{
	try
	{
		uint32_t totalFrameCount = getImpl().cameraRollSWTrigOverlapParams->OverlapFrameCount;
		uint32_t nRoiWidth = getImpl().cameraRollSWTrigOverlapParams->FrameWidth;
		uint32_t nRoiHeight = getImpl().cameraRollSWTrigOverlapParams->FrameHeight;
		uint32_t nInterval = getImpl().cameraRollSWTrigOverlapParams->TriggerIntervalMs;
		uint32_t nOffsetX = getImpl().cameraRollSWTrigOverlapParams->OffsetX;
		uint32_t nOffsetY = getImpl().cameraRollSWTrigOverlapParams->OffsetY;

		if (totalFrameCount <= 0)
			return;

		unsigned char* buffer = getImpl().sonyFrameBuffer;

		int nPixelFormat = getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		uint32_t frameCount = 0;
		uint32_t frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		if (getCamera()->IsStreaming())
			getCamera()->EndAcquisition();

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

		getCamera()->BeginAcquisition();

		do {
			//auto t0 = steady_clock::now();

			for (int i = 0; i < totalFrameCount; ++i)
			{
				// liveMode 체크를 뮤텍스로 보호
				{
					unique_lock<mutex> lock(getImpl().mutexLock);
					if (!getImpl().liveMode)
						break;
				}

				//auto t1 = steady_clock::now();

				getCamera()->TriggerSoftware.Execute();

				//auto t2 = steady_clock::now();

				getImpl().rawImage = getCamera()->GetNextImage();

				//auto t3 = steady_clock::now();

				std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

				//auto t4 = steady_clock::now();

				CallbackRegistry::getInstance()->runColorCameraRollSWTrigOverlapFrameCaptured(buffer, nRoiWidth, nRoiHeight, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

				//auto t5 = steady_clock::now();

				getImpl().rawImage->Release();

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
				//LogD() << "getCamera()->GetNextImage = " << ms;

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

		} while (getImpl().liveMode);

		getCamera()->EndAcquisition();

		getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraRollSWTrigOverlapLive!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraRollSWTrigOverlapCapture(void)
{
	try
	{
		uint32_t totalFrameCount = getImpl().cameraRollSWTrigOverlapParams->OverlapFrameCount;
		uint32_t nRoiWidth = getImpl().cameraRollSWTrigOverlapParams->FrameWidth;
		uint32_t nRoiHeight = getImpl().cameraRollSWTrigOverlapParams->FrameHeight;
		uint32_t nInterval = getImpl().cameraRollSWTrigOverlapParams->TriggerIntervalMs;
		uint32_t nOffsetX = getImpl().cameraRollSWTrigOverlapParams->OffsetX;
		uint32_t nOffsetY = getImpl().cameraRollSWTrigOverlapParams->OffsetY;

		if (totalFrameCount <= 0)
			return;

		unsigned char* buffer = getImpl().sonyFrameBuffer;

		int nPixelFormat = getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();

		uint32_t frameCount = 0;
		uint32_t frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

		if (getCamera()->IsStreaming())
			getCamera()->EndAcquisition();

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

		getCamera()->BeginAcquisition();

		for (int i = 0; i < totalFrameCount; ++i)
		{
			getCamera()->TriggerSoftware.Execute();

			getImpl().rawImage = getCamera()->GetNextImage();

			std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

			CallbackRegistry::getInstance()->runColorCameraRollSWTrigOverlapFrameCaptured(buffer, nRoiWidth, nRoiHeight, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

			getImpl().rawImage->Release();

			frameCount++;
			if (frameCount == totalFrameCount)
			{
				frameCount = 0;
			}

			Sleep(nInterval);
		}

		getCamera()->EndAcquisition();

		getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		getImpl().liveMode = false;

		return;
	}
	catch (const std::exception& e)
	{
		auto elog = e.what();
		LogD() << "Exception occurred acquireCameraRollSWTrigOverlapCapture!";
		LogD() << elog;
	}
}

void wso_device::ColorCamera::acquireCameraRollHWTriggerCaptureData(void)
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();

	int nPixelFormat = getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);

	uint32_t frameCount = 0;

	unsigned char* buffer = getImpl().sonyFrameBuffer;

	uint32_t nTotalFrameCount;
	if (getAcquisitionMode() == AcquisitionMode_SingleFrame)
	{
		nTotalFrameCount = 1;
	}
	else
	{
		nTotalFrameCount = (int)getCamera()->AcquisitionFrameCount.GetValue();
	}

	getCamera()->BeginAcquisition();

	for (uint32_t frameCount = 0; frameCount < nTotalFrameCount; ++frameCount)
	{
		try
		{
			getImpl().rawImage = getCamera()->GetNextImage(3000);

			// 정상적으로 이미지를 받은 경우
			std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);
		}
		catch (const Spinnaker::Exception& e)
		{
			// 타임아웃 또는 기타 오류 발생 시
			LogD() << "Failed to get image at frame " << frameCount << ": " << e.what();

			// buffer를 채워서 회색 이미지 생성
			std::memset(buffer, 0x80, frameSize);
		}

		unique_lock<mutex> lock(getImpl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraImageCaptured(buffer, width, height, frameCount, nTotalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);
	}

	getCamera()->EndAcquisition();
}

bool wso_device::ColorCamera::onSequencer(void)
{
	bool bRet = false;

	try
	{
		int nIndexNextSet = 0;
		int nSeqSetCount = getImpl().cameraSeqParams->FrameCount;

		// Sequencer Mode Off
		getCamera()->SequencerMode.SetValue(SequencerMode_Off);
		// Sequencer Configuration Mode > On
		getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_On);

		////////////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < nSeqSetCount; ++i)
		{
			getCamera()->SequencerSetSelector.SetValue(i);

			// Set i의 카메라 파라미터 설정
			//getCamera()->ExposureTime.SetValue(2000);  // 20ms 노출 시간
			//getCamera()->Gain.SetValue(2.0);            // 낮은 게인
			getCamera()->OffsetX.SetValue(0);         
			getCamera()->OffsetY.SetValue(0);

			getCamera()->Width.SetValue(getImpl().cameraSeqParams->pParams[i].Width);          // ROI 폭
			getCamera()->Height.SetValue(getImpl().cameraSeqParams->pParams[i].Height);         // ROI 높이
			getCamera()->OffsetX.SetValue(getImpl().cameraSeqParams->pParams[i].OffsetX);         // X 오프셋
			getCamera()->OffsetY.SetValue(getImpl().cameraSeqParams->pParams[i].OffsetY);         // Y 오프셋

			// Set 0의 Sequencer 트리거 설정
			getCamera()->SequencerTriggerSource.SetValue(SequencerTriggerSource_FrameStart);
			getCamera()->SequencerTriggerActivation.SetValue(SequencerTriggerActivation_RisingEdge);

			
			// Set Next Sequence Set 
			++nIndexNextSet;
			if (nIndexNextSet >= nSeqSetCount) nIndexNextSet = 0;

			getCamera()->SequencerSetNext.SetValue(nIndexNextSet);

			// Set i 저장
			getCamera()->SequencerSetSave.Execute();

			LogD() << "Sequencer Set " << i << " configured and saved";
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		// 6. 시작 세트 지정
		getCamera()->SequencerSetStart.SetValue(0);

		// 7. Sequencer Configuration 모드를 Off로 설정 (설정 완료)
		getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);

		// 8. Sequencer 설정 유효성 검사
		if (getCamera()->SequencerConfigurationValid.GetValue() == SequencerConfigurationValid_Yes) {
			LogD() << "Sequencer configuration is valid";

			// 9. Sequencer 모드 활성화
			getCamera()->SequencerMode.SetValue(SequencerMode_On);
			LogD() << "Sequencer mode activated for BFS-U3-122S6C";

			bRet = true;
		}
		else {
			LogD() << "Sequencer configuration is invalid";
			getCamera()->SequencerMode.SetValue(SequencerMode_Off);

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
			getCamera()->SequencerMode.SetValue(SequencerMode_Off);
			getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);
		}
		catch (...) {
			LogD() << "Failed to cleanup Sequencer after exception";
		}

		bRet = false;
	}

	return bRet;
}

bool wso_device::ColorCamera::setSequencer(std::vector<FrameSeqROIParam> paramList)
{
	bool bRet = false;

	try
	{
		int nIndexNextSet = 0;
		int nSeqSetCount = paramList.size();

		// Sequencer Mode Off
		int nMode = getCamera()->SequencerMode.GetValue();
		if (nMode == SequencerMode_On)
		{
			getCamera()->SequencerMode.SetValue(SequencerMode_Off);
		}
		// Sequencer Configuration Mode > On
		getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_On);

		////////////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < nSeqSetCount; ++i)
		{
			getCamera()->SequencerSetSelector.SetValue(i);

			// Set i의 카메라 파라미터 설정
			//getCamera()->ExposureTime.SetValue(2000);  // 20ms 노출 시간
			//getCamera()->Gain.SetValue(2.0);            // 낮은 게인
			getCamera()->OffsetX.SetValue(0);
			getCamera()->OffsetY.SetValue(0);

			getCamera()->Width.SetValue(paramList[i].Width);          // ROI 폭
			getCamera()->Height.SetValue(paramList[i].Height);         // ROI 높이
			getCamera()->OffsetX.SetValue(paramList[i].OffsetX);         // X 오프셋
			getCamera()->OffsetY.SetValue(paramList[i].OffsetY);         // Y 오프셋

			// Set 0의 Sequencer 트리거 설정
			getCamera()->SequencerTriggerSource.SetValue(SequencerTriggerSource_FrameStart);
			getCamera()->SequencerTriggerActivation.SetValue(SequencerTriggerActivation_RisingEdge);


			// Set Next Sequence Set 
			++nIndexNextSet;
			if (nIndexNextSet >= nSeqSetCount) nIndexNextSet = 0;

			getCamera()->SequencerSetNext.SetValue(nIndexNextSet);

			// Set i 저장
			getCamera()->SequencerSetSave.Execute();

			LogD() << "Sequencer Set " << i << " configured and saved";
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		// 6. 시작 세트 지정
		getCamera()->SequencerSetStart.SetValue(0);

		// 7. Sequencer Configuration 모드를 Off로 설정 (설정 완료)
		getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);

		// 8. Sequencer 설정 유효성 검사
		if (getCamera()->SequencerConfigurationValid.GetValue() == SequencerConfigurationValid_Yes) {
			LogD() << "Sequencer configuration is valid";

			// 9. Sequencer 모드 활성화
			getCamera()->SequencerMode.SetValue(SequencerMode_On);
			LogD() << "Sequencer mode activated for BFS-U3-122S6C";

			bRet = true;
		}
		else {
			LogD() << "Sequencer configuration is invalid";
			//getCamera()->SequencerMode.SetValue(SequencerMode_Off);

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
			getCamera()->SequencerMode.SetValue(SequencerMode_Off);
			getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);
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
	getCamera()->SequencerMode.SetValue(SequencerMode_Off);

	//loadUserSet_(1);
}

std::vector<std::vector<FrameSeqROIParam>> wso_device::ColorCamera::divideParamVector_(int nFrameCount, FrameSeqROIParam* arrayParams)
{
	std::vector<std::vector<FrameSeqROIParam>> result;

	if (arrayParams == nullptr || nFrameCount <= 0) {
		return result;
	}

	const int chunkSize = 8;
	int currentIndex = 0;

	while (currentIndex < nFrameCount) {
		std::vector<FrameSeqROIParam> chunk;

		// 현재 청크에 최대 8개 또는 남은 개수만큼 추가
		int endIndex = std::min(currentIndex + chunkSize, nFrameCount);

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
				getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet0);
			}
			break;
		case 2:
			{
				getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet1);
			}
			break;
		default:
			{
				getCamera()->UserSetSelector.SetValue(UserSetSelector_Default);
			}
			break;
	}
	getCamera()->UserSetSave.Execute();
}

void wso_device::ColorCamera::loadUserSet_(int eUserSet)
{
	switch (eUserSet)
	{
	case 1:
	{
		getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet0);
	}
	break;
	case 2:
	{
		getCamera()->UserSetSelector.SetValue(UserSetSelector_UserSet1);
	}
	break;
	default:
	{
		getCamera()->UserSetSelector.SetValue(UserSetSelector_Default);
	}
	break;
	}

	getCamera()->UserSetLoad.Execute();
}

void wso_device::ColorCamera::acquireCameraCaptureData(void)
{
	uint32_t width = getFrameWidth();
	uint32_t height = getFrameHeight();

	int nPixelFormat = getCamera()->PixelFormat.GetValue();
	int nBytesPerPixel = getBytesPerPixel();

	uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);

	uint32_t frameCount = 0;

	unsigned char* buffer = getImpl().sonyFrameBuffer;

	uint32_t nTotalFrameCount;
	if (getAcquisitionMode() == AcquisitionMode_SingleFrame)
	{
		nTotalFrameCount = 1;
	}
	else
	{
		nTotalFrameCount = (int)getCamera()->AcquisitionFrameCount.GetValue();
	}

	getCamera()->BeginAcquisition();

	for (uint32_t frameCount = 0; frameCount < nTotalFrameCount; ++frameCount)
	{
		getImpl().rawImage = getCamera()->GetNextImage();

		//ImageProcessor processor;
		//ImagePtr convertedImage = processor.Convert(getImpl().rawImage, PixelFormat_RGB8);

		//// Create a unique filename
		//ostringstream filename;

		//filename << "Acquisition-";
		//filename << nIndex << ".jpg";
		//convertedImage->Save(filename.str().c_str());

		std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);
		unique_lock<mutex> lock(getImpl().mutexLock);
		CallbackRegistry::getInstance()->runColorCameraImageCaptured(buffer, width, height, frameCount, nTotalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);
	}

	getCamera()->EndAcquisition();
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

void wso_device::ColorCamera::SetEventMode(bool enableEvents)
{
	try
	{
		if (!isInitiated()) {
			LogD() << "ColorCamera is not initialized!";
			return;
		}

		//if (!getCamera()->IsValid()) {
		//	WsoLogError("Camera is not valid!");
		//	return;
		//}

		//if (getCamera()->IsStreaming()) {
		//	getCamera()->EndAcquisition();
		//	WsoLogDebug("Acquisition stopped for event mode setup");
		//}

		if (enableEvents)
		{
			// DeviceEventHandler 생성 (아직 생성되지 않은 경우)
			if (!m_deviceEventHandler) {
				m_deviceEventHandler = std::make_unique<ColorCameraDeviceEventHandler>(this);
			}

			// ExposureEnd 이벤트 설정 및 핸들러 등록
			getCamera()->EventSelector.SetValue(EventSelector_ExposureEnd);
			getCamera()->EventNotification.SetValue(EventNotification_On);
			//getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventExposureEnd");
			getCamera()->RegisterEventHandler(*m_deviceEventHandler);
			LogD() << "ExposureEnd event enabled and handler registered";

			//// Error 이벤트 설정 및 핸들러 등록
			//getCamera()->EventSelector.SetValue(EventSelector_Error);
			//getCamera()->EventNotification.SetValue(EventNotification_On);
			//getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventError");
			//WsoLogInfo("Error event enabled and handler registered");

			//// SerialPortReceive 이벤트 설정 및 핸들러 등록 (필요한 경우)
			//getCamera()->EventSelector.SetValue(EventSelector_SerialPortReceive);
			//getCamera()->EventNotification.SetValue(EventNotification_On);
			//getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventSerialPortReceive");
			//WsoLogInfo("SerialPortReceive event enabled and handler registered");

			LogD() << "Event Control Mode activated for BFS-U3-122S6";
		}
		else
		{
			// EventHandler 등록 해제
			if (m_deviceEventHandler) {
				try {
					getCamera()->UnregisterEventHandler(*m_deviceEventHandler);
					LogD() << "Event handlers unregistered";
				}
				catch (const std::exception& e) {
					LogD() << "Error unregistering event handlers: " << std::string(e.what());
				}
			}

			// 모든 이벤트 비활성화
			getCamera()->EventSelector.SetValue(EventSelector_ExposureEnd);
			getCamera()->EventNotification.SetValue(EventNotification_Off);

			//getCamera()->EventSelector.SetValue(EventSelector_Error);
			//getCamera()->EventNotification.SetValue(EventNotification_Off);

			//getCamera()->EventSelector.SetValue(EventSelector_SerialPortReceive);
			//getCamera()->EventNotification.SetValue(EventNotification_Off);

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

void wso_device::ColorCamera::ColorCameraDeviceEventHandler::OnDeviceEvent(Spinnaker::GenICam::gcstring eventName)
{
	if (m_camera) {
		m_camera->HandleDeviceEvent(eventName);
	}
}

void wso_device::ColorCamera::HandleDeviceEvent(const Spinnaker::GenICam::gcstring& eventName)
{
	try
	{
		std::string strEventName = eventName.c_str();

		if (strEventName == "EventExposureEnd")
		{
			// ExposureEnd 이벤트 처리
			try {

				auto t0 = steady_clock::now();

				unique_lock<mutex> lock(getImpl().mutexLock);

				uint64_t frameID = getCamera()->EventExposureEndFrameID.GetValue();
				uint64_t timestamp = getCamera()->EventExposureEndTimestamp.GetValue();

				LogD() << "ExposureEnd Event - FrameID: " + std::to_string(frameID) << ", Timestamp: " + std::to_string(timestamp);

				uint32_t frameCount = getImpl().nOffsetFrameCount;
				uint32_t totalFrameCount = getImpl().cameraOffsetRoiParams->FrameCount;
				uint32_t nRoiWidth = getImpl().cameraOffsetRoiParams->RoiWidth;
				uint32_t nRoiHeight = getImpl().cameraOffsetRoiParams->RoiHeight;
				
				unsigned char* buffer = getImpl().sonyFrameBuffer;

				int nPixelFormat = getCamera()->PixelFormat.GetValue();
				int nBytesPerPixel = getBytesPerPixel();
				
				uint32_t frameSize = nRoiWidth * nRoiHeight * sizeof(char) * nBytesPerPixel;

				getImpl().rawImage = getCamera()->GetNextImage();

				std::memcpy(buffer, getImpl().rawImage->GetData(), frameSize);

				//unique_lock<mutex> lock(getImpl().mutexHandleLock);
				CallbackRegistry::getInstance()->runColorCameraOffsetROIFrameCaptured(buffer, nRoiWidth, nRoiHeight, 0, 0, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);

				getImpl().rawImage->Release();

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
		//		uint64_t errorCode = getCamera()->EventErrorCode.GetValue();
		//		uint64_t timestamp = getCamera()->EventErrorTimestamp.GetValue();

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
		//		uint64_t timestamp = getCamera()->EventSerialPortReceiveTimestamp.GetValue();
		//		std::string serialData = getCamera()->EventSerialData.GetValue().c_str();

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

#pragma endregion

