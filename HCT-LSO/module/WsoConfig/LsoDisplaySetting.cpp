#include "pch.h"
#include "LsoDisplaySetting.h"


using namespace wso_config;

struct LsoDisplaySetting::LsoDisplaySettingImpl
{
	LsoImageAdjustPreset adjustPreset;
	LsoImageMaskPreset maskPreset;

	LsoDisplaySettingImpl() {
		initializeLsoDisplaySettingImpl();
	}

	void initializeLsoDisplaySettingImpl(void) {
	}
};


LsoDisplaySetting::LsoDisplaySetting() :
	d_ptr(make_unique<LsoDisplaySettingImpl>())
{}


wso_config::LsoDisplaySetting::~LsoDisplaySetting() = default;
wso_config::LsoDisplaySetting::LsoDisplaySetting(LsoDisplaySetting&& rhs) = default;
LsoDisplaySetting& wso_config::LsoDisplaySetting::operator=(LsoDisplaySetting&& rhs) = default;


wso_config::LsoDisplaySetting::LsoDisplaySetting(const LsoDisplaySetting& rhs)
	: d_ptr(make_unique<LsoDisplaySettingImpl>(*rhs.d_ptr))
{}


LsoDisplaySetting& wso_config::LsoDisplaySetting::operator=(const LsoDisplaySetting& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

void wso_config::LsoDisplaySetting::initializeLsoDisplaySetting(void)
{
	resetToDefaultValues();
}

void wso_config::LsoDisplaySetting::resetToDefaultValues(void)
{
	impl().adjustPreset = {};
	impl().maskPreset = {};
	return;
}

bool wso_config::LsoDisplaySetting::importFromBoardProfile(const HbsConfiguration* config)
{
	return true;
}

bool wso_config::LsoDisplaySetting::exportToBoardProfile(HbsConfiguration* config) const
{
	return true;
}

LsoImageMaskPreset* wso_config::LsoDisplaySetting::getImageMaskPreset(void) const
{
	return &impl().maskPreset;
}

void wso_config::LsoDisplaySetting::setImageMaskPreset(const LsoImageMaskPreset& param)
{
	impl().maskPreset = param;
	return;
}

int wso_config::LsoDisplaySetting::getMaskIrRadius(void) const
{
	return impl().maskPreset.mask.irRadius;
}

int wso_config::LsoDisplaySetting::getMaskColorRadius(void) const
{
	return impl().maskPreset.mask.colorRadius;
}

void wso_config::LsoDisplaySetting::setMaskIrRadius(int radius)
{
	impl().maskPreset.mask.irRadius = radius;
	return;
}

void wso_config::LsoDisplaySetting::setMaskColorRadius(int radius)
{
	impl().maskPreset.mask.colorRadius = radius;
	return;
}

LsoImageAdjustPreset* wso_config::LsoDisplaySetting::getImageAdjustPreset(void) const
{
	return &impl().adjustPreset;
}

void wso_config::LsoDisplaySetting::setImageAdjustPreset(const LsoImageAdjustPreset& param)
{
	impl().adjustPreset = param;
	return;
}

int wso_config::LsoDisplaySetting::getAdjustBrightness(void) const
{
	return impl().adjustPreset.adjust.brightness;
}

void wso_config::LsoDisplaySetting::setAdjustBrightness(int bright)
{
	impl().adjustPreset.adjust.brightness = bright;
	return;
}

LsoDisplaySetting::LsoDisplaySettingImpl& wso_config::LsoDisplaySetting::impl(void) const
{
	return *d_ptr;
}
