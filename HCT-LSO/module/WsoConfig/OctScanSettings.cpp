#include "pch.h"
#include "OctScanSettings.h"


using namespace wso_config;

struct OctScanSettings::OctScanSettingsImpl
{
	double wavelengthParams[WAVELENGTH_PARAMS_SIZE] = { 0.0 };
	double dispersionParams[DISPERSION_PARAMS_SIZE] = { 0.0 };

	OctScanSettingsImpl() {}
};


OctScanSettings::OctScanSettings() :
	d_ptr(make_unique<OctScanSettingsImpl>())
{
	initializeOctScanSettings();
}


wso_config::OctScanSettings::~OctScanSettings() = default;
wso_config::OctScanSettings::OctScanSettings(OctScanSettings&& rhs) = default;
OctScanSettings& wso_config::OctScanSettings::operator=(OctScanSettings&& rhs) = default;


wso_config::OctScanSettings::OctScanSettings(const OctScanSettings& rhs)
	: d_ptr(make_unique<OctScanSettingsImpl>(*rhs.d_ptr))
{}


OctScanSettings& wso_config::OctScanSettings::operator=(const OctScanSettings& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void wso_config::OctScanSettings::initializeOctScanSettings(void)
{
	resetToDefaultValues();
}

void wso_config::OctScanSettings::resetToDefaultValues(void)
{
	for (int i = 0; i < WAVELENGTH_PARAMS_SIZE; i++) {
		getImpl().wavelengthParams[i] = 0.0;
	}

	for (int i = 0; i < DISPERSION_PARAMS_SIZE; i++) {
		getImpl().dispersionParams[i] = 0.0;
	}
	return;
}


double wso_config::OctScanSettings::getWavelengthParameter(int index) const
{
	if (index >= 0 && index < WAVELENGTH_PARAMS_SIZE) {
		return getImpl().wavelengthParams[index];
	}
	return 0.0;
}


void wso_config::OctScanSettings::getWavelengthParameter(double* param) const
{
	for (int i = 0; i < WAVELENGTH_PARAMS_SIZE; i++) {
		param[i] = getImpl().wavelengthParams[i];
	}
	return;
}


double wso_config::OctScanSettings::getDispersionParameter(int index) const
{
	if (index >= 0 && index < DISPERSION_PARAMS_SIZE) {
		return getImpl().dispersionParams[index];
	}
	return 0.0;
}


void wso_config::OctScanSettings::getDispersionParameter(double* param) const
{
	for (int i = 0; i < DISPERSION_PARAMS_SIZE; i++) {
		param[i] = getImpl().dispersionParams[i];
	}
	return;
}


void wso_config::OctScanSettings::setWavelengthParameter(int index, double value)
{
	if (index >= 0 && index < WAVELENGTH_PARAMS_SIZE) {
		getImpl().wavelengthParams[index] = value;
	}
	return;
}


void wso_config::OctScanSettings::setDispersionParameter(int index, double value)
{
	if (index >= 0 && index < DISPERSION_PARAMS_SIZE) {
		getImpl().dispersionParams[index] = value;
	}
	return;
}


OctScanSettings::OctScanSettingsImpl& wso_config::OctScanSettings::getImpl(void) const
{
	return *d_ptr;
}
