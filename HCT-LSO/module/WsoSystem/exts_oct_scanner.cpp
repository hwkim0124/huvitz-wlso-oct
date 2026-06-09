#include "pch.h"
#include "exts_oct_scanner.h"
#include "OctScanning.h"
#include "Hardware.h"

using namespace wso_system;
using namespace std;


float WSOSYSTEM_DLL_API _stdcall wso_system::getOctLineCameraExposureTime(void)
{
    if (auto* grab = Hardware::getInstance()->getUsb3Grabber(); grab) {
        float time = grab->getLineCameraExposureTime();
        return time;
    }
    return 0.0f;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setOctLineCameraExposureTime(float exp_time)
{
    if (auto* grab = Hardware::getInstance()->getUsb3Grabber(); grab) {
        return grab->setLineCameraExposureTime(exp_time);
    }
    return false;
}

float WSOSYSTEM_DLL_API _stdcall wso_system::getOctGalvanoTriggerTimeStep(void)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        return galv->getTriggerTimeStep();
    }
    return 0.0f;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctGalvanoTriggerTimeDelay(void)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        return galv->getTriggerTimeDelay();
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctGalvanoTriggerForePaddings(OctScanSpeed speed)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        return galv->getTriggerForePaddings(speed);
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctGalvanoTriggerPostPaddings(OctScanSpeed speed)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        return galv->getTriggerPostPaddings(speed);
    }
    return 0;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setOctGalvanoTriggerTimeStep(float time_step)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
		galv->setTriggerTimeStep(time_step);
        return true;
	}
    return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setOctGalvanoTriggerTimeDelay(int time_delay)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        galv->setTriggerTimeDelay(time_delay);
        return true;
    }
    return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setOctGalvanoTriggerForePaddings(OctScanSpeed speed, int padds)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        galv->setTriggerForePaddings(speed, padds);
        return true;
    }
    return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::setOctGalvanoTriggerPostPaddings(OctScanSpeed speed, int padds)
{
    if (auto* galv = Hardware::getInstance()->getMainBoard()->getGalvanometer(); galv) {
        galv->setTriggerPostPaddings(speed, padds);
        return true;
    }
    return false;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternCount(bool not_point)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        return p->getScanPatternCount(not_point);
    }
    return 0;
}


