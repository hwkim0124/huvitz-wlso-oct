#include "pch.h"
#include "exts_slo_scanner.h"

#include "Hardware.h"



bool WSOSYSTEM_DLL_API _stdcall wso_system::isSloScannerWorking(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->isTurnedOn();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isSloScannerGrabbing(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->isGrabbing();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isSloScannerChannelOn(SloColorChannel channel)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->isChannelOn(channel);
	}
	return false;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getSloScannerChannelCount(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->getChannelCount();
	}
	return 0;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::turnSloScannerSwitchOn(bool flag)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->controlSwitch((int)flag);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startSloScannerGrabbing(SloScanFrameDataAcquired clb_frame, SloScanFrameImageCaptured clb_preview)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		if (auto* p = inst->getMainBoard()->getSloScanner(); p) {
			inst->connectSloScanFrameDataAcquired(clb_frame);
			inst->connectSloScanFrameImageCaptured(clb_preview);
			return p->startGrabbing();
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::pauseSloScannerGrabbing(void)
{
	if (auto* inst = Hardware::getInstance(); inst) {
		inst->disconnectSloScanFrameDataCaptured();
		inst->disconnectSloScanFrameImageCaptured();
		if (auto* p = inst->getMainBoard()->getSloScanner(); p) {
			p->pauseGrabbing(true);
		}
	}
}


bool WSOSYSTEM_DLL_API _stdcall wso_system::moveSloGalvanoPositionY(int value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->controlYGalvoMove(value);
	}
	return false;
}


bool WSOSYSTEM_DLL_API _stdcall wso_system::submitSloPixelDataScalerParam(const SloPixelDataScalerParam* param)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->applyPixelDataScalerParam(param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::obtainSloPixelDataScalerParam(SloPixelDataScalerParam* param)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->fetchPixelDataScalerParam(param);
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::resetSloPixelDataScalerParam(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		p->resetPixelDataScalerParam();
	}
	return ;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::obtainSloScannerControlParam(SloScannerControlParam* param)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->fetchControlParameters(param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::applySloScannerControlParam(const SloScannerControlParam* param)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->changeControlParameters(param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::submitSloScannerAdcOffset(SloColorChannel channel, float offset)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->changeAdcOffset((int)channel, offset);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::submitSloScannerAdcGain(SloColorChannel channel, float gain)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloScanner(); p) {
		return p->changeAdcGain((int)channel, gain);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::submitSloDetectorGain(SloColorChannel channel, float gain)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloDetector(); p) {
		return p->changeDetectorGain((int)channel, gain);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::obtainSloDetectorStatus(SloDetectorStatus* status, bool reload)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getSloDetector(); p) {
		if (reload) {
			p->updateDetectorStatus();
		}
		return p->fetchDetectorStatus(status);
	}
	return false;
}
