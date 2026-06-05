#include "pch.h"
#include "ChainSetup.h"
#include "Resampler.h"


/*
#include "CL\cl.h"
#include "openCL.misc.h"
#include "clFFT.h"
*/


#include <vector>
#include <mutex>

#include "wso_config.h"
#include "wso_board.h"
#include "sig_proc.h"

using namespace sig_chain;
using namespace sig_proc;
using namespace wso_config;
using namespace wso_board;


struct ChainSetup::ChainSetupImpl
{
	Resampler resampler;
	PhaseCorrector phaseCorr;
	Background background;

	OctScanPattern pattern;
	bool isScanAmplitudesStored = false;

	vector<float> resampleFrameData;
	vector<float> magnitudeFrameData;

	bool useBackgroundSubtraction;
	bool useFixedNoiseReduction;
	bool useDispersionCompensation;
	bool useAdaptiveGrayscale;
	bool useFFTZeroPadding;
	bool useRefreshDispersionParams;
	bool useACALinePatternPreview;
	bool useFFTWindowing;
	bool useKLinearResampling;

	bool useSpectrumDataCallback;
	bool useResampledDataCallback;
	bool useIntensityDataCallback;

	int multipleOfZeroPadding;

	mutex mutexRegist;

	ChainSetupImpl()  
		:	useBackgroundSubtraction(true), useFixedNoiseReduction(true), useDispersionCompensation(true), 
			useAdaptiveGrayscale(true), useFFTZeroPadding(false), multipleOfZeroPadding(4), useFFTWindowing(false),
			useRefreshDispersionParams(false), useACALinePatternPreview(false), useKLinearResampling(true), 
			useSpectrumDataCallback(false), useResampledDataCallback(false), useIntensityDataCallback(false)
	{
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<ChainSetup::ChainSetupImpl> ChainSetup::d_ptr(new ChainSetupImpl());


ChainSetup::ChainSetup()
{
	// initialize();
}


ChainSetup::~ChainSetup()
{
}


Resampler * sig_chain::ChainSetup::getResampler(void)
{
	return &getImpl().resampler;
}


PhaseCorrector * sig_chain::ChainSetup::getPhaseCorrector(void)
{
	return &getImpl().phaseCorr;
}


sig_proc::Background * sig_chain::ChainSetup::getBackground(void)
{
	return &getImpl().background;
}


void sig_chain::ChainSetup::initializeChainSetup(void)
{
	double coeffs[WAVELENGTH_FUNCTION_COEFFS_SIZE] = { 0.0 };
	coeffs[0] = RESAMPLE_INIT_PARAM1;
	coeffs[1] = RESAMPLE_INIT_PARAM2;
	coeffs[2] = RESAMPLE_INIT_PARAM3;
	coeffs[3] = RESAMPLE_INIT_PARAM4;
	getResampler()->setWavelengthFunctionCoefficients(coeffs);

	getPhaseCorrector()->setWavenumbers(getResampler()->getkValueLinearized());

	double param1 = PHASE_SHIFT_INIT_PARAM1;
	double param2 = PHASE_SHIFT_INIT_PARAM2;
	double param3 = PHASE_SHIFT_INIT_PARAM3;
	getPhaseCorrector()->setPhaseShiftParameterToRetina(0, param1);
	getPhaseCorrector()->setPhaseShiftParameterToRetina(1, param2);
	getPhaseCorrector()->setPhaseShiftParameterToRetina(2, param3);

	getPhaseCorrector()->setPhaseParameterOffsetToRetina(0, 0.0);
	getPhaseCorrector()->setPhaseParameterOffsetToRetina(1, 0.0);
	getPhaseCorrector()->setPhaseParameterOffsetToRetina(2, 0.0);

	getPhaseCorrector()->setPhaseShiftParameterToCornea(0, param1);
	getPhaseCorrector()->setPhaseShiftParameterToCornea(1, param2);
	getPhaseCorrector()->setPhaseShiftParameterToCornea(2, param3);

	getPhaseCorrector()->setPhaseParameterOffsetToCornea(0, 0.0);
	getPhaseCorrector()->setPhaseParameterOffsetToCornea(1, 0.0);
	getPhaseCorrector()->setPhaseParameterOffsetToCornea(2, 0.0);

	getPhaseCorrector()->resetPhaseShiftOptimizer();
	getPhaseCorrector()->enablePhaseShiftOptimizer(false);
	return ;
}


bool sig_chain::ChainSetup::isCorneaScan(void)
{
	return (getImpl().pattern.isCorneaScan());
}


bool sig_chain::ChainSetup::isRetinaScan(void)
{
	return (!getImpl().pattern.isCorneaScan());
}


bool sig_chain::ChainSetup::isAngioPattern(void)
{
	return getImpl().pattern.isAngioPattern();
}


bool sig_chain::ChainSetup::isCubePattern(void)
{
	return getImpl().pattern.isCubePattern();
}

void sig_chain::ChainSetup::setScanPattern(OctScanPattern pattern)
{
	getImpl().pattern = pattern;
	return;
}

OctScanPattern& sig_chain::ChainSetup::getScanPattern(void)
{
	return getImpl().pattern;
}

void sig_chain::ChainSetup::setScanAmplitudesStored(bool flag)
{
	getImpl().isScanAmplitudesStored = flag;
	return;
}


bool sig_chain::ChainSetup::isScanAmplitudesStored(void)
{
	return getImpl().isScanAmplitudesStored;
}



bool sig_chain::ChainSetup::loadSignalSettings(wso_config::SignalSetting * sset)
{
	double coeffs[WAVELENGTH_FUNCTION_COEFFS_SIZE] = { 0.0 };
	for (int i = 0; i < WAVELENGTH_FUNCTION_COEFFS_SIZE; i++) {
		coeffs[i] = sset->getWavelengthParameter(i);
	}
	getResampler()->setWavelengthFunctionCoefficients(coeffs);

	getPhaseCorrector()->setWavenumbers(getResampler()->getkValueLinearized());

	double param1 = sset->getDispersionParameter(0);
	double param2 = sset->getDispersionParameter(1);
	double param3 = sset->getDispersionParameter(2);

	getPhaseCorrector()->setPhaseShiftParameterToRetina(0, param1);
	getPhaseCorrector()->setPhaseShiftParameterToRetina(1, param2);
	getPhaseCorrector()->setPhaseShiftParameterToRetina(2, param3);

	getPhaseCorrector()->setPhaseShiftParameterToCornea(0, param1);
	getPhaseCorrector()->setPhaseShiftParameterToCornea(1, param2);
	getPhaseCorrector()->setPhaseShiftParameterToCornea(2, param3);
	return true;
}


bool sig_chain::ChainSetup::saveSignalSettings(wso_config::SignalSetting * sset)
{
	Resampler* resampler = getResampler();
	PhaseCorrector* phaseCorr = getPhaseCorrector();

	for (int i = 0; i < WAVELENGTH_FUNCTION_COEFFS_SIZE; i++) {
		sset->setWavelengthParameter(i, resampler->getWavelengthFunctionCoefficient(i));
	}

	for (int i = 0; i < PHASE_SHIFT_PARAMETER_SIZE; i++) {
		sset->setDispersionParameter(i, phaseCorr->getPhaseShiftParameterToRetina(i));
	}
	return true;
}


void sig_chain::ChainSetup::getSpectrometerParameters(double * params)
{
	for (int i = 0; i < WAVELENGTH_FUNCTION_COEFFS_SIZE; i++) {
		params[i] = getResampler()->getWavelengthFunctionCoefficient(i);
	}
	return;
}


void sig_chain::ChainSetup::setSpectrometerParameters(double * params)
{
	getResampler()->setWavelengthFunctionCoefficients(params);
	return;
}


double sig_chain::ChainSetup::getSpectrometerParameter(int index)
{
	return getResampler()->getWavelengthFunctionCoefficient(index);
}


void sig_chain::ChainSetup::setSpectrometerParameter(int index, double value)
{
	getResampler()->setWavelengthFunctionCoefficient(index, value);

}


double sig_chain::ChainSetup::getDispersionParameterToRetina(int index)
{
	return getPhaseCorrector()->getPhaseShiftParameterToRetina(index);
}


double sig_chain::ChainSetup::getDispersionParameterToCornea(int index)
{
	return getPhaseCorrector()->getPhaseShiftParameterToCornea(index);
}


void sig_chain::ChainSetup::setDispersionParameterToRetina(int index, double value)
{
	getPhaseCorrector()->setPhaseShiftParameterToRetina(index, value);
	return;
}


void sig_chain::ChainSetup::setDispersionParameterToCornea(int index, double value)
{
	getPhaseCorrector()->setPhaseShiftParameterToCornea(index, value);
	return;
}


void sig_chain::ChainSetup::updateSpectrometerParameters(void)
{
	if (auto* calib = SystemCaliber::getInstance(); calib) {
		double coeffs[WAVELENGTH_FUNCTION_COEFFS_SIZE] = { 0.0 };
		coeffs[0] = calib->spectrometerParameter(0);
		coeffs[1] = calib->spectrometerParameter(1);
		coeffs[2] = calib->spectrometerParameter(2);
		coeffs[3] = calib->spectrometerParameter(3);
		getResampler()->setWavelengthFunctionCoefficients(coeffs);

		LogD() << "Spectrometer params updated, coeffs: " << coeffs[0] << ", " << coeffs[1] << ", " << coeffs[2] << ", " << coeffs[3];
		getPhaseCorrector()->setWavenumbers(getResampler()->getkValueLinearized());
	}
	return;
}


void sig_chain::ChainSetup::updateDispersionParameters(double offset1, double offset2)
{
	static double preset1 = 0.0;
	static double preset2 = 0.0;
	static double preset3 = 0.0;

	if (auto* calib = SystemCaliber::getInstance(); calib) {
		if (isRetinaScan()) {
			double param1 = calib->dispersionParameterToRetina(0);
			double param2 = calib->dispersionParameterToRetina(1);
			double param3 = calib->dispersionParameterToRetina(2);
			getPhaseCorrector()->setPhaseShiftParameterToRetina(0, param1);
			getPhaseCorrector()->setPhaseShiftParameterToRetina(1, param2);
			getPhaseCorrector()->setPhaseShiftParameterToRetina(2, param3);

			getPhaseCorrector()->clearPhaseParameterOffsetsOfRetina();
			getPhaseCorrector()->setPhaseParameterOffsetToRetina(0, offset1);
			getPhaseCorrector()->setPhaseParameterOffsetToRetina(1, offset2);
			getPhaseCorrector()->updatePhaseShiftValuesToRetina();

			if (preset1 != param1 || preset2 != param2 || preset3 != param3) {
				LogD() << "Dispersion params for retina: " << param1 << ", " << param2 << ", " << param3;
				LogD() << "Dispersion offsets: " << offset1 << ", " << offset2;
				preset1 = param1;
				preset2 = param2;
				preset3 = param3;
			}
		}
		else {
			double param1 = calib->dispersionParameterToCornea(0);
			double param2 = calib->dispersionParameterToCornea(1);
			double param3 = calib->dispersionParameterToCornea(2);
			getPhaseCorrector()->setPhaseShiftParameterToCornea(0, param1);
			getPhaseCorrector()->setPhaseShiftParameterToCornea(1, param2);
			getPhaseCorrector()->setPhaseShiftParameterToCornea(2, param3);

			getPhaseCorrector()->clearPhaseParameterOffsetsOfCornea();
			getPhaseCorrector()->setPhaseParameterOffsetToCornea(0, offset1);
			getPhaseCorrector()->setPhaseParameterOffsetToCornea(1, offset2);
			getPhaseCorrector()->updatePhaseShiftValuesToCornea();

			if (preset1 != param1 || preset2 != param2 || preset3 != param3) {
				LogD() << "Dispersion params for cornea: " << param1 << ", " << param2 << ", " << param3;
				LogD() << "Dispersion offsets: " << offset1 << ", " << offset2;
				preset1 = param1;
				preset2 = param2;
				preset3 = param3;
			}
		}
	}
	return;
}


float* sig_chain::ChainSetup::prepareDispersionCorrection(bool update, double offset1, double offset2)
{
	Resampler* resampler = getResampler();
	PhaseCorrector* phaseCorr = getPhaseCorrector();

	// Spectrometer calibration has to be preceded for linearized k values. 
	// phaseCorr->setCenterOfWavelengths(WAVELENGTH_CENTER_OF_SOURCE);
	// phaseCorr->setWavenumbers(resampler->getkValueLinearized());
	bool refresh = useRefreshDispersionParams();

	if (refresh) {
		updateDispersionParameters();
	}
	else if (update) {
		updateDispersionParameters(offset1, offset2);
	}

	auto values = getCurrentPhaseShiftValues(false);
	return values;
}

void sig_chain::ChainSetup::updateScanAxialResolutions(void)
{
	auto resol1 = getAxialPixelResolution(INAIR_REFLECTIVE_INDEX);
	OctDataSetup::setInAirScanAxialResolution(resol1);
	auto resol2 = getAxialPixelResolution(CORNEA_REFLECTIVE_INDEX);
	OctDataSetup::setCorneaScanAxialResolution(resol2);
	auto resol3 = getAxialPixelResolution(RETINA_REFLECTIVE_INDEX);
	OctDataSetup::setRetinaScanAxialResolution(resol3);

	LogD() << "In-air scan axial resolution: " << resol1;
	LogD() << "Cornea scan axial resolution: " << resol2;
	LogD() << "Retina scan axial resolution: " << resol3;
	return;
}


float * sig_chain::ChainSetup::getCurrentPhaseShiftValues(bool recalc)
{
	PhaseCorrector* phaseCorr = getPhaseCorrector();

	// Update the amounts of phase shift proportional to parameters along k values. 
	if (isCorneaScan()) {
		return phaseCorr->getPhaseShiftValuesToCornea(recalc);
	}
	else {
		return phaseCorr->getPhaseShiftValuesToRetina(recalc);
	}
}

double sig_chain::ChainSetup::getCurrentPhaseShiftOffset(int index)
{
	auto offset = 0.0;
	if (isCorneaScan()) {
		offset = getPhaseCorrector()->getPhaseParameterOffsetToCornea(index);
	}
	else {
		offset = getPhaseCorrector()->getPhaseParameterOffsetToRetina(index);
	}
	return offset;
}

void sig_chain::ChainSetup::resetDynamicDispersionCorrection(void)
{
	getPhaseCorrector()->resetPhaseShiftOptimizer();
	return;
}


void sig_chain::ChainSetup::pauseDynamicDispersionCorrection(bool flag)
{
	getPhaseCorrector()->pausePhaseShiftOptimizer(flag);
	return;
}

bool sig_chain::ChainSetup::isDynamicDispersionCorrection(void)
{
	return getPhaseCorrector()->isPhaseShiftOptimizing();
}


bool sig_chain::ChainSetup::isBackgroundSpectrum(void)
{
	return !getBackground()->isEmpty();
}


bool sig_chain::ChainSetup::clearBackgroundSpectrum(void)
{
	getBackground()->setEmpty(true);
	return true;
}


bool sig_chain::ChainSetup::resetBackgroundSpectrum(const unsigned short * data, int width, int height, float quality)
{
	// if (quality <= 0.5f) {
	if (true) {
		getBackground()->makeBackgroundSpectrum(data, width, height);
		getBackground()->dumpToFile();
		return true;
	}
	return false;
}


unsigned short * sig_chain::ChainSetup::getBackgroundSpectrum(void)
{
	return getBackground()->getProfileData();
}


void sig_chain::ChainSetup::setBackgroundSpectrum(const unsigned short * data)
{
	getBackground()->setProfileData(data);
	return;
}


float * sig_chain::ChainSetup::getResampleFrameData(void)
{
	if (getImpl().resampleFrameData.empty()) {
		getImpl().resampleFrameData = vector<float>(FRAME_DATA_BUFFER_SIZE, 0.0f);
	}
	return &getImpl().resampleFrameData[0];
}


float * sig_chain::ChainSetup::getMagnitudeFrameData(void)
{
	if (getImpl().magnitudeFrameData.empty()) {
		getImpl().magnitudeFrameData = vector<float>(FRAME_DATA_BUFFER_SIZE, 0.0f);
	}
	return &getImpl().magnitudeFrameData[0];
}

double* sig_chain::ChainSetup::getDetectorWavelengths(void)
{
	auto* p = getResampler()->detectorWaveLength();
	return p;
}


double sig_chain::ChainSetup::getAxialPixelResolution(double refractiveIndex)
{
	double resol = getResampler()->getPixelResolution(refractiveIndex);

	return resol;
}

double sig_chain::ChainSetup::getAxialPixelResolution(double fwhm, double ref_index)
{
	double resol = getResampler()->getPixelResolutionInTissue(fwhm, ref_index);
	return resol;
}

double sig_chain::ChainSetup::getWavelengthAtPixelPosition(float index)
{
	double wlen = getResampler()->getWavelengthResampledAtPixelPosition(index);
	return wlen;
}


bool sig_chain::ChainSetup::useDynamicDispersionCorrection(bool isSet, bool flag)
{
	if (isSet) {
		getPhaseCorrector()->enablePhaseShiftOptimizer(flag);
	}

	return getPhaseCorrector()->isPhaseShiftOptimzerEnabled();
}

bool sig_chain::ChainSetup::useBackgroundSubtraction(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useBackgroundSubtraction = flag;
	}
	return getImpl().useBackgroundSubtraction;
}


bool sig_chain::ChainSetup::useFixedNoiseReduction(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useFixedNoiseReduction = flag;
	}
	return getImpl().useFixedNoiseReduction;
}


