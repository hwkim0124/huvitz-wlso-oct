#include "pch.h"
#include "CameraSettings.h"


using namespace wso_config;

struct CameraSettings::CameraSettingsImpl
{
	CorneaCameraConfigParam param;

	CameraSettingsImpl() {
		initializeCameraSettingsImpl();
	}

	void initializeCameraSettingsImpl(void) {
		param.again_left = IR_CORNEA_AGAIN_INIT;
		param.again_right = IR_CORNEA_AGAIN_INIT;
		param.again_lower = IR_CORNEA_AGAIN_INIT;

		param.dgain_left = IR_CORNEA_DGAIN_INIT;
		param.dgain_right = IR_CORNEA_DGAIN_INIT;
		param.dgain_lower = IR_CORNEA_DGAIN_INIT;
	}
};


CameraSettings::CameraSettings() :
	d_ptr(make_unique<CameraSettingsImpl>())
{}


wso_config::CameraSettings::~CameraSettings() = default;
wso_config::CameraSettings::CameraSettings(CameraSettings&& rhs) = default;
CameraSettings& wso_config::CameraSettings::operator=(CameraSettings&& rhs) = default;


wso_config::CameraSettings::CameraSettings(const CameraSettings& rhs)
	: d_ptr(make_unique<CameraSettingsImpl>(*rhs.d_ptr))
{}


CameraSettings& wso_config::CameraSettings::operator=(const CameraSettings& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_config::CameraSettings::importFromBoardProfile(const HbsConfiguration* config)
{
	if (config) {
		/*
		auto* camera = &config->cornea_camera;
		setCorneaAgain(camera->again);
		setCorneaDgain(camera->dgain);

		for (int i = 0; i < N_WORKING_DOTS; i++) {
			auto* wdot = &config->wdots[i];
			setWdotIntensity(i, wdot->intenisty);
		}
		*/
		return true;
	}
	return false;
}

bool wso_config::CameraSettings::exportToBoardProfile(HbsConfiguration* config) const
{
	if (config) {
		/*
		auto* camera = &config->cornea_camera;
		camera->again = getCorneaAgain();
		camera->dgain = getCorneaDgain();

		for (int i = 0; i < N_WORKING_DOTS; i++) {
			auto* wdot = &config->wdots[i];
			wdot->intenisty = getWdotIntensity(i);
		}
		*/
		return true;
	}
	return false;
}

void wso_config::CameraSettings::resetToDefaultValues(void)
{
	impl().initializeCameraSettingsImpl();
	return;
}

CorneaCameraConfigParam* wso_config::CameraSettings::getCorneaCameraConfigParam(void) const
{
	return &impl().param;
}

void wso_config::CameraSettings::setCorneaCameraConfigParam(const CorneaCameraConfigParam& param)
{
	impl().param = param;
	return;
}

float wso_config::CameraSettings::getCorneaAgain(CameraType type) const
{
	auto value = 0.0f;
	if (type == CameraType::IR_CORNEA_LEFT) {
		value = impl().param.again_left;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		value = impl().param.again_right;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		value = impl().param.again_lower;
	}
	return value;
}

float wso_config::CameraSettings::getCorneaDgain(CameraType type) const
{
	auto value = 0.0f;
	if (type == CameraType::IR_CORNEA_LEFT) {
		value = impl().param.dgain_left;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		value = impl().param.dgain_right;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		value = impl().param.dgain_lower;
	}
	return value;
}


void wso_config::CameraSettings::setCorneaAgain(CameraType type, float value)
{
	if (type == CameraType::IR_CORNEA_LEFT) {
		impl().param.again_left = value;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		impl().param.again_right = value;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		impl().param.again_lower = value;
	}
	return;
}

void wso_config::CameraSettings::setCorneaDgain(CameraType type, float value)
{
	if (type == CameraType::IR_CORNEA_LEFT) {
		impl().param.dgain_left = value;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		impl().param.dgain_right = value;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		impl().param.dgain_lower = value;
	}
	return;
}


CameraSettings::CameraSettingsImpl& wso_config::CameraSettings::impl(void) const
{
	return *d_ptr;
}
