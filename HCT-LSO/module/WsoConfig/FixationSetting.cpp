#include "pch.h"
#include "FixationSetting.h"


using namespace wso_config;
using namespace std;


struct FixationSetting::FixationSettingImpl
{
	InternalFixationPreset param;

	FixationSettingImpl() {
	}
};


FixationSetting::FixationSetting() :
	d_ptr(make_unique<FixationSettingImpl>())
{
	initializeFixationSetting();
}


wso_config::FixationSetting::~FixationSetting() = default;
wso_config::FixationSetting::FixationSetting(FixationSetting&& rhs) = default;
FixationSetting& wso_config::FixationSetting::operator=(FixationSetting&& rhs) = default;


wso_config::FixationSetting::FixationSetting(const FixationSetting& rhs)
	: d_ptr(make_unique<FixationSettingImpl>(*rhs.d_ptr))
{
}


FixationSetting& wso_config::FixationSetting::operator=(const FixationSetting& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void wso_config::FixationSetting::initializeFixationSetting(void)
{
	resetToDefaultValues();
	return;
}


void wso_config::FixationSetting::resetToDefaultValues(void)
{
	auto param = InternalFixationPreset{
		true, false,
		4000, 5000, 60, 2,
		{67, 63}, {9, 0}, {26, -3},
		{-40, 0}, {-26, -19}, {-26, 20},
		{20, 0}, {6, -19}, {6, 20},
		{67, 63}, {-9, 0}, {-26, -3},
		{-20, 0}, {-6, -19}, {-6, 20},
		{40, 0}, {26, -19}, {26, 20},
	};

	impl().param = param;
	return;
}

bool wso_config::FixationSetting::importFromBoardProfile(const HbsConfiguration* config)
{
	if (config) {
		/*
		for (int i = 0; i < 2; i++) {
			auto* preset = &config->fixation_preset[i];
			auto side = (i == 0 ? EyeSide::OD : EyeSide::OS);

			setCenter(side, { preset->center.row, preset->center.col });
			setFundus(side, { preset->fundus.row, preset->fundus.col });
			setOpticDisc(side, { preset->optic_disc.row, preset->optic_disc.col });
			setLeftSide(side, { preset->left_side.row, preset->left_side.col });
			setLeftTop(side, { preset->left_top.row, preset->left_top.col });
			setLeftBottom(side, { preset->left_bottom.row, preset->left_bottom.col });
			setRightSide(side, { preset->right_side.row, preset->right_side.col });
			setRightTop(side, { preset->right_top.row, preset->right_top.col });
			setRightBottom(side, { preset->right_bottom.row, preset->right_bottom.col });
		}

		auto* setup = &config->fixation_setup;
		setBrightness(setup->bright);
		setBlinkPeriod(setup->period);
		setBlinkOnTime(setup->on_time);
		setFixationType(setup->fix_type);
		useLcdBlinkOn(true, (bool)setup->blink_on);
		useLcdFixation(true, (bool)setup->lcdfix_on);
		*/
		return true;
	}
	return false;
}

bool wso_config::FixationSetting::exportToBoardProfile(HbsConfiguration* config) const
{
	if (config) {
		/*
		for (int i = 0; i < 2; i++) {
			auto* preset = &config->fixation_preset[i];
			auto side = (i == 0 ? EyeSide::OD : EyeSide::OS);

			auto center = getCenter(side);
			auto fundus = getFundus(side);
			auto opticDisc = getOpticDisc(side);
			auto leftSide = getLeftSide(side);
			auto leftTop = getLeftTop(side);
			auto leftBottom = getLeftBottom(side);
			auto rightSide = getRightSide(side);
			auto rightTop = getRightTop(side);
			auto rightBottom = getRightBottom(side);

			preset->center.row = center.first;
			preset->center.col = center.second;
			preset->fundus.row = fundus.first;
			preset->fundus.col = fundus.second;
			preset->optic_disc.row = opticDisc.first;
			preset->optic_disc.col = opticDisc.second;
			preset->left_side.row = leftSide.first;
			preset->left_side.col = leftSide.second;
			preset->left_top.row = leftTop.first;
			preset->left_top.col = leftTop.second;
			preset->left_bottom.row = leftBottom.first;
			preset->left_bottom.col = leftBottom.second;
			preset->right_side.row = rightSide.first;
			preset->right_side.col = rightSide.second;
			preset->right_top.row = rightTop.first;
			preset->right_top.col = rightTop.second;
			preset->right_bottom.row = rightBottom.first;
			preset->right_bottom.col = rightBottom.second;
		}

		auto* setup = &config->fixation_setup;
		setup->bright = getBrightness();
		setup->period = getBlinkPeriod();
		setup->on_time = getBlinkOnTime();
		setup->fix_type = getFixationType();
		setup->blink_on = useLcdBlinkOn();
		setup->lcdfix_on = useLcdFixation();
		*/
		return true;
	}
	return false;
}


InternalFixationPreset* wso_config::FixationSetting::getInternalFixationPreset(void) const
{
	return &impl().param;
}


void wso_config::FixationSetting::setInternalFixationPreset(const InternalFixationPreset& param)
{
	impl().param = param;
	return;
}


std::pair<int, int> wso_config::FixationSetting::getCenter(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.centerOD : impl().param.centerOS;
	return getPairOfFixation(fixp);
}


std::pair<int, int> wso_config::FixationSetting::getFundus(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.fundusOD : impl().param.fundusOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getOpticDisc(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.opticDiscOD : impl().param.opticDiscOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getLeftSide(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.leftSideOD : impl().param.leftSideOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getLeftTop(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.leftTopOD : impl().param.leftTopOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getLeftBottom(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.leftBottomOD : impl().param.leftBottomOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getRightSide(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.rightSideOD : impl().param.rightSideOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getRightTop(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.rightTopOD : impl().param.rightTopOS;
	return getPairOfFixation(fixp);
}

std::pair<int, int> wso_config::FixationSetting::getRightBottom(EyeSide side) const
{
	auto fixp = side == EyeSide::OD ? impl().param.rightBottomOD : impl().param.rightBottomOS;
	return getPairOfFixation(fixp);
}


int wso_config::FixationSetting::getBrightness(void) const
{
	return impl().param.brightness;
}


int wso_config::FixationSetting::getBlinkPeriod(void) const
{
	return impl().param.blinkPeriod;
}


int wso_config::FixationSetting::getBlinkOnTime(void) const
{
	return impl().param.blinkOnTime;
}

int wso_config::FixationSetting::getFixationType(void) const
{
	return impl().param.fixationType;
}

void wso_config::FixationSetting::setCenter(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.centerOD : impl().param.centerOS), pos);
	return;
}

void wso_config::FixationSetting::setFundus(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.fundusOD : impl().param.fundusOS), pos);
	return;
}

void wso_config::FixationSetting::setOpticDisc(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.opticDiscOD : impl().param.opticDiscOS), pos);
	return;
}

void wso_config::FixationSetting::setLeftSide(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.leftSideOD : impl().param.leftSideOS), pos);
	return;
}

