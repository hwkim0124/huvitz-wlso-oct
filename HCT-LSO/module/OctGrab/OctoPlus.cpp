#include "pch.h"
#include "OctoPlus.h"
#include "OctoPlusUsb3_RegisterAddress.h"
#include "OctoPlusUsb3_Utility.h"

#include "CamCmosOctUsb3.h"
#include "Usb3Grabber.h"


#include <atomic>
#include <functional>
#include <mutex>


using namespace OctoPlus_USB_Register_Address;
using namespace oct_grab;
using namespace cpp_util;
using namespace std;


struct OctoPlus::OctoPlusImpl
{
	bool initialized;
	atomic<bool> cameraOpened;
	atomic<bool> cameraWorking;
	atomic<bool> cameraStopping;

	size_t imageHeight = 1024;
	size_t numOfBuffer = 128;
	unsigned long imagesToAcquire = 0;
	Usb3Grabber* grabber;

	tCameraInfo CameraInfo;
	CAM_HANDLE hCamera;

	thread worker;

	OctoPlusImpl() : initialized(false), cameraOpened(false), grabber(nullptr),
		cameraStopping(false), cameraWorking(false), hCamera(NULL) {

	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<OctoPlus::OctoPlusImpl> OctoPlus::d_ptr(new OctoPlusImpl());


OctoPlus::OctoPlus()
{
}


OctoPlus::~OctoPlus()
{
}


bool oct_grab::OctoPlus::initializeLibrary(void)
{
	int nError = USB3_InitializeLibrary();
	if (nError != CAM_ERR_SUCCESS) {
		getImpl().initialized = false;
		LogD() << "USB3_InitializeLibrary Error: " << nError;
		LogD() << getErrorText(nError);
	}
	else {
		getImpl().initialized = true;
		LogD() << "Usb3 Cmos camera library initialized: " << nError;
	}

	getImpl().cameraOpened = false;
	getImpl().cameraWorking = false;
	getImpl().cameraStopping = false;
	return getImpl().initialized;
}


void oct_grab::OctoPlus::releaseLibrary(void)
{
	if (getImpl().initialized) {
		int nError = USB3_TerminateLibrary();
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_TerminateLibrary Error: " << nError;
			LogD() << getErrorText(nError);
		}
	}
	return;
}


bool oct_grab::OctoPlus::isLibraryInitialized(void)
{
	return getImpl().initialized;
}


std::string oct_grab::OctoPlus::getErrorText(int errCode)
{
	char text[512];
	size_t size = sizeof(text);

	if (USB3_GetErrorText(errCode, text, &size) != CAM_ERR_SUCCESS) {
		text[0] = '\0';
	}
	return string(text);
}


void oct_grab::OctoPlus::setupUsb3Grabber(Usb3Grabber * grabber)
{
	getImpl().grabber = grabber;
	return;
}


bool oct_grab::OctoPlus::openLineCamera(void)
{
	getImpl().cameraOpened = false;
	getImpl().hCamera = NULL;

	unsigned long ulNbCameras;
	int nError = USB3_UpdateCameraList(&ulNbCameras);

	if (nError != CAM_ERR_SUCCESS) {
		LogD() << "USB3_UpdateCameraList Error: " << nError;
		LogD() << getErrorText(nError);
		return false;
	}
	if (ulNbCameras == 0) {
		LogD() << "Usb3 cmos camera not found";
		return false;
	}

	nError = USB3_GetCameraInfo(0, &getImpl().CameraInfo);
	if (nError != CAM_ERR_SUCCESS) {
		LogD() << "USB3_GetCameraInfo Error: " << nError;
		LogD() << getErrorText(nError);
		return false;
	}
	else {
		LogD() << "Usb3 cmos opened, id: " << getImpl().CameraInfo.pcID;
	}
	
	try {
		nError = USB3_OpenCamera(&getImpl().CameraInfo, &getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_OpenCamera Error: " << nError;
			LogD() << getErrorText(nError);
			return false;
		}
		/*
		size_t iImageHeight = 100;
		size_t iNbOfBuffer = 10;
		printf("USB3_SetImageParameters\n");
		nError = USB3_SetImageParameters(getImpl().hCamera, iImageHeight, iNbOfBuffer);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_SetImageParameters Error: " << nError;
			LogD() << getErrorText(nError);
			return false;
		}
		*/
		getImpl().imageHeight = 0;
		getImpl().cameraOpened = true;
	}
	catch (...) {
		LogD() << "Unknown exception during opening Usb3 cmos camera";
	}
	return getImpl().cameraOpened;
}


void oct_grab::OctoPlus::closeLineCamera(void)
{
	if (getImpl().hCamera != NULL) {
		int nError = USB3_CloseCamera(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_CloseCamera Error: " << nError;
			LogD() << getErrorText(nError);
		}
	}
	return;
}


bool oct_grab::OctoPlus::isLineCameraOpened(void)
{
	return getImpl().cameraOpened;
}


bool oct_grab::OctoPlus::isLineCameraGrabbing(void)
{
	return isLineCameraWorking();
}


bool oct_grab::OctoPlus::startAcquisition(bool restart)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraGrabbing()) {
		if (!restart) {
			return true;
		}
		cancelAcquisition();
	}

	try {
		int nError = USB3_StartAcquisition(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "OctoPlus :: startAcquisition failed!";
			LogD() << "USB3_StartAcquisition Error: " << nError;
			LogD() << getErrorText(nError);
			cancelAcquisition();
			return false;
		}
		getImpl().cameraWorking = true;
	}
	catch (...) {
		LogD() << "OctoPlus :: startAcquisition exception occurred!";
	}
	return true;
}


