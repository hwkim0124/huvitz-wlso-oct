#include "pch.h"
#include "exts_oct_scanning.h"
#include "OctScanning.h"


bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanWorking(void)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->isWorking();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanPrevewing(void)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->isPreviewing();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanMeasuring(void)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->isMeasuring();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctScanOptimizing(void)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->isAutoOptimizing();
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setupOctScanProtocol(OctProtocolInitParam param)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->prepareScan(param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startOctScanPreview(OctPreviewImageCaptured clb_image, OctEnfaceImageCaptured clb_enface)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->startScan(clb_image, clb_enface);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::closeOctScanPreview(bool to_measure, OctScanProtocolCompleted clb_protocol)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->closeScan(to_measure, clb_protocol);
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::cancelOctScanning(void)
{
	if (auto* p = OctScanning::getInstance(); p) {
		p->cancelScan();
	}
	return;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startOctScanAutoDiopterFocus(OctAutoFocusOptimized clb_focus)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->startAutoDiopterFocus(clb_focus);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startOctScanAutoReference(OctAutoReferOptimized clb_refer)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->startAutoReference(clb_refer);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startOctScanAutoPolarization(OctAutoPolarOptimized clb_polar)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->startAutoPolarization(clb_polar);
	}	
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::startOctScanAutoOptimize(OctAutoScanOptimized clb_scan)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->startAutoScanOptimize(clb_scan);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::cancelOctScanAutoOptimizing(void)
{
	if (auto* p = OctScanning::getInstance(); p) {
		return p->cancelAutoOptimizing();
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::connectOctSpectrumDataCaptured(OctSpectrumDataCaptured clb_spectrum)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctSpectrumDataCaptured(clb_spectrum);
		ChainSetup::useSpectrumDataCallback(true, true);
	}
	return ;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::connectOctResampleDataCaptured(OctResampleDataCaptured clb_resample)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctResampledDataCaptured(clb_resample);
		ChainSetup::useResampledDataCallback(true, true);
	}
	return ;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::connectOctIntensityDataCaptured(OctIntensityDataCaptured clb_intensity)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctIntensityDataCaptured(clb_intensity);
		ChainSetup::useIntensityDataCallback(true, true);
	}
}

void WSOSYSTEM_DLL_API _stdcall wso_system::releaseOctSpectrumDataCaptured(void)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctSpectrumDataCaptured(nullptr);
		ChainSetup::useSpectrumDataCallback(true, false);
	}
	return ;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::releaseOctResampleDataCaptured(void)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctResampledDataCaptured(nullptr);
		ChainSetup::useResampledDataCallback(true, false);
	}
	return ;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::releaseOctIntensityDataCaptured(void)
{
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->setOctIntensityDataCaptured(nullptr);
		ChainSetup::useIntensityDataCallback(true, false);
	}
	return;
}
