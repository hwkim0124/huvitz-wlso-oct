#pragma once

#include "SigChain2.h"

#include <memory>
#include <vector>

#include "CL\cl.h"
#include "openCL.misc.h"

#ifndef _NCLFFT
#include "clFFT.h"
#endif

namespace sig_chain
{
	class OclSystem2;
	class OclTaskPlan;


	class SIGCHAIN_DLL_API OclContext
	{
	public:
		OclContext();
		virtual ~OclContext();

	public:
		static bool initializeContext(void);
		static bool isInitiated(void);
		static void releaseContext(void);
		static bool prepareContext(void);
		static bool executeContext(OclTaskPlan* plan, const std::uint16_t* input, std::uint8_t* output, std::uint8_t* laterals, float* output2 = nullptr);

		static OclTaskPlan* requestTaskPlanForPreview(int dataLen, int batchSize);
		static OclTaskPlan* requestTaskPlanForEnface(int dataLen, int batchSize);
		static OclTaskPlan* requestTaskPlanForMeasure(int dataLen, int batchSize);
		static OclTaskPlan* getTaskPlanForPreview(int dataLen, int batchSize);
		static OclTaskPlan* getTaskPlanForEnface(int dataLen, int batchSize);
		static OclTaskPlan* getTaskPlanForMeasure(int dataLen, int batchSize);
		static void releaseTaskPlans(void);

	protected:
		static OclTaskPlan* requestTaskPlan(int dataLen, int batchSize, bool unique = false);
		static int insertTaskPlan(int dataLen, int batchSize);

		static OclTaskPlan* getCurrentTaskPlan(void);
		static void setCurrentTaskPlan(OclTaskPlan* plan);
		static bool realizeTaskPlan(OclTaskPlan* plan);

		static bool prepareResamplingParameters(void);
		static bool prepareSubtractionParameters(void);
		static bool preparePhaseShiftParameters(OclTaskPlan* plan);

		static bool performSpectrumDataCallback(OclTaskPlan* plan, const std::uint16_t* input);
		static bool performBackgroundSubtraction(OclTaskPlan* plan, const std::uint16_t* input);
		// Full range plans with the moving background option set: subtracts from every spectrum
		// sample its mean over the neighbouring A-lines. Does nothing otherwise.
		static bool performMovingBackground(OclTaskPlan* plan);
		// Full range plans with the axial flattening option set: removes the slowly varying part
		// of each background subtracted spectrum, then applies the spectral window. Does nothing
		// otherwise, which leaves the windowing to the background subtraction.
		static bool performAxialFlattening(OclTaskPlan* plan);
		static bool performResamplingInput(OclTaskPlan* plan);
		static bool performPhaseShiftTransform(OclTaskPlan* plan);
		// Turns the resampled real spectra into the complex spectra the depth FFT consumes:
		// the full range lateral sideband on a full range plan, the per A-line analytic
		// signal with dispersion correction otherwise.
		static bool performComplexSpectrumTransform(OclTaskPlan* plan);
		static bool performFullRangeTransform(OclTaskPlan* plan);
		static bool performZeroPaddingInput(OclTaskPlan* plan);
		static bool performInverseTransform(OclTaskPlan* plan);
		static bool performZeroPaddingOutput(OclTaskPlan* plan);
		static bool performFixedNoiseReduction(OclTaskPlan* plan);
		static bool performMagnitudeOfComplex(OclTaskPlan* plan);
		static bool performMagnitudeHistogram(OclTaskPlan* plan);
		static bool performAdaptiveGrayScale(OclTaskPlan* plan, std::uint8_t* output);
		static bool performIntensityAmplitude(OclTaskPlan* plan, float* output);
		static bool performAxialProjection(OclTaskPlan* plan, std::uint8_t* laterals);
		static bool performPhaseShiftOptimization(OclTaskPlan* plan);

		static bool initializeKernels(void);
		static void releaseKernels(void);
		static bool createCommandQueue(void);
		static bool createBackgroundSubtractionKernel(void);
		static bool createResamplingInputKernel(void);
		static bool createFixedNoiseReductionKernel(void);
		static bool createComplexMagnitudeKernel(void);
		static bool createMagnitudeHistogramKernel(void);
		static bool createAdaptiveGrayScaleKernel(void);
		static bool createIntensityAmplitudeKernel(void);
		static bool createAxialProjectionKernel(void);
		static bool createFullRangeKernel(void);

		static void releaseCommandQueue(void);
		static void releaseBackgroundSubtractionKernel(void);
		static void releaseResamplingInputKernel(void);
		static void releaseFixedNoiseReductionKernel(void);
		static void releaseComplexMagnitudeKernel(void);
		static void releaseMagnitudeHistogramKernel(void);
		static void releaseAdaptiveGrayScaleKernel(void);
		static void releaseIntensityAmplitudeKernel(void);
		static void releaseAxialProjectionKernel(void);
		static void releaseFullRangeKernel(void);

	private:
		struct OclContextImpl;
		static std::unique_ptr<OclContextImpl> d_ptr;
		static OclContextImpl& getImpl(void);

		friend class OclSystem2;
	};
}

