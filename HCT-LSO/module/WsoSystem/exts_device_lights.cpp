#include "pch.h"
#include "exts_device_lights.h"

#include "Hardware.h"



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