bool sig_chain::ChainSetup::useDispersionCompensation(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useDispersionCompensation = flag;
	}
	return getImpl().useDispersionCompensation;
}


bool sig_chain::ChainSetup::useAdaptiveGrayscale(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useAdaptiveGrayscale = flag;
	}
	return getImpl().useAdaptiveGrayscale;
}


bool sig_chain::ChainSetup::useFFTZeroPadding(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useFFTZeroPadding = flag;
	}
	return getImpl().useFFTZeroPadding;
}


bool sig_chain::ChainSetup::useRefreshDispersionParams(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useRefreshDispersionParams = flag;
	}
	return getImpl().useRefreshDispersionParams;
}


bool sig_chain::ChainSetup::useACALinePatternPreview(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useACALinePatternPreview = flag;
	}
	return getImpl().useACALinePatternPreview;
}


bool sig_chain::ChainSetup::useFFTWindowing(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useFFTWindowing = flag;
	}
	return getImpl().useFFTWindowing;
}

bool sig_chain::ChainSetup::useKLinearResampling(bool isSet, bool flag)
{
	if (isSet) {
		getImpl().useKLinearResampling = flag;
	}
	return getImpl().useKLinearResampling;
}

bool sig_chain::ChainSetup::useSpectrumDataCallback(bool isset, bool flag)
{
	if (isset) {
		getImpl().useSpectrumDataCallback = flag;
	}
	return getImpl().useSpectrumDataCallback;
}

