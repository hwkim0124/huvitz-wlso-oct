#include "pch.h"
#include "SystemCaliber.h"


using namespace wso_config;
using namespace std;



struct SystemCaliber::SystemCalibImpl
{
    optional<HbsDataProfile*> profile{ nullopt };
    SystemCalibImpl()
    {
    }
};



SystemCaliber::SystemCaliber() :
    d_ptr(make_unique<SystemCalibImpl>())
{
}


wso_config::SystemCaliber::~SystemCaliber() = default;


SystemCaliber* wso_config::SystemCaliber::getInstance(void)
{
    // Instance is constructed by public static method.
    // Static local variable initialization is thread-safe 
    // and will be initailized only once. 
    static SystemCaliber instance;
    return &instance;
}

void wso_config::SystemCaliber::setupBoardProfile(HbsDataProfile* profile)
{
    impl().profile = profile;
    return;
}


double wso_config::SystemCaliber::dispersionParameterToRetina(int index, bool isSet, double value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctParams*>(p->getHbsCalibOctParams());
		if (isSet) {
			switch (index) {
			case 0:
				cal->RetinaDispersion.a2 = value;
				break;
			case 1:
				cal->RetinaDispersion.a3 = value;
				break;
			case 2:
				cal->RetinaDispersion.a4 = value;
				break;
			}
			return value;
		}
		else {
			switch (index) {
			case 0:
				return cal->RetinaDispersion.a2;
			case 1:
				return cal->RetinaDispersion.a3;
			case 2:
				return cal->RetinaDispersion.a4;
			}
		}
	}
	return 0.0;
}

double wso_config::SystemCaliber::dispersionParameterToCornea(int index, bool isSet, double value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctParams*>(p->getHbsCalibOctParams());
		if (isSet) {
			switch (index) {
			case 0:
				cal->CorneaDispersion.a2 = value;
				break;
			case 1:
				cal->CorneaDispersion.a3 = value;
				break;
			case 2:
				cal->CorneaDispersion.a4 = value;
				break;
			}
			return value;
		}
		else {
			double param = 0.0;
			switch (index) {
			case 0:
				param = cal->CorneaDispersion.a2;
				break;
			case 1:
				param = cal->CorneaDispersion.a3;
				break;
			case 2:
				param = cal->CorneaDispersion.a4;
				break;
			}
			param = std::isnan(param) ? 0.0 : param;
			return param;
		}
	}
	return 0.0;
}

double wso_config::SystemCaliber::spectrometerParameter(int index, bool isSet, double value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctParams*>(p->getHbsCalibOctParams());
		if (isSet) {
			switch (index) {
			case 0:
				cal->SpectroCal.a0 = value;
				break;
			case 1:
				cal->SpectroCal.a1 = value;
				break;
			case 2:
				cal->SpectroCal.a2 = value;
				break;
			case 3:
				cal->SpectroCal.a3 = value;
				break;
			}
			return value;
		}
		else {
			double param = 0.0;
			switch (index) {
			case 0:
				param = cal->SpectroCal.a0;
				break;
			case 1:
				param = cal->SpectroCal.a1;
				break;
			case 2:
				param = cal->SpectroCal.a2;
				break;
			case 3:
				param = cal->SpectroCal.a3;
				break;
			}
			param = std::isnan(param) ? 0.0 : param;
			return param;
		}
	}
	return 0.0;
}

float wso_config::SystemCaliber::galvanometerOffsetX(bool isset, float value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctGalvano*>(p->getHbsCalibOctGalvano());
		if (isset) {
			cal->OctGalvano_Xcal.Galvano_offset = value;
			return value;
		}
		else {
			float param = 0.0f;
			param = cal->OctGalvano_Xcal.Galvano_offset;
			param = std::isnan(param) ? 0.0f : param;
			return param;
		}
	}
	return 0.0f;
}

float wso_config::SystemCaliber::galvanometerOffsetY(bool isset, float value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctGalvano*>(p->getHbsCalibOctGalvano());
		if (isset) {
			cal->OctGalvano_Ycal.Galvano_offset = value;
			return value;
		}
		else {
			float param = 0.0f;
			param = cal->OctGalvano_Ycal.Galvano_offset;
			param = std::isnan(param) ? 0.0f : param;
			return param;
		}
	}
	return 0.0f;
}

float wso_config::SystemCaliber::galvanometerScaleX(bool isset, float value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctGalvano*>(p->getHbsCalibOctGalvano());
		if (isset) {
			cal->OctGalvano_Xcal.Galvano_Range = value;
			return value;
		}
		else {
			float param = 0.0f;
			param = cal->OctGalvano_Xcal.Galvano_Range;
			param = std::isnan(param) ? 1.0f : param;
			return param;
		}
	}
	return 1.0f;
}

float wso_config::SystemCaliber::galvanometerScaleY(bool isset, float value)
{
	if (auto* p = getBoardProfile(); p) {
		auto* cal = const_cast<HbsCalibOctGalvano*>(p->getHbsCalibOctGalvano());
		if (isset) {
			cal->OctGalvano_Ycal.Galvano_Range = value;
			return value;
		}
		else {
			float param = 0.0f;
			param = cal->OctGalvano_Ycal.Galvano_Range;
			param = std::isnan(param) ? 1.0f : param;
			return param;
		}
	}
	return 1.0f;
}


HbsDataProfile* wso_config::SystemCaliber::getBoardProfile(void) const
{
    if (impl().profile.has_value())
    {
        return *impl().profile;
    }
    return nullptr;
}

SystemCaliber::SystemCalibImpl& wso_config::SystemCaliber::impl(void) const
{
    return *d_ptr;
}