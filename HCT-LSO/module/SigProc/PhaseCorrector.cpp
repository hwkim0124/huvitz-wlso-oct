#include "pch.h"
#include "PhaseCorrector.h"

#include <cmath>
#include <queue>

using namespace sig_proc;


struct PhaseCorrector::PhaseCorrectorImpl
{
	double waveNumbers[LINE_CAMERA_CCD_PIXELS];
	double centerOfWavenumbers;

	float shiftValuesToRetina[LINE_CAMERA_CCD_PIXELS];
	float shiftValuesToCornea[LINE_CAMERA_CCD_PIXELS];
	double shiftParamsToRetina[PHASE_SHIFT_PARAMETER_SIZE];
	double shiftParamsToCornea[PHASE_SHIFT_PARAMETER_SIZE];

	double paramsOffsetToRetina[PHASE_SHIFT_PARAMETER_SIZE];
	double paramsOffsetToCornea[PHASE_SHIFT_PARAMETER_SIZE];

	double phaseShiftOffset1 = 0.0;
	double phaseShiftOffset2 = 0.0;

	int countOfPhaseShiftOptimized = 0;
	int countOfTargetToPhaseShift = 0;
	int countOfBackgrToPhaseShift = 0;
	int countOfImageProcessed = 0;
	int countOfPhaseShiftClosed = 0; 

	bool optimizerEnabled = false;
	bool optimizerPaused = false;
	bool optimizerCompleted = false;


	std::priority_queue<pair<double, double>> phaseSignalQueue1;
	std::priority_queue<pair<double, double>> phaseSignalQueue2;

	PhaseCorrectorImpl() 
		: shiftParamsToRetina{ 0.0 }, shiftParamsToCornea{ 0.0 }, 
			shiftValuesToRetina{ 0.0f }, shiftValuesToCornea{ 0.0f }, 
		paramsOffsetToRetina{0.0f}, paramsOffsetToCornea{0.0f}
	{
	}
};


PhaseCorrector::PhaseCorrector() :
	d_ptr(make_unique<PhaseCorrectorImpl>())
{
	setCenterOfWavelengths(WAVELENGTH_CENTER_OF_SOURCE);
}


PhaseCorrector::~PhaseCorrector()
{
}


sig_proc::PhaseCorrector::PhaseCorrector(PhaseCorrector && rhs) = default;
PhaseCorrector & sig_proc::PhaseCorrector::operator=(PhaseCorrector && rhs) = default;


void sig_proc::PhaseCorrector::setCenterOfWavelengths(float center)
{
	getImpl().centerOfWavenumbers = (1.0 / center); // ((2.0 * M_PI) / center);
	return;
}


double sig_proc::PhaseCorrector::getCenterOfWavelengths(void) const
{
	return getImpl().centerOfWavenumbers;
}


void sig_proc::PhaseCorrector::setWavenumbers(float * kvalues)
{
	for (int i = 0; i < LINE_CAMERA_CCD_PIXELS; i++) {
		getImpl().waveNumbers[i] = (kvalues[i] / (2.0 * M_PI));
	}
	return;
}


double * sig_proc::PhaseCorrector::getWavenumbers(void) const
{
	return getImpl().waveNumbers;
}


void sig_proc::PhaseCorrector::setPhaseShiftParameterToRetina(int index, double value)
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		getImpl().shiftParamsToRetina[index] = value;
	}
	return;
}


void sig_proc::PhaseCorrector::setPhaseShiftParameterToCornea(int index, double value)
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		getImpl().shiftParamsToCornea[index] = value;
	}
	return;
}


void sig_proc::PhaseCorrector::setPhaseParameterOffsetToRetina(int index, double value)
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		getImpl().paramsOffsetToRetina[index] = value;
	}
	return;
}


void sig_proc::PhaseCorrector::setPhaseParameterOffsetToCornea(int index, double value)
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		getImpl().paramsOffsetToCornea[index] = value;
	}
	return;
}

auto sig_proc::PhaseCorrector::getPhaseParameterOffsetToRetina(int index) -> double
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		return getImpl().paramsOffsetToRetina[index];
	}
	return 0.0;
}

auto sig_proc::PhaseCorrector::getPhaseParameterOffsetToCornea(int index) -> double
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		return getImpl().paramsOffsetToCornea[index];
	}
	return 0.0;
}

auto sig_proc::PhaseCorrector::clearPhaseParameterOffsetsOfRetina(void) -> void
{
	for (int index = 0; index < PHASE_SHIFT_PARAMETER_SIZE; index++) {
		getImpl().paramsOffsetToRetina[index] = 0.0f;
	}
	return;
}

auto sig_proc::PhaseCorrector::clearPhaseParameterOffsetsOfCornea(void) -> void
{
	for (int index = 0; index < PHASE_SHIFT_PARAMETER_SIZE; index++) {
		getImpl().paramsOffsetToCornea[index] = 0.0f;
	}
	return;
}

