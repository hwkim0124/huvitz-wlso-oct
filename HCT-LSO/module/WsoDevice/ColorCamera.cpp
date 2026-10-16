#include "pch.h"
#include "ColorCamera.h"
#include "ColorCameraApplySequence.h"
#include "MainBoard.h"
#include "Spinnaker.h"

#include <format>
#include <chrono> // steady clock
#include <stdexcept>


using namespace wso_device;

using namespace Spinnaker;
using namespace std::chrono; // steady clock


namespace
{
	const char* geometryNodeName_(wso_device::detail::CameraGeometryNode node)
	{
		using wso_device::detail::CameraGeometryNode;
		switch (node) {
		case CameraGeometryNode::Width: return "Width";
		case CameraGeometryNode::Height: return "Height";
		case CameraGeometryNode::OffsetX: return "OffsetX";
		case CameraGeometryNode::OffsetY: return "OffsetY";
		case CameraGeometryNode::BinningHorizontal: return "BinningHorizontal";
		case CameraGeometryNode::BinningVertical: return "BinningVertical";
		}
		return "UnknownGeometryNode";
	}

	bool applyCameraGeometry_(const Spinnaker::CameraPtr& camera,
		const wso_device::detail::ColorCameraGeometryTarget& requested,
		wso_device::detail::ColorCameraGeometryTarget* applied = nullptr)
	{
		using namespace Spinnaker::GenApi;
		using wso_device::detail::CameraGeometryNode;
		using wso_device::detail::CameraIntegerRange;

		INodeMap& map = camera->GetNodeMap();
		auto readRange = [&](CameraGeometryNode id, CameraIntegerRange& range) {
			const char* name = geometryNodeName_(id);
			try {
				CIntegerPtr node = map.GetNode(name);
				if (!IsAvailable(node) || !IsReadable(node)) {
					WsoLogError(std::string("[CAM-GEO] ") + name + " range is not readable");
					return false;
				}
				range.minimum = node->GetMin();
				range.maximum = node->GetMax();
				range.increment = node->GetInc() > 0 ? node->GetInc() : 1;
				return true;
			}
			catch (const std::exception& e) {
				WsoLogError(std::string("[CAM-GEO] ") + name + " range read failed: " + e.what());
				return false;
			}
		};
		auto write = [&](CameraGeometryNode id, long long value) {
			const char* name = geometryNodeName_(id);
			try {
				CIntegerPtr node = map.GetNode(name);
				if (!IsAvailable(node) || !IsReadable(node)) {
					WsoLogError(std::string("[CAM-GEO] ") + name + " is not readable");
					return false;
				}
				if (node->GetValue() != value) {
					if (!IsWritable(node)) {
						WsoLogError(std::string("[CAM-GEO] ") + name + " is not writable");
						return false;
					}
					node->SetValue(value);
				}
				if (node->GetValue() != value) {
					WsoLogError(std::string("[CAM-GEO] ") + name + " read-back mismatch");
					return false;
				}
				return true;
			}
			catch (const std::exception& e) {
				WsoLogError(std::string("[CAM-GEO] ") + name + " write failed: " + e.what());
				return false;
			}
		};

		return wso_device::detail::runColorCameraGeometryApply(
			requested, write, readRange, applied);
	}
}


class ColorCameraDeviceEventHandler : public Spinnaker::DeviceEventHandler
{
public:
	ColorCameraDeviceEventHandler(ColorCamera* camera) : m_camera(camera) {}
	virtual ~ColorCameraDeviceEventHandler() {}

	// DeviceEventHandler?? ???? ???? ??? ????
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
	atomic<bool> captureMode{ false };
	atomic<bool> captureCancel{ false };
	// True from the first node write in setupHwTriggerSetting() until the HW-specific
	// device state is put back. Teardown restores only what HW mode installed, so a
	// Normal capture keeps issuing exactly the node writes it issued before.
	atomic<bool> hwTriggerInstalled{ false };
	// FrameStart trigger delay as it was before startTriggeredLiveMode() wrote the preview's
	// value. Only that path writes TriggerDelay, so the flag keeps every other teardown
	// (Normal capture, HW capture) from inventing a value for a node it never touched.
	atomic<bool> hasSavedTriggerDelay{ false };
	atomic<double> savedTriggerDelayUs{ 0.0 };
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
	// Defensive: a prior aborted attempt (or a start without a matching release)
	// in this process may have left a camera/system handle around. Tear it down
	// so re-initialization always starts from a clean state.
	if (impl().pCam || impl().systemPtr) {
		uninitialize();
	}

	try {
		impl().systemPtr = System::GetInstance();
		impl().camList = impl().systemPtr->GetCameras();
		unsigned int numCameras = impl().camList.GetSize();

		if (numCameras == 0)
		{
			impl().camList.Clear();
			impl().systemPtr->ReleaseInstance();
			impl().systemPtr = nullptr;
			LogE() << "Color camera init failed! ";
			return false;
		}
		impl().pCam = impl().camList.GetByIndex(0);
		impl().pCam->Init();

		if (!setDefaultParameters(true)) {
			WsoLogError("Color camera initialize failed after retries");
			return false;
		}

		impl().initiated = true;
		return true;
	}
	// Architecture rule: never let a Spinnaker/GenICam exception cross the C ABI
	// boundary. On failure return false so callers can retry or report cleanly.
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during ColorCamera initialize! " + std::string(e.what()));
		impl().initiated = false;
		return false;
	}
	catch (...) {
		WsoLogError("Unknown exception occurred during ColorCamera initialize!");
		impl().initiated = false;
		return false;
	}
}

bool wso_device::ColorCamera::isInitiated(void) const
{
	return impl().initiated;
}

void wso_device::ColorCamera::uninitialize(void)
{
	// 1. worker ?????? ???? ????
	if (impl().liveMode) {
		impl().liveMode = false;
		if (impl().worker.joinable()) {
			impl().worker.join();
		}
	}

	// rawImage grab ????
	impl().rawImage = nullptr;

	// 2. ???? ???? ??????? ????? ????
	if (impl().pCam && impl().pCam->IsStreaming()) {
		try { impl().pCam->EndAcquisition(); }
		catch (...) {}
	}
	// 3. ???? ??? ????
	if (impl().m_deviceEventHandler && impl().pCam && impl().pCam->IsValid()) {
		try { impl().pCam->UnregisterEventHandler(*(impl().m_deviceEventHandler)); }
		catch (...) {}
		impl().m_deviceEventHandler.reset();
	}
	// 4. DeInit ??? (?? ???? ???? ????)
	if (impl().pCam && impl().pCam->IsValid()) {
		try { impl().pCam->DeInit(); }
		catch (...) {}
	}
	// 5. ????? ????
	impl().pCam = nullptr;
	impl().camList.Clear();
	// 6. systemPtr null u? ?? ReleaseInstance
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

			// Trigger Mode ??????
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
			impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_MultiFrame); // Exposure Auto, Gain Auto, BalanceWhite Auto Off ?????? ???? ????? 

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
			WsoLogError("Exception occurred during ColorCamera init! " + std::string(e.what()));

			// Recovery must not throw across the C ABI. If it fails, stop retrying.
			if (!recoverCameraAfterFault()) {
				break;
			}
		}
	}
}

bool wso_device::ColorCamera::setDefaultParameters(bool update)
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

			// A prior SW/HW-trigger session can leave TriggerMode On on the device (it is
			// persistent state). Reset it so the camera comes up free-running -- otherwise
			// the grab test below blocks in GetNextImage() waiting for a trigger that never
			// arrives. Must be done while not streaming.
			int nTriggerMode = impl().getCamera()->TriggerMode.GetValue();
			if (nTriggerMode == TriggerMode_On)
				impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);
			WsoLogDebug("Color Camera Set TriggerMode Off...ok!");

			// Set Indicator LED
			impl().getCamera()->DeviceIndicatorMode.SetValue(DeviceIndicatorMode_Inactive);
			WsoLogDebug("Color Camera Set Indicator LED...ok!");

			// Pixel format is part of the dependency-sensitive geometry transition. A
			// previous session may have persisted Mono8 + 2x2, where WidthMax is only
			// 2736, so make the live ROI harmless before resetting binning and only then
			// apply the startup window against the new live node ranges.
			impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG8);
			const detail::ColorCameraGeometryTarget startupGeometry{
				1, 1, 4000, 3000, 736, 324
			};
			detail::ColorCameraGeometryTarget appliedGeometry;
			if (!applyCameraGeometry_(impl().getCamera(), startupGeometry, &appliedGeometry)) {
				throw std::runtime_error("startup camera geometry apply failed");
			}
			WsoLogDebug("Color Camera Set ImageSize bin=1x1 roi="
				+ std::to_string(appliedGeometry.width) + "x" + std::to_string(appliedGeometry.height)
				+ "+" + std::to_string(appliedGeometry.offsetX)
				+ "+" + std::to_string(appliedGeometry.offsetY) + "...ok!");

			// Set Acquisition mode SingleFrame
			//impl().getCamera()->ReverseX.SetValue(false);
			//impl().getCamera()->ReverseY.SetValue(true);	// Horizontal Flip
			//impl().getCamera()->PixelFormat.SetValue(PixelFormat_BayerRG16);
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
			return true;
		}
		catch (const std::exception& e)
		{
			WsoLogError("Exception occurred during ColorCamera init!" + std::string(e.what()));

			// Recovery must not throw across the C ABI. If it fails, stop retrying.
			if (!recoverCameraAfterFault()) {
				return false;
			}
		}
	}

	// All attempts exhausted without a successful configuration + grab test.
	return false;
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

