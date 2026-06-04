#include "pch.h"
#include "MeasureSetting.h"


using namespace wso_config;

struct MeasureSetting::MeasureSettingImpl
{
	StageReadyPositionPreset ready;

	MeasureSettingImpl() {
		initializeMeasureSettingImpl();
	}

	void initializeMeasureSettingImpl(void) {}
};


MeasureSetting::MeasureSetting() :
	d_ptr(make_unique<MeasureSettingImpl>())
{}


wso_config::MeasureSetting::~MeasureSetting() = default;
wso_config::MeasureSetting::MeasureSetting(MeasureSetting&& rhs) = default;
MeasureSetting& wso_config::MeasureSetting::operator=(MeasureSetting&& rhs) = default;


wso_config::MeasureSetting::MeasureSetting(const MeasureSetting& rhs)
	: d_ptr(make_unique<MeasureSettingImpl>(*rhs.d_ptr))
{}


MeasureSetting& wso_config::MeasureSetting::operator=(const MeasureSetting& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

void wso_config::MeasureSetting::initializeMeasureSetting(void)
{
	resetToDefaultValues();
	return;
}

void wso_config::MeasureSetting::resetToDefaultValues(void)
{
	auto ready = StageReadyPositionPreset{
	{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
	};
	impl().ready = ready;
	return;
}

bool wso_config::MeasureSetting::importFromBoardProfile(const HbsConfiguration * config)
{
	if (config) {
		return true;
	}
	return false;
}

bool wso_config::MeasureSetting::exportToBoardProfile(HbsConfiguration* config) const
{
	if (config) {
		return true;
	}
	return false;
}

StageReadyPositionPreset* wso_config::MeasureSetting::getStageReadyPositionPreset(void) const
{
	return &impl().ready;
}

void wso_config::MeasureSetting::setStageReadyPositionPreset(const StageReadyPositionPreset& preset)
{
	impl().ready = preset;
	return;
}

std::tuple<int, int, int> wso_config::MeasureSetting::getCenterPos(void) const
{
	return std::tuple<int, int, int>(impl().ready.center.x, impl().ready.center.y, impl().ready.center.z);
}

std::tuple<int, int, int> wso_config::MeasureSetting::getReadyPosOD(void) const
{
	return std::tuple<int, int, int>(impl().ready.od.x, impl().ready.od.y, impl().ready.od.z);
}

std::tuple<int, int, int> wso_config::MeasureSetting::getReadyPosOS(void) const
{
	return std::tuple<int, int, int>(impl().ready.os.x, impl().ready.os.y, impl().ready.os.z);
}

std::tuple<int, int, int> wso_config::MeasureSetting::getReadyPosME(void) const
{
	return std::tuple<int, int, int>(impl().ready.me.x, impl().ready.me.y, impl().ready.me.z);
}

void wso_config::MeasureSetting::setCenterPos(std::tuple<int, int, int> pos)
{
	impl().ready.center.x = std::get<0>(pos);
	impl().ready.center.y = std::get<1>(pos);
	impl().ready.center.z = std::get<2>(pos);
	return;
}

void wso_config::MeasureSetting::setReadyPosOD(std::tuple<int, int, int> pos)
{
	impl().ready.od.x = std::get<0>(pos);
	impl().ready.od.y = std::get<1>(pos);
	impl().ready.od.z = std::get<2>(pos);
	return;
}

void wso_config::MeasureSetting::setReadyPosOS(std::tuple<int, int, int> pos)
{
	impl().ready.os.x = std::get<0>(pos);
	impl().ready.os.y = std::get<1>(pos);
	impl().ready.os.z = std::get<2>(pos);
	return;
}

void wso_config::MeasureSetting::setReadyPosME(std::tuple<int, int, int> pos)
{
	impl().ready.me.x = std::get<0>(pos);
	impl().ready.me.y = std::get<1>(pos);
	impl().ready.me.z = std::get<2>(pos);
	return;
}

MeasureSetting::MeasureSettingImpl& wso_config::MeasureSetting::impl(void) const
{
	return *d_ptr;
}
