#include "pch.h"
#include "AlignmentSettings.h"


using namespace wso_config;

struct AlignmentSettings::AlignmentSettingsImpl
{
	EyeAlignConfigParam param;
	PupilModelConfig model;

	AlignmentSettingsImpl() {
		initializeAlignmentSettingsImpl();
	}

	void initializeAlignmentSettingsImpl(void) {
		// The in-class initializers of EyeAlignConfigParam are the defaults.
		param = EyeAlignConfigParam();
		model = PupilModelConfig();
	}
};


AlignmentSettings::AlignmentSettings() :
	d_ptr(make_unique<AlignmentSettingsImpl>())
{}


wso_config::AlignmentSettings::~AlignmentSettings() = default;
wso_config::AlignmentSettings::AlignmentSettings(AlignmentSettings&& rhs) = default;
AlignmentSettings& wso_config::AlignmentSettings::operator=(AlignmentSettings&& rhs) = default;


wso_config::AlignmentSettings::AlignmentSettings(const AlignmentSettings& rhs)
	: d_ptr(make_unique<AlignmentSettingsImpl>(*rhs.d_ptr))
{}


AlignmentSettings& wso_config::AlignmentSettings::operator=(const AlignmentSettings& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

void wso_config::AlignmentSettings::resetToDefaultValues(void)
{
	impl().initializeAlignmentSettingsImpl();
}

EyeAlignConfigParam* wso_config::AlignmentSettings::getParam(void) const
{
	return &impl().param;
}

void wso_config::AlignmentSettings::setParam(const EyeAlignConfigParam& param)
{
	impl().param = param;
}

PupilDetectParam wso_config::AlignmentSettings::getDetectParam(CameraType type) const
{
	switch (type) {
	case CameraType::IR_CORNEA_LEFT:  return impl().param.detLeft;
	case CameraType::IR_CORNEA_RIGHT: return impl().param.detRight;
	case CameraType::IR_CORNEA_LOWER: return impl().param.detLower;
	default: return PupilDetectParam();
	}
}

void wso_config::AlignmentSettings::setDetectParam(CameraType type, const PupilDetectParam& param)
{
	switch (type) {
	case CameraType::IR_CORNEA_LEFT:  impl().param.detLeft = param; break;
	case CameraType::IR_CORNEA_RIGHT: impl().param.detRight = param; break;
	case CameraType::IR_CORNEA_LOWER: impl().param.detLower = param; break;
	default: break;
	}
}

PupilModelConfig wso_config::AlignmentSettings::getModelConfig(void) const
{
	return impl().model;
}

void wso_config::AlignmentSettings::setModelConfig(const PupilModelConfig& config)
{
	impl().model = config;
}

AlignmentSettings::AlignmentSettingsImpl& wso_config::AlignmentSettings::impl(void) const
{
	return *d_ptr;
}
