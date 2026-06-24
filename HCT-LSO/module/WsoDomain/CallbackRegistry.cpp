#include "pch.h"
#include "CallbackRegistry.h"
#include "DeviceDefines.h"


using namespace wso_domain;


std::mutex CallbackRegistry::singleMutex_;


struct CallbackRegistry::CallbackRegistryImpl
{
	optional<WsoLogMsgCallback> logMsgCallback;
	optional<FirmwareMsgCallback> firmwareMsgCallback;

	optional<JoystickButtonPressedCallback> joystickPressed;
	optional<OptimizeButtonPressedCallback> optimizePressed;
	optional<AdapterLensAttachedCallback> adapterLensAttached;

	unordered_map<MotorType, optional<StepMotorPositionChanged>> motorPosChanged; 
	
	optional<CorneaCameraFrameCaptured> corneaLeftFrameCaptured;
	optional<CorneaCameraFrameCaptured> corneaRightFrameCaptured;
	optional<CorneaCameraFrameCaptured> corneaLowerFrameCaptured;
	optional<RetinaCameraFrameCaptured> retinaFrameCaptured;

	optional<ColorCameraFrameCaptured> colorFrameCaptured;
	optional<ColorCameraSeqROIFrameCaptured> colorSeqROIFrameCaptured;
	optional<ColorCameraOffsetROIFrameCaptured> colorOffsetROIFrameCaptured;
	optional<ColorCameraRollSWTrigOverlapFrameCaptured> colorRollSWTrigOverlapFrameCaptured;
	optional<ColorCameraImageCaptured> colorImageCaptured;
	optional<ColorCameraFrameCaptured> colorSWTriggerFrameCaptured;
	
	optional<OctSpectrumDataCaptured> octSpectrumDataCaptured;
	optional<OctResampleDataCaptured> octResampledDataCaptured;
	optional<OctIntensityDataCaptured> octIntensityDataCaptured;
	optional<OctPreviewImageCaptured> octPreviewImageCaptured;
	optional<OctEnfaceImageCaptured> octEnfaceImageCaptured;


	optional<OctScanPatternAcquired> octScanPatternAcquired;
	optional<OctScanProtocolCompleted> octScanProtocolCompleted;

	optional<OctAutoFocusOptimized> octAutoFocusOptimized;
	optional<OctAutoReferOptimized> octAutoReferOptimized;
	optional<OctAutoPolarOptimized> octAutoPolarOptimized;
	optional<OctAutoScanOptimized> octAutoScanOptimized;


	CallbackRegistryImpl() {
		initializeCallbackRegistryImpl();
	}


	void initializeCallbackRegistryImpl(void) {
		logMsgCallback = nullopt;
		firmwareMsgCallback = nullopt;

		joystickPressed = nullopt;
		optimizePressed = nullopt;
		adapterLensAttached = nullopt;

		for (int i = 0; i < static_cast<int>(MotorType::NUM_MOTORS); i++) {
			motorPosChanged[static_cast<MotorType>(i)] = nullopt;
		}

		corneaLeftFrameCaptured = nullopt;
		corneaRightFrameCaptured = nullopt;
		corneaLowerFrameCaptured = nullopt;
		retinaFrameCaptured = nullopt;

		colorFrameCaptured = nullopt;
		colorSeqROIFrameCaptured = nullopt;
		colorOffsetROIFrameCaptured = nullopt;
		colorRollSWTrigOverlapFrameCaptured = nullopt;
		colorImageCaptured = nullopt;
		colorSWTriggerFrameCaptured = nullopt;

		octSpectrumDataCaptured = nullopt;
		octResampledDataCaptured = nullopt;
		octIntensityDataCaptured = nullopt;
		octPreviewImageCaptured = nullopt;
		octEnfaceImageCaptured = nullopt;

		octScanPatternAcquired = nullopt;
		octScanProtocolCompleted = nullopt;

		octAutoFocusOptimized = nullopt;
		octAutoReferOptimized = nullopt;
		octAutoPolarOptimized = nullopt;
		octAutoScanOptimized = nullopt;
	}
};



