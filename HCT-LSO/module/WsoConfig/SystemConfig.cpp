#include "pch.h"
#include "SystemConfig.h"
#include "SysConfigFile.h"
#include "CameraSettings.h"
#include "FixationSettings.h"
#include "LsoCaptureSettings.h"
#include "LsoDisplaySettings.h"

using namespace wso_config;
using namespace std;



struct SystemConfig::SystemConfigImpl
{
    optional<HbsDataProfile*> hbsProfile{ nullopt };

    FixationSettings fixationSet;
    CameraSettings cameraSet;
	LsoCaptureSettings lsoCaptureSet;
	LsoDisplaySettings lsoDisplaySet;

    SysConfigFile configFile;

    SystemConfigImpl()
    {
    }
};



SystemConfig::SystemConfig() :
    d_ptr(make_unique<SystemConfigImpl>())
{
}


wso_config::SystemConfig::~SystemConfig() = default;


SystemConfig* wso_config::SystemConfig::getInstance(void)
{
    // Instance is constructed by public static method.
    // Static local variable initialization is thread-safe 
    // and will be initailized only once. 
    static SystemConfig instance;
    return &instance;
}

void wso_config::SystemConfig::setupBoardProfile(HbsDataProfile* profile)
{
    impl().hbsProfile = profile;
    return;
}

void wso_config::SystemConfig::resetToDefaultValues(void)
{
	impl().cameraSet.resetToDefaultValues();
	impl().fixationSet.resetToDefaultValues();
	impl().lsoCaptureSet.resetToDefaultValues();
	impl().lsoDisplaySet.resetToDefaultValues();
    return;
}


bool wso_config::SystemConfig::updateFromBoardProfile(HbsDataProfile* profile)
{
    if (profile) {
        if (auto* config = profile->getHbsConfiguration(); config) {
            impl().fixationSet.importFromBoardProfile(config);
            impl().cameraSet.importFromBoardProfile(config);
			impl().lsoCaptureSet.importFromBoardProfile(config);
            impl().lsoDisplaySet.importFromBoardProfile(config);
            return true;
        }
    }
    return false;
}


bool wso_config::SystemConfig::uploadToBoardProfile(HbsDataProfile* profile)
{
    if (profile) {
		if (auto* config = profile->getHbsConfiguration(); config) {
            auto* p = const_cast<HbsConfiguration*>(config);
			impl().fixationSet.exportToBoardProfile(p);
			impl().cameraSet.exportToBoardProfile(p);
			impl().lsoCaptureSet.exportToBoardProfile(p);
			impl().lsoDisplaySet.exportToBoardProfile(p);
			return true;
		}
	}
    return false;
}

bool wso_config::SystemConfig::loadSysConfigFile(const char* name)
{
    string path = (name == nullptr ? getDefaultSysConfigFilePath() : name);
    if (!impl().configFile.loadSystemConfig(path.c_str(), this)) {
        LogW() << "Failed to load system config file, path: " << path;
        return false;
    }
    else {
        LogI() << "System config file loaded, path: " << path;
        return true;
    }
}

bool wso_config::SystemConfig::saveSysConfigFile(const char* name)
{
    string path = (name == nullptr ? getDefaultSysConfigFilePath() : name);
    if (!impl().configFile.saveSystemConfig(path.c_str(), this)) {
        LogW() << "Failed to save system config file, path: " << path;
        return false;
    }
    else {
        LogI() << "System config file saved, path: " << path;
        return true;
    }
}

std::string wso_config::SystemConfig::getDefaultSysConfigFilePath(void)
{
    string cstr = getDefaultSysConfigDirPath();
    cstr += "\\";
    cstr += SYS_CONFIG_FILE_NAME;
    return cstr;
}

std::string wso_config::SystemConfig::getDefaultSysConfigDirPath(void)
{
    wchar_t buffer[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buffer);
    std::string path = wtoa(buffer);
    return path;
}


CameraSettings* wso_config::SystemConfig::getCameraSettings(void) const
{
    return &impl().cameraSet;
}

FixationSettings* wso_config::SystemConfig::getFixationSettings(void) const
{
    return &impl().fixationSet;
}

LsoCaptureSettings* wso_config::SystemConfig::getLsoCaptureSettings(void) const
{
    return &impl().lsoCaptureSet;
}

LsoDisplaySettings* wso_config::SystemConfig::getLsoDisplaySettings(void) const
{
    return &impl().lsoDisplaySet;
}

HbsDataProfile* wso_config::SystemConfig::getBoardProfile(void) const
{
    if (impl().hbsProfile.has_value())
	{
		return *impl().hbsProfile;
	}
    return nullptr;
}

SystemConfig::SystemConfigImpl& wso_config::SystemConfig::impl(void) const
{
    return *d_ptr;
}