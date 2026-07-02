#include "pch.h"
#include "exts_lso_camera.h"
#include "Hardware.h"

using namespace wso_system;
using namespace std;



void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraLive(ColorCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraFrameCaptured(clbFrame);
			p->startLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSingleFrameLive(ColorCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraFrameCaptured(clbFrame);
			p->startSingleFrameLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSeqROILive(ColorCameraSeqROIFrameCaptured clbFrame, const LsoCaptureFrameSeqROIPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraSeqROIFrameCaptured(clbFrame);
			p->setCaptureSequencerPreset(*preset);
			p->startFrameSeqROILiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSwTriggerOffsetROILive(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraOffsetROIFrameCaptured(clbFrame);
			p->setCaptureOffsetRoiPreset(*preset);
			p->startFrameOffsetROILiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSwTriggerOffsetROICapture(ColorCameraOffsetROIFrameCaptured clbFrame, const LsoCaptureFrameOffsetROIPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraOffsetROIFrameCaptured(clbFrame);
			p->setCaptureOffsetRoiPreset(*preset);
			p->startFrmaeOffsetROICaptureMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraRollSwTrigOverlapLive(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraRollSwTrigOverlapFrameCaptured(clbFrame);
			p->setRollSwTrigOverlapPreset(*preset);
			p->startFrameRollSwTrigOverlabLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraRollSwTrigOverlapCapture(ColorCameraRollSwTrigOverlapFrameCaptured clbFrame, const LsoCaptureFrameRollSwTrigOverlapPreset* preset)
{
	if (preset == nullptr) {
		return;
	}
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraRollSwTrigOverlapFrameCaptured(clbFrame);
			p->setRollSwTrigOverlapPreset(*preset);
			p->startFrameRollSwTrigOverlabCaptureMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraSwTriggerLive(ColorCameraFrameCaptured clbFrame)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraSwTriggerFrameCaptured(clbFrame);
			p->startSwTriggerLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::stopColorCameraSwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->stopSwTriggerLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::shootColorCameraSwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->shootSwTrigger();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraHwTriggerLive(ColorCameraImageCaptured clbImage)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraImageCaptured(clbImage);
			p->startHwTriggerLiveMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::stopColorCameraHwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->stopHwTriggerLiveMode();
		}
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isColorCameraIsStreaming(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			return p->isCameraStreaming();
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::setupColorCameraHwTriggerLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->setupHwTriggerSetting(false);
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::startColorCameraOriginal(ColorCameraImageCaptured clbImage)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			inst->connectColorCameraImageCaptured(clbImage);
			p->startOriginalMode();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::pauseColorCameraOriginal(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->pauseOriginalMode();
			inst->disconnectColorCameraImageCaptured();
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::pauseColorCameraLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			p->pauseLiveMode();
			inst->disconnectColorCameraFrameCaptured();
			inst->disconnectColorCameraImageCaptured();
		}
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isColorCameraLive(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			auto result = p->isLiveMode();
			return result;
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::getColorCameraSettingParam(ColorCameraSettingParam* param)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			if (param) {
				auto sets = p->getCameraSettings();
				memcpy(param, &sets, sizeof(ColorCameraSettingParam));
			}
			return;
		}
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::setColorCameraSettingParam(const ColorCameraSettingParam* param)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getColorCamera(); p) {
			if (param) {
				p->setCameraSettings(*param);
			}
			return;
		}
	}
	return;
}