bool wso_device::ColorCamera::startTriggeredLiveMode(unsigned int triggerDelayUs, unsigned int armTimeoutMs,
	bool triggerAlreadyInstalled)
{
	if (!isInitiated()) {
		WsoLogError("[COLOR-PRV] triggered live start refused: camera not initiated");
		return false;
	}
	if (impl().liveMode || impl().captureMode) {
		WsoLogError("[COLOR-PRV] triggered live start refused: a live/capture session is active");
		return false;
	}

	// Claimed before the first node write so a concurrent start is refused above, and so
	// the failure paths below can hand it straight back.
	impl().liveMode = true;

	try {
		// triggerAlreadyInstalled: the caller applied the preset's own trigger, GPIO and
		// TriggerDelay values through applyFullSettings and verified every one of them.
		// Re-running the hardcoded setup here would replace exactly those stated values,
		// which is the overwrite A-1 exists to remove - so this path only arms.
		if (!triggerAlreadyInstalled) {
			if (!setupHwTriggerSetting()) {
				impl().liveMode = false;
				abortCaptureFrames();	// puts back whatever the half-done setup installed
				return false;
			}

			// The DevKit camera configs carry TriggerDelay (288us for both preview and capture)
			// and setupHwTriggerSetting() does not write it, so it is applied - and remembered -
			// here rather than by widening the shared trigger setup.
			if (Spinnaker::GenApi::IsWritable(impl().getCamera()->TriggerDelay)) {
				if (Spinnaker::GenApi::IsReadable(impl().getCamera()->TriggerDelay)) {
					impl().savedTriggerDelayUs = impl().getCamera()->TriggerDelay.GetValue();
					impl().hasSavedTriggerDelay = true;
				}
				impl().getCamera()->TriggerDelay.SetValue((double)triggerDelayUs);
			}
			else {
				WsoLogInfo("[COLOR-PRV] TriggerDelay is not writable on this camera - requested "
					+ std::to_string(triggerDelayUs) + "us not applied");
			}
		}
		else {
			WsoLogInfo("[COLOR-PRV] trigger configuration was installed by the preset apply - arming only");
		}

		impl().getCamera()->BeginAcquisition();

		// Bounded arm verification BEFORE the caller starts the board: a trigger that
		// arrives while the camera is not waiting for it is simply lost.
		if (!waitUntilTriggerArmed(armTimeoutMs)) {
			impl().liveMode = false;
			abortCaptureFrames();
			return false;
		}

		WsoLogInfo("[COLOR-PRV] triggered live armed (delayUs=" + std::to_string(triggerDelayUs)
			+ " armBudgetMs=" + std::to_string(armTimeoutMs) + ")");

		impl().worker = thread{ &ColorCamera::acquireCameraTriggeredLiveData, this };
		return true;
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-PRV] triggered live start failed: " + std::string(e.what()));
		impl().liveMode = false;
		try {
			abortCaptureFrames();
		}
		catch (...) {
		}
		return false;
	}
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

	// Software Trigger ????
	setupSoftwareTrigger(1); // Single Frame

	// Acquisition ????
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

	// Trigger Mode ??????
	int nTriggerMode = impl().getCamera()->TriggerMode.GetValue();
	if (nTriggerMode == TriggerMode_On)
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

	impl().liveMode = false;
	return;
}

void wso_device::ColorCamera::setupSoftwareTrigger(int nMode)
{
	try {
		// 1. Acquisition ???
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}

		// 2. Trigger Mode ????
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_On);

		// 3. Trigger Source?? Software?? ????
		impl().getCamera()->TriggerSource.SetValue(TriggerSource_Software);

		// 4. Trigger Activation ???? (Rising Edge)
		//impl().getCamera()->TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// 5. Acquisition Mode ???? (SingleFrame ??? MultiFrame)
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

		// 6. Trigger Selector ???? (Frame Start)
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

	setupHwTriggerSetting();

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

bool wso_device::ColorCamera::setupHwTriggerSetting(void)
{
	try {
		// 1. Acquisition ???
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}
		// Mark before the first write: a setup that fails halfway still needs teardown.
		impl().hwTriggerInstalled = true;
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);

		// 6. Trigger Selector ???? (Frame Start)
		impl().getCamera()->TriggerSelector.SetValue(TriggerSelector_FrameStart);

		// 3. Trigger Source?? Software?? ????
		impl().getCamera()->TriggerSource.SetValue(TriggerSource_Line3);

		// 4. Trigger Activation?? ReadOut???? ????
		impl().getCamera()->TriggerActivation.SetValue(TriggerActivation_RisingEdge);

		// 4. Trigger Overlap?? ReadOut???? ????
		impl().getCamera()->TriggerOverlap.SetValue(TriggerOverlap_ReadOut);

		// 4. LineSelector?? Line2?? ????
		impl().getCamera()->LineSelector.SetValue(LineSelector_Line2);
		impl().getCamera()->LineSource.SetValue(LineSource_Off);

		// 4. LineMode?? Output?? ????
		impl().getCamera()->LineMode.SetValue(LineMode_Output);

		// 4. LineSource?? Output?? ????
		impl().getCamera()->LineSource.SetValue(LineSource_ExposureActive);
		impl().getCamera()->LineInverter.SetValue(false);

		// 4. LineSelector?? Line2?? ????
		impl().getCamera()->LineSelector.SetValue(LineSelector_Line3);

		// 4. LineMode?? Output?? ????
		impl().getCamera()->LineMode.SetValue(LineMode_Input);
		impl().getCamera()->LineInverter.SetValue(false);

		// 5. Acquisition Mode ???? (Continuous ??? MultiFrame)
		impl().getCamera()->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
		impl().getCamera()->ExposureAuto.SetValue(ExposureAuto_Off);

		// 2. Trigger Mode ????
		impl().getCamera()->TriggerMode.SetValue(TriggerMode_On);

		// Select the acquisition state that waitUntilTriggerArmed() polls while the node is
		// still writable - BeginAcquisition() locks configuration nodes.
		if (Spinnaker::GenApi::IsWritable(impl().getCamera()->AcquisitionStatusSelector)) {
			impl().getCamera()->AcquisitionStatusSelector.SetValue(AcquisitionStatusSelector_FrameTriggerWait);
		}
		else {
			// Measured on this instrument: the camera does not implement this SFNC feature at
			// all. Informational, not an error - waitUntilTriggerArmed() skips the check.
			WsoLogInfo("[COLOR-CAP] AcquisitionStatusSelector unavailable on this camera");
		}

		WsoLogInfo("Hardware trigger setup completed");
		return true;
	}
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during hardware trigger setup: " + std::string(e.what()));
		return false;
	}
}

bool wso_device::ColorCamera::setupCameraTriggerOnOffOnly(bool bOn)
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
			// Normal/free-run must not inherit the HW capture's ExposureActive output, even
			// if the HW session that installed it never reached its own teardown. The same
			// applies to the triggered preview's FrameStart trigger delay.
			impl().getCamera()->LineSelector.SetValue(LineSelector_Line2);
			impl().getCamera()->LineSource.SetValue(LineSource_Off);
			impl().hwTriggerInstalled = false;
			restoreTriggerDelay_();
			//WsoLogInfo("Camera trigger mode : Off");
		}
		return true;
	}
	catch (const std::exception& e) {
		WsoLogError("Exception occurred during to change camera trigger mode : " + std::string(e.what()));
		return false;
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

bool wso_device::ColorCamera::beginCaptureFrames(bool hwTrigger, bool triggerAlreadyInstalled)
{
	if (!isInitiated() || impl().liveMode || impl().captureMode) {
		return false;
	}

	// Cancellation is NOT cleared here. This runs on the capture worker, so clearing it
	// erased a stop that arrived between the accepted start and the worker actually
	// starting - the capture then ran to completion and the stop call blocked on it.
	// armCaptureSession() owns the clear, on the starting thread, under the caller's lock.
	impl().captureMode = true;

	try {
		// With the trigger already installed and verified by the preset apply, writing the
		// hardcoded setup again would discard the operator's stated values - and for a
		// Normal capture it would also drop a Line2 strobe the preset deliberately set.
		const bool configured = triggerAlreadyInstalled
			? true
			: (hwTrigger ? setupHwTriggerSetting() : setupCameraTriggerOnOffOnly(false));
		if (!configured) {
			abortCaptureFrames();
			return false;
		}

		impl().getCamera()->BeginAcquisition();
		return true;
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-CAP] begin acquisition failed: " + std::string(e.what()));
		abortCaptureFrames();
		return false;
	}
}

bool wso_device::ColorCamera::waitUntilTriggerArmed(unsigned int timeoutMs)
{
	try {
		auto camera = impl().getCamera();
		const bool streaming = camera->IsStreaming();
		const bool selectorWritable = Spinnaker::GenApi::IsWritable(camera->AcquisitionStatusSelector);
		const bool statusReadable = Spinnaker::GenApi::IsReadable(camera->AcquisitionStatus);

		if (!streaming) {
			WsoLogError("[COLOR-CAP] camera is not streaming, cannot verify trigger arming");
			return false;
		}

		// Measured on this instrument: AcquisitionStatus / AcquisitionStatusSelector are not
		// implemented by this camera (selectorWritable=0, statusReadable=0 both before and after
		// BeginAcquisition). The HW spec's closed-loop arming pre-condition therefore cannot be
		// evaluated here. Failing the capture for that would block a capture the hardware can
		// actually perform, which is exactly what the pre-T2 path did anyway (it polled
		// IsStreaming and started the scanner regardless). So: report it and proceed unverified.
		if (!statusReadable) {
			WsoLogInfo("[COLOR-CAP] AcquisitionStatus unsupported on this camera"
				" (selectorWritable=" + std::to_string(selectorWritable ? 1 : 0)
				+ ") - trigger arming NOT verified, proceeding");
			return true;
		}
		if (selectorWritable) {
			camera->AcquisitionStatusSelector.SetValue(AcquisitionStatusSelector_FrameTriggerWait);
		}
		const auto started = steady_clock::now();
		const auto deadline = started + milliseconds(timeoutMs);
		do {
			if (camera->AcquisitionStatus.GetValue()) {
				const auto ms = duration_cast<milliseconds>(steady_clock::now() - started).count();
				WsoLogInfo("[COLOR-CAP] FrameTriggerWait armed in " + std::to_string(ms) + "ms");
				return true;
			}
			std::this_thread::sleep_for(milliseconds(1));
		} while (steady_clock::now() < deadline && !impl().captureCancel);

		// Source order is already spec-correct here, so a timeout means the device did not
		// report the state. Log what the camera actually says so the next run tells us whether
		// this is a budget problem or a node that never becomes true on this model.
		std::string diag = "[COLOR-CAP] FrameTriggerWait readiness timed out after "
			+ std::to_string(timeoutMs) + "ms";
		diag += impl().captureCancel ? " (cancelled)" : "";
		try {
			diag += " streaming=" + std::to_string(camera->IsStreaming() ? 1 : 0);
			if (Spinnaker::GenApi::IsReadable(camera->TriggerMode)) {
				diag += " triggerMode=" + std::string(camera->TriggerMode.ToString().c_str());
			}
			if (Spinnaker::GenApi::IsReadable(camera->TriggerSource)) {
				diag += " triggerSource=" + std::string(camera->TriggerSource.ToString().c_str());
			}
			if (Spinnaker::GenApi::IsReadable(camera->AcquisitionMode)) {
				diag += " acqMode=" + std::string(camera->AcquisitionMode.ToString().c_str());
			}
			if (selectorWritable) {
				camera->AcquisitionStatusSelector.SetValue(AcquisitionStatusSelector_AcquisitionActive);
				diag += " acquisitionActive=" + std::to_string(camera->AcquisitionStatus.GetValue() ? 1 : 0);
			}
		}
		catch (const std::exception& e) {
			diag += " (diag read failed: " + std::string(e.what()) + ")";
		}
		WsoLogError(diag);
		return false;
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-CAP] FrameTriggerWait readiness failed: " + std::string(e.what()));
		return false;
	}
}

