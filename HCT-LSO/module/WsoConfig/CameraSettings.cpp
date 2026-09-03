#include "pch.h"
#include "CameraSettings.h"


using namespace wso_config;

struct CameraSettings::CameraSettingsImpl
{
	CorneaCameraConfigParam cornea;
	RetinaCameraConfigParam retina;

	CameraSettingsImpl() {
		initializeCameraSettingsImpl();
	}

	void initializeCameraSettingsImpl(void) {
		cornea.again_left = IR_CORNEA_AGAIN_INIT;
		cornea.again_right = IR_CORNEA_AGAIN_INIT;
		cornea.again_lower = IR_CORNEA_AGAIN_INIT;

		cornea.dgain_left = IR_CORNEA_DGAIN_INIT;
		cornea.dgain_right = IR_CORNEA_DGAIN_INIT;
		cornea.dgain_lower = IR_CORNEA_DGAIN_INIT;

		cornea.exposTime_left = IR_CORNEA_EXPOSURE_INIT;
		cornea.exposTime_right = IR_CORNEA_EXPOSURE_INIT;
		cornea.exposTime_lower = IR_CORNEA_EXPOSURE_INIT;

		retina.again = IR_RETINA_AGAIN_INIT;
		retina.dgain = IR_RETINA_DGAIN_INIT;
		retina.exposTime = IR_RETINA_EXPOSURE_INIT;
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
	return &impl().cornea;
}

RetinaCameraConfigParam* wso_config::CameraSettings::getRetinaCameraConfigParam(void) const
{
	return &impl().retina;
}

void wso_config::CameraSettings::setCorneaCameraConfigParam(const CorneaCameraConfigParam& param)
{
	impl().cornea = param;
	return;
}

void wso_config::CameraSettings::setRetinaCameraConfigParam(const RetinaCameraConfigParam& param)
{
	impl().retina = param;
	return;
}

float wso_config::CameraSettings::getRetinaAgain(void) const
{
	return impl().retina.again;
}

float wso_config::CameraSettings::getRetinaDgain(void) const
{
	return impl().retina.dgain;
}

float wso_config::CameraSettings::getRetinaExposureTime(void) const
{
	return impl().retina.exposTime;
}

float wso_config::CameraSettings::getCorneaAgain(CameraType type) const
{
	auto value = 0.0f;
	if (type == CameraType::IR_CORNEA_LEFT) {
		value = impl().cornea.again_left;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		value = impl().cornea.again_right;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		value = impl().cornea.again_lower;
	}
	return value;
}

float wso_config::CameraSettings::getCorneaDgain(CameraType type) const
{
	auto value = 0.0f;
	if (type == CameraType::IR_CORNEA_LEFT) {
		value = impl().cornea.dgain_left;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		value = impl().cornea.dgain_right;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		value = impl().cornea.dgain_lower;
	}
	return value;
}

float wso_config::CameraSettings::getCorneaExposureTime(CameraType type) const
{
	auto value = 0.0f;
	if (type == CameraType::IR_CORNEA_LEFT) {
		value = impl().cornea.exposTime_left;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		value = impl().cornea.exposTime_right;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		value = impl().cornea.exposTime_lower;
	}
	return value;
}

void wso_config::CameraSettings::setRetinaAgain(float value)
{
	impl().retina.again = value;
}

void wso_config::CameraSettings::setRetinaDgain(float value)
{
	impl().retina.dgain = value;
}

void wso_config::CameraSettings::setRetinaExposureTime(float value)
{
	impl().retina.exposTime = value;
}

void wso_config::CameraSettings::setCorneaAgain(CameraType type, float value)
{
	if (type == CameraType::IR_CORNEA_LEFT) {
		impl().cornea.again_left = value;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		impl().cornea.again_right = value;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		impl().cornea.again_lower = value;
	}
	return;
}

void wso_config::CameraSettings::setCorneaDgain(CameraType type, float value)
{
	if (type == CameraType::IR_CORNEA_LEFT) {
		impl().cornea.dgain_left = value;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		impl().cornea.dgain_right = value;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		impl().cornea.dgain_lower = value;
	}
	return;
}

void wso_config::CameraSettings::setCorneaExposureTime(CameraType type, float value)
{
	if (type == CameraType::IR_CORNEA_LEFT) {
		impl().cornea.exposTime_left = value;
	}
	else if (type == CameraType::IR_CORNEA_RIGHT) {
		impl().cornea.exposTime_right = value;
	}
	else if (type == CameraType::IR_CORNEA_LOWER) {
		impl().cornea.exposTime_lower = value;
	}
	return;
}

CameraSettings::CameraSettingsImpl& wso_config::CameraSettings::impl(void) const
{
	return *d_ptr;
}