wso_domain::CallbackRegistry::CallbackRegistry() :
	d_ptr(std::make_unique<CallbackRegistryImpl>())
{
}


wso_domain::CallbackRegistry::~CallbackRegistry()
{
}


CallbackRegistry* wso_domain::CallbackRegistry::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static CallbackRegistry instance;
	return &instance;
}


void wso_domain::CallbackRegistry::setWsoLogMsgCallback(WsoLogMsgCallback clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().logMsgCallback = clb;
}

void wso_domain::CallbackRegistry::setFirmwareMsgCallback(FirmwareMsgCallback clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().firmwareMsgCallback = clb;
}

void wso_domain::CallbackRegistry::setJoystickButtonPressed(JoystickButtonPressedCallback clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().joystickPressed = clb;
}


void wso_domain::CallbackRegistry::setOptimizeButtonPressed(OptimizeButtonPressedCallback clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().optimizePressed = clb;
}

void wso_domain::CallbackRegistry::setAdapterLensAttached(AdapterLensAttachedCallback clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().adapterLensAttached = clb;
}

void wso_domain::CallbackRegistry::setStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().motorPosChanged[type] = clb;
}

void wso_domain::CallbackRegistry::setCorneaLeftCameraFrameCaptured(CorneaCameraFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().corneaLeftFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setCorneaRightCameraFrameCaptured(CorneaCameraFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().corneaRightFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setCorneaLowerCameraFrameCaptured(CorneaCameraFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().corneaLowerFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setRetinaCameraFrameCaptured(RetinaCameraFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().retinaFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setColorCameraFrameCaptured(ColorCameraFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().colorFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setColorCameraSeqROIFrameCaptured(ColorCameraSeqROIFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().colorSeqROIFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setColorCameraOffsetROIFrameCaptured(ColorCameraOffsetROIFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().colorOffsetROIFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setColorCameraRollSWTrigOverlapFrameCaptured(ColorCameraRollSWTrigOverlapFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().colorRollSWTrigOverlapFrameCaptured = clb;
}

void wso_domain::CallbackRegistry::setColorCameraImageCaptured(ColorCameraImageCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().colorImageCaptured = clb;
}

void wso_domain::CallbackRegistry::setColorCameraSWTriggerFrameCaptured(ColorCameraFrameCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().colorSWTriggerFrameCaptured = clb;
}


void wso_domain::CallbackRegistry::setOctSpectrumDataCaptured(OctSpectrumDataCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octSpectrumDataCaptured = clb;
}

void wso_domain::CallbackRegistry::setOctResampledDataCaptured(OctResampleDataCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octResampledDataCaptured = clb;
}

void wso_domain::CallbackRegistry::setOctIntensityDataCaptured(OctIntensityDataCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octIntensityDataCaptured = clb;
}

void wso_domain::CallbackRegistry::setOctPreviewImageCaptured(OctPreviewImageCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octPreviewImageCaptured = clb;
}

void wso_domain::CallbackRegistry::setOctEnfaceImageCaptured(OctEnfaceImageCaptured clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octEnfaceImageCaptured = clb;
}

void wso_domain::CallbackRegistry::setOctScanPatternAcquired(OctScanPatternAcquired clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octScanPatternAcquired = clb;
}

void wso_domain::CallbackRegistry::setOctScanProtocolCompleted(OctScanProtocolCompleted clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octScanProtocolCompleted = clb;
}

void wso_domain::CallbackRegistry::setOctAutoFocusOptimized(OctAutoFocusOptimized clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octAutoFocusOptimized = clb;
}

void wso_domain::CallbackRegistry::setOctAutoReferOptimized(OctAutoReferOptimized clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octAutoReferOptimized = clb;
}

void wso_domain::CallbackRegistry::setOctAutoPolarOptimized(OctAutoPolarOptimized clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octAutoPolarOptimized = clb;
}

void wso_domain::CallbackRegistry::setOctAutoScanOptimized(OctAutoScanOptimized clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octAutoScanOptimized = clb;
}


void wso_domain::CallbackRegistry::runWsoLogMsgCallback(string msg, int type)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().logMsgCallback) {
		if (auto func = *impl().logMsgCallback; func) {
			auto wmsg = atow(msg);
			func(wmsg.c_str(), type);
		}
	}
}

void wso_domain::CallbackRegistry::runFirmwareMsgCallback(std::string strMessage, int nCurrentState, int nPercent, bool bReset, bool bError)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().firmwareMsgCallback) {
		if (auto func = *impl().firmwareMsgCallback; func) {
			auto wmsg = atow(strMessage);
			func(wmsg.c_str(), nCurrentState, nPercent, bReset, bError);
		}
	}
}

void wso_domain::CallbackRegistry::runJoystickButtonPressed(void)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().joystickPressed) {
		if (auto func = *impl().joystickPressed; func) {
			func();
		}
	}
}

void wso_domain::CallbackRegistry::runOptimizeButtonPressed(void)
{
	lock_guard<mutex> lock(singleMutex_);
	// Contextual conversion to bool (as has_value() to check)
	// and extract optional value with the * operator (as value())
	if (impl().optimizePressed) {
		if (auto func = *impl().optimizePressed; func) {
			func();
		}
	}
}

void wso_domain::CallbackRegistry::runAdapterLensAttached(int type)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().adapterLensAttached) {
		if (auto func = *impl().adapterLensAttached; func) {
			func(type);
		}
	}
}

