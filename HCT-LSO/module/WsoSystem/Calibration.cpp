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
	/*
	if (!loadFactorySetup1Calibration(fetch, region)) {
		return false;
	}
	if (!loadFactorySetup2Calibration(fetch, region)) {
		return false;
	}
	*/
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
	/*
	if (!saveFactorySetup1Calibration(write, region)) {
		return false;
	}
	if (!saveFactorySetup2Calibration(write, region)) {
		return false;
	}
	*/
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
			board->getLsoFocusMotor()->loadCalibParamFromProfile();
			board->getOctFocusMotor()->loadCalibParamFromProfile();
			board->getOctReferMotor()->loadCalibParamFromProfile();
			board->getOctPolarMotor()->loadCalibParamFromProfile();
			board->getRetMirrorMotor()->loadCalibParamFromProfile();
			board->getOctAntLensMotor()->loadCalibParamFromProfile();

			board->getCorneaIrLeftLed()->loadCalibParamFromProfile();
			board->getCorneaIrRightLed()->loadCalibParamFromProfile();
			board->getRetinaIrLed()->loadCalibParamFromProfile();
			board->getLsoWhiteLed()->loadCalibParamFromProfile();
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

bool wso_system::Calibration::obtainSystemCalibration(SystemCalibration* sys_calib, bool fetch)
{
	if (!loadSystemCalibration(fetch)) {
		return false;
	}

	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (auto* param = profile->getHbsCalibMotorSets(); param) {
				sys_calib->diopterParam.octFocusZeroPos = param->MotorCalPos.oct_focus_motor_0D_pos;
				sys_calib->diopterParam.lsoFocusZeroPos = param->MotorCalPos.lso_focus_motor_0D_pos;

				sys_calib->motorParam.referRetinaOriginPos = param->MotorCalPos.REF_RetinaPos;
				sys_calib->motorParam.referCorneaOriginPos = param->MotorCalPos.REF_CorneaPos;
				sys_calib->motorParam.polarOriginPos = param->MotorCalPos.PolarizationPos;
				sys_calib->motorParam.returnMirror.inPos = param->MotorCalPos.ReturnMirrorPos.InPos;
				sys_calib->motorParam.returnMirror.outPos = param->MotorCalPos.ReturnMirrorPos.OutPos;
				sys_calib->motorParam.octAnteriorLens.inPos = param->MotorCalPos.OCT_AntLensPos.InPos;
				sys_calib->motorParam.octAnteriorLens.outPos = param->MotorCalPos.OCT_AntLensPos.OutPos;
			}

			if (auto* param = profile->getHbsCalibOctParams(); param) {
				sys_calib->dispersionRetina.a2 = param->RetinaDispersion.a2;
				sys_calib->dispersionRetina.a3 = param->RetinaDispersion.a3;
				sys_calib->dispersionRetina.a4 = param->RetinaDispersion.a4;

				sys_calib->dispersionCornea.a2 = param->CorneaDispersion.a2;
				sys_calib->dispersionCornea.a3 = param->CorneaDispersion.a3;
				sys_calib->dispersionCornea.a4 = param->CorneaDispersion.a4;

				sys_calib->spectrometer.a0 = param->SpectroCal.a0;
				sys_calib->spectrometer.a1 = param->SpectroCal.a1;
				sys_calib->spectrometer.a2 = param->SpectroCal.a2;
				sys_calib->spectrometer.a3 = param->SpectroCal.a3;
			}

			if (auto* param = profile->getHbsCalibOctGalvano(); param) {
				sys_calib->octGalvano.offsetX = param->OctGalvano_Xcal.Galvano_offset;
				sys_calib->octGalvano.offsetY = param->OctGalvano_Ycal.Galvano_offset;
				sys_calib->octGalvano.rangeX = param->OctGalvano_Xcal.Galvano_Range;
				sys_calib->octGalvano.rangeY = param->OctGalvano_Ycal.Galvano_Range;
			}

			if (auto* param = profile->getHbsCalibLedSource(); param) {
				sys_calib->sldParam.pdCurrMax = param->SLD_Param.IM_MAX;
				sys_calib->sldParam.pdCurrMin = param->SLD_Param.IM_MIN;
				sys_calib->sldParam.sldCurrMax = param->SLD_Param.IS_MAX;
				sys_calib->sldParam.sldCurrMin = param->SLD_Param.IS_MIN;
				sys_calib->sldParam.refEpdMax = param->SLD_Param.DN_REF_EPD_MAX;
				sys_calib->sldParam.refEpdMin = param->SLD_Param.DN_REF_EPD_MIN;

				sys_calib->sldParam.rmonHighCode = param->SLD_Param.RmonHighCode;
				sys_calib->sldParam.rmonLowCode1 = param->SLD_Param.RmonLowCode1;
				sys_calib->sldParam.rmonLowCode2 = param->SLD_Param.RmonLowCode2;
				sys_calib->sldParam.rmonRsiCode = param->SLD_Param.RsiCode;

				sys_calib->ledParam.anteriorIrIntensity1 = param->LED_Info.AntIR1_intensity;
				sys_calib->ledParam.anteriorIrIntensity2 = param->LED_Info.AntIR2_intensity;
				sys_calib->ledParam.retinaIrIntensity = param->LED_Info.RetIR_intensity;
				sys_calib->ledParam.whiteIntensity = param->LED_Info.WLED_intensity;
				sys_calib->ledParam.blueIntensity = 0; // param->LED_Info.Bled_intensity;
				sys_calib->ledParam.greenIntensity = 0; // param->LED_Info.Gled_inentity;
			}
			return true;
		}
	}
	return false;
}

