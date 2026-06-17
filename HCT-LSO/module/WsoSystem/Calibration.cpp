#include "pch.h"
#include "Calibration.h"
#include "Hardware.h"
#include "HbsDataProfile.h"

using namespace wso_system;
using namespace std;


std::mutex Calibration::singleMutex_;


struct Calibration::CalibrationImpl
{
	CalibrationImpl() {
		initializeCalibrationImpl();
	}

	void initializeCalibrationImpl(void) {}
};



wso_system::Calibration::Calibration() :
	d_ptr(std::make_unique<CalibrationImpl>())
{}


wso_system::Calibration::~Calibration()
{}


Calibration* wso_system::Calibration::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static Calibration instance;
	return &instance;
}

bool wso_system::Calibration::loadSystemCalibration(bool fetch, int region)
{
	if (!loadMotorSetupCalibration(fetch, region)) {
		return false;
	}
	if (!loadOctParamsCalibration(fetch, region)) {
		return false;
	}
	if (!loadLedSourceCalibration(fetch, region)) {
		return false;
	}
	if (!loadOctGalvanoCalibration(fetch, region)) {
		return false;
	}
	if (!loadDeviceConfigCalibration(fetch, region)) {
		return false;
	}
	if (!loadStepMotorsCalibration(fetch, region)) {
		return false;
	}
	if (!loadFactorySetup1Calibration(fetch, region)) {
		return false;
	}
	if (!loadFactorySetup2Calibration(fetch, region)) {
		return false;
	}
	return true;
}

bool wso_system::Calibration::saveSystemCalibration(bool write, int region)
{
	if (!saveMotorSetupCalibration(write, region)) {
		return false;
	}
	if (!saveOctParamsCalibration(write, region)) {
		return false;
	}
	if (!saveLedSourceCalibration(write, region)) {
		return false;
	}
	if (!saveOctGalvanoCalibration(write, region)) {
		return false;
	}
	if (!saveDeviceConfigCalibration(write, region)) {
		return false;
	}
	if (!saveStepMotorsCalibration(write, region)) {
		return false;
	}
	if (!saveFactorySetup1Calibration(write, region)) {
		return false;
	}
	if (!saveFactorySetup2Calibration(write, region)) {
		return false;
	}
	return true;
}

bool wso_system::Calibration::applySystemCalibration(void)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* calib = SystemCaliber::getInstance(); calib) {
			auto* profile = board->getHbsDataProfile();
			calib->setupBoardProfile(profile);
		}

		if (auto* calib = SystemCaliber::getInstance(); calib) {
			ChainSetup::updateSpectrometerParameters();
			ChainSetup::updateDispersionParameters();
			ChainSetup::updateScanAxialResolutions();

			/*
			auto offsetX = calib->galvanometerOffsetX();
			auto offsetY = calib->galvanometerOffsetY();
			auto scaleX = calib->galvanometerScaleX();
			auto scaleY = calib->galvanometerScaleY();
			if (auto* p = OctScanOptions::getInstance(); p) {
				for (int i = 0; i < 3; i++) {
					p->setCorneaPatternRangeOffset(i, offsetX, offsetY);
					p->setCorneaPatternRangeScale(i, scaleX, scaleY);
					p->setRetinaPatternRangeOffset(i, offsetX, offsetY);
					p->setRetinaPatternRangeScale(i, scaleX, scaleY);
				}
			}
			*/
		}
		return true;
	}
	return false;
}

bool wso_system::Calibration::loadMotorSetupCalibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockMotorSets(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadOctParamsCalibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockOctParams(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadLedSourceCalibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockLedSource(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadOctGalvanoCalibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockOctGalvano(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadDeviceConfigCalibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockDeviceCfg(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadStepMotorsCalibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockStepMotors(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadFactorySetup1Calibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockFactorySet1(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::loadFactorySetup2Calibration(bool fetch, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->loadCalibBlockFactorySet2(fetch, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveMotorSetupCalibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockMotorSets(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveOctParamsCalibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockOctParams(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveLedSourceCalibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockLedSource(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveOctGalvanoCalibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockOctGalvano(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveDeviceConfigCalibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockDeviceCfg(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveStepMotorsCalibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockStepMotors(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveFactorySetup1Calibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockFactorySet1(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::saveFactorySetup2Calibration(bool write, int region)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (!profile->saveCalibBlockFactorySet2(write, region)) {
				return false;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::obtainSystemCalibration(wso_board::HbsCalibration* sys_calib, bool fetch)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (fetch) {
			if (!board->pullSystemCalibFromMemory()) {
				return false;
			}
		}
		if (auto* profile = board->getHbsDataProfile(); profile) {
			memcpy(sys_calib, profile->getHbsCalibration(), sizeof(wso_board::HbsCalibration));
		}
		return true;
	}
	return false;
}

bool wso_system::Calibration::submitSystemCalibration(const wso_board::HbsCalibration* sys_calib, bool write)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			auto* calib = const_cast<HbsCalibration*>(profile->getHbsCalibration());
			memcpy(calib, sys_calib, sizeof(wso_board::HbsCalibration));
			if (write) {
				if (!board->pushSystemCalibToMemory()) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}


Calibration::CalibrationImpl& wso_system::Calibration::impl(void) const
{
	return *d_ptr;
}