void oct_grab::OctoPlus::cancelAcquisition(void)
{
	if (!isLineCameraOpened()) {
		return;
	}
	if (!isLineCameraWorking()) {
		return;
	}
	
	getImpl().cameraWorking = false;

	try {
		int nError = USB3_AbortGetBuffer(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "OctoPlus :: cancelAcquisition failed!";
			LogD() << "USB3_AbortGetBuffer Error: " << nError;
			LogD() << getErrorText(nError);
			return;
		}

		nError = USB3_StopAcquisition(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "OctoPlus :: cancelAcquisition failed!";
			LogD() << "USB3_StopAcquisition Error: " << nError;
			LogD() << getErrorText(nError);
			return;
		}

		nError = USB3_FlushBuffers(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "OctoPlus :: cancelAcquisition failed!";
			LogD() << "USB3_FlushBuffers Error: " << nError;
			LogD() << getErrorText(nError);
			return;
		}
	}
	catch (...) {
		LogD() << "OctoPlus :: cancelAcquisition exception occurred!";
	}
	return;
}


bool oct_grab::OctoPlus::acquireLineCameraImage(int count)
{
	int nError;
	tImageInfos ImageInfos;
	unsigned long imagesAcquired = 0;
	unsigned long imagesToAcquire = count;

	const unsigned long MAX_TIMEOUT_ACQ_IN_MS = 3000;
	
	try {
		while (imagesAcquired < imagesToAcquire) {
			nError = USB3_GetBuffer(getImpl().hCamera, &ImageInfos, MAX_TIMEOUT_ACQ_IN_MS);

			if (nError == CAM_ERR_SUCCESS) {
				imagesAcquired += 1;

				nError = USB3_RequeueBuffer(getImpl().hCamera, ImageInfos.hBuffer);
				if (nError != CAM_ERR_SUCCESS) {
				}
			}
			else {
				if (nError == CAM_ERR_TIMEOUT) {
					LogD() << "USB3_GetBuffer timeout!";
				}
				else {
					LogD() << "USB3_GetBuffer Error: " << nError;
				}
				LogD() << getErrorText(nError);
				break;
			}
		}

		if (imagesToAcquire == imagesAcquired) {
			return true;
		}
	}
	catch (...) {
		LogD() << "OctoPlus :: acquireLineCameraImage exception occurred!";
	}
	return false;
}