bool wso_system::Calibration::submitSystemCalibration(const SystemCalibration* sys_calib, bool write)
{
	if (auto* board = Hardware::getInstance()->getMainBoard(); board) {
		if (auto* profile = board->getHbsDataProfile(); profile) {
			if (auto* param = const_cast<HbsCalibMotorSets*>(profile->getHbsCalibMotorSets()); param) {
				param->MotorCalPos.oct_focus_motor_0D_pos = sys_calib->diopterParam.octFocusZeroPos;
				param->MotorCalPos.lso_focus_motor_0D_pos = sys_calib->diopterParam.lsoFocusZeroPos;

				param->MotorCalPos.REF_RetinaPos = sys_calib->motorParam.referRetinaOriginPos;
				param->MotorCalPos.REF_CorneaPos = sys_calib->motorParam.referCorneaOriginPos;
				param->MotorCalPos.PolarizationPos = sys_calib->motorParam.polarOriginPos;
				param->MotorCalPos.ReturnMirrorPos.InPos = sys_calib->motorParam.returnMirror.inPos;
				param->MotorCalPos.ReturnMirrorPos.OutPos = sys_calib->motorParam.returnMirror.outPos;
				param->MotorCalPos.OCT_AntLensPos.InPos = sys_calib->motorParam.octAnteriorLens.inPos;
				param->MotorCalPos.OCT_AntLensPos.OutPos = sys_calib->motorParam.octAnteriorLens.outPos;
			}

			if (auto* param = const_cast<HbsCalibOctParams*>(profile->getHbsCalibOctParams()); param) {
				param->RetinaDispersion.a2 = sys_calib->dispersionRetina.a2;
				param->RetinaDispersion.a3 = sys_calib->dispersionRetina.a3;
				param->RetinaDispersion.a4 = sys_calib->dispersionRetina.a4;

				param->CorneaDispersion.a2 = sys_calib->dispersionCornea.a2;
				param->CorneaDispersion.a3 = sys_calib->dispersionCornea.a3;
				param->CorneaDispersion.a4 = sys_calib->dispersionCornea.a4;

				param->SpectroCal.a0 = sys_calib->spectrometer.a0;
				param->SpectroCal.a1 = sys_calib->spectrometer.a1;
				param->SpectroCal.a2 = sys_calib->spectrometer.a2;
				param->SpectroCal.a3 = sys_calib->spectrometer.a3;
			}

			if (auto* param = const_cast<HbsCalibOctGalvano*>(profile->getHbsCalibOctGalvano()); param) {
				param->OctGalvano_Xcal.Galvano_offset = sys_calib->octGalvano.offsetX;
				param->OctGalvano_Ycal.Galvano_offset = sys_calib->octGalvano.offsetY;
				param->OctGalvano_Xcal.Galvano_Range = sys_calib->octGalvano.rangeX;
				param->OctGalvano_Ycal.Galvano_Range = sys_calib->octGalvano.rangeY;
			}

			if (auto* param = const_cast<HbsCalibLedSource*>(profile->getHbsCalibLedSource()); param) {
				param->SLD_Param.IM_MAX = sys_calib->sldParam.pdCurrMax;
				param->SLD_Param.IM_MIN = sys_calib->sldParam.pdCurrMin;
				param->SLD_Param.IS_MAX = sys_calib->sldParam.sldCurrMax;
				param->SLD_Param.IS_MIN = sys_calib->sldParam.sldCurrMin;
				param->SLD_Param.DN_REF_EPD_MAX = sys_calib->sldParam.refEpdMax;
				param->SLD_Param.DN_REF_EPD_MIN = sys_calib->sldParam.refEpdMin;

				param->SLD_Param.RmonHighCode = sys_calib->sldParam.rmonHighCode;
				param->SLD_Param.RmonLowCode1 = sys_calib->sldParam.rmonLowCode1;
				param->SLD_Param.RmonLowCode2 = sys_calib->sldParam.rmonLowCode2;
				param->SLD_Param.RsiCode = sys_calib->sldParam.rmonRsiCode;

				param->LED_Info.AntIR1_intensity = sys_calib->ledParam.anteriorIrIntensity1;
				param->LED_Info.AntIR2_intensity = sys_calib->ledParam.anteriorIrIntensity2;
				param->LED_Info.RetIR_intensity = sys_calib->ledParam.retinaIrIntensity;
				param->LED_Info.WLED_intensity = sys_calib->ledParam.whiteIntensity;
				param->LED_Info.Bled_intensity = 0; // sys_calib->ledParam.blueIntensity;
				param->LED_Info.Gled_inentity = 0; // sys_calib->ledParam.greenIntensity;
			}
		}
	}

	if (!saveSystemCalibration(write)) {
		return false;
	}

	applySystemCalibration();
	return true;
}


Calibration::CalibrationImpl& wso_system::Calibration::impl(void) const
{
	return *d_ptr;
}