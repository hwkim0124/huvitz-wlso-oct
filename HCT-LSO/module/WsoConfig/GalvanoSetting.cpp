#include "pch.h"
#include "GalvanoSetting.h"


using namespace wso_config;

struct GalvanoSetting::GalvanoSettingImpl
{
	OctGalvanoConfigParam param;

	GalvanoSettingImpl() {
		initializeGalvanoSettingImpl();
	}

	void initializeGalvanoSettingImpl(void) {
		param.xoffset = 0.0f;
		param.yoffset = 0.0f;
		for (int i = 0; i < OCT_GALVANO_SCALE_NUM; i++) {
			param.xscale[i] = 1.0f;
			param.yscale[i] = 1.0f;
		}
	}
};



GalvanoSetting::GalvanoSetting() :
	d_ptr(make_unique<GalvanoSettingImpl>())
{
}


wso_config::GalvanoSetting::~GalvanoSetting() = default;
wso_config::GalvanoSetting::GalvanoSetting(GalvanoSetting&& rhs) = default;
GalvanoSetting& wso_config::GalvanoSetting::operator=(GalvanoSetting&& rhs) = default;


wso_config::GalvanoSetting::GalvanoSetting(const GalvanoSetting& rhs)
	: d_ptr(make_unique<GalvanoSettingImpl>(*rhs.d_ptr))
{
}


GalvanoSetting& wso_config::GalvanoSetting::operator=(const GalvanoSetting& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_config::GalvanoSetting::importFromBoardProfile(const HbsConfiguration* config)
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

bool wso_config::GalvanoSetting::exportToBoardProfile(HbsConfiguration* config) const
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

OctGalvanoConfigParam* wso_config::GalvanoSetting::getOctGalvanoConfigParam(void) const
{
	return &impl().param;
}

void wso_config::GalvanoSetting::setOctGalvanoConfigParam(const OctGalvanoConfigParam& param)
{
	impl().param = param;
	return;
}

void wso_config::GalvanoSetting::setScaleX(int index, float value)
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		impl().param.xscale[index] = min(max(value, OCT_GALVANO_SCALE_MIN), OCT_GALVANO_SCALE_MAX);
	}
	return;
}

void wso_config::GalvanoSetting::setScaleY(int index, float value)
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		impl().param.yscale[index] = min(max(value, OCT_GALVANO_SCALE_MIN), OCT_GALVANO_SCALE_MAX);
	}
	return;
}

float wso_config::GalvanoSetting::getScaleX(int index) const
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		auto value = impl().param.xscale[index];
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		return value;
	}
	return 0.0f;
}

float wso_config::GalvanoSetting::getScaleY(int index) const
{
	if (index >= 0 && index < OCT_GALVANO_SCALE_NUM) {
		auto value = impl().param.yscale[index];
		value = (value >= OCT_GALVANO_SCALE_MIN && value <= OCT_GALVANO_SCALE_MAX) ? value : 1.0f;
		return value;
	}
	return 0.0f;
}

void wso_config::GalvanoSetting::setOffsetX(float value)
{
	impl().param.xoffset = value;
	return;
}

void wso_config::GalvanoSetting::setOffsetY(float value)
{
	impl().param.yoffset = value;
	return;
}

float wso_config::GalvanoSetting::getOffsetX(void) const
{
	return impl().param.xoffset;
}

float wso_config::GalvanoSetting::getOffsetY(void) const
{
	return impl().param.yoffset;
}

void wso_config::GalvanoSetting::resetToDefaultValues(void)
{
	impl().initializeGalvanoSettingImpl();
	return;
}


GalvanoSetting::GalvanoSettingImpl& wso_config::GalvanoSetting::impl(void) const
{
	return *d_ptr;
}