bool oct_grab::OctoPlus::acquirePreviewImages(std::vector<int> bufferIds)
{
	int nError;
	tImageInfos ImageInfos;
	size_t imagesAcquired = 0;
	size_t imagesToAcquire = bufferIds.size();
	
	const unsigned long MAX_TIMEOUT_ACQ_IN_MS = 5000;

	try {
		while (imagesAcquired < imagesToAcquire) {
			// auto t1 = GetTickCount64();
			nError = USB3_GetBuffer(getImpl().hCamera, &ImageInfos, MAX_TIMEOUT_ACQ_IN_MS);
			// auto t2 = GetTickCount64();

			// LogD() << "USB3_GetBuffer: " << (t2 - t1)/1000.0f;

			if (nError == CAM_ERR_SUCCESS) {
				if (getImpl().grabber != nullptr) {
					int buffId = bufferIds[imagesAcquired];
					auto buffer = (unsigned short*)ImageInfos.pDatas;
					auto dsize = ImageInfos.iImageSize;

					auto t1 = GetTickCount64();
					getImpl().grabber->hookGrabPreviewBuffer(buffer, buffId, dsize);
					auto t2 = GetTickCount64();
					// LogD() << "hookGrabPreviewBuffer: " << (t2 - t1) / 1000.0f;
				}

				imagesAcquired += 1;

				auto t1 = GetTickCount64();
				nError = USB3_RequeueBuffer(getImpl().hCamera, ImageInfos.hBuffer);
				auto t2 = GetTickCount64();

				// LogD() << "USB3_RequeueBuffer: " << (t2 - t1) / 1000.0f;

				if (nError != CAM_ERR_SUCCESS) {
					LogD() << "USB3_RequeueBuffer Error: " << nError;
					LogD() << getErrorText(nError);
					break;
				}
			}
			else {
				LogD() << "USB3_GetBuffer Error: " << nError;
				LogD() << getErrorText(nError);
				break;
			}
		}

		if (imagesToAcquire == imagesAcquired) {
			return true;
		}
	}
	catch (...) {
		LogD() << "OctoPlus :: acquirePreviewImages exception occurred!";
	}
	return false;
}


bool oct_grab::OctoPlus::acquireMeasureImages(std::vector<int> bufferIds)
{
	int nError;
	tImageInfos ImageInfos;
	size_t imagesAcquired = 0;
	size_t imagesToAcquire = bufferIds.size();

	const unsigned long MAX_TIMEOUT_ACQ_IN_MS = 5000;

	try {
		while (imagesAcquired < imagesToAcquire) {
			nError = USB3_GetBuffer(getImpl().hCamera, &ImageInfos, MAX_TIMEOUT_ACQ_IN_MS);

			if (nError == CAM_ERR_SUCCESS) {
				if (getImpl().grabber != nullptr) {
					int buffId = bufferIds[imagesAcquired];
					auto buffer = (unsigned short*)ImageInfos.pDatas;
					auto dsize = ImageInfos.iImageSize;
					getImpl().grabber->hookGrabMeasureBuffer(buffer, buffId, dsize);
				}

				imagesAcquired += 1;
				nError = USB3_RequeueBuffer(getImpl().hCamera, ImageInfos.hBuffer);
				if (nError != CAM_ERR_SUCCESS) {
					LogD() << "USB3_RequeueBuffer Error: " << nError;
					LogD() << getErrorText(nError);
					break;
				}
			}
			else {
				LogD() << "USB3_GetBuffer Error: " << nError;
				LogD() << getErrorText(nError);
				break;
			}
		}

		if (imagesToAcquire == imagesAcquired) {
			return true;
		}
	}
	catch (...) {
		LogD() << "Usb cmos camera : acquireMeasureImages exception occurred!";
	}
	return false;
}


