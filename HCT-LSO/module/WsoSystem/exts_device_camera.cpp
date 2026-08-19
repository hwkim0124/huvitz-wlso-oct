#include "pch.h"
#include "exts_device_camera.h"

#include "Hardware.h"



bool WSOSYSTEM_DLL_API __stdcall wso_system::startCorneaCameraPreview(CorneaCameraFrameCaptured clbLeft, CorneaCameraFrameCaptured clbRight, CorneaCameraFrameCaptured clbLower)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		inst->connectCorneaCameraImageCaptured(clbLeft, clbRight, clbLower);
		if (auto* p = inst->getMainBoard()->getCorneaCamera(); p) {
			return p->play();
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::closeCorneaCameraPreview(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getCorneaCamera(); p) {
			p->pause();
			inst->disconnectCorneaCameraImageCaptured();
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isCorneaCameraPreviewing(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getCorneaCamera(); p) {
			auto result = p->isPlaying();
			return result;
		}
	}
	return false;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getCorneaCameraAgain(IrCameraId camId)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->getAnalogGain(camId);
	}
	return 0.0f;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getCorneaCameraDgain(IrCameraId camId)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->getDigitalGain(camId);
	}
	return 0.0f;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraAgain(IrCameraId camId, float value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setAnalogGain(camId, value, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraDgain(IrCameraId camId, float value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setDigitalGain(camId, value, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraViewPreset(IrCameraViewPreset preset)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setActiveViewPreset(preset);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraMirrorMode(IrCameraId camId, IrCameraMirrorMode hflip, IrCameraMirrorMode vflip)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setMirrorMode(camId, hflip, vflip);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setCorneaCameraOutputMode(IrCameraId camId, IrCameraOutputMode mode, IrCameraTestPattern pattern, int option)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		return p->setOutputMode(camId, mode, pattern, option);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getCorneaCameraMirrorMode(IrCameraId camId, IrCameraMirrorMode* hflip, IrCameraMirrorMode* vflip)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		p->getMirrorMode(camId, *hflip, *vflip);
		return true;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getCorneaCameraOutputMode(IrCameraId camId, IrCameraOutputMode* mode, IrCameraTestPattern* pattern, int* option)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getCorneaCamera(); p) {
		p->getOutputMode(camId, *mode, *pattern, *option);
		return true;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::startRetinaCameraPreview(RetinaCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		inst->connectRetinaCameraImageCaptured(clbFrame);
		if (auto* p = inst->getMainBoard()->getRetinaCamera(); p) {
			return p->play();
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::closeRetinaCameraPreview(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getRetinaCamera(); p) {
			p->pause();
			inst->disconnectRetinaCameraImageCaptured();
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isRetinaCameraPreviewing(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getRetinaCamera(); p) {
			auto result = p->isPlaying();
			return result;
		}
	}
	return false;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getRetinaCameraAgain(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getRetinaCamera(); p) {
		return p->getAnalogGain();
	}
	return 0.0f;
}

float WSOSYSTEM_DLL_API __stdcall wso_system::getRetinaCameraDgain(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getRetinaCamera(); p) {
		return p->getDigitalGain();
	}
	return 0.0f;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setRetinaCameraAgain(float value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getRetinaCamera(); p) {
		return p->setDigitalGain(value, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setRetinaCameraDgain(float value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getRetinaCamera(); p) {
		return p->setDigitalGain(value, true);
	}
	return false;
}
