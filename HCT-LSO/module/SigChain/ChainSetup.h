#pragma once

#include "SigChain2.h"

#include <memory>
#include <string>


namespace sig_proc
{
	class PhaseCorrector;
	class Resampler;
	class Background;
}


namespace wso_config 
{
	class SignalSetting;
}


namespace sig_chain
{
	class SIGCHAIN_DLL_API ChainSetup
	{
	public:
		ChainSetup();
		virtual ~ChainSetup();

	public:
		static sig_proc::Resampler* getResampler(void);
		static sig_proc::PhaseCorrector* getPhaseCorrector(void);
		static sig_proc::Background* getBackground(void);

		static void initializeChainSetup(void);
		static bool isCorneaScan(void);
		static bool isRetinaScan(void);
		static bool isAngioPattern(void);
		static bool isCubePattern(void);

		static void setScanPattern(OctScanPattern pattern);
		static OctScanPattern& getScanPattern(void) ;

		static void setScanAmplitudesStored(bool flag);
		static bool isScanAmplitudesStored(void) ;

		static bool loadSignalSettings(wso_config::SignalSetting* sset);
		static bool saveSignalSettings(wso_config::SignalSetting* sset);

		static void getSpectrometerParameters(double* params);
		static void setSpectrometerParameters(double* params);
		static double getSpectrometerParameter(int index);
		static void setSpectrometerParameter(int index, double value);

		static double getDispersionParameterToRetina(int index);
		static double getDispersionParameterToCornea(int index);
		static void setDispersionParameterToRetina(int index, double value);
		static void setDispersionParameterToCornea(int index, double value);

		static void updateSpectrometerParameters(void);
		static void updateDispersionParameters(double offset1=0.0, double offset2=0.0);
		static float* prepareDispersionCorrection(bool update = false, double offset1 = 0.0, double offset = 0.0);
		static void updateScanAxialResolutions(void);

		static void resetDynamicDispersionCorrection(void);
		static void pauseDynamicDispersionCorrection(bool flag);
		static bool isDynamicDispersionCorrection(void);
		static float* getCurrentPhaseShiftValues(bool recalc = false);
		static double getCurrentPhaseShiftOffset(int index);

		static bool useDynamicDispersionCorrection(bool isSet = false, bool flag = false);
		static bool useBackgroundSubtraction(bool isSet = false, bool flag = false);
		static bool useFixedNoiseReduction(bool isSet = false, bool flag = false);
		static bool useDispersionCompensation(bool isSet = false, bool flag = false);
		static bool useAdaptiveGrayscale(bool isSet = false, bool flag = false);
		static bool useFFTZeroPadding(bool isSet = false, bool flag = false);
		static bool useRefreshDispersionParams(bool isSet = false, bool flag = false);
		static bool useACALinePatternPreview(bool isSet = false, bool flag = false);
		static bool useFFTWindowing(bool isSet = false, bool flag = false);
		static bool useKLinearResampling(bool isSet = false, bool flag = false);

		static bool useSpectrumDataCallback(bool isset = false, bool flag = false);
		static bool useResampledDataCallback(bool isset = false, bool flag = false);
		static bool useIntensityDataCallback(bool isset = false, bool flag = false);

		static int getFFTImageRowOffset(void);
		static int getMultipleOfZeroPaddingSize(void);
		static void setMultipleOfZeroPaddingSize(int size);

		static bool isBackgroundSpectrum(void);
		static bool clearBackgroundSpectrum(void);
		static bool resetBackgroundSpectrum(const unsigned short* data, int width, int height, float quality);
		static unsigned short* getBackgroundSpectrum(void);
		static void setBackgroundSpectrum(const unsigned short* data);

		static float* getResampleFrameData(void);
		static float* getMagnitudeFrameData(void);
		static double* getDetectorWavelengths(void);
		static double getAxialPixelResolution(double refractiveIndex);
		static double getAxialPixelResolution(double fwhm, double ref_index);
		static double getWavelengthAtPixelPosition(float index);

	private:
		struct ChainSetupImpl;
		static std::unique_ptr<ChainSetupImpl> d_ptr;
		static ChainSetupImpl& getImpl(void) ;

		friend class ImageForm;
	};
}