// Ends the acquisition and puts back the per-mode device state this class installed, so
// the next capture starts from the same camera state regardless of which mode ran before.
// What HW mode installs and what happens to it here:
//  - TriggerMode=On            -> set back to Off (below); everything TriggerSelector /
//                                 TriggerSource / TriggerActivation / TriggerOverlap
//                                 configures only acts while TriggerMode is On.
//  - Line2 LineSource=ExposureActive -> set back to Off (below). This is the only installed
//                                 node that drives a pin with TriggerMode Off, and Off is
//                                 the only value this module ever writes to LineSource.
//  - TriggerDelay              -> put back to the value read before the triggered preview
//                                 overwrote it (restoreTriggerDelay_). Only that path
//                                 writes the node, so for a capture this is a no-op.
//  - Line2 LineMode=Output, ExposureAuto=Off -> left as-is: setDefaultParameters() installs
//                                 exactly these values at init, so they are not drift.
//  - AcquisitionMode=Continuous -> left as-is: every capture start re-applies AND re-verifies
//                                 it through setCameraSettingsChecked(), so it self-heals.
//  - Line3 LineMode=Input, Line2/Line3 LineInverter=false -> left as-is: no other write site
//                                 for these exists in this module, so there is no in-code
//                                 baseline to restore, and Input drives nothing. Picking a
//                                 value here would be firmware speculation.
void wso_device::ColorCamera::abortCaptureFrames(void)
{
	if (!isInitiated()) {
		impl().captureMode = false;
		return;
	}

	try {
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}
		if (impl().getCamera()->TriggerMode.GetValue() == TriggerMode_On) {
			impl().getCamera()->TriggerMode.SetValue(TriggerMode_Off);
		}
		if (impl().hwTriggerInstalled) {
			impl().getCamera()->LineSelector.SetValue(LineSelector_Line2);
			impl().getCamera()->LineSource.SetValue(LineSource_Off);
			impl().hwTriggerInstalled = false;
		}
		restoreTriggerDelay_();
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-CAP] acquisition cleanup failed: " + std::string(e.what()));
	}
	impl().captureMode = false;
}

// Only startTriggeredLiveMode() writes TriggerDelay, and it records the previous value
// first. Without that record there is no in-code baseline to restore, so this does
// nothing rather than pick one.
void wso_device::ColorCamera::restoreTriggerDelay_(void)
{
	if (!impl().hasSavedTriggerDelay) {
		return;
	}
	impl().hasSavedTriggerDelay = false;
	try {
		if (Spinnaker::GenApi::IsWritable(impl().getCamera()->TriggerDelay)) {
			impl().getCamera()->TriggerDelay.SetValue(impl().savedTriggerDelayUs);
		}
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-PRV] trigger delay restore failed: " + std::string(e.what()));
	}
}

// Capture one-call worker body: bounded, cancellable acquisition. Modeled on
// acquireCameraCaptureData(), but reports how it ended instead of running silently.
// Runs on the caller's thread (the capture one-call calls it from its worker), and
// requires a successful beginCaptureFrames() first - the acquisition is already running
// when this is entered. totalFrameCount is the caller's bound (device readback for
// Normal, the requested count for the Continuous HW-trigger stream); hwTrigger only
// selects the per-image wait budget. Always ends in abortCaptureFrames().
int wso_device::ColorCamera::acquireCaptureFrames(int* framesAcquired, unsigned int totalFrameCount, bool hwTrigger)
{
	if (framesAcquired) {
		*framesAcquired = 0;
	}

	if (!isInitiated() || !impl().captureMode) {
		abortCaptureFrames();
		return wso_domain::CAP_STAGE_CAMERA;
	}

	int result = wso_domain::CAP_STAGE_OK;
	int frames = 0;

	try {
		uint32_t width = getFrameWidth();
		uint32_t height = getFrameHeight();
		int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		int nBytesPerPixel = getBytesPerPixel();
		uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);
		unsigned char* buffer = impl().sonyFrameBuffer;

		WsoLogInfo("[COLOR-CAP] begin pf=" + std::to_string(nPixelFormat)
			+ " w=" + std::to_string(width) + " h=" + std::to_string(height)
			+ " total=" + std::to_string(totalFrameCount));

		int consecutiveTimeouts = 0;
		const unsigned int imageTimeoutMs = hwTrigger
			? getExposureTime() / 1000U + 200U
			: 1000U;
		for (uint32_t frameCount = 0; frameCount < totalFrameCount;) {
			if (impl().captureCancel) {
				result = wso_domain::CAP_STAGE_CANCELLED;
				break;
			}

			try {
				impl().rawImage = impl().getCamera()->GetNextImage(imageTimeoutMs);
			}
			catch (const Spinnaker::Exception&) {
				// Timeout or transient read fault: re-check cancel, retry a few times.
				if (impl().captureCancel) {
					result = wso_domain::CAP_STAGE_CANCELLED;
					break;
				}
				if (++consecutiveTimeouts >= 3) {
					WsoLogError("[COLOR-CAP] GetNextImage timed out 3x - aborting");
					result = wso_domain::CAP_STAGE_CAMERA;
					break;
				}
				continue;
			}
			consecutiveTimeouts = 0;

			try {
				std::memcpy(buffer, impl().rawImage->GetData(), frameSize);
				unique_lock<mutex> lock(impl().mutexLock);
				CallbackRegistry::getInstance()->runColorCameraImageCaptured(buffer, width, height, frameCount, totalFrameCount, getFlipMode(), nPixelFormat, nBytesPerPixel);
			}
			catch (...) {
				impl().rawImage->Release();
				throw;
			}
			impl().rawImage->Release();
			++frameCount;
			++frames;
		}

	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-CAP] exception: " + std::string(e.what()));
		result = wso_domain::CAP_STAGE_CAMERA;
	}
	catch (...) {
		WsoLogError("[COLOR-CAP] unknown exception");
		result = wso_domain::CAP_STAGE_CAMERA;
	}

	abortCaptureFrames();
	if (framesAcquired) {
		*framesAcquired = frames;
	}
	WsoLogInfo("[COLOR-CAP] end result=" + std::to_string(result) + " frames=" + std::to_string(frames));
	return result;
}

// Begins a capture session: the ONE place that clears cancellation. The capture exports
// call it on the starting thread while they hold their capture-state lock, so a stop can
// only land before it (and is then cleared by the new session) or after it (and is then
// observed). Clearing it anywhere on the worker loses that ordering.
void wso_device::ColorCamera::armCaptureSession(void)
{
	impl().captureCancel = false;
}

void wso_device::ColorCamera::cancelCaptureFrames(void)
{
	impl().captureCancel = true;
}

bool wso_device::ColorCamera::isCaptureMode(void) const
{
	return impl().captureMode;
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
	setCameraSettings_(params, true);
}

