#include "pch.h"
#include "CameraSetting.h"


using namespace wso_config;

struct CameraSetting::CameraSettingImpl
{
	CorneaCameraConfigParam param;

	CameraSettingImpl() {
		initializeCameraSettingImpl();
	}

	void initializeCameraSettingImpl(void) {
		param.again = IR_CORNEA_AGAIN_INIT;
		param.dgain = IR_CORNEA_DGAIN_INIT;

		param.wdots[0] = IR_WDOT_INTENSITY_INIT;
		param.wdots[1] = IR_WDOT_INTENSITY_INIT;
	}
};


CameraSetting::CameraSetting() :
	d_ptr(make_unique<CameraSettingImpl>())
{
}


wso_config::CameraSetting::~CameraSetting() = default;
wso_config::CameraSetting::CameraSetting(CameraSetting&& rhs) = default;
CameraSetting& wso_config::CameraSetting::operator=(CameraSetting&& rhs) = default;


wso_config::CameraSetting::CameraSetting(const CameraSetting& rhs)
	: d_ptr(make_unique<CameraSettingImpl>(*rhs.d_ptr))
{
}


CameraSetting& wso_config::CameraSetting::operator=(const CameraSetting& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_config::CameraSetting::importFromBoardProfile(const HbsConfiguration* config)
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

bool wso_config::CameraSetting::exportToBoardProfile(HbsConfiguration* config) const
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

void wso_config::CameraSetting::resetToDefaultValues(void)
{
	impl().initializeCameraSettingImpl();
	return;
}

CorneaCameraConfigParam* wso_config::CameraSetting::getCorneaCameraConfigParam(void) const
{
	return &impl().param;
}

void wso_config::CameraSetting::setCorneaCameraConfigParam(const CorneaCameraConfigParam& param)
{
	impl().param = param;
	return;
}

float wso_config::CameraSetting::getCorneaAgain(void) const
{
	return impl().param.again;
}

float wso_config::CameraSetting::getCorneaDgain(void) const
{
	return impl().param.dgain;
}

int wso_config::CameraSetting::getWdotIntensity(int index) const
{
	return impl().param.wdots[index];
}

void wso_config::CameraSetting::setCorneaAgain(float value)
{
	impl().param.again = value;
}

void wso_config::CameraSetting::setCorneaDgain(float value)
{
	impl().param.dgain = value;
}

void wso_config::CameraSetting::setWdotIntensity(int index, int value)
{
	impl().param.wdots[index] = value;
}

CameraSetting::CameraSettingImpl& wso_config::CameraSetting::impl(void) const
{
	return *d_ptr;
}
