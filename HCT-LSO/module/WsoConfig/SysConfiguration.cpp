#include "pch.h"
#include "SysConfiguration.h"
#include "SysConfigFile.h"
#include "CameraSetting.h"
#include "FixationSetting.h"
#include "SignalSetting.h"
#include "GalvanoSetting.h"

#include "MeasureSetting.h"
#include "LsoCaptureSetting.h"
#include "LsoDisplaySetting.h"

using namespace wso_config;
using namespace std;


struct SysConfiguration::SystemConfigImpl
{
    CameraSetting cameraSet;
    FixationSetting fixationSet;
    SignalSetting signalSet;
	GalvanoSetting galvanoSet;

	MeasureSetting measureSet;
	LsoCaptureSetting lsoCaptureSet;
	LsoDisplaySetting lsoDisplaySet;


    SysConfigFile configFile;

    SystemConfigImpl()
    {
    }
};



SysConfiguration::SysConfiguration() :
    d_ptr(make_unique<SystemConfigImpl>())
{
}


wso_config::SysConfiguration::~SysConfiguration() = default;


SysConfiguration* wso_config::SysConfiguration::getInstance(void)
{
    // Instance is constructed by public static method.
    // Static local variable initialization is thread-safe 
    // and will be initailized only once. 
    static SysConfiguration instance;
    return &instance;
}


void wso_config::SysConfiguration::resetToDefaultValues(void)
{
    getFixationSetting()->resetToDefaultValues();
    getSignalSetting()->resetToDefaultValues();
    getCameraSetting()->resetToDefaultValues();
	getGalvanoSetting()->resetToDefaultValues();

	getMeasureSetting()->resetToDefaultValues();
	getLsoCaptureSetting()->resetToDefaultValues();
	getLsoDisplaySetting()->resetToDefaultValues();
}


bool wso_config::SysConfiguration::loadConfigFile(const char* name)
{
    string path = (name == nullptr ? getDefaultConfigFilePath() : name);
    if (!getImpl().configFile.loadSystemConfig(path.c_str(), this)) {
        LogW() << "Failed to load system config file, path: " << path;
        return false;
    }
    else {
        LogI() << "System config file loaded, path: " << path;
        return true;
    }
}


bool wso_config::SysConfiguration::saveConfigFile(const char* name)
{
    string path = (name == nullptr ? getDefaultConfigFilePath() : name);
    if (!getImpl().configFile.saveSystemConfig(path.c_str(), this)) {
        LogW() << "Failed to save system config file, path: " << path;
        return false;
    }
    else {
        LogI() << "System profile saved, path: " << path;
        return true;
    }
}

std::string wso_config::SysConfiguration::getDefaultConfigFilePath(void)
{
    string cstr = getDefaultConfigDirPath();
    cstr += "\\";
    cstr += SYS_CONFIG_FILE_NAME;
    return cstr;
}

std::string wso_config::SysConfiguration::getDefaultConfigDirPath(void)
{
    wchar_t buffer[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buffer);
    std::string path = wtoa(buffer);
    return path;
}


CameraSetting* wso_config::SysConfiguration::getCameraSetting(void) const
{
    return &getImpl().cameraSet;
}

FixationSetting* wso_config::SysConfiguration::getFixationSetting(void) const
{
    return &getImpl().fixationSet;
}

SignalSetting* wso_config::SysConfiguration::getSignalSetting(void) const
{
    return &getImpl().signalSet;
}

GalvanoSetting* wso_config::SysConfiguration::getGalvanoSetting(void) const
{
    return &getImpl().galvanoSet;
}

MeasureSetting* wso_config::SysConfiguration::getMeasureSetting(void) const
{
    return &getImpl().measureSet;
}

LsoCaptureSetting* wso_config::SysConfiguration::getLsoCaptureSetting(void) const
{
    return &getImpl().lsoCaptureSet;
}

LsoDisplaySetting* wso_config::SysConfiguration::getLsoDisplaySetting(void) const
{
    return &getImpl().lsoDisplaySet;
}


SysConfiguration::SystemConfigImpl& wso_config::SysConfiguration::getImpl(void) const
{
    return *d_ptr;
}