void wso_device::ColorCamera::setCameraSettings_(ColorCameraSettingParam params, bool applyGain)
{
	try
	{
		/////?????? Width, Height ???? ????? offset ???? ?????? ?????? ??????, o???? 0,0???? ?????? ???? ???????.
		// ???? ???(???? ???? ???): ?? ????? Mono8 ???? binning ???? 2 ???,
		// binning ?? ???? ROI(Width/Height) ??�N ???????(bin2 ???? Width max=2736, bin1 ???? 5472).
		// ???? (1) pixelFormat ???? ? Mono8 ?? 'binning>=2' ?????? ???? ????(??: Bayer ?? ???)
		//        (2) binning     ? Bayer ?????? 1 ???
		//        (3) binning ??? ?? ROI ? Width/Height max ?? ??? binning ?? ????
		// ?? ??????? Mono8/bin2 ?? Bayer/bin1 ??????? OutOfRange(Width 3648>2736, Binning 1<2) ???? ???????.
		// Each write is guarded on its own. One refused node used to abort the whole
		// sequence through the single catch below, so a locked ExposureTime (which is what
		// ExposureAuto On makes it) silently skipped the acquisition mode, frame count, gain
		// and ADC depth that follow it - and the caller was then told the ROI had mismatched.
		// A step that cannot be written is logged and the rest still runs; the read-back
		// verification in setCameraSettingsChecked is what decides success.
		auto step = [](const char* what, auto&& write) {
			try {
				write();
			}
			catch (const std::exception& e) {
				WsoLogError(std::string("[SET] ") + what + " not applied: " + e.what());
			}
		};

		step("PixelFormat", [&] { setPixelFormat(params.pixelFormat); });
		const detail::ColorCameraGeometryTarget geometry{
			params.binningHorizontal, params.binningVertical,
			params.roi_x_width, params.roi_y_height,
			params.roi_x_offset, params.roi_y_offset
		};
		if (!applyCameraGeometry_(impl().getCamera(), geometry)) {
			WsoLogError("[SET] dependency-safe binning/ROI geometry was not applied");
		}

		// The manual exposure is NOT forced while the camera is metering for itself:
		// ExposureAuto Once/Continuous makes the node read-only, and writing it anyway is
		// both an exception and the "Auto 중 수동 노출값 강제 덮어쓰기" the A-1 spec forbids.
		// Whoever wants manual control sets ExposureAuto Off - that is a separate, explicit
		// field in the preset, not something this write decides on their behalf.
		if (IsWritable(impl().getCamera()->ExposureTime)) {
			step("ExposureTime", [&] { setExposureTime(params.exposure_time); });
		}
		else {
			WsoLogInfo("[SET] ExposureTime is not writable (ExposureAuto is on?) - the requested "
				+ std::to_string(params.exposure_time) + "us was not forced");
		}

		step("AcquisitionMode", [&] { setAcquisitionMode(params.acqusitionMode); });
		step("AcquisitionFrameCount", [&] { setAcquisitionFrameCount(params.acqusitionFrameCount); });
		if (applyGain) {
			step("Gain", [&] { setGain(params.gain); });
		}
		step("AdcBitDepth", [&] { setAdcBitDepth(params.adcDepthIndex); });

		// [??? ????] ??u?? vs ???? ???�S. ???? ?? ????.
		WsoLogInfo("[SET] req pf=" + std::to_string(params.pixelFormat) + " binH=" + std::to_string(params.binningHorizontal)
			+ " -> now pf=" + std::to_string(getPixelFormat()) + " binH=" + std::to_string(getBinningHorizontal())
			+ " binV=" + std::to_string(getBinningVertical()));

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

bool wso_device::ColorCamera::setCameraSettingsChecked(const ColorCameraSettingParam& params)
{
	return setCameraSettingsChecked_(params, true);
}

bool wso_device::ColorCamera::setCameraSettingsChecked_(const ColorCameraSettingParam& params, bool applyGain)
{
	setCameraSettings_(params, applyGain);

	try {
		bool ok = getPixelFormat() == params.pixelFormat
			&& getBinningHorizontal() == params.binningHorizontal
			&& getBinningVertical() == params.binningVertical
			&& getROI_X_Width() == params.roi_x_width
			&& getROI_Y_Height() == params.roi_y_height
			&& getROI_X_Offset() == params.roi_x_offset
			&& getROI_Y_Offset() == params.roi_y_offset
			&& getAcquisitionMode() == params.acqusitionMode
			&& getAdcBitDepth() == params.adcDepthIndex;

		// Frame count only matters (and is only re-checked) in MultiFrame mode (2).
		if (ok && params.acqusitionMode == 2) {
			ok = getAcquisitionFrameCount() == params.acqusitionFrameCount;
		}

		if (!ok) {
			WsoLogError("[COLOR-CAP] settings verify mismatch: req pf=" + std::to_string(params.pixelFormat)
				+ " binH=" + std::to_string(params.binningHorizontal) + " binV=" + std::to_string(params.binningVertical)
				+ " roi=" + std::to_string(params.roi_x_width) + "x" + std::to_string(params.roi_y_height)
				+ "+" + std::to_string(params.roi_x_offset) + "+" + std::to_string(params.roi_y_offset)
				+ " acqMode=" + std::to_string(params.acqusitionMode) + " acqCount=" + std::to_string(params.acqusitionFrameCount)
				+ " / act pf=" + std::to_string(getPixelFormat())
				+ " binH=" + std::to_string(getBinningHorizontal()) + " binV=" + std::to_string(getBinningVertical())
				+ " roi=" + std::to_string(getROI_X_Width()) + "x" + std::to_string(getROI_Y_Height())
				+ "+" + std::to_string(getROI_X_Offset()) + "+" + std::to_string(getROI_Y_Offset())
				+ " acqMode=" + std::to_string(getAcquisitionMode()) + " acqCount=" + std::to_string(getAcquisitionFrameCount()));
		}
		return ok;
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-CAP] settings verify exception: " + std::string(e.what()));
		return false;
	}
	catch (...) {
		return false;
	}
}

// --- A-1 extended configuration: capability, apply, read-back -------------------------
// Values travel as GenApi SYMBOLIC names, so nothing here depends on the SDK's integer
// enum values (which differ per model and per release) and nothing hardcodes a list of
// "supported" entries: the camera's own node map answers what exists.
namespace
{
	using namespace Spinnaker::GenApi;
	using namespace wso_domain;

	void copyName_(char* dst, const char* src)
	{
		if (dst == nullptr) { return; }
		if (src == nullptr) { dst[0] = '\0'; return; }
		strncpy_s(dst, LSO_CAMERA_NAME_MAX, src, _TRUNCATE);
	}

	void copyNumber_(char* dst, double value)
	{
		char buf[LSO_CAMERA_NAME_MAX] = {};
		snprintf(buf, sizeof(buf), "%.6g", value);
		copyName_(dst, buf);
	}

	// A field is only a claim when it is non-empty. An ABI field may use all 32 bytes
	// without a terminator, so every read of one is bounded by the field width.
	bool stated_(const char* name)
	{
		return name != nullptr && name[0] != '\0';
	}

	std::string fieldText_(const char* field)
	{
		size_t n = 0;
		while (n < (size_t)LSO_CAMERA_NAME_MAX && field[n] != '\0') { ++n; }
		return std::string(field, n);
	}

	ColorCameraFieldResult* beginRow_(ColorCameraApplyResult* r, const char* node, const char* requested)
	{
		if (r == nullptr || r->fieldCount >= LSO_CAMERA_FIELD_MAX) { return nullptr; }
		ColorCameraFieldResult& row = r->fields[r->fieldCount++];
		copyName_(row.node, node);
		copyName_(row.requested, requested);
		row.actual[0] = '\0';
		row.status = CAM_FIELD_NOT_REQUESTED;
		return &row;
	}

	int finishRow_(ColorCameraApplyResult* r, ColorCameraFieldResult* row, int status, const char* actual)
	{
		if (row != nullptr) {
			row->status = status;
			copyName_(row->actual, actual);
		}
		if (r != nullptr && status != CAM_FIELD_APPLIED && status != CAM_FIELD_NOT_REQUESTED) {
			++r->failedCount;
		}
		return status;
	}

	// Writes one enum node by symbolic name and verifies the read-back.
	// "node absent", "entry not implemented", "not writable now", "write refused" and
	// "read-back differs" are five different answers, and the caller needs all five: only
	// the first two mean "this camera cannot do it".
	int applyEnumNode_(INodeMap& map, const char* nodeName, const char* value, ColorCameraApplyResult* r)
	{
		if (!stated_(value)) { return CAM_FIELD_NOT_REQUESTED; }

		const std::string requested = fieldText_(value);
		ColorCameraFieldResult* row = beginRow_(r, nodeName, requested.c_str());
		try {
			CEnumerationPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) {
				return finishRow_(r, row, CAM_FIELD_UNSUPPORTED, "node not implemented");
			}

			CEnumEntryPtr entry = node->GetEntryByName(requested.c_str());
			if (!IsAvailable(entry) || !IsReadable(entry)) {
				return finishRow_(r, row, CAM_FIELD_UNSUPPORTED, "entry not implemented");
			}

			CEnumEntryPtr current = node->GetCurrentEntry();
			const bool already = IsReadable(current)
				&& requested == std::string(current->GetSymbolic().c_str());
			if (!already) {
				if (!IsWritable(node)) {
					return finishRow_(r, row, CAM_FIELD_LOCKED, "not writable in this state");
				}
				node->SetIntValue(entry->GetValue());
			}

			current = node->GetCurrentEntry();
			if (!IsReadable(current)) {
				return finishRow_(r, row, CAM_FIELD_MISMATCH, "value not readable back");
			}
			const std::string actual(current->GetSymbolic().c_str());
			return finishRow_(r, row, actual == requested ? CAM_FIELD_APPLIED : CAM_FIELD_MISMATCH,
				actual.c_str());
		}
		catch (const std::exception& e) {
			WsoLogError(std::string("[CAM-EXT] ") + nodeName + " write failed: " + e.what());
			return finishRow_(r, row, CAM_FIELD_WRITE_FAILED, "SDK refused the write");
		}
	}

	// Integer node. The value is clamped into the device's range and aligned down to its
	// increment first, and the read-back is compared against THAT target - not against the
	// raw request. A device that quantised the value did what was asked of it as closely as
	// it can, so the row is ADJUSTED (reported, not fatal); only a read-back that does not
	// match the target the write aimed at is a MISMATCH.
	int applyIntNode_(INodeMap& map, const char* nodeName, long long value, ColorCameraApplyResult* r)
	{
		if (value < 0) { return CAM_FIELD_NOT_REQUESTED; }

		char requested[LSO_CAMERA_NAME_MAX] = {};
		snprintf(requested, sizeof(requested), "%lld", value);
		ColorCameraFieldResult* row = beginRow_(r, nodeName, requested);
		try {
			CIntegerPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) {
				return finishRow_(r, row, CAM_FIELD_UNSUPPORTED, "node not implemented");
			}

			long long target = value;
			const long long lo = node->GetMin();
			const long long hi = node->GetMax();
			const long long inc = node->GetInc() > 0 ? node->GetInc() : 1;
			if (target < lo) { target = lo; }
			if (target > hi) { target = hi; }
			target -= ((target - lo) % inc);

			// Already holding it: nothing to write, so a node locked in this state is not a
			// problem - the camera is what the caller asked it to be.
			if (node->GetValue() != target) {
				if (!IsWritable(node)) {
					return finishRow_(r, row, CAM_FIELD_LOCKED, "not writable in this state");
				}
				node->SetValue(target);
			}

			const long long actual = node->GetValue();
			char actualText[LSO_CAMERA_NAME_MAX] = {};
			snprintf(actualText, sizeof(actualText), "%lld", actual);
			if (actual != target) {
				return finishRow_(r, row, CAM_FIELD_MISMATCH, actualText);
			}
			return finishRow_(r, row, (target == value) ? CAM_FIELD_APPLIED : CAM_FIELD_ADJUSTED, actualText);
		}
		catch (const std::exception& e) {
			WsoLogError(std::string("[CAM-EXT] ") + nodeName + " write failed: " + e.what());
			return finishRow_(r, row, CAM_FIELD_WRITE_FAILED, "SDK refused the write");
		}
	}

	// Float node. Same rule as the integer path: clamp into range first, compare the
	// read-back against that target, and report a clamp as ADJUSTED rather than as a
	// failure. The device quantises to its own increment, so the comparison carries that
	// increment as its tolerance, and the reported actual is always the device's value.
	int applyFloatNode_(INodeMap& map, const char* nodeName, double value, ColorCameraApplyResult* r,
		double* previousValue)
	{
		if (value < 0.0) { return CAM_FIELD_NOT_REQUESTED; }

		char requested[LSO_CAMERA_NAME_MAX] = {};
		snprintf(requested, sizeof(requested), "%.6g", value);
		ColorCameraFieldResult* row = beginRow_(r, nodeName, requested);
		try {
			CFloatPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) {
				return finishRow_(r, row, CAM_FIELD_UNSUPPORTED, "node not implemented");
			}

			const double before = node->GetValue();
			if (previousValue != nullptr) { *previousValue = before; }

			const double lo = node->GetMin();
			const double hi = node->GetMax();
			const double target = detail::clampCameraFloat(value, lo, hi);
			const double inc = (node->HasInc() && node->GetInc() > 0.0) ? node->GetInc() : 0.0;

			// Already holding it: skip the write, exactly as the enum, integer and boolean
			// helpers do. Without this a camera that has the right value but reports the
			// node locked in this state fails a start that would have been fine.
			if (!detail::cameraFloatReadbackMatches(target, before, inc)) {
				if (!IsWritable(node)) {
					return finishRow_(r, row, CAM_FIELD_LOCKED, "not writable in this state");
				}
				node->SetValue(target);
			}

			const double actual = node->GetValue();
			char actualText[LSO_CAMERA_NAME_MAX] = {};
			snprintf(actualText, sizeof(actualText), "%.6g", actual);

			if (!detail::cameraFloatReadbackMatches(target, actual, inc)) {
				return finishRow_(r, row, CAM_FIELD_MISMATCH, actualText);
			}
			return finishRow_(r, row, detail::cameraFloatReadbackMatches(value, actual, inc)
				? CAM_FIELD_APPLIED : CAM_FIELD_ADJUSTED, actualText);
		}
		catch (const std::exception& e) {
			WsoLogError(std::string("[CAM-EXT] ") + nodeName + " write failed: " + e.what());
			return finishRow_(r, row, CAM_FIELD_WRITE_FAILED, "SDK refused the write");
		}
	}

	int applyBoolNode_(INodeMap& map, const char* nodeName, int value, ColorCameraApplyResult* r)
	{
		if (value < 0) { return CAM_FIELD_NOT_REQUESTED; }

		const bool target = (value != 0);
		ColorCameraFieldResult* row = beginRow_(r, nodeName, target ? "true" : "false");
		try {
			CBooleanPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) {
				return finishRow_(r, row, CAM_FIELD_UNSUPPORTED, "node not implemented");
			}
			if (node->GetValue() != target) {
				if (!IsWritable(node)) {
					return finishRow_(r, row, CAM_FIELD_LOCKED, "not writable in this state");
				}
				node->SetValue(target);
			}
			const bool actual = node->GetValue();
			return finishRow_(r, row, actual == target ? CAM_FIELD_APPLIED : CAM_FIELD_MISMATCH,
				actual ? "true" : "false");
		}
		catch (const std::exception& e) {
			WsoLogError(std::string("[CAM-EXT] ") + nodeName + " write failed: " + e.what());
			return finishRow_(r, row, CAM_FIELD_WRITE_FAILED, "SDK refused the write");
		}
	}

	// What counts as "the camera is now what the caller asked for". ADJUSTED is included:
	// the write succeeded and the device holds the nearest value it can, which is a
	// configuration it can actually run. It still shows up in the result table with the
	// requested and actual values side by side, so it is reported, never silent.
	bool rowOk_(int status)
	{
		return status == CAM_FIELD_APPLIED
			|| status == CAM_FIELD_NOT_REQUESTED
			|| status == CAM_FIELD_ADJUSTED;
	}

	// Current symbolic value of an enum node, or "" when it cannot be read. A snapshot
	// built from this therefore states only what could actually be read back, so restoring
	// it never invents a value for a node this module cannot see.
	void readEnumNode_(INodeMap& map, const char* nodeName, char* dst)
	{
		dst[0] = '\0';
		try {
			CEnumerationPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) { return; }
			CEnumEntryPtr current = node->GetCurrentEntry();
			if (!IsReadable(current)) { return; }
			copyName_(dst, current->GetSymbolic().c_str());
		}
		catch (const std::exception&) {
		}
	}

	long long readIntNode_(INodeMap& map, const char* nodeName)
	{
		try {
			CIntegerPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) { return -1; }
			return node->GetValue();
		}
		catch (const std::exception&) {
			return -1;
		}
	}

	double readFloatNode_(INodeMap& map, const char* nodeName)
	{
		try {
			CFloatPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) { return -1.0; }
			return node->GetValue();
		}
		catch (const std::exception&) {
			return -1.0;
		}
	}

	int readBoolNode_(INodeMap& map, const char* nodeName)
	{
		try {
			CBooleanPtr node = map.GetNode(nodeName);
			if (!IsAvailable(node) || !IsReadable(node)) { return -1; }
			return node->GetValue() ? 1 : 0;
		}
		catch (const std::exception&) {
			return -1;
		}
	}

	void lineName_(char* dst, int index)
	{
		snprintf(dst, LSO_CAMERA_NAME_MAX, "Line%d", index);
	}
}

