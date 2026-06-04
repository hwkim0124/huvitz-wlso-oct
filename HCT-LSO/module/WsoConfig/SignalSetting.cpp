#include "pch.h"
#include "SignalSetting.h"


using namespace wso_config;

struct SignalSetting::SignalSettingImpl
{
	double wavelengthParams[WAVELENGTH_PARAMS_SIZE] = { 0.0 };
	double dispersionParams[DISPERSION_PARAMS_SIZE] = { 0.0 };

	SignalSettingImpl() {
	}
};


SignalSetting::SignalSetting() :
	d_ptr(make_unique<SignalSettingImpl>())
{
	initializeSignalSetting();
}


wso_config::SignalSetting::~SignalSetting() = default;
wso_config::SignalSetting::SignalSetting(SignalSetting && rhs) = default;
SignalSetting & wso_config::SignalSetting::operator=(SignalSetting && rhs) = default;


wso_config::SignalSetting::SignalSetting(const SignalSetting & rhs)
	: d_ptr(make_unique<SignalSettingImpl>(*rhs.d_ptr))
{
}


SignalSetting & wso_config::SignalSetting::operator=(const SignalSetting & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void wso_config::SignalSetting::initializeSignalSetting(void)
{
	resetToDefaultValues();
}

void wso_config::SignalSetting::resetToDefaultValues(void)
{
	for (int i = 0; i < WAVELENGTH_PARAMS_SIZE; i++) {
		getImpl().wavelengthParams[i] = 0.0;
	}

	for (int i = 0; i < DISPERSION_PARAMS_SIZE; i++) {
		getImpl().dispersionParams[i] = 0.0;
	}
	return;
}


double wso_config::SignalSetting::getWavelengthParameter(int index) const
{
	if (index >= 0 && index < WAVELENGTH_PARAMS_SIZE) {
		return getImpl().wavelengthParams[index];
	}
	return 0.0;
}


void wso_config::SignalSetting::getWavelengthParameter(double * param) const
{
	for (int i = 0; i < WAVELENGTH_PARAMS_SIZE; i++) {
		param[i] = getImpl().wavelengthParams[i];
	}
	return;
}


double wso_config::SignalSetting::getDispersionParameter(int index) const
{
	if (index >= 0 && index < DISPERSION_PARAMS_SIZE) {
		return getImpl().dispersionParams[index];
	}
	return 0.0;
}


void wso_config::SignalSetting::getDispersionParameter(double * param) const
{
	for (int i = 0; i < DISPERSION_PARAMS_SIZE; i++) {
		param[i] = getImpl().dispersionParams[i];
	}
	return;
}


void wso_config::SignalSetting::setWavelengthParameter(int index, double value)
{
	if (index >= 0 && index < WAVELENGTH_PARAMS_SIZE) {
		getImpl().wavelengthParams[index] = value;
	}
	return;
}


void wso_config::SignalSetting::setDispersionParameter(int index, double value)
{
	if (index >= 0 && index < DISPERSION_PARAMS_SIZE) {
		getImpl().dispersionParams[index] = value;
	}
	return;
}


SignalSetting::SignalSettingImpl& wso_config::SignalSetting::getImpl(void) const
{
	return *d_ptr;
}
