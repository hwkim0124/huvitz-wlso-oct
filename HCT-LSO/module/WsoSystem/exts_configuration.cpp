#include "pch.h"
#include "exts_configuration.h"
#include "Bootstrapper.h"
#include "Configuration.h"


using namespace wso_system;

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainInternalFixationPreset(InternalFixationPreset* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getFixationSettings()->getInternalFixationPreset(); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainCorneaCameraConfigParam(CorneaCameraConfigParam* param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* data = config->getCameraSettings()->getCorneaCameraConfigParam(); data) {
			*param = *data;
			return true;
		}
	}
	return false;
}

void WSOSYSTEM_DLL_API __stdcall wso_system::submitInternalFixationPreset(InternalFixationPreset param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getFixationSettings()->setInternalFixationPreset(param);
	}
	return;
}


void WSOSYSTEM_DLL_API __stdcall wso_system::submitCorneaCameraConfigParam(CorneaCameraConfigParam param)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		config->getCameraSettings()->setCorneaCameraConfigParam(param);
	}
	return;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::importSystemConfigFile(void)
{
	auto result = SystemConfig::getInstance()->loadSysConfigFile();
	return result;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::exportSystemConfigFile(void)
{
	auto result = SystemConfig::getInstance()->saveSysConfigFile();
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