bool wso_device::ColorCamera::applyExtendedSettings(const ColorCameraExtendedParam& params,
	ColorCameraApplyResult* result)
{
	if (result != nullptr) {
		result->structSize = (unsigned int)sizeof(ColorCameraApplyResult);
		result->version = LSO_CAMERA_CONFIG_VERSION;
		result->fieldCount = 0;
		result->failedCount = 0;
	}
	return applyExtendedInto_(params, result);
}

bool wso_device::ColorCamera::applyExtendedInto_(const ColorCameraExtendedParam& params,
	ColorCameraApplyResult* result)
{
	if (!isInitiated()) {
		return false;
	}
	if (params.structSize != (unsigned int)sizeof(ColorCameraExtendedParam)
		|| params.version != LSO_CAMERA_CONFIG_VERSION) {
		WsoLogError("[CAM-EXT] extended parameter ABI mismatch - nothing applied");
		return false;
	}

	bool ok = true;
	try {
		INodeMap& map = impl().getCamera()->GetNodeMap();

		// 1) Trigger off first. Everything structural below is locked while the camera is
		//    armed, and a half-written trigger configuration must never be live.
		//    Only done when this block actually states a trigger value: a preset that says
		//    nothing about the trigger must not disturb the mode the caller set up.
		const bool touchesTrigger = stated_(params.triggerMode) || stated_(params.triggerSource)
			|| stated_(params.triggerActivation) || stated_(params.triggerSelector)
			|| stated_(params.triggerOverlap) || params.triggerDelayUs >= 0.0;
		if (touchesTrigger) {
			try {
				CEnumerationPtr mode = map.GetNode("TriggerMode");
				if (IsAvailable(mode) && IsWritable(mode)) {
					CEnumEntryPtr off = mode->GetEntryByName("Off");
					if (IsAvailable(off) && IsReadable(off)) {
						mode->SetIntValue(off->GetValue());
					}
				}
			}
			catch (const std::exception& e) {
				WsoLogError("[CAM-EXT] pre-apply TriggerMode Off failed: " + std::string(e.what()));
			}
		}

		// 2) Sensor / acquisition group.
		ok &= rowOk_(applyEnumNode_(map, "ExposureAuto", params.exposureAuto, result));
		ok &= rowOk_(applyEnumNode_(map, "BinningSelector", params.binningSelector, result));
		ok &= rowOk_(applyEnumNode_(map, "BinningHorizontalMode", params.binningHorizontalMode, result));
		ok &= rowOk_(applyEnumNode_(map, "BinningVerticalMode", params.binningVerticalMode, result));
		ok &= rowOk_(applyEnumNode_(map, "SensorShutterMode", params.sensorShutterMode, result));
		ok &= rowOk_(applyBoolNode_(map, "IspEnable", params.ispEnable, result));
		ok &= rowOk_(applyEnumNode_(map, "TestPattern", params.testPattern, result));
		ok &= rowOk_(applyIntNode_(map, "AcquisitionBurstFrameCount", params.acquisitionBurstFrameCount, result));
		// The writable limit node, NOT the read-only DeviceLinkThroughput reading - the two
		// are easy to confuse and only one of them is a setting.
		ok &= rowOk_(applyIntNode_(map, "DeviceLinkThroughputLimit", params.deviceLinkThroughput, result));

		// 3) GPIO lines. Each row is selected, its source dropped to Off first (a line whose
		//    source is driving cannot change mode), then mode/source/inverter. The selector
		//    is restored afterwards, so editing one row never leaves another row's value
		//    behind the selector the operator sees.
		bool touchedLines = false;
		for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
			const ColorCameraLineConfig& line = params.lines[i];
			if (!stated_(line.mode) && !stated_(line.source) && line.inverter < 0) {
				continue;
			}
			touchedLines = true;

			char selector[LSO_CAMERA_NAME_MAX] = {};
			lineName_(selector, i);
			const int selectStatus = applyEnumNode_(map, "LineSelector", selector, result);
			ok &= rowOk_(selectStatus);
			if (selectStatus != CAM_FIELD_APPLIED) {
				continue;   // the row cannot be addressed; its fields are not claimed applied
			}

			if (stated_(line.mode)) {
				// Drop the source before the mode, exactly as the DevKit does: an output
				// that is currently driving refuses a mode change.
				try {
					CEnumerationPtr source = map.GetNode("LineSource");
					if (IsAvailable(source) && IsWritable(source)) {
						CEnumEntryPtr off = source->GetEntryByName("Off");
						if (IsAvailable(off) && IsReadable(off)) {
							source->SetIntValue(off->GetValue());
						}
					}
				}
				catch (const std::exception&) {
				}
			}
			ok &= rowOk_(applyEnumNode_(map, "LineMode", line.mode, result));
			ok &= rowOk_(applyEnumNode_(map, "LineSource", line.source, result));
			ok &= rowOk_(applyBoolNode_(map, "LineInverter", line.inverter, result));
		}
		if (stated_(params.lineSelector)) {
			ok &= rowOk_(applyEnumNode_(map, "LineSelector", params.lineSelector, result));
		}

		// 4) Trigger group, TriggerMode last: the camera must be fully configured before it
		//    is allowed to arm.
		ok &= rowOk_(applyEnumNode_(map, "TriggerSelector", params.triggerSelector, result));
		ok &= rowOk_(applyEnumNode_(map, "TriggerSource", params.triggerSource, result));
		ok &= rowOk_(applyEnumNode_(map, "TriggerActivation", params.triggerActivation, result));
		ok &= rowOk_(applyEnumNode_(map, "TriggerOverlap", params.triggerOverlap, result));

		ok &= rowOk_(applyEnumNode_(map, "TriggerMode", params.triggerMode, result));

		// TriggerDelay LAST, after the mode is armed. The path that worked before A-1 wrote
		// it exactly there - startTriggeredLiveMode() ran it after setupHwTriggerSetting()
		// had installed and enabled the trigger - and on this family the node is only
		// writable once the trigger selector and mode are set. Writing it while the trigger
		// was still Off (which is where this used to sit) makes the node report itself
		// locked, and every hardware-triggered capture would then fail at apply time.
		double previousDelay = 0.0;
		const int delayStatus = applyFloatNode_(map, "TriggerDelay", params.triggerDelayUs, result, &previousDelay);
		ok &= rowOk_(delayStatus);
		if (delayStatus != CAM_FIELD_NOT_REQUESTED && !impl().hasSavedTriggerDelay) {
			// Same ledger startTriggeredLiveMode() uses: the value is remembered once, so
			// the teardown puts back what was there before anything overwrote it.
			impl().savedTriggerDelayUs = previousDelay;
			impl().hasSavedTriggerDelay = true;
		}

		// A stated trigger or GPIO configuration is device state this class installed, so
		// the teardown has to put the strobe line back exactly as it does for the hardcoded
		// HW-trigger setup.
		if (touchesTrigger || touchedLines) {
			impl().hwTriggerInstalled = true;
		}
	}
	catch (const std::exception& e) {
		WsoLogError("[CAM-EXT] extended apply failed: " + std::string(e.what()));
		return false;
	}
	catch (...) {
		WsoLogError("[CAM-EXT] extended apply failed (unknown exception)");
		return false;
	}

	if (!ok && result != nullptr) {
		for (int i = 0; i < result->fieldCount; ++i) {
			const ColorCameraFieldResult& row = result->fields[i];
			if (row.status == CAM_FIELD_APPLIED || row.status == CAM_FIELD_NOT_REQUESTED) {
				continue;
			}
			WsoLogError("[CAM-EXT] " + fieldText_(row.node) + " req=" + fieldText_(row.requested)
				+ " actual=" + fieldText_(row.actual) + " status=" + std::to_string(row.status));
		}
	}
	return ok;
}