bool oct_grab::OctoPlus::acquireEnfaceImages(std::vector<int> bufferIds)
{
	int nError;
	tImageInfos ImageInfos;
	size_t imagesAcquired = 0;
	size_t imagesToAcquire = bufferIds.size();

	const unsigned long MAX_TIMEOUT_ACQ_IN_MS = 5000;

	try {
		while (imagesAcquired < imagesToAcquire) {
			nError = USB3_GetBuffer(getImpl().hCamera, &ImageInfos, MAX_TIMEOUT_ACQ_IN_MS);

			if (nError == CAM_ERR_SUCCESS) {
				if (getImpl().grabber != nullptr) {
					int buffId = bufferIds[imagesAcquired];
					auto buffer = (unsigned short*)ImageInfos.pDatas;
					auto dsize = ImageInfos.iImageSize;
					getImpl().grabber->hookGrabEnfaceBuffer(buffer, buffId, dsize);
				}

				imagesAcquired += 1;
				nError = USB3_RequeueBuffer(getImpl().hCamera, ImageInfos.hBuffer);
				if (nError != CAM_ERR_SUCCESS) {
					LogD() << "USB3_RequeueBuffer Error: " << nError;
					LogD() << getErrorText(nError);
					break;
				}
			}
			else {
				LogD() << "USB3_GetBuffer Error: " << nError;
				LogD() << getErrorText(nError);
				break;
			}
		}

		if (imagesToAcquire == imagesAcquired) {
			return true;
		}
	}
	catch (...) {
		LogD() << "Usb cmos camera : acquireEnfaceImages exception occurred!";
	}
	return false;
}


void oct_grab::OctoPlus::threadLineCameraImage(const OctoPlus::OctoPlusImpl* impl)
{
	int nError;
	tImageInfos ImageInfos;
	unsigned long timeoutAcq = 3000;
	unsigned long imagesAcquired = 0;

	const unsigned long MAX_TIMEOUT_ACQ_IN_MS = 3000;

	try {
		while (true) {
			if (impl->cameraStopping) {
				break;
			}
			if (impl->imagesToAcquire == imagesAcquired) {
				break;
			}

			nError = USB3_GetBuffer(impl->hCamera, &ImageInfos, timeoutAcq);

			if (!impl->cameraStopping) {
				if (nError == CAM_ERR_SUCCESS) {
					imagesAcquired += 1;

					nError = USB3_RequeueBuffer(impl->hCamera, ImageInfos.hBuffer);
					if (nError != CAM_ERR_SUCCESS) {
					}
				}
				else {
					if (nError == CAM_ERR_TIMEOUT) {
						LogD() << "USB3_GetBuffer timeout!";
					}
					else {
						LogD() << "USB3_GetBuffer Error: " << nError;
					}
				}
			}
		}
	}
	catch (...) {
		LogD() << "Usb cmos camera : threadLineCameraImage exception occurred!";
	}
	return;
}


bool oct_grab::OctoPlus::startAcquisition2(unsigned long imagesToAcquire)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraWorking()) {
		cancelAcquisition2();
	}

	getImpl().worker = thread{ &OctoPlus::threadLineCameraImage, &getImpl() };
	getImpl().cameraWorking = true;

	try {
		int nError = USB3_StartAcquisition(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_StartAcquisition Error: " << nError;
			LogD() << getErrorText(nError);
			cancelAcquisition2();
			return false;
		}
	}
	catch (...) {
		LogD() << "Usb cmos camera : startAcquisition exception occurred!";
	}
	return true;
}