void wso_domain::CallbackRegistry::runStepMotorPositionChanged(MotorType type, int pos, float value)
{
	lock_guard<mutex> lock(singleMutex_);
	auto p = impl().motorPosChanged.find(type);
	if (p != impl().motorPosChanged.end() && p->second) {
		if (auto func = *p->second; func) {
			func(pos, value);
		}
	}
}

void wso_domain::CallbackRegistry::runCorneaLeftCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().corneaLeftFrameCaptured) {
		if (auto func = *impl().corneaLeftFrameCaptured; func) {
			func(bits, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runCorneaRightCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().corneaRightFrameCaptured) {
		if (auto func = *impl().corneaRightFrameCaptured; func) {
			func(bits, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runCorneaLowerCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().corneaLowerFrameCaptured) {
		if (auto func = *impl().corneaLowerFrameCaptured; func) {
			func(bits, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runRetinaCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().retinaFrameCaptured) {
		if (auto func = *impl().retinaFrameCaptured; func) {
			func(bits, width, height);
		}
	}
}


void wso_domain::CallbackRegistry::runColorCameraFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().colorFrameCaptured) {
		if (auto func = *impl().colorFrameCaptured; func) {
			func(bits, width, height, frameCount, flipMode, nPixelFormat, nBytesPerPixel);
		}
	}
}

void wso_domain::CallbackRegistry::runColorCameraSeqROIFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t offsetX, int32_t offsetY, int32_t frameCount, int32_t totalframeCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().colorSeqROIFrameCaptured) {
		if (auto func = *impl().colorSeqROIFrameCaptured; func) {
			func(bits, width, height, offsetX, offsetY, frameCount, totalframeCount, flipMode, nPixelFormat, nBytesPerPixel);
		}
	}
}

void wso_domain::CallbackRegistry::runColorCameraOffsetROIFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t offsetX, int32_t offsetY, int32_t frameCount, int32_t totalframeCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().colorOffsetROIFrameCaptured) {
		if (auto func = *impl().colorOffsetROIFrameCaptured; func) {
			func(bits, width, height, offsetX, offsetY, frameCount, totalframeCount, flipMode, nPixelFormat, nBytesPerPixel);
		}
	}
}

void wso_domain::CallbackRegistry::runColorCameraRollSWTrigOverlapFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t totalframeCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().colorRollSWTrigOverlapFrameCaptured) {
		if (auto func = *impl().colorRollSWTrigOverlapFrameCaptured; func) {
			func(bits, width, height, frameCount, totalframeCount, flipMode, nPixelFormat, nBytesPerPixel);
		}
	}
}