bool wso_device::ColorCamera::readExtendedSettings(ColorCameraExtendedParam* out, bool includeLines)
{
	if (out == nullptr) {
		return false;
	}

	*out = ColorCameraExtendedParam{};
	out->structSize = (unsigned int)sizeof(ColorCameraExtendedParam);
	out->version = LSO_CAMERA_CONFIG_VERSION;
	out->acquisitionBurstFrameCount = -1;
	out->deviceLinkThroughput = -1;
	out->triggerDelayUs = -1.0;
	out->ispEnable = -1;
	for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
		out->lines[i].inverter = -1;
	}

	if (!isInitiated()) {
		return false;
	}

	try {
		INodeMap& map = impl().getCamera()->GetNodeMap();

		readEnumNode_(map, "ExposureAuto", out->exposureAuto);
		readEnumNode_(map, "BinningSelector", out->binningSelector);
		readEnumNode_(map, "BinningHorizontalMode", out->binningHorizontalMode);
		readEnumNode_(map, "BinningVerticalMode", out->binningVerticalMode);
		readEnumNode_(map, "TestPattern", out->testPattern);
		readEnumNode_(map, "SensorShutterMode", out->sensorShutterMode);
		readEnumNode_(map, "TriggerMode", out->triggerMode);
		readEnumNode_(map, "TriggerSource", out->triggerSource);
		readEnumNode_(map, "TriggerActivation", out->triggerActivation);
		readEnumNode_(map, "TriggerSelector", out->triggerSelector);
		readEnumNode_(map, "TriggerOverlap", out->triggerOverlap);
		out->acquisitionBurstFrameCount = readIntNode_(map, "AcquisitionBurstFrameCount");
		out->deviceLinkThroughput = readIntNode_(map, "DeviceLinkThroughputLimit");
		out->triggerDelayUs = readFloatNode_(map, "TriggerDelay");
		out->ispEnable = readBoolNode_(map, "IspEnable");

		if (!includeLines) {
			// The caller does not need the GPIO rows, so the selector is left exactly where
			// the operator put it. Reading a row means WRITING the selector, and that is
			// device state - not something a read should spend on a hot path that will not
			// use the answer.
			readEnumNode_(map, "LineSelector", out->lineSelector);
			return true;
		}

		// Reading the four rows moves the selector, so the operator's selection is put back
		// before returning. If it cannot be read there is nothing to put back, and walking
		// the rows anyway would leave the camera selecting whichever line this loop ended
		// on - so the rows are left unstated instead, which is the honest answer.
		char selectorBefore[LSO_CAMERA_NAME_MAX] = {};
		readEnumNode_(map, "LineSelector", selectorBefore);
		if (!stated_(selectorBefore)) {
			WsoLogInfo("[CAM-EXT] LineSelector is unreadable - the GPIO rows are reported "
				"unstated rather than moving a selector this read cannot restore");
			return true;
		}

		for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
			char selector[LSO_CAMERA_NAME_MAX] = {};
			lineName_(selector, i);
			if (applyEnumNode_(map, "LineSelector", selector, nullptr) != CAM_FIELD_APPLIED) {
				continue;
			}
			readEnumNode_(map, "LineMode", out->lines[i].mode);
			readEnumNode_(map, "LineSource", out->lines[i].source);
			out->lines[i].inverter = readBoolNode_(map, "LineInverter");
		}

		applyEnumNode_(map, "LineSelector", selectorBefore, nullptr);
		copyName_(out->lineSelector, selectorBefore);
		return true;
	}
	catch (const std::exception& e) {
		WsoLogError("[CAM-EXT] extended read-back failed: " + std::string(e.what()));
		return false;
	}
	catch (...) {
		return false;
	}
}

bool wso_device::ColorCamera::applyFullSettings(const ColorCameraSettingParam& base,
	const ColorCameraExtendedParam& ext, ColorCameraApplyResult* result, bool* baseVerified)
{
	if (baseVerified != nullptr) { *baseVerified = false; }
	if (result != nullptr) {
		result->structSize = (unsigned int)sizeof(ColorCameraApplyResult);
		result->version = LSO_CAMERA_CONFIG_VERSION;
		result->fieldCount = 0;
		result->failedCount = 0;
	}

	if (!isInitiated()) {
		return false;
	}

	// Configuration nodes are locked while the camera streams; ending the acquisition here
	// is what makes the whole contract writable in one pass instead of silently skipping
	// half of it.
	try {
		if (impl().getCamera()->IsStreaming()) {
			impl().getCamera()->EndAcquisition();
		}
	}
	catch (const std::exception& e) {
		WsoLogError("[CAM-EXT] could not end the acquisition before applying: " + std::string(e.what()));
		return false;
	}

	// The extended block runs in two halves around the 14 structural fields, because the
	// binning selector and its modes decide what ROI ranges the structural write is then
	// validated against, while the trigger and GPIO nodes must land after it.
	ColorCameraExtendedParam sensorPhase = ext;
	ColorCameraExtendedParam devicePhase = ext;

	// First half: binning selector/modes and the exposure-auto decision only.
	sensorPhase.testPattern[0] = '\0';
	sensorPhase.sensorShutterMode[0] = '\0';
	sensorPhase.ispEnable = -1;
	sensorPhase.acquisitionBurstFrameCount = -1;
	sensorPhase.deviceLinkThroughput = -1;
	sensorPhase.triggerMode[0] = '\0';
	sensorPhase.triggerSource[0] = '\0';
	sensorPhase.triggerActivation[0] = '\0';
	sensorPhase.triggerSelector[0] = '\0';
	sensorPhase.triggerOverlap[0] = '\0';
	sensorPhase.triggerDelayUs = -1.0;
	sensorPhase.lineSelector[0] = '\0';
	for (int i = 0; i < LSO_CAMERA_LINE_COUNT; ++i) {
		sensorPhase.lines[i] = ColorCameraLineConfig{};
		sensorPhase.lines[i].inverter = -1;
	}

	// Second half: everything the first half left out.
	devicePhase.exposureAuto[0] = '\0';
	devicePhase.binningSelector[0] = '\0';
	devicePhase.binningHorizontalMode[0] = '\0';
	devicePhase.binningVerticalMode[0] = '\0';

	auto applyCheckedFloat = [&](const char* nodeName, double value) {
		try {
			INodeMap& map = impl().getCamera()->GetNodeMap();
			const bool automatic = std::strcmp(nodeName, "ExposureTime") == 0
				&& impl().getCamera()->ExposureAuto.GetValue() != ExposureAuto_Off;
			return detail::applyManualCameraFloat(automatic, [&] {
				return rowOk_(applyFloatNode_(map, nodeName, value, result, nullptr));
			});
		}
		catch (const std::exception& e) {
			WsoLogError(std::string("[CAM-EXT] final ") + nodeName + " verify failed: " + e.what());
			return false;
		}
		catch (...) {
			WsoLogError(std::string("[CAM-EXT] final ") + nodeName + " verify failed");
			return false;
		}
	};

	const detail::ColorCameraFullApplyVerdict verdict = detail::runColorCameraFullApply(
		[&] { return applyExtendedInto_(sensorPhase, result); },
		[&] {
			// The 14 structural/acquisition fields, with their own read-back verification.
			const bool verified = setCameraSettingsChecked_(base, false);

			// A single row for the structural group, so the caller's report covers all of it.
			if (result != nullptr && result->fieldCount < LSO_CAMERA_FIELD_MAX) {
				ColorCameraFieldResult& row = result->fields[result->fieldCount++];
				copyName_(row.node, "PixelFormat/Binning/ROI");
				copyNumber_(row.requested, (double)base.roi_x_width);
				copyNumber_(row.actual, (double)getROI_X_Width());
				row.status = verified ? CAM_FIELD_APPLIED : CAM_FIELD_MISMATCH;
				if (!verified) { ++result->failedCount; }
			}
			return verified;
		},
		[&] { return applyExtendedInto_(devicePhase, result); },
		[&] { return applyCheckedFloat("ExposureTime", (double)base.exposure_time); },
		[&] { return applyCheckedFloat("Gain", base.gain < 0.0f ? 0.0 : (double)base.gain); });
	const bool ok = verdict.extendedVerified;
	const bool baseOk = verdict.baseVerified;
	if (baseVerified != nullptr) { *baseVerified = baseOk; }

	WsoLogInfo("[CAM-EXT] full apply extendedOk=" + std::to_string(ok ? 1 : 0)
		+ " baseVerified=" + std::to_string(baseOk ? 1 : 0)
		+ " fields=" + std::to_string(result ? result->fieldCount : 0)
		+ " failed=" + std::to_string(result ? result->failedCount : 0));
	return ok;
}

