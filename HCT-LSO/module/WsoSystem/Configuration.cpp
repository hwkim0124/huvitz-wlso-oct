#include "pch.h"
#include "Configuration.h"
#include "Hardware.h"
#include "HbsDataProfile.h"

using namespace wso_system;
using namespace std;


std::mutex Configuration::singleMutex_;


struct Configuration::ConfigurationImpl
{
	ConfigurationImpl() {
		initializeConfigurationImpl();
	}

	void initializeConfigurationImpl(void) {
	}
};



wso_system::Configuration::Configuration() :
	d_ptr(std::make_unique<ConfigurationImpl>())
{
}


wso_system::Configuration::~Configuration()
{
}


Configuration* wso_system::Configuration::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static Configuration instance;
	return &instance;
}

bool wso_system::Configuration::loadSystemConfiguration(bool fetch)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (fetch) {
			if (!board->pullSystemConfigFromMemory()) {
				return false;
			}
		}
		if (true) { // isSystemConfigurationValid()) {
			if (auto* config = SystemConfig::getInstance(); config) {
				auto* profile = board->getHbsDataProfile();
				if (config->updateFromBoardProfile(profile)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool wso_system::Configuration::saveSystemConfiguration(bool write)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* config = SystemConfig::getInstance(); config) {
			auto* profile = board->getHbsDataProfile();
			if (!config->uploadToBoardProfile(profile)) {
				return false;
			}
			if (write) {
				if (!board->pushSystemConfigToMemory()) {
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool wso_system::Configuration::applySystemConfiguration(void)
{
	if (auto* config = SystemConfig::getInstance(); config) {
		if (auto* fix = config->getFixationSetting(); fix) {
			if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
				auto bright = fix->getBrightness();
				auto ontime = fix->getBlinkOnTime();
				auto period = fix->getBlinkPeriod();
				auto type = fix->getFixationType();
				auto blink = fix->useLcdBlinkOn();
				if (!board->updateLcdParameters(bright, blink, period, ontime, type)) {
					LogD() << "Fixation Lcd parameters applied failed!";
				}
				else {
					LogD() << "Fixation Lcd parameters applied, bright: " << bright << ", ontime: " << ontime << ", period: " << period << ", type: " << type << ", blink: " << blink;
				}
			}
		}
		/*
		if (auto* camera = config->getCameraSetting(); camera) {
			if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
				if (auto* cam = board->getCorneaCamera(); cam) {
					auto again = camera->getCorneaAgain();
					auto dgain = camera->getCorneaDgain();
					cam->setAnalogGain(again);
					cam->setDigitalGain(dgain);

					auto wdot1 = camera->getWdotIntensity(0);
					auto wdot2 = camera->getWdotIntensity(1);
					cam->setWdotIntensity(0, wdot1);
					cam->setWdotIntensity(1, wdot2);
					LogD() << "Cornea camera parameters applied, again: " << again << ", dgain: " << dgain << ", wdots: " << wdot1 << ", " << wdot2;
				}
			}
		}
		*/
		auto* galset = config->getGalvanoSetting();
		if (auto* p = OctScanOptions::getInstance(); p) {
			for (int i = 0; i < 3; i++) {
				auto offsetX = galset->getOffsetX();
				auto offsetY = galset->getOffsetY();
				p->setCorneaPatternRangeOffset(i, offsetX, offsetY);
				p->setRetinaPatternRangeOffset(i, offsetX, offsetY);

				for (int j = 0; j < OCT_GALVANO_SCALE_NUM; j++) {
					auto scaleX = galset->getScaleX(j);
					auto scaleY = galset->getScaleY(j);
					p->setCorneaPatternRangeScale(i, j, scaleX, scaleY);
					p->setRetinaPatternRangeScale(i, j, scaleX, scaleY);
				}
			}
		}
	}
	return true;
}

bool wso_system::Configuration::isSystemConfigurationValid(void) const
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (auto* config = profile->getHbsConfiguration(); config) {
				if (config->chksum == sizeof(HbsConfiguration)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool wso_system::Configuration::loadSystemCalibration(bool fetch)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		/*
		if (fetch) {
			if (!board->pullSystemCalibFromMemory()) {
				return false;
			}
		}
		*/
		if (auto* calib = SystemCaliber::getInstance(); calib) {
			auto* profile = board->getHbsDataProfile();
			calib->setupBoardProfile(profile);
		}
		return true;
	}
	return false;
}

bool wso_system::Configuration::saveSystemCalibration(bool write)
{
	if (write) {
		if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
			if (!board->pushSystemCalibToMemory()) {
				return false;
			}
		}
	}
	return true;
}

bool wso_system::Configuration::applySystemCalibration(void)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
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


bool wso_system::Configuration::obtainSystemCalibration(wso_board::HbsCalibration* sys_calib, bool fetch)
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


bool wso_system::Configuration::submitSystemCalibration(const wso_board::HbsCalibration* sys_calib, bool write)
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


Configuration::ConfigurationImpl& wso_system::Configuration::impl(void) const
{
	return *d_ptr;
}