double sig_proc::PhaseCorrector::getPhaseShiftParameterToRetina(int index) const
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		return getImpl().shiftParamsToRetina[index];
	}
	return 0.0;
}


double sig_proc::PhaseCorrector::getPhaseShiftParameterToCornea(int index) const
{
	if (index >= 0 && index < PHASE_SHIFT_PARAMETER_SIZE) {
		return getImpl().shiftParamsToCornea[index];
	}
	return 0.0;
}


double sig_proc::PhaseCorrector::getPhaseShiftParameter(bool isRetina, int index) const
{
	return (isRetina ? getPhaseShiftParameterToRetina(index) : getPhaseShiftParameterToCornea(index));
}


float * sig_proc::PhaseCorrector::getPhaseShiftValuesToRetina(bool update)
{
	if (update) {
		updatePhaseShiftValuesToRetina();
	}
	return getImpl().shiftValuesToRetina;
}


float * sig_proc::PhaseCorrector::getPhaseShiftValuesToCornea(bool update)
{
	if (update) {
		updatePhaseShiftValuesToCornea();
	}
	return getImpl().shiftValuesToCornea;
}


PhaseCorrector::PhaseCorrectorImpl & sig_proc::PhaseCorrector::getImpl(void) const
{
	return *d_ptr;
}


void sig_proc::PhaseCorrector::updatePhaseShiftValuesToRetina(void)
{
	double offset1 = getImpl().paramsOffsetToRetina[0];
	double offset2 = getImpl().paramsOffsetToRetina[1];
	double param1 = (getImpl().shiftParamsToRetina[0] + offset1) * PHASE_SHIFT_PARAM1_SCALER;
	double param2 = (getImpl().shiftParamsToRetina[1] + offset2) * PHASE_SHIFT_PARAM2_SCALER;
	double param3 = getImpl().shiftParamsToRetina[2] * PHASE_SHIFT_PARAM3_SCALER;
	double center = getImpl().centerOfWavenumbers;

	double waveDiff;
	double shiftVal[LINE_CAMERA_CCD_PIXELS];

	for (int i = 0; i < LINE_CAMERA_CCD_PIXELS; i++) {
		waveDiff = getImpl().waveNumbers[i] - center;
		shiftVal[i] = (pow(waveDiff, 2.0) * param1 * -1.0 + pow(waveDiff, 3.0) * param2 * -1.0 + pow(waveDiff, 4.0) * param3 * -1.0);
		getImpl().shiftValuesToRetina[i] = (float) shiftVal[i];
	}
	return;
}


void sig_proc::PhaseCorrector::updatePhaseShiftValuesToCornea(void)
{
	double offset1 = getImpl().paramsOffsetToCornea[0];
	double offset2 = getImpl().paramsOffsetToCornea[1];
	double param1 = (getImpl().shiftParamsToCornea[0] + offset1) * PHASE_SHIFT_PARAM1_SCALER;
	double param2 = (getImpl().shiftParamsToCornea[1] + offset2) * PHASE_SHIFT_PARAM2_SCALER;
	double param3 = getImpl().shiftParamsToCornea[2] * PHASE_SHIFT_PARAM3_SCALER;
	double center = getImpl().centerOfWavenumbers;

	double waveDiff;
	double shiftVal[LINE_CAMERA_CCD_PIXELS];

	for (int i = 0; i < LINE_CAMERA_CCD_PIXELS; i++) {
		waveDiff = getImpl().waveNumbers[i] - center;
		shiftVal[i] = (pow(waveDiff, 2.0) * param1 * -1.0 + pow(waveDiff, 3.0) * param2 * -1.0 + pow(waveDiff, 4.0) * param3 * -1.0);
		getImpl().shiftValuesToCornea[i] = (float)shiftVal[i];
	}
	return;
}


bool sig_proc::PhaseCorrector::updateImageProcessedCount(float quality)
{
	const float TARGET_QUALITY = 3.5f;
	const int TARGET_COUNT_MIN = 25; // 40;
	const int BACKGR_COUNT_MIN = 25; // 40;

	getImpl().countOfImageProcessed += 1;

	if (quality > TARGET_QUALITY) {
		getImpl().countOfTargetToPhaseShift += 1;
		if (getImpl().countOfTargetToPhaseShift > TARGET_COUNT_MIN) {
			getImpl().countOfBackgrToPhaseShift = 0;
		}
	}
	else {
		getImpl().countOfBackgrToPhaseShift += 1;
		if (getImpl().countOfBackgrToPhaseShift > BACKGR_COUNT_MIN) {
			getImpl().countOfTargetToPhaseShift = 0;
			getImpl().optimizerCompleted = false;
		}
	}

	return isReadyForPhaseShiftOptimize(quality);
}


bool sig_proc::PhaseCorrector::isReadyForPhaseShiftOptimize(float quality)
{
	const float TARGET_QUALITY = 3.5f;
	const int TARGET_PERIOD = 25; // 50;

	if (quality > TARGET_QUALITY) {
		if (getImpl().countOfTargetToPhaseShift > 0 &&
			getImpl().countOfTargetToPhaseShift % TARGET_PERIOD == 0 && 
			getImpl().optimizerCompleted != true) {
			return true;
		}
	}
	return false;
}

void sig_proc::PhaseCorrector::enablePhaseShiftOptimizer(bool flag)
{
	getImpl().optimizerEnabled = flag;
}

void sig_proc::PhaseCorrector::pausePhaseShiftOptimizer(bool flag)
{
	getImpl().optimizerPaused = flag;
}

bool sig_proc::PhaseCorrector::isPhaseShiftOptimizing(void)
{
	return getImpl().optimizerEnabled && !getImpl().optimizerPaused;
}

bool sig_proc::PhaseCorrector::isPhaseShiftOptimzerEnabled(void)
{
	return getImpl().optimizerEnabled;
}


auto sig_proc::PhaseCorrector::resetPhaseShiftOptimizer(void) -> void 
{
	clearPhaseShiftTargetCounts(true);
	clearPhaseShiftSignalQueues();

	getImpl().phaseShiftOffset1 = 0.0;
	getImpl().phaseShiftOffset2 = 0.0;
	// getImpl().optimizerEnabled = start;
	getImpl().optimizerPaused = false;
	getImpl().optimizerCompleted = false;
	return;
}


auto sig_proc::PhaseCorrector::clearPhaseShiftSignalQueues(void) -> void
{
	getImpl().phaseSignalQueue1 = priority_queue<pair<double, double>>();
	getImpl().phaseSignalQueue2 = priority_queue<pair<double, double>>();
	return;
}


auto sig_proc::PhaseCorrector::clearPhaseShiftTargetCounts(bool reset) -> void
{
	getImpl().countOfBackgrToPhaseShift = 0;
	getImpl().countOfTargetToPhaseShift = 0;

	if (reset) {
		getImpl().countOfPhaseShiftOptimized = 0;
		getImpl().countOfImageProcessed = 0;
		getImpl().countOfPhaseShiftClosed = 0;
	}
	return;
}


auto sig_proc::PhaseCorrector::getPhaseShiftDistanceTable(int index) -> std::vector<double>
{
	// return std::vector<double> { -5.0, +5.0, -3.0, +3.0, -1.5, +1.5, -0.5, +0.5, 0.0 };
	return std::vector<double> { -1.0, +1.0, -0.5, +0.5, -0.25, +0.25, 0.0 };
}


auto sig_proc::PhaseCorrector::putPhaseShiftSignalResult(int index, double delta, double signal) -> void
{
	getImpl().phaseSignalQueue1.push(make_pair(signal, delta));
	return;
}


auto sig_proc::PhaseCorrector::fetchPhaseShiftDeltaOfMax(int index) -> double
{
	auto delta = 0.0;
	
	if (!getImpl().phaseSignalQueue1.empty()) {
		auto pair = getImpl().phaseSignalQueue1.top();
		delta = pair.second;
	}
	return delta;
}


auto sig_proc::PhaseCorrector::optimizePhaseShiftOffsets(void) -> void
{
	getImpl().countOfPhaseShiftOptimized += 1;

	auto index = 0;
	auto delta1 = fetchPhaseShiftDeltaOfMax(index);
	auto offset1 = getImpl().phaseShiftOffset1;
	auto km_fact = (1.0 / sqrt((double)getImpl().countOfPhaseShiftOptimized));
	auto km_dist = delta1 * km_fact;
	auto predict = offset1 + km_dist;

	LogD() << "Phase shift optimized count: " << getImpl().countOfPhaseShiftOptimized;
	LogD() << "Max signal by delta: " << delta1 << ", km_factor: " << km_fact;
	LogD() << "Shift offset next: " << offset1 << " => " << predict;

	auto& queue = getImpl().phaseSignalQueue1;
	while (!queue.empty()) {
		auto pair = queue.top();
		LogD() << "Signal: " << pair.first << ", delta: " << pair.second << ", offset: " << (pair.second + offset1);
		queue.pop();
	}

	getImpl().phaseShiftOffset1 = predict;

	if (km_dist < 0.5) {
		getImpl().countOfPhaseShiftClosed += 1;
		if (getImpl().countOfPhaseShiftClosed > 3) {
			getImpl().optimizerCompleted = true;
		}
	}
	else {
		getImpl().countOfPhaseShiftClosed = 0;
	}
	return;
}

auto sig_proc::PhaseCorrector::getPhaseShiftOffset1(void) -> double
{
	return getImpl().phaseShiftOffset1;
}

auto sig_proc::PhaseCorrector::getPhaseShiftOffset2(void) -> double
{
	return getImpl().phaseShiftOffset2;
}


bool sig_proc::PhaseCorrector::isPhaseShiftOptimized(void)
{
	return getImpl().countOfPhaseShiftOptimized > 0;
}