int wso_device::ColorCamera::enumerateNodeEntries(const char* nodeName,
	ColorCameraEnumValue* buffer, int maxCount)
{
	if (!isInitiated() || nodeName == nullptr || buffer == nullptr || maxCount <= 0) {
		return 0;
	}

	try {
		INodeMap& map = impl().getCamera()->GetNodeMap();
		CEnumerationPtr node = map.GetNode(nodeName);
		if (!IsAvailable(node) || !IsReadable(node)) {
			return 0;
		}

		NodeList_t entries;
		node->GetEntries(entries);

		int written = 0;
		for (size_t i = 0; i < entries.size() && written < maxCount; ++i) {
			CEnumEntryPtr entry = entries[i];
			if (!IsAvailable(entry) || !IsReadable(entry)) {
				continue;   // implemented by the SDK but not by this model / not in this state
			}
			copyName_(buffer[written].name, entry->GetSymbolic().c_str());
			++written;
		}
		return written;
	}
	catch (const std::exception& e) {
		WsoLogError(std::string("[CAM-EXT] entry enumeration failed for ") + nodeName + ": " + e.what());
		return 0;
	}
	catch (...) {
		return 0;
	}
}

bool wso_device::ColorCamera::queryNodeRange(const char* nodeName, double* minValue,
	double* maxValue, double* increment, bool* writable)
{
	if (minValue) { *minValue = 0.0; }
	if (maxValue) { *maxValue = 0.0; }
	if (increment) { *increment = 0.0; }
	if (writable) { *writable = false; }

	if (!isInitiated() || nodeName == nullptr) {
		return false;
	}

	try {
		INodeMap& map = impl().getCamera()->GetNodeMap();
		// Kept as the raw INode*: a typed CPointer is constructed from that, not from
		// another CPointer, which the GenApi smart pointers do not convert between.
		INode* raw = map.GetNode(nodeName);
		CNodePtr node = raw;
		if (!IsAvailable(node) || !IsReadable(node)) {
			return false;
		}

		switch (node->GetPrincipalInterfaceType()) {
		case intfIInteger: {
			CIntegerPtr value = raw;
			if (minValue) { *minValue = (double)value->GetMin(); }
			if (maxValue) { *maxValue = (double)value->GetMax(); }
			if (increment) { *increment = (double)value->GetInc(); }
			if (writable) { *writable = IsWritable(value); }
			return true;
		}
		case intfIFloat: {
			CFloatPtr value = raw;
			if (minValue) { *minValue = value->GetMin(); }
			if (maxValue) { *maxValue = value->GetMax(); }
			if (increment) { *increment = value->HasInc() ? value->GetInc() : 0.0; }
			if (writable) { *writable = IsWritable(value); }
			return true;
		}
		default:
			return false;
		}
	}
	catch (const std::exception& e) {
		WsoLogError(std::string("[CAM-EXT] range query failed for ") + nodeName + ": " + e.what());
		return false;
	}
	catch (...) {
		return false;
	}
}

bool wso_device::ColorCamera::queryNodeValue(const char* nodeName, double* value)
{
	if (value) { *value = 0.0; }

	if (!isInitiated() || nodeName == nullptr || value == nullptr) {
		return false;
	}

	try {
		INodeMap& map = impl().getCamera()->GetNodeMap();
		INode* raw = map.GetNode(nodeName);
		CNodePtr node = raw;
		if (!IsAvailable(node) || !IsReadable(node)) {
			return false;
		}

		switch (node->GetPrincipalInterfaceType()) {
		case intfIInteger: {
			CIntegerPtr typed = raw;
			*value = (double)typed->GetValue();
			return true;
		}
		case intfIFloat: {
			CFloatPtr typed = raw;
			*value = typed->GetValue();
			return true;
		}
		default:
			return false;
		}
	}
	catch (const std::exception& e) {
		WsoLogError(std::string("[CAM-EXT] value query failed for ") + nodeName + ": " + e.what());
		return false;
	}
	catch (...) {
		return false;
	}
}

