#include "pch.h"
#include "LsoDisplaySettings.h"


using namespace wso_config;

struct LsoDisplaySettings::LsoDisplaySettingsImpl
{
	LsoImageAdjustPreset adjustPreset;
	LsoImageMaskPreset maskPreset;

	LsoDisplaySettingsImpl() {
		initializeLsoDisplaySettingsImpl();
	}

	void initializeLsoDisplaySettingsImpl(void) {}
};


LsoDisplaySettings::LsoDisplaySettings() :
	d_ptr(make_unique<LsoDisplaySettingsImpl>())
{}


wso_config::LsoDisplaySettings::~LsoDisplaySettings() = default;
wso_config::LsoDisplaySettings::LsoDisplaySettings(LsoDisplaySettings&& rhs) = default;
LsoDisplaySettings& wso_config::LsoDisplaySettings::operator=(LsoDisplaySettings&& rhs) = default;


wso_config::LsoDisplaySettings::LsoDisplaySettings(const LsoDisplaySettings& rhs)
	: d_ptr(make_unique<LsoDisplaySettingsImpl>(*rhs.d_ptr))
{}


LsoDisplaySettings& wso_config::LsoDisplaySettings::operator=(const LsoDisplaySettings& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

void wso_config::LsoDisplaySettings::initializeLsoDisplaySettings(void)
{
	resetToDefaultValues();
}

void wso_config::LsoDisplaySettings::resetToDefaultValues(void)
{
	impl().adjustPreset = {};
	impl().maskPreset = {};
	return;
}

bool wso_config::LsoDisplaySettings::importFromBoardProfile(const HbsConfiguration* config)
{
	return true;
}

bool wso_config::LsoDisplaySettings::exportToBoardProfile(HbsConfiguration* config) const
{
	return true;
}

LsoImageMaskPreset* wso_config::LsoDisplaySettings::getImageMaskPreset(void) const
{
	return &impl().maskPreset;
}

void wso_config::LsoDisplaySettings::setImageMaskPreset(const LsoImageMaskPreset& param)
{
	impl().maskPreset = param;
	return;
}

int wso_config::LsoDisplaySettings::getMaskIrRadius(void) const
{
	return impl().maskPreset.mask.irRadius;
}

int wso_config::LsoDisplaySettings::getMaskColorRadius(void) const
{
	return impl().maskPreset.mask.colorRadius;
}

void wso_config::LsoDisplaySettings::setMaskIrRadius(int radius)
{
	impl().maskPreset.mask.irRadius = radius;
	return;
}

void wso_config::LsoDisplaySettings::setMaskColorRadius(int radius)
{
	impl().maskPreset.mask.colorRadius = radius;
	return;
}

LsoImageAdjustPreset* wso_config::LsoDisplaySettings::getImageAdjustPreset(void) const
{
	return &impl().adjustPreset;
}

void wso_config::LsoDisplaySettings::setImageAdjustPreset(const LsoImageAdjustPreset& param)
{
	impl().adjustPreset = param;
	return;
}

int wso_config::LsoDisplaySettings::getAdjustBrightness(void) const
{
	return impl().adjustPreset.adjust.brightness;
}

void wso_config::LsoDisplaySettings::setAdjustBrightness(int bright)
{
	impl().adjustPreset.adjust.brightness = bright;
	return;
}

LsoDisplaySettings::LsoDisplaySettingsImpl& wso_config::LsoDisplaySettings::impl(void) const
{
	return *d_ptr;
}