void wso_domain::CallbackRegistry::runColorCameraImageCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t totalFrameCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().colorImageCaptured) {
		if (auto func = *impl().colorImageCaptured; func) {
			func(bits, width, height, frameCount, totalFrameCount, flipMode, nPixelFormat, nBytesPerPixel);
		}
	}
}

void wso_domain::CallbackRegistry::runColorCameraSWTriggerFrameCaptured(uint8_t* bits, int32_t width, int32_t height, int32_t frameCount, int32_t flipMode, int32_t nPixelFormat, int32_t nBytesPerPixel)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().colorSWTriggerFrameCaptured) {
		if (auto func = *impl().colorSWTriggerFrameCaptured; func) {
			func(bits, width, height, frameCount, flipMode, nPixelFormat, nBytesPerPixel);
		}
	}
}

void wso_domain::CallbackRegistry::runOctSpectrumDataCaptured(uint16_t* data, int width, int height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octSpectrumDataCaptured) {
		if (auto func = *impl().octSpectrumDataCaptured; func) {
			func(data, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runOctResampledDataCaptured(float* data, int width, int height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octResampledDataCaptured) {
		if (auto func = *impl().octResampledDataCaptured; func) {
			func(data, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runOctIntensityDataCaptured(float* data, int width, int height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octIntensityDataCaptured) {
		if (auto func = *impl().octIntensityDataCaptured; func) {
			func(data, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runOctPreviewImageCaptured(uint8_t* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octPreviewImageCaptured) {
		if (auto func = *impl().octPreviewImageCaptured; func) {
			func(bits, width, height, qindex, sn_ratio, ref_point, image_index);
		}
	}
}

void wso_domain::CallbackRegistry::runOctEnfaceImageCaptured(unsigned char* bits, int width, int height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octPreviewImageCaptured) {
		if (auto func = *impl().octEnfaceImageCaptured; func) {
			func(bits, width, height);
		}
	}
}

void wso_domain::CallbackRegistry::runOctScanPatternAcquired(bool result)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octScanPatternAcquired) {
		if (auto func = *impl().octScanPatternAcquired; func) {
			func(result);
		}
	}
}

void wso_domain::CallbackRegistry::runOctScanProtocolCompleted(bool result)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octScanProtocolCompleted) {
		if (auto func = *impl().octScanProtocolCompleted; func) {
			func(result);
		}
	}
}

void wso_domain::CallbackRegistry::runOctAutoFocusOptimized(bool result, float quality, float diopter)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octAutoFocusOptimized) {
		if (auto func = *impl().octAutoFocusOptimized; func) {
			func(result, quality, diopter);
		}
	}
}

void wso_domain::CallbackRegistry::runOctAutoReferOptimized(bool result, float quality, int ref_point, int position)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octAutoReferOptimized) {
		if (auto func = *impl().octAutoReferOptimized; func) {
			func(result, quality, ref_point, position);
		}
	}
}

void wso_domain::CallbackRegistry::runOctAutoPolarOptimized(bool result, float quality, float degree)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octAutoPolarOptimized) {
		if (auto func = *impl().octAutoPolarOptimized; func) {
			func(result, quality, degree);
		}
	}
}

void wso_domain::CallbackRegistry::runOctAutoScanOptimized(bool result)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octAutoScanOptimized) {
		if (auto func = *impl().octAutoScanOptimized; func) {
			func(result);
		}
	}
}

void wso_domain::CallbackRegistry::resetWsoLogMsgCallback(void)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().logMsgCallback.reset();
}

void wso_domain::CallbackRegistry::resetFirmwareMsgCallback(void)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().firmwareMsgCallback.reset();
	impl().firmwareMsgCallback = nullopt;
}

void wso_domain::CallbackRegistry::resetJoystickButtonPressed(void)
{
	lock_guard<mutex> lock(singleMutex_);
	// Explicitly remove the value stored in optional object and make it empty to reuse. 
	impl().joystickPressed.reset();
}

void wso_domain::CallbackRegistry::resetOptimizeButtonPressed(void)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().optimizePressed.reset();
}


CallbackRegistry::CallbackRegistryImpl& wso_domain::CallbackRegistry::impl(void) const
{
	return *d_ptr;
}