bool wso_device::ColorCamera::squareRoi(void)
{
	try
	{
		if (impl().getCamera()->IsStreaming())
		{
			impl().getCamera()->EndAcquisition();
		}

		// Read the currently applied ROI (already binned / format-set by setCameraSettings).
		int w = (int)impl().getCamera()->Width.GetValue();
		int h = (int)impl().getCamera()->Height.GetValue();
		int ox = (int)impl().getCamera()->OffsetX.GetValue();
		int oy = (int)impl().getCamera()->OffsetY.GetValue();

		int side = (w < h) ? w : h;

		// The square side must satisfy both Width and Height increments.
		int winc = (int)impl().getCamera()->Width.GetInc();
		int hinc = (int)impl().getCamera()->Height.GetInc();
		int sideStep = (winc > hinc) ? winc : hinc;
		if (sideStep < 1) { sideStep = 1; }
		side -= (side % sideStep);
		if (side <= 0) { return false; }

		// Center the square inside the current window; align to offset increments.
		int oxInc = (int)impl().getCamera()->OffsetX.GetInc();
		int oyInc = (int)impl().getCamera()->OffsetY.GetInc();
		if (oxInc < 1) { oxInc = 1; }
		if (oyInc < 1) { oyInc = 1; }
		int newOx = ox + (w - side) / 2; newOx -= (newOx % oxInc);
		int newOy = oy + (h - side) / 2; newOy -= (newOy % oyInc);

		// Shrink size first, then move the offset (offset range grows once size shrinks).
		setROI_X_Width(side);
		setROI_Y_Height(side);
		setROI_X_Offset(newOx);
		setROI_Y_Offset(newOy);

		WsoLogInfo("[SQUARE] " + std::to_string(w) + "x" + std::to_string(h)
			+ " -> " + std::to_string(side) + "x" + std::to_string(side)
			+ " off(" + std::to_string(newOx) + "," + std::to_string(newOy) + ")");
		return true;
	}
	catch (const std::exception& e)
	{
		WsoLogError("Exception occurred during ColorCamera squareRoi!");
		WsoLogError(e.what());
		return false;
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
	auto& node = impl().getCamera()->Gain;
	// A GenICam float node rejects a value one epsilon past its live limit and that throws
	// away the WHOLE write: a preset holding 27.046 dB for a camera whose Gain.Max is
	// 27.045664 left the gain at whatever the previous session applied - a dark preview whose
	// only symptom was one [SET] error line. Clamp to the live range and say so instead.
	const double lo = node.GetMin();
	const double hi = node.GetMax();
	const double req = (double)val;
	const double out = (req < lo ? lo : (req > hi ? hi : req));
	if (out != req) {
		WsoLogInfo("[SET] Gain clamped to the camera range: requested " + std::to_string(req)
			+ ", min " + std::to_string(lo) + ", max " + std::to_string(hi)
			+ " -> applied " + std::to_string(out));
	}
	node.SetValue(out);
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

bool wso_device::ColorCamera::recoverCameraAfterFault(void)
{
	// Called from init retry catch handlers, below the C ABI boundary ? must never
	// throw. Resets the (possibly locked/streaming) device and waits for it to
	// re-enumerate so the caller can retry with a valid, writable handle.
	try {
		if (impl().pCam && impl().pCam->IsValid()) {
			try { if (impl().pCam->IsStreaming()) impl().pCam->EndAcquisition(); }
			catch (...) {}
			try { impl().pCam->DeviceReset.Execute(); }
			catch (...) {}
		}

		// The old handle is invalid after a device reset; drop it before re-enumerating.
		impl().pCam = nullptr;
		impl().camList.Clear();

		if (!impl().systemPtr) {
			impl().systemPtr = System::GetInstance();
		}

		// USB3 re-enumeration after DeviceReset can take several seconds. Poll with a
		// bounded timeout instead of a tight/infinite spin. UpdateCameras() forces a
		// fresh enumeration; GetCameras() alone would return the stale cached list.
		const int kTimeoutMs = 15000;
		const int kPollMs = 200;
		unsigned int numCameras = 0;
		for (int waited = 0; waited < kTimeoutMs; waited += kPollMs) {
			std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
			impl().systemPtr->UpdateCameras();
			impl().camList = impl().systemPtr->GetCameras();
			numCameras = impl().camList.GetSize();
			if (numCameras > 0) {
				break;
			}
		}

		if (numCameras == 0) {
			WsoLogError("Color camera did not re-enumerate after device reset");
			return false;
		}

		impl().pCam = impl().camList.GetByIndex(0);
		impl().pCam->Init();
		return true;
	}
	catch (const std::exception& e) {
		WsoLogError("Color camera fault recovery failed: " + std::string(e.what()));
		return false;
	}
	catch (...) {
		WsoLogError("Color camera fault recovery failed: unknown exception");
		return false;
	}
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

		// Read once. ReverseX/ReverseY are configuration nodes and are locked while the
		// camera streams, so re-reading them per frame only adds two USB register reads
		// inside the callback lock - which is exactly what a UI-thread camera access then
		// waits on, for every frame.
		const unsigned int flipMode = getFlipMode();

		impl().getCamera()->BeginAcquisition();

		do {
			frameCount++;

			impl().rawImage = impl().getCamera()->GetNextImage();
			std::memcpy(buffer, impl().rawImage->GetData(), frameSize);

			unique_lock<mutex> lock(impl().mutexLock);
			CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, flipMode, nPixelFormat, nBytesPerPixel);

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

// Hardware-triggered aiming live worker. Entered with the acquisition already running and
// the camera armed (startTriggeredLiveMode did both), so this only pumps frames until
// pauseLiveMode() clears liveMode.
// The per-image wait is BOUNDED: under a hardware trigger "no image yet" is the normal
// state between board sweeps, so a blocking GetNextImage() would make pauseLiveMode() -
// which joins this thread - hang until a trigger happened to arrive, i.e. forever if the
// board was never started or has already been stopped.
void wso_device::ColorCamera::acquireCameraTriggeredLiveData(void)
{
	try {
		const uint32_t width = getFrameWidth();
		const uint32_t height = getFrameHeight();
		const int nPixelFormat = impl().getCamera()->PixelFormat.GetValue();
		const int nBytesPerPixel = getBytesPerPixel();
		const uint32_t frameSize = getFrameSizeForBpp(nBytesPerPixel);
		// Read once, for the same reason the free-running loop does: a per-frame node read
		// inside the callback lock is what a UI-thread camera call ends up queueing behind.
		const unsigned int flipMode = getFlipMode();
		unsigned char* buffer = impl().sonyFrameBuffer;

		// Same budget rule the HW-trigger capture loop uses: one exposure plus a margin.
		const unsigned int imageTimeoutMs = getExposureTime() / 1000U + 200U;

		WsoLogInfo("[COLOR-PRV] triggered live begin pf=" + std::to_string(nPixelFormat)
			+ " w=" + std::to_string(width) + " h=" + std::to_string(height)
			+ " waitMs=" + std::to_string(imageTimeoutMs));

		uint32_t frameCount = 0;
		while (impl().liveMode) {
			try {
				impl().rawImage = impl().getCamera()->GetNextImage(imageTimeoutMs);
			}
			catch (const Spinnaker::Exception&) {
				// No trigger within the budget: re-test liveMode and wait again. This is
				// the normal idle path, so it must not be logged per iteration.
				continue;
			}

			try {
				std::memcpy(buffer, impl().rawImage->GetData(), frameSize);
				++frameCount;
				unique_lock<mutex> lock(impl().mutexLock);
				CallbackRegistry::getInstance()->runColorCameraFrameCaptured(buffer, width, height, frameCount, flipMode, nPixelFormat, nBytesPerPixel);
			}
			catch (...) {
				impl().rawImage->Release();
				throw;
			}
			impl().rawImage->Release();
		}

		WsoLogInfo("[COLOR-PRV] triggered live end frames=" + std::to_string(frameCount));
	}
	catch (const std::exception& e) {
		WsoLogError("[COLOR-PRV] triggered live exception: " + std::string(e.what()));
	}
	catch (...) {
		WsoLogError("[COLOR-PRV] triggered live unknown exception");
	}

	// Ends the acquisition and puts back exactly what the trigger setup installed
	// (TriggerMode, the Line2 output source and the FrameStart trigger delay).
	abortCaptureFrames();
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

		// Width, Height ????
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);

		//SetEventMode(true);
		// Software Trigger ????
		setupSoftwareTrigger();

		impl().getCamera()->BeginAcquisition();

		do {

			impl().nOffsetFrameCount = 0;

			auto t0 = steady_clock::now();

			for (int i = 0; i < totalFrameCount; ++i)
			{
				// liveMode u??? ??????? ???
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

		// Width, Height ????
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);

		//SetEventMode(true);
		// Software Trigger ????
		setupSoftwareTrigger(2); // Multiframe
		setAcquisitionFrameCount(totalFrameCount);

		impl().getCamera()->BeginAcquisition();

		auto t0 = steady_clock::now();

		for (int i = 0; i < totalFrameCount; ++i)
		{
			// liveMode u??? ??????? ???
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

		// Width, Height ????
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nMaxWidth);
		setROI_Y_Height(nMaxHeight);

		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);
		setROI_X_Offset(nOffsetX);
		setROI_Y_Offset(nOffsetY);

		//SetEventMode(true);
		// Software Trigger ????
		setupSoftwareTrigger();

		impl().getCamera()->BeginAcquisition();

		do {
			//auto t0 = steady_clock::now();

			for (int i = 0; i < totalFrameCount; ++i)
			{
				// liveMode u??? ??????? ???
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

		// Width, Height ????
		int nMaxWidth = getROI_Max_Width();
		int nMaxHeight = getROI_Max_Height();

		// Width, Height ????
		setROI_X_Offset(0);
		setROI_Y_Offset(0);
		setROI_X_Width(nMaxWidth);
		setROI_Y_Height(nMaxHeight);

		setROI_X_Width(nRoiWidth);
		setROI_Y_Height(nRoiHeight);
		setROI_X_Offset(nOffsetX);
		setROI_Y_Offset(nOffsetY);

		// Software Trigger ????
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

			// ?????????? ??????? ???? ???
			std::memcpy(buffer, impl().rawImage->GetData(), frameSize);
		}
		catch (const Spinnaker::Exception& e)
		{
			// ????? ??? ??? ???? ??? ??
			LogD() << "Failed to get image at frame " << frameCount << ": " << e.what();

			// buffer?? a???? ??? ????? ????
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

			// Set i?? ???? ?????? ????
			//impl().getCamera()->ExposureTime.SetValue(2000);  // 20ms ???? ????
			//impl().getCamera()->Gain.SetValue(2.0);            // ???? ????
			impl().getCamera()->OffsetX.SetValue(0);         
			impl().getCamera()->OffsetY.SetValue(0);

			impl().getCamera()->Width.SetValue(rois[i].width);          // ROI ??
			impl().getCamera()->Height.SetValue(rois[i].height);         // ROI ????
			impl().getCamera()->OffsetX.SetValue(rois[i].offsetX);         // X ??????
			impl().getCamera()->OffsetY.SetValue(rois[i].offsetY);         // Y ??????

			// Set 0?? Sequencer ????? ????
			impl().getCamera()->SequencerTriggerSource.SetValue(SequencerTriggerSource_FrameStart);
			impl().getCamera()->SequencerTriggerActivation.SetValue(SequencerTriggerActivation_RisingEdge);

			
			// Set Next Sequence Set 
			++nIndexNextSet;
			if (nIndexNextSet >= nSeqSetCount) nIndexNextSet = 0;

			impl().getCamera()->SequencerSetNext.SetValue(nIndexNextSet);

			// Set i ????
			impl().getCamera()->SequencerSetSave.Execute();

			LogD() << "Sequencer Set " << i << " configured and saved";
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		// 6. ???? ??? ????
		impl().getCamera()->SequencerSetStart.SetValue(0);

		// 7. Sequencer Configuration ??? Off?? ???? (???? ???)
		impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);

		// 8. Sequencer ???? ????? ???
		if (impl().getCamera()->SequencerConfigurationValid.GetValue() == SequencerConfigurationValid_Yes) {
			LogD() << "Sequencer configuration is valid";

			// 9. Sequencer ??? ????
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

		// ???? ??? ?? Sequencer ??????
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

			// Set i?? ???? ?????? ????
			//impl().getCamera()->ExposureTime.SetValue(2000);  // 20ms ???? ????
			//impl().getCamera()->Gain.SetValue(2.0);            // ???? ????
			impl().getCamera()->OffsetX.SetValue(0);
			impl().getCamera()->OffsetY.SetValue(0);

			impl().getCamera()->Width.SetValue(paramList[i].width);          // ROI ??
			impl().getCamera()->Height.SetValue(paramList[i].height);         // ROI ????
			impl().getCamera()->OffsetX.SetValue(paramList[i].offsetX);         // X ??????
			impl().getCamera()->OffsetY.SetValue(paramList[i].offsetY);         // Y ??????

			// Set 0?? Sequencer ????? ????
			impl().getCamera()->SequencerTriggerSource.SetValue(SequencerTriggerSource_FrameStart);
			impl().getCamera()->SequencerTriggerActivation.SetValue(SequencerTriggerActivation_RisingEdge);


			// Set Next Sequence Set 
			++nIndexNextSet;
			if (nIndexNextSet >= nSeqSetCount) nIndexNextSet = 0;

			impl().getCamera()->SequencerSetNext.SetValue(nIndexNextSet);

			// Set i ????
			impl().getCamera()->SequencerSetSave.Execute();

			LogD() << "Sequencer Set " << i << " configured and saved";
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		// 6. ???? ??? ????
		impl().getCamera()->SequencerSetStart.SetValue(0);

		// 7. Sequencer Configuration ??? Off?? ???? (???? ???)
		impl().getCamera()->SequencerConfigurationMode.SetValue(SequencerConfigurationMode_Off);

		// 8. Sequencer ???? ????? ???
		if (impl().getCamera()->SequencerConfigurationValid.GetValue() == SequencerConfigurationValid_Yes) {
			LogD() << "Sequencer configuration is valid";

			// 9. Sequencer ??? ????
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

		// ???? ??? ?? Sequencer ??????
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

		// ???? u??? ??? 8?? ??? ???? ??????? ???
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

	// [??? ????] ��o ?????? ???? ???? ????(????/binning/???) ??��?. ???? ?? ????.
	WsoLogInfo("[CAP] pf=" + std::to_string(nPixelFormat) + " (0=Mono8,4=BayerRG8) binH="
		+ std::to_string(getBinningHorizontal()) + " binV=" + std::to_string(getBinningVertical())
		+ " w=" + std::to_string(width) + " h=" + std::to_string(height)
		+ " bpp=" + std::to_string(nBytesPerPixel));

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
			// ExposureEnd ???? o??
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

				// ??? ???? (???? CallbackRegistry?? ???)
				//CallbackRegistry::getInstance()->runColorCameraExposureEndEvent(frameID, timestamp);
			}
			catch (const std::exception& e) {
				LogD() << "Error processing ExposureEnd event: " << std::string(e.what());
			}
		}
		//else if (strEventName == "EventError")
		//{
		//	// Error ???? o??
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
		//	// SerialPortReceive ???? o??
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
			// DeviceEventHandler ???? (???? ???????? ???? ???)
			if (!impl().m_deviceEventHandler) {
				impl().m_deviceEventHandler = std::make_unique<ColorCameraDeviceEventHandler>(this);
			}

			// ExposureEnd ???? ???? ?? ??? ???
			impl().getCamera()->EventSelector.SetValue(EventSelector_ExposureEnd);
			impl().getCamera()->EventNotification.SetValue(EventNotification_On);
			//impl().getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventExposureEnd");
			impl().getCamera()->RegisterEventHandler(*(impl().m_deviceEventHandler));
			LogD() << "ExposureEnd event enabled and handler registered";

			//// Error ???? ???? ?? ??? ???
			//impl().getCamera()->EventSelector.SetValue(EventSelector_Error);
			//impl().getCamera()->EventNotification.SetValue(EventNotification_On);
			//impl().getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventError");
			//WsoLogInfo("Error event enabled and handler registered");

			//// SerialPortReceive ???? ???? ?? ??? ??? (????? ???)
			//impl().getCamera()->EventSelector.SetValue(EventSelector_SerialPortReceive);
			//impl().getCamera()->EventNotification.SetValue(EventNotification_On);
			//impl().getCamera()->RegisterEventHandler(*m_deviceEventHandler, "EventSerialPortReceive");
			//WsoLogInfo("SerialPortReceive event enabled and handler registered");

			LogD() << "Event Control Mode activated for BFS-U3-122S6";
		}
		else
		{
			// EventHandler ??? ????
			if (impl().m_deviceEventHandler) {
				try {
					impl().getCamera()->UnregisterEventHandler(*(impl().m_deviceEventHandler));
					LogD() << "Event handlers unregistered";
				}
				catch (const std::exception& e) {
					LogD() << "Error unregistering event handlers: " << std::string(e.what());
				}
			}

			// ??? ???? ??????
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

