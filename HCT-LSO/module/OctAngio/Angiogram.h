#pragma once

#include "OctAngio.h"

#include <memory>


namespace cpp_util {
	class CvImage;
}


namespace oct_angio
{
	class AngioFile;
	class AngioData;
	class AngioLayout;
	class AngioLayers;
	class AngioDecorr;
	class AngioChart;
	class AngioMotion;
	class AngioPost;

	class OCTANGIO_DLL_API Angiogram
	{
	public:
		Angiogram();
		virtual ~Angiogram();

		Angiogram(Angiogram&& rhs);
		Angiogram& operator=(Angiogram&& rhs);

	public:
		void setupAngioPattern(const OctScanPattern& pattern);
		bool setupAngioData(std::vector<std::vector<CvImage>>&& ampls);
		bool setupAngioLayers(LayerMapArrays&& layers);

		void resetLayout(int lines, int points, int repeats, bool vertical = false);
		void resetScanRange(float rangeX, float rangeY, float centerX=0.0f, float centerY=0.0f, bool isDisc=false);
		void resetSlabRange(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset);

		std::string importPath(void);
		void setImportPath(std::string path);

		bool loadDataBuffer(void);
		bool loadDataFiles(const std::string dirPath = ".//export", const std::string fileName = "angio.dat");
		bool loadDataFile2(const std::string dirPath = ".//export", const std::string fileName = "angio2.dat");
		bool loadDataImages(const std::string dirPath = ".//export");
		bool saveDataFiles(const std::string dirPath = ".//export", const std::string fileName = "angio.dat");
		bool saveDataFile2(const std::string dirPath = ".//export", const std::string fileName = "angio2.dat");

		bool isAmplitduesValid(void);
		bool isDecorrelationsValid(void);
		bool isAngioImage(void);
		bool isFoveaAvascularZone(void);

		bool prepareData(bool align=true);
		bool prepareData2(void);

		/*
		bool loadLayerSegments(const std::string dirPath = "");
		bool loadLayerSegments(OcularLayerType upper, OcularLayerType lower, float upperOffset=0.0f, float lowerOffset=0.0f, const std::string dirPath = "");
		void setLayerSegments(AngioLayers&& layers);
		void setLayerSegments(OcularLayerType upper, OcularLayerType lower, 
								LayerArrays& upperLayers, LayerArrays& lowerLayers,
								LayerArrays& upperLayers2, LayerArrays& lowerLayers2, 
								LayerArrays& upperLayers3, LayerArrays& lowerLayers3);
		*/

		bool processData(bool calcStats = true);
		bool processData2(void);

		bool buildImageBitmap(bool decorr=false);
		const unsigned char* imageBits(void);
		int imageWidth(void) ;
		int imageHeight(void) ;

		cpp_util::CvImage createAnigoImage(bool enhance);
		cpp_util::CvImage createOffsetImage(void);
		cpp_util::CvImage createDecorrImage(int lineIdx);
		cpp_util::CvImage createScanImage(int lineIdx);
	
		std::vector<float> getDecorrelationsOnHorzLines(bool axialMax=false);
		std::vector<float> getDecorrelationsOnVertLines(bool axialMax=false);
		std::vector<int> getUpperLayerPoints(int imageIdx);
		std::vector<int> getLowerLayerPoints(int imageIdx);

		int numberOfDecorrImages(void);
		int numberOfOverlapImages(void);
		float* getDecorrImageBits(int lineIdx);

		unsigned char* getScanImageBits(int lineIdx, int repeatIdx=0);
		int scanImageWidth(void);
		int scanImageHeight(void);
		float scanRangeX(void);
		float scanRangeY(void);

	protected:
		void alignDataToBase(void);
		bool produceFlowSignals(bool align);
		bool buildProjectionMasks(void);
		bool buildProjectionImages(bool calcStats);
		bool processProjectionImages(void);
		bool normalizeProjectionImages(void);

	public:
		bool& useAlignAxial(void);
		bool& useAlignLateral(void);
		bool& useDecorrCircular(void);

		bool& useLayersSelected(void);
		bool& useVascularLayers(void);
		bool& useMotionCorrection(void);
		bool& useBiasFieldCorrection(void);
		bool& usePostProcessing(void);
		bool& useNormProjection(void);
		bool& useDecorrOutput(void);
		bool& useDifferOutput(void);
		bool& useProjectionArtifactRemoval(void);
		bool& useReflectionCorrection(void);
		bool& useProjectionStatistics(void);

		float& intensityUpperThreshold(void);
		float& intensityLowerThreshold(void);
		float& decorrLowerThreshold(void);
		float& decorrUpperThreshold(void);
		float& differLowerThreshold(void);
		float& differUpperThreshold(void);

		float& decorrBaseThreshold(void);
		float& biasFieldSigma(void);
		float& noiseReductionRate(void);
		float& normalizeDropOff(void);

		int& numberOfOverlaps(void);
		bool& usePixelAverage(void);
		int& pixelAverageOffset(void);


		oct_angio::AngioLayout& Layout(void);
		oct_angio::AngioData& Data(void);
		oct_angio::AngioLayers& Layers(void);
		// oct_angio::AngioLayers& Layers2(void);
		// oct_angio::AngioLayers& Layers3(void);
		oct_angio::AngioDecorr& Decorr(void);
		oct_angio::AngioDecorr& Decorr2(void);
		oct_angio::AngioDecorr& Decorr3(void);
		oct_angio::AngioMotion& Motion(void);
		oct_angio::AngioPost& Post(void);

	private:
		struct AngiogramImpl;
		std::unique_ptr<AngiogramImpl> d_ptr;
		AngiogramImpl& getImpl(void) const;
	};
}