void wso_config::FixationSetting::setLeftTop(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.leftTopOD : impl().param.leftTopOS), pos);
	return;
}

void wso_config::FixationSetting::setLeftBottom(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.leftBottomOD : impl().param.leftBottomOS), pos);
	return;
}

void wso_config::FixationSetting::setRightSide(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.rightSideOD : impl().param.rightSideOS), pos);
	return;
}

void wso_config::FixationSetting::setRightTop(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.rightTopOD : impl().param.rightTopOS), pos);
	return;
}

void wso_config::FixationSetting::setRightBottom(EyeSide side, std::pair<int, int> pos)
{
	setFixationByPair((side == EyeSide::OD ? impl().param.rightBottomOD : impl().param.rightBottomOS), pos);
	return;
}


void wso_config::FixationSetting::setBrightness(int bright)
{
	impl().param.brightness = bright;
}


void wso_config::FixationSetting::setBlinkPeriod(int period)
{
	impl().param.blinkPeriod = period;
	return;
}


void wso_config::FixationSetting::setBlinkOnTime(int onTime)
{
	impl().param.blinkOnTime = onTime;
	return;
}


void wso_config::FixationSetting::setFixationType(int type)
{
	impl().param.fixationType = ((type >= 0 && type <= 4) ? type : 2);
	return;
}


bool wso_config::FixationSetting::useLcdFixation(bool isset, bool flag) const
{
	if (isset) {
		impl().param.useLcdFix = flag;
	}
	return impl().param.useLcdFix;
}


bool wso_config::FixationSetting::useLcdBlinkOn(bool isset, bool flag) const
{
	if (isset) {
		impl().param.useBlinkOn = flag;
	}
	return impl().param.useBlinkOn;
}


std::pair<int, int> wso_config::FixationSetting::getPairOfFixation(const FixationPoint& point) const
{
	return std::pair<int, int>(point.x, point.y);
}

void wso_config::FixationSetting::setFixationByPair(FixationPoint& point, std::pair<int, int> data) const
{
	point.x = max(min(data.first, +999), -999);
	point.y = max(min(data.second, +999), -999);
}


FixationSetting::FixationSettingImpl& wso_config::FixationSetting::impl(void) const
{
	return *d_ptr;
}
