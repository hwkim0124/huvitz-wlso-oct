#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		float WSOSYSTEM_DLL_API _stdcall getOctLineCameraExposureTime(void);
		bool WSOSYSTEM_DLL_API _stdcall setOctLineCameraExposureTime(float exp_time);

		float WSOSYSTEM_DLL_API _stdcall getOctGalvanoTriggerTimeStep(void);
		int WSOSYSTEM_DLL_API _stdcall getOctGalvanoTriggerTimeDelay(void);
		int WSOSYSTEM_DLL_API _stdcall getOctGalvanoTriggerForePaddings(OctScanSpeed speed);
		int WSOSYSTEM_DLL_API _stdcall getOctGalvanoTriggerPostPaddings(OctScanSpeed speed);

		bool WSOSYSTEM_DLL_API _stdcall setOctGalvanoTriggerTimeStep(float time_step);
		bool WSOSYSTEM_DLL_API _stdcall setOctGalvanoTriggerTimeDelay(int time_delay);
		bool WSOSYSTEM_DLL_API _stdcall setOctGalvanoTriggerForePaddings(OctScanSpeed speed, int padds);
		bool WSOSYSTEM_DLL_API _stdcall setOctGalvanoTriggerPostPaddings(OctScanSpeed speed, int padds);

		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternCount(bool not_point);
		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternList(OctPatternDescript* patterns, int buff_size, bool not_point);

		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternAscanList(int* ascans, int buff_size, OctPatternCode code);
		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternBscanList(int* bscans, int buff_size, OctPatternCode code, int ascans);
		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternOverlapList(int* overlaps, int buff_size, OctPatternCode code, int ascans, int bscans);
		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternRangeXList(float* ranges, int buff_size, OctPatternCode code);
		int WSOSYSTEM_DLL_API _stdcall getOctScanPatternRangeYList(float* ranges, int buff_size, OctPatternCode code);

		int WSOSYSTEM_DLL_API _stdcall getOctDetectorWavelengths(double* wlens);
		double WSOSYSTEM_DLL_API _stdcall getOctAxialPixelResolution(double fwhm, double ref_index);
		double WSOSYSTEM_DLL_API _stdcall getOctWavelengthAtPixelPosition(float index);

		bool WSOSYSTEM_DLL_API __stdcall obtainOctSpectrometerParam(OctSpectrometerParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitOctDispersionParam(const OctDispersionParam* param);
		bool WSOSYSTEM_DLL_API __stdcall obtainOctDispersionParam(OctDispersionParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitOctSpectrometerParam(const OctSpectrometerParam* param);
		
		bool WSOSYSTEM_DLL_API __stdcall isOctFFTZeroPaddingEnabled(bool isset, bool enable, int multi);
		bool WSOSYSTEM_DLL_API __stdcall isOctRefreshPhaseShiftParamEnabled(bool isset, bool enable);
		bool WSOSYSTEM_DLL_API __stdcall isOctBackgroundSubtractionEnabled(bool isset, bool enable);
		bool WSOSYSTEM_DLL_API __stdcall isOctDispersionCompensationEnabled(bool isset, bool enable);
		bool WSOSYSTEM_DLL_API __stdcall isOctFixedNoiseReductionEnabled(bool isset, bool enable);
		bool WSOSYSTEM_DLL_API __stdcall isOctAdaptiveGrayScalingEnabled(bool isset, bool enable);
		bool WSOSYSTEM_DLL_API __stdcall isOctFFTWindowingEnabled(bool isset, bool enable);
		bool WSOSYSTEM_DLL_API __stdcall isOctKLinearResamplingEnabled(bool isset, bool enable);

	}
}