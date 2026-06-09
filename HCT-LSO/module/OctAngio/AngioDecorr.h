#pragma once

#include "OctAngio.h"
#include "AngioLayers.h"

namespace oct_angio
{
	class AngioData;
	class AngioLayers;
	class AngioLayout;

	class OCTANGIO_DLL_API AngioDecorr
	{
	public:
		AngioDecorr();
		virtual ~AngioDecorr();

		AngioDecorr(AngioDecorr&& rhs);
		AngioDecorr& operator=(AngioDecorr&& rhs);

	public:
		auto estimateThresholds(const AngioLayout& layout, const AngioData& data, const AngioLayers& layers) -> bool;
		auto calculateSignals(const AngioLayout& layout, const AngioData& data, const AngioLayers& layers,
							int overlaps, bool pixelAvg=true, int avgOffset=3, bool circular=false) -> bool;

		auto updateProjectionMasks(const AngioLayout& layout, const LayerArrays& uppers, const LayerArrays& lowers, bool outerFlow=false) -> bool;
		auto updateProjectionProfiles(const AngioLayout& layout, const AngioLayers& layers, 
							bool calcStats = false, bool applyPAR = false,
							const std::vector<float>& maskDecorr = std::vector<float>(),
							const std::vector<float>& maskDiffer = std::vector<float>()) -> bool;


		bool calculateDecorrelations(Amplitudes& amplitudes, Reals& reals, Imaginaries& imags, const AngioLayers& layers = AngioLayers(),
										int angioOverlaps = 3, float upperThresh=0.0f, float lowerThresh=0.0f, bool pixelAverage=true, int averageOffset=2,
										bool differOutput=false, bool circular=false, float decorrThresh=0.0f, bool reflectCorrect=true);
		bool updateProjectionProfiles(const AngioLayout& layout, const LayerArrays& uppers, const LayerArrays& lowers, 
										bool calcStats = false, bool calcMask = false, bool reflectCorrect = true,
										const AngioLayers& maskLayers = AngioLayers(),
										const std::vector<float>& maskDecorr = std::vector<float>(),
										const std::vector<float>& maskDiffer = std::vector<float>());
		bool updateProjectionProfiles2(const AngioLayout& layout, const LayerArrays& uppers, const LayerArrays& lowers, 
										bool calcStats = false, bool calcMask = false, bool reflectCorrect = true,
										const AngioLayers& maskLayers = AngioLayers(),
										const std::vector<float>& maskDecorr = std::vector<float>(),
										const std::vector<float>& maskDiffer = std::vector<float>());
		bool checkIfDecorrelationsLoaded(int lines, int points, int repeats);

		bool calculateBscanImageStats(const AngioLayout& layout);
		bool calculateProjectionStats(void);

		bool normalizeProjectionProfiles(const AngioLayout& layout, const AngioLayers& layers, float decorrMin, float decorrMax, float differMin, 
										float differMax, float dropoff);
		bool normalizeDifferAngiogram(const AngioLayout& layout, const AngioLayers& layers, float rangeMin = 0.0f, float rangeMax = 0.0f, float dropoff = 0.0f);
		bool normalizeDecorrAngiogram(const AngioLayout& layout, const AngioLayers& layers, float rangeMin = 0.0f, float rangeMax = 0.0f, float dropoff = 0.0f);
		bool denoiseProjectionProfiles(int lines, int points, float decorrMin, float decorrMax);

		std::vector<cpp_util::CvImage>& decorrelations(void) const;
		std::vector<cpp_util::CvImage>& differentials(void) const;
		std::vector<float>& decorrAngiogram(void) const;
		std::vector<float>& differAngiogram(void) const;
		std::vector<float>& outputAngiogram(void) const;
		std::vector<float>& decorrProjectionMax(void) const;
		std::vector<float>& decorrProjectionMask(void) const;
		std::vector<float>& differProjectionMask(void) const;
		std::vector<float>& decorrAxialOffsets(void) const;

		cpp_util::CvImage decorrAngiogramImage(bool axialMax=false) const;
		cpp_util::CvImage differAngiogramImage(void) const;

		float* getDecorrelationData(int index);
		float* getDifferentialsData(int index);

		cpp_util::CvImage getDecorrelationImage(int index);
		cpp_util::CvImage getDifferentialsImage(int index);

		float& backgroundThreshold(void);

		void getDecorrelationStat(float& mean, float& stdev, float& maxval);
		void getIntensityStat(float& mean, float& stdev, float& maxval);
		void getDecorrelationStatOfBscan(int index, float& mean, float& stdev, float& maxval);
		void getIntensityStatOfBscan(int index, float& mean, float& stdev, float& maxval);


	private:
		struct AngioDecorrImpl;
		std::unique_ptr<AngioDecorrImpl> d_ptr;
		AngioDecorrImpl& getImpl(void) const;
	};
}


