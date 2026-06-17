#include "pch.h"
#include "GalvanoSettings.h"


using namespace wso_config;

struct GalvanoSettings::GalvanoSettingsImpl
{
	OctGalvanoConfigParam param;

	GalvanoSettingsImpl() {
		initializeGalvanoSettingsImpl();
	}

	void initializeGalvanoSettingsImpl(void) {
		param.xoffset = 0.0f;
		param.yoffset = 0.0f;
		for (int i = 0; i < OCT_GALVANO_SCALE_NUM; i++) {
			param.xscale[i] = 1.0f;
			param.yscale[i] = 1.0f;
		}
	}
};



GalvanoSettings::GalvanoSettings() :
	d_ptr(make_unique<GalvanoSettingsImpl>())
{}


wso_config::GalvanoSettings::~GalvanoSettings() = default;
wso_config::GalvanoSettings::GalvanoSettings(GalvanoSettings&& rhs) = default;
GalvanoSettings& wso_config::GalvanoSettings::operator=(GalvanoSettings&& rhs) = default;


wso_config::GalvanoSettings::GalvanoSettings(const GalvanoSettings& rhs)
	: d_ptr(make_unique<GalvanoSettingsImpl>(*rhs.d_ptr))
{}


GalvanoSettings& wso_config::GalvanoSettings::operator=(const GalvanoSettings& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_config::GalvanoSettings::importFromBoardProfile(const HbsConfiguration* config)
{
	if (config) {
		/*
		auto* galvano = &config->oct_galvano;
		for (int i = 0; i < OCT_GALVANO_SCALE_NUM; i++) {
			setScaleX(i, galvano->xscales[i]);
			setScaleY(i, galvano->yscales[i]);
		}
		setOffsetX(galvano->xoffset);
		setOffsetY(galvano->yoffset);
		*/
		return true;
	}
	return false;
}

bool wso_config::GalvanoSettings::exportToBoardProfile(HbsConfiguration* config) const
{
	if (config) {
		/*
		auto* galvano = &config->oct_galvano;

		for (int i = 0; i < OCT_GALVANO_SCALE_NUM; i++) {
			galvano->xscales[i] = getScaleX(i);
			galvano->yscales[i] = getScaleY(i);
		}
		galvano->xoffset = getOffsetX();
		galvano->yoffset = getOffsetY();
		*/
		return true;
	}
	return false;
}

OctGalvanoConfigParam* wso_config::GalvanoSettings::getOctGalvanoConfigParam(void) const
{
	return &impl().param;
}

void wso_config::GalvanoSettings::setOctGalvanoConfigParam(const OctGalvanoConfigParam& param)
{
	impl().param = param;
	return;
}

void wso_config::GalvanoSettings::setScaleX(int index, float value)
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		impl().param.xscale[index] = min(max(value, OCT_GALVANO_SCALE_MIN), OCT_GALVANO_SCALE_MAX);
	}
	return;
}

void wso_config::GalvanoSettings::setScaleY(int index, float value)
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		impl().param.yscale[index] = min(max(value, OCT_GALVANO_SCALE_MIN), OCT_GALVANO_SCALE_MAX);
	}
	return;
}

float wso_config::GalvanoSettings::getScaleX(int index) const
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		auto value = impl().param.xscale[index];
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		return value;
	}
	return 0.0f;
}

float wso_config::GalvanoSettings::getScaleY(int index) const
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		auto value = impl().param.yscale[index];
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		return value;
	}
	return 0.0f;
}

void wso_config::GalvanoSettings::setOffsetX(float value)
{
	impl().param.xoffset = value;
	return;
}

void wso_config::GalvanoSettings::setOffsetY(float value)
{
	impl().param.yoffset = value;
	return;
}

float wso_config::GalvanoSettings::getOffsetX(void) const
{
	return impl().param.xoffset;
}

float wso_config::GalvanoSettings::getOffsetY(void) const
{
	return impl().param.yoffset;
}

void wso_config::GalvanoSettings::resetToDefaultValues(void)
{
	impl().initializeGalvanoSettingsImpl();
	return;
}


GalvanoSettings::GalvanoSettingsImpl& wso_config::GalvanoSettings::impl(void) const
{
	return *d_ptr;
}
