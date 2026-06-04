#include "pch.h"
#include "SystemConfig.h"
#include "CameraSetting.h"
#include "FixationSetting.h"
#include "GalvanoSetting.h"
#include "MeasureSetting.h"
#include "LsoCaptureSetting.h"
#include "LsoDisplaySetting.h"

using namespace wso_config;
using namespace std;



struct SystemConfig::SystemConfigImpl
{
    optional<HbsDataProfile*> hbsProfile{ nullopt };

    FixationSetting fixationSet;
    CameraSetting cameraSet;
	GalvanoSetting galvanoSet;

	MeasureSetting measureSet;
	LsoCaptureSetting lsoCaptureSet;
	LsoDisplaySetting lsoDisplaySet;

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

void wso_config::SystemConfig::resetToDefaults(void)
{
	impl().cameraSet.resetToDefaultValues();
	impl().fixationSet.resetToDefaultValues();
    impl().galvanoSet.resetToDefaultValues();

	impl().measureSet.resetToDefaultValues();
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
            impl().galvanoSet.importFromBoardProfile(config);

			impl().measureSet.importFromBoardProfile(config);
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
			impl().galvanoSet.exportToBoardProfile(p);

			impl().measureSet.exportToBoardProfile(p);
			impl().lsoCaptureSet.exportToBoardProfile(p);
			impl().lsoDisplaySet.exportToBoardProfile(p);
			return true;
		}
	}
    return false;
}


CameraSetting* wso_config::SystemConfig::getCameraSetting(void) const
{
    return &impl().cameraSet;
}

FixationSetting* wso_config::SystemConfig::getFixationSetting(void) const
{
    return &impl().fixationSet;
}

GalvanoSetting* wso_config::SystemConfig::getGalvanoSetting(void) const
{
    return &impl().galvanoSet;
}

MeasureSetting* wso_config::SystemConfig::getMeasureSetting(void) const
{
    return &impl().measureSet;
}

LsoCaptureSetting* wso_config::SystemConfig::getLsoCaptureSetting(void) const
{
    return &impl().lsoCaptureSet;
}

LsoDisplaySetting* wso_config::SystemConfig::getLsoDisplaySetting(void) const
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