#include "pch.h"
#include "exts_sys_config.h"
#include "Bootstrapper.h"
#include "Configuration.h"


using namespace wso_system;


bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainInternalFixationPreset(InternalFixationPreset* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getFixationSetting()->getInternalFixationPreset(); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainSloScanCaptureModeParam(int index, SloScanCaptureModeParam* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getCaptureSetting()->getSloScanCaptureModeParam(index); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainSloScanImagingModeParam(int index, SloScanImagingModeParam* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getCaptureSetting()->getSloScanImagingModeParam(index); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainSloScanCaptureInitParam(SloScanCaptureInitParam* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getCaptureSetting()->getSloScanCaptureInitParam(); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainCorneaCameraConfigParam(CorneaCameraConfigParam* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getCameraSetting()->getCorneaCameraConfigParam(); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainOctGalvanoConfigParam(OctGalvanoConfigParam* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getGalvanoSetting()->getOctGalvanoConfigParam(); data) {
			*param = *data;
			return true;
		}
	}
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitInternalFixationPreset(InternalFixationPreset param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getFixationSetting()->setInternalFixationPreset(param);
	}
	return ;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitSloScanCaptureModeParam(int index, SloScanCaptureModeParam param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getCaptureSetting()->setSloScanCaptureModeParam(index, param);
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitSloScanImagingModeParam(int index, SloScanImagingModeParam param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getCaptureSetting()->setSloScanImagingModeParam(index, param);
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitSloScanCaptureInitParam(SloScanCaptureInitParam param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getCaptureSetting()->setSloScanCaptureInitParam(param);
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitCorneaCameraConfigParam(CorneaCameraConfigParam param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getCameraSetting()->setCorneaCameraConfigParam(param);
	}
	return;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitOctGalvanoConfigParam(OctGalvanoConfigParam param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getGalvanoSetting()->setOctGalvanoConfigParam(param);
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::loadSystemConfigFile(void)
{
	auto result = SysConfiguration::getInstance()->loadConfigFile();
	return result;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::saveSystemConfigFile(void)
{
	auto result = SysConfiguration::getInstance()->saveConfigFile();
	return result;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::loadSystemConfiguration(void)
{
	if (auto* config = Configuration::getInstance(); config) {
		bool flag = config->loadSystemConfiguration(true);
		return flag;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::saveSystemConfiguration(void)
{
	if (auto* config = Configuration::getInstance(); config) {
		bool flag = config->saveSystemConfiguration(true);
		return flag;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::applySystemConfiguration(void)
{
	if (auto* config = Configuration::getInstance(); config) {
		bool flag = config->applySystemConfiguration();
		return flag;
	}
	return false;
}

