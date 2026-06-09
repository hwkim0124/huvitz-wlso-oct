#include "pch.h"
#include "exts_slo_scanning.h"

#include "SloScanning.h"
#include "Hardware.h"

bool WSOSYSTEM_DLL_API _stdcall wso_system::isSloScanPrevewing(void)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->isPreviewing();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isSloScanMeasuring(void)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->isMeasuring();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isSloScanOptimizing(void)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->isAutoOptimizing();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setupSloScanCapture(SloCaptureInitParam param)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->prepareScan(param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startSloScanPreview(SloScanPreviewImageCaptured clb_preview, SloScanPreviewFrameAcquired clb_frame)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->startPreviewScan(clb_preview, clb_frame);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::enterSloScanMeasure(SloCaptureTakeParam param, SloScanMeasureImageCaptured clb_measure, SloScanMeasureFrameAcquired clb_frame)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->startMeasureScan(param, clb_measure, clb_frame);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::exitSloScanMeasure(bool to_preview)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->exitMeasureScan(to_preview);
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::cancelSloScan(void)
{
	if (auto* p = SloScanning::getInstance(); p) {
		p->cancelScan();
	}
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startSloAutoDiopterFocus(SloAutoFocusOptimized clb_focus, bool with_gain)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->startAutoDiopterFocus(clb_focus, with_gain);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startSloAutoGainControl(SloAutoGainOptimized clb_focus)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->startAutoGainControl(clb_focus);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::cancelSloAutoOptimizing(void)
{
	if (auto* p = SloScanning::getInstance(); p) {
		return p->cancelAutoOptimizing();
	}
	return false;
}
