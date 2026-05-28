#include "pch.h"
#include "WsoSettings.h"

using namespace wso_domain;


std::mutex WsoSettings::singleMutex_;


struct WsoSettings::WsoSettingsImpl
{
	bool userModeOn;
	bool octTasksDefaultPresetOn;
	bool octEnfaceImageCorrectOn;

	WsoSettingsImpl()
	{
		userModeOn = true;
		octTasksDefaultPresetOn = false;
		octEnfaceImageCorrectOn = false;
	}
};


wso_domain::WsoSettings::WsoSettings() :
	d_ptr(std::make_unique<WsoSettingsImpl>())
{
}


wso_domain::WsoSettings::~WsoSettings()
{
}


WsoSettings* wso_domain::WsoSettings::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static WsoSettings instance;
	return &instance;
}

bool wso_domain::WsoSettings::isUserModeOn(void)
{
	return impl().userModeOn;
}

bool wso_domain::WsoSettings::isOctTasksDefaultPresetOn(void)
{
	return impl().octTasksDefaultPresetOn;
}

bool wso_domain::WsoSettings::isOctEnfaceImageCorrectOn(void)
{
	return false;
}

void wso_domain::WsoSettings::applyUserModeSettings(bool flag)
{
	impl().userModeOn = flag;

	if (flag) {
		setOctTasksDefaultPresetOn(true);
		setOctEnfaceImageCorrectOn(true);
	}
	else {
		setOctTasksDefaultPresetOn(false);
		setOctEnfaceImageCorrectOn(false);
	}
	return;
}

void wso_domain::WsoSettings::setUserModeOn(bool flag)
{
	impl().userModeOn = flag;
	return;
}

void wso_domain::WsoSettings::setOctTasksDefaultPresetOn(bool flag)
{
	impl().octTasksDefaultPresetOn = flag;
	return;
}

void wso_domain::WsoSettings::setOctEnfaceImageCorrectOn(bool flag)
{
	impl().octEnfaceImageCorrectOn = flag;
	return;
}


WsoSettings::WsoSettingsImpl& wso_domain::WsoSettings::impl(void) const
{
	return *d_ptr;
}