bool sig_chain::ChainSetup::useResampledDataCallback(bool isset, bool flag)
{
	if (isset) {
		getImpl().useResampledDataCallback = flag;
	}
	return getImpl().useResampledDataCallback;
}

bool sig_chain::ChainSetup::useIntensityDataCallback(bool isset, bool flag)
{
	if (isset) {
		getImpl().useIntensityDataCallback = flag;
	}
	return getImpl().useIntensityDataCallback;
}

int sig_chain::ChainSetup::getFFTImageRowOffset(void)
{
	/*
	if (useACALinePatternPreview()) {
		return FFT_IMAGE_ROW_OFFSET_FOR_ACA_LINE_PREVIEW;
	}
	else {
		return FFT_IMAGE_ROW_OFFSET;
	}
	*/
	return FFT_IMAGE_ROW_OFFSET;
}


int sig_chain::ChainSetup::getMultipleOfZeroPaddingSize(void)
{
	return getImpl().multipleOfZeroPadding;
}


void sig_chain::ChainSetup::setMultipleOfZeroPaddingSize(int size)
{
	getImpl().multipleOfZeroPadding = size;
	return;
}

ChainSetup::ChainSetupImpl & sig_chain::ChainSetup::getImpl(void) 
{
	return *d_ptr;
}


