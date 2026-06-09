#include "pch.h"
#include "exts_device_lights.h"

#include "Hardware.h"



bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainQldStatusParam(LightType type, QldStatusParam* param, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		if (reload) {
			driver->reloadQldChannelStatus();
			driver->reloadQldCalibration();
		}
		return driver->getQldStatusParam(type, param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainQldChannelStatus(LightType type, QldStatusParam* param, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver && param) {
		if (reload) {
			driver->reloadQldChannelStatus();
		}
		return driver->getQldChannelStatus(type, &param->channelStatus);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainQldChannelHeader(LightType type, QldStatusParam* param, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver && param) {
		if (reload) {
			driver->reloadQldChannelHeader(type);
		}
		return driver->getQldChannelHeader(type, &param->calibData);
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainQldChannelProfile(LightType type, int profile, QldStatusParam* param, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver && param) {
		if (reload) {
			driver->reloadQldChannelProfile(type, profile);
		}
		return driver->getQldChannelProfile(type, &param->calibData);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::submitQldChannelHeader(LightType type, const QldStatusParam* param)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver && param) {
		return driver->setQldChannelHeader(type, &param->calibData);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::submitQldChannelProfile(LightType type, int profile, const QldStatusParam* param)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver && param) {
		return driver->setQldChannelProfile(type, profile, &param->calibData);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::commitQldChannelHeaderToMemory(LightType type)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->commitQldChannelHeader(type);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::commitQldChannelProfileToMemory(LightType type, int profile)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->commitQldChannelProfile(type, profile);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchQldChannelHeaderFromMemory(LightType type)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->fetchQldChannelHeader(type);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchQldChannelProfileFromMemory(LightType type, int profile)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->fetchQldChannelProfile(type, profile);
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::turnOnLight(LightType type)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getLightLed(type); p) {
		return p->lightOn();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::turnOffLight(LightType type)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getLightLed(type); p) {
		return p->lightOff();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isLightOn(LightType type)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getLightLed(type); p) {
		return p->isLightOn();
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::setQldApcAccMode(LightType type, int mode)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->setQldApcAccMode(type, mode, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setQldCurrentLimitLevel(LightType type, int limit)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->setCurrentLimitLevel(type, limit, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setQldReferenceIntensity(LightType type, int profile, int normal, int value)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->setReferenceIntensity(type, profile, normal, value, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setQldApcGainResistorDcode(LightType type, int profile, int dcode)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->setApcGainDcode(type, profile, dcode, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setQldActiveProfile(LightType type, int profile, int normal)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		return driver->setActiveProfile(type, profile, normal, true);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainQldControlResult(LightType type, int* result, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getQldLaserDriver(); driver) {
		if (reload) {
			driver->reloadQldChannelStatus();
		}
		*result = driver->getControlResult(type);
		return true;
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainAdcSensorStatus(AdcSensorStatus* status, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getZyncXadcDriver(); driver) {
		if (reload) {
			driver->reloadZynqXadcStatus();
		}
		return driver->fetchZynqXadcStatus(status);
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainOctSldStatusParam(OctSldStatusParam* status, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getSldLaserDriver(); driver) {
		if (reload) {
			driver->reloadOctSldStatus();
		}
		return driver->getOctSldStatusParam(status);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainOctSldCalibParam(OctSldCalibParam* calib, bool reload)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getSldLaserDriver(); driver) {
		if (reload) {
			driver->reloadSystemProfile();
		}
		return driver->getOctSldCalibParam(calib);
	}
	return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::setOctSldHighCode(int value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctSldLed(); p) {
		return p->setHighCode(value);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setOctSldLowCode1(int value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctSldLed(); p) {
		return p->setLowCode1(value);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setOctSldLowCode2(int value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctSldLed(); p) {
		return p->setLowCode2(value);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setOctSldRsiCode(int value)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctSldLed(); p) {
		return p->setRsiCode(value);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::saveOctSldCalibration(void)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getSldLaserDriver(); driver) {
		return driver->saveSldCalibration();
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::loadOctSldCalibration(void)
{
	if (auto* driver = Hardware::getInstance()->getMainBoard()->getSldLaserDriver(); driver) {
		return driver->loadSldCalibration();
	}
	return false;
}