void oct_grab::OctoPlus::cancelAcquisition2(void)
{
	if (!isLineCameraWorking()) {
		return;
	}

	getImpl().cameraStopping = true;
	if (getImpl().worker.joinable()) {
		getImpl().worker.join();
	}
	getImpl().cameraWorking = false;
	getImpl().cameraStopping = false;

	try {
		int nError = USB3_AbortGetBuffer(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_AbortGetBuffer Error: " << nError;
			LogD() << getErrorText(nError);
			return;
		}

		nError = USB3_StopAcquisition(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_StopAcquisition Error: " << nError;
			LogD() << getErrorText(nError);
			return;
		}

		nError = USB3_FlushBuffers(getImpl().hCamera);
		if (nError != CAM_ERR_SUCCESS) {
			LogD() << "USB3_FlushBuffers Error: " << nError;
			LogD() << getErrorText(nError);
			return;
		}
	}
	catch (...) {
		LogD() << "Usb cmos camera : cancelAcquisition exception occurred!";
	}
	return;
}


bool oct_grab::OctoPlus::isLineCameraWorking(void)
{
	return getImpl().cameraWorking;
}

int oct_grab::OctoPlus::getImageHeight(void)
{
	return (int)getImpl().imageHeight;
}


void oct_grab::OctoPlus::setImageHeight(size_t height)
{
	getImpl().imageHeight = height;
	return;
}

void oct_grab::OctoPlus::setNumberOfBuffers(size_t number)
{
	getImpl().numOfBuffer = number;
	return;
}

bool oct_grab::OctoPlus::setImageParameters(int imageHeight, int numOfBuffer)
{
	if (numOfBuffer == 0) {
		numOfBuffer = 64;
	}
	int nError = USB3_SetImageParameters(getImpl().hCamera, imageHeight, numOfBuffer);
	if (nError != CAM_ERR_SUCCESS) {
		LogD() << "USB3_SetImageParameters Error: " << nError;
		LogD() << getErrorText(nError);
		return false;
	}
	else {
		getImpl().imageHeight = imageHeight;
		getImpl().numOfBuffer = numOfBuffer;
		LogD() << "Usb cmos camera updated, height: " << getImpl().imageHeight << ", buffers: " << getImpl().numOfBuffer;
		USB3_FlushBuffers(getImpl().hCamera);
	}
	return true;
}


bool oct_grab::OctoPlus::updateImageParameters(int imageHeight, int numOfBuffer)
{
	bool ret = setImageParameters(imageHeight, numOfBuffer);
	return ret;
}


bool oct_grab::OctoPlus::setTriggerMode(ETriggerMode mode)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraGrabbing()) {
		return false;
	}

	auto value = getValueFromTriggerMode(mode);
	if (value < 0 || !writeRegister(cUl_Address_Trigger_Mode, value)) {
		LogD() << "OctoPlus::setTriggerMode() failed, mode: " << getStringTriggerMode(mode);
		return false;
	}
	LogD() << "OctoPlus::setTriggerMode(): " << getStringTriggerMode(mode);
	return true;
}


bool oct_grab::OctoPlus::getTriggerMode(ETriggerMode * mode)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Trigger_Mode, &value)) {
		LogD() << "OctoPlus::getTriggerMode() failed";
		return false;
	}

	*mode = getTriggerModeFromValue(value);
	LogD() << "OctoPlus::getTriggerMode(): " << getStringTriggerMode(*mode);
	return true;
}

bool oct_grab::OctoPlus::setBufferType(EBufferType type)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraGrabbing()) {
		return false;
	}

	auto value = getValueFromBufferType(type);
	if (value < 0 || !writeRegister(cUl_Address_Buffer_Type, value)) {
		LogD() << "OctoPlus::setBufferType() failed, type: " << getStringBufferType(type);
		return false;
	}
	LogD() << "OctoPlus::setBufferType(): " << getStringBufferType(type);
	return true;
}