int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternList(OctPatternDescript* patterns, int buff_size, bool not_point)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        vector<OctPatternDescript> list;
        if (p->getScanPatternList(list, not_point) > 0) {
            for (int i = 0; i < list.size() && i < buff_size; i++) {
                patterns[i] = list[i];
            }
            return (int)list.size();
        }
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternAscanList(int* ascans, int buff_size, OctPatternCode code)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        vector<int> list;
        if (p->getPatternAscanList(list, code) > 0) {
            for (int i = 0; i < list.size() && i < buff_size; i++) {
                ascans[i] = list[i];
            }
            return (int)list.size();
        }
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternBscanList(int* bscans, int buff_size, OctPatternCode code, int ascans)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        vector<int> list;
        if (p->getPatternBscanList(list, code, ascans) > 0) {
            for (int i = 0; i < list.size() && i < buff_size; i++) {
                bscans[i] = list[i];
            }
            return (int)list.size();
        }
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternOverlapList(int* overlaps, int buff_size, OctPatternCode code, int ascans, int bscans)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        vector<int> list;
        if (p->getPatternOverlapList(list, code, ascans, bscans) > 0) {
            for (int i = 0; i < list.size() && i < buff_size; i++) {
                overlaps[i] = list[i];
            }
            return (int)list.size();
        }
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternRangeXList(float* ranges, int buff_size, OctPatternCode code)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        vector<float> xlist, ylist;
        if (p->getPatternRangeList(xlist, ylist, code) > 0) {
            for (int i = 0; i < xlist.size() && i < buff_size; i++) {
                ranges[i] = xlist[i];
            }
            return (int)xlist.size();
        }
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctScanPatternRangeYList(float* ranges, int buff_size, OctPatternCode code)
{
    if (auto* p = OctScanHelper::getInstance(); p) {
        vector<float> xlist, ylist;
        if (p->getPatternRangeList(xlist, ylist, code) > 0) {
            for (int i = 0; i < ylist.size() && i < buff_size; i++) {
                ranges[i] = ylist[i];
            }
            return (int)ylist.size();
        }
    }
    return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctDetectorWavelengths(double* wlens)
{
    if (auto* p = ChainSetup::getDetectorWavelengths(); p && wlens) {
        int size = LINE_CAMERA_CCD_PIXELS;
        memcpy(wlens, p, sizeof(double) * size);
		return size;
	}
	return 0;
}

double WSOSYSTEM_DLL_API _stdcall wso_system::getOctAxialPixelResolution(double fwhm, double ref_index)
{
    auto value = ChainSetup::getAxialPixelResolution(fwhm, ref_index);
    return value;
}

double WSOSYSTEM_DLL_API _stdcall wso_system::getOctWavelengthAtPixelPosition(float index)
{
    auto value = ChainSetup::getWavelengthAtPixelPosition(index);
    return value;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainOctSpectrometerParam(OctSpectrometerParam* param)
{
    if (auto* p = SystemCaliber::getInstance(); p && param) {
        for (int i = 0; i < OCT_SPECTROMETER_COEFFS_SIZE; i++) {
            param->wfCoeffs[i] = p->spectrometerParameter(i);
        }
        return true;
    }
    return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::submitOctSpectrometerParam(const OctSpectrometerParam* param)
{
    if (auto* p = SystemCaliber::getInstance(); p && param) {
        for (int i = 0; i < OCT_SPECTROMETER_COEFFS_SIZE; i++) {
            p->spectrometerParameter(i, true, param->wfCoeffs[i]);
        }
        // Apply the new parameters to the spectrometer when scanning initialized.
        return true;
    }
    return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainOctDispersionParam(OctDispersionParam* param)
{
    if (auto* p = SystemCaliber::getInstance(); p && param) {
        for (int i = 0; i < OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE; i++) {
            param->retinaCoeffs[i] = p->dispersionParameterToRetina(i);
            param->corneaCoeffs[i] = p->dispersionParameterToCornea(i);
        }
        return true;
    }
    return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::submitOctDispersionParam(const OctDispersionParam* param)
{
    if (auto* p = SystemCaliber::getInstance(); p && param) {
        for (int i = 0; i < OCT_DISPERSION_PHASE_SHIFT_COEFFS_SIZE; i++) {
            p->dispersionParameterToRetina(i, true, param->retinaCoeffs[i]);
            p->dispersionParameterToCornea(i, true, param->corneaCoeffs[i]);
        }
        // Apply the new parameters to the phase shift when scanning initialized or 
        // the realtime refresh option is enabled.
        return true;
    }
    return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctFFTZeroPaddingEnabled(bool isset, bool enable, int multi)
{
    if (isset) {
        auto result = ChainSetup::useFFTZeroPadding(true, enable);
		ChainSetup::setMultipleOfZeroPaddingSize(multi);
		return result;
	}
	else {
        auto flag = ChainSetup::useFFTZeroPadding();
        return flag;
    }
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctRefreshPhaseShiftParamEnabled(bool isset, bool enable)
{
    if (isset) {
        auto result = ChainSetup::useRefreshDispersionParams(true, enable);
        return result;
    }
    else {
        auto flag = ChainSetup::useRefreshDispersionParams();
		return flag;
    }
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctBackgroundSubtractionEnabled(bool isset, bool enable)
{
    if (isset) {
        auto result = ChainSetup::useBackgroundSubtraction(true, enable);
        return result;
    }
    else {
		auto flag = ChainSetup::useBackgroundSubtraction();
		return flag;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctDispersionCompensationEnabled(bool isset, bool enable)
{
    if (isset) {
        auto result = ChainSetup::useDispersionCompensation(true, enable);
        return result;
    }
    else {
        auto flag = ChainSetup::useDispersionCompensation();
        return flag;
    }
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctFixedNoiseReductionEnabled(bool isset, bool enable)
{
    if (isset) {
		auto result = ChainSetup::useFixedNoiseReduction(true, enable);
		return result;
	}
    else {
		auto flag = ChainSetup::useFixedNoiseReduction();
		return flag;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctAdaptiveGrayScalingEnabled(bool isset, bool enable)
{
    if (isset) {
		auto result = ChainSetup::useAdaptiveGrayscale(true, enable);
		return result;
	}
    else {
		auto flag = ChainSetup::useAdaptiveGrayscale();
		return flag;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctFFTWindowingEnabled(bool isset, bool enable)
{
    if (isset) {
        auto result = ChainSetup::useFFTWindowing(true, enable);
        return result;
    }
    else {
        auto flag = ChainSetup::useFFTWindowing();
		return flag;
    }
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isOctKLinearResamplingEnabled(bool isset, bool enable)
{
    if (isset) {
        auto result = ChainSetup::useKLinearResampling(true, enable);
		return result;
	}
    else {
		auto flag = ChainSetup::useKLinearResampling();
		return flag;
    }
}