bool oct_grab::OctoPlus::getBufferType(EBufferType * type)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Buffer_Type, &value)) {
		LogD() << "OctoPlus::getBufferType() failed";
		return false;
	}

	*type = getBufferTypeFromValue(value);
	LogD() << "OctoPlus::getBufferType(): " << getStringBufferType(*type);
	return true;
}


bool oct_grab::OctoPlus::setOutputMode(EOutputMode mode)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraGrabbing()) {
		return false;
	}

	auto value = getValueFromOutputMode(mode);
	if (value < 0 || !writeRegister(cUl_Address_Output_Mode, value)) {
		LogD() << "OctoPlus::setOutputMode() failed, mode: " << getStringOutputMode(mode);
		return false;
	}
	LogD() << "OctoPlus::setOutputMode(): " << getStringOutputMode(mode);
	return true;
}

bool oct_grab::OctoPlus::getOutputMode(EOutputMode * mode)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Output_Mode, &value)) {
		LogD() << "OctoPlus::getOutputMode() failed";
		return false;
	}

	*mode = getOutputModeFromValue(value);
	LogD() << "OctoPlus::getOutputMode(): " << getStringOutputMode(*mode);
	return true;
}

std::string oct_grab::OctoPlus::getTriggerMode(void)
{
	ETriggerMode mode = ETriggerMode::Mode_Trigger_Unknown;
	if (getTriggerMode(&mode)) {
	}
	return getStringTriggerMode(mode);
}

std::string oct_grab::OctoPlus::getBufferType(void)
{
	EBufferType type = EBufferType::Type_Unknown;
	if (getBufferType(&type)) {
	}
	return getStringBufferType(type);
}

std::string oct_grab::OctoPlus::getOutputMode(void)
{
	EOutputMode mode = EOutputMode::Mode_Pixel_Unknown;
	if (getOutputMode(&mode)) {
	}
	return getStringOutputMode(mode);
}

bool oct_grab::OctoPlus::setEnableCircularBuffer(bool enable)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraGrabbing()) {
		return false;
	}

	auto value = enable;
	if (!writeRegister(cUl_Address_Circular_Buffer, value)) {
		LogD() << "OctoPlus::setEnableCircularBuffer() failed";
		return false;
	}
	LogD() << "OctoPlus::setEnableCircularBuffer(): " << enable;
	return true;
}

bool oct_grab::OctoPlus::isEnabledCircularBuffer(void)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Circular_Buffer, &value)) {
		LogD() << "OctoPlus::isEnabledCircularBuffer() failed";
		return false;
	}

	LogD() << "OctoPlus::isEnabledCircularBuffer(): " << value;
	return true;
}

bool oct_grab::OctoPlus::setLinePeriod(double time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}
	if (isLineCameraGrabbing()) {
		return false;
	}

	int value = MicroSecToTimeStep(time_us);
	if (!writeRegister(cUl_Address_Line_Period, value)) {
		LogD() << "OctoPlus::setLinePeriod(), time_us: " << time_us << " failed";
		return false;
	}
	LogD() << "OctoPlus::setLinePeriod(), time_us: " << time_us;
	return true;
}

bool oct_grab::OctoPlus::getLinePeriod(double * time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Line_Period, &value)) {
		LogD() << "OctoPlus::getLinePeriod() failed";
		return false;
	}

	*time_us = TimeStepToMicroSec(value);
	LogD() << "OctoPlus::getLinePeriod(), time_us: " << *time_us;
	return true;
}

bool oct_grab::OctoPlus::getLinePeriodMin(double * time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Line_Period_Min, &value)) {
		LogD() << "OctoPlus::getLinePeriodMin() failed";
		return false;
	}

	*time_us = TimeStepToMicroSec(value);
	LogD() << "OctoPlus::getLinePeriodMin(), time_us: " << *time_us;
	return true;
}

bool oct_grab::OctoPlus::setExposureTime(double time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = MicroSecToTimeStep(time_us);
	if (!writeRegister(cUl_Address_Exposure_Time, value)) {
		LogD() << "OctoPlus::setExposureTime() failed, time_us: " << time_us;
		return false;
	}
	LogD() << "OctoPlus::setExposureTime(), time_us: " << time_us;
	return true;
}

bool oct_grab::OctoPlus::getExposureTime(double * time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Exposure_Time, &value)) {
		LogD() << "OctoPlus::getExposureTime() failed";
		return false;
	}

	*time_us = TimeStepToMicroSec(value);
	LogD() << "OctoPlus::getExposureTime(), time_us: " << *time_us;
	return true;
}

bool oct_grab::OctoPlus::getExposureTimeMin(double * time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Exposure_Time_Min, &value)) {
		LogD() << "OctoPlus::getExposureTimeMin() failed";
		return false;
	}

	*time_us = TimeStepToMicroSec(value);
	LogD() << "OctoPlus::getExposureTimeMin(), time_us: " << *time_us;
	return true;
}

bool oct_grab::OctoPlus::getExposureTimeMax(double * time_us)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Exposure_Time_Max, &value)) {
		LogD() << "OctoPlus::getExposureTimeMax() failed";
		return false;
	}

	*time_us = TimeStepToMicroSec(value);
	LogD() << "OctoPlus::getExposureTimeMax(), time_us: " << *time_us;
	return true;
}

bool oct_grab::OctoPlus::setAnalogGain(EAnalogGain gain)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = getValueFromAnalogGain(gain);
	if (value < 0 || !writeRegister(cUl_Address_Analog_Gain, value)) {
		LogD() << "OctoPlus::setAnalogGain() failed, value: " << value;
		return false;
	}
	LogD() << "OctoPlus::setAnalogGain(): " << getNumericAnalogGain(gain);
	return true;
}

bool oct_grab::OctoPlus::getAnalogGain(double * gain)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Analog_Gain, &value)) {
		LogD() << "OctoPlus::getAnalogGain() failed";
		return false;
	}

	auto mode = getAnalogGainFromValue(value);
	*gain = getNumericAnalogGain(mode);
	LogD() << "OctoPlus::getAnalogGain(): " << *gain;
	return true;
}

bool oct_grab::OctoPlus::setDigitalGain(double gain)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = DigitalGainValueToStep(gain);
	if (!writeRegister(cUl_Address_Digital_Gain, value)) {
		LogD() << "OctoPlus::setDigitalGain() failed, value: " << gain;
		return false;
	}
	LogD() << "OctoPlus::setDigitalGain(): " << gain;
	return true;
}

bool oct_grab::OctoPlus::getDigitalGain(double * gain)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	int value = 0;
	if (!readRegister(cUl_Address_Digital_Gain, &value)) {
		LogD() << "OctoPlus::getDigitalGain() failed";
		return false;
	}

	*gain = DigitalGainStepToValue(value);
	LogD() << "OctoPlus::getDigitalGain(): " << *gain;
	return true;
}

bool oct_grab::OctoPlus::writeRegister(unsigned long address, int value)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	size_t size = sizeof(int);
	auto ret = USB3_WriteRegister(getImpl().hCamera, address, &value, &size);
	if (ret != CAM_ERR_SUCCESS) {
		LogD() << "USB3_WriteRegister error: " << getErrorText(ret);
		return false;
	}
	return true;
}

bool oct_grab::OctoPlus::readRegister(unsigned long address, int * value)
{
	if (!isLineCameraOpened()) {
		return false;
	}

	size_t size = sizeof(int);
	auto ret = USB3_ReadRegister(getImpl().hCamera, address, value, &size);
	if (ret != CAM_ERR_SUCCESS) {
		LogD() << "USB3_ReadRegister error: " << getErrorText(ret);
		return false;
	}

	return true;
}

OctoPlus::OctoPlusImpl & oct_grab::OctoPlus::getImpl(void)
{
	return *d_ptr;
}
