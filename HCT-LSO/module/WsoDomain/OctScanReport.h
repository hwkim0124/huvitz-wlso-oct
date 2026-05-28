#pragma once

#include "OctDataDefines.h"


namespace wso_domain
{
	// Constant Definitions
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int OCT_GCC_CHART_SECTION_SIZE = 6;
	constexpr float OCT_GCC_CHART_INNER_RANGE = 1.0f;
	constexpr float OCT_GCC_CHART_OUTER_RANGE = 4.0f;

	constexpr int OCT_ETDRS_CHART_SECTION_SIZE = 9;
	constexpr int OCT_ETDRS_CHART_SIDE_SIZE = 4;

	constexpr int OCT_RNFL_CHART_SECTION_SIZE = 12;
	constexpr int OCT_RNFL_CHART_QUAD_SIZE = 4;
	constexpr int OCT_RNFL_TSNIT_GRAPH_SIZE = 360;

	constexpr int OCT_ANGIO_FLOWS_SECTION_SIZE = 4;


	// Oct Layer Descriptor
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct OctLayerStratumParam
	{
		OcularLayerType upperType = OcularLayerType::UNKNOWN;
		OcularLayerType lowerType = OcularLayerType::UNKNOWN;
		float upperOffset = 0.0f;
		float lowerOffset = 0.0f;
		float chartCenterX = 0.0f;
		float chartCenterY = 0.0f;
	};


	// Oct Image Descriptor 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct OctScanImageDescript
	{
	public:
		const unsigned char* pData = nullptr;
		int width = 0;
		int height = 0;

		float quality = 0.0f;
		float sigRatio = 0.0f;
		int refPoint = 0;

		int imageIndex = 0;
		float scanRange = 6.0f;
		bool isReversed = false;

	public:
		OctScanImageDescript() {
			init();
		}

		OctScanImageDescript(const unsigned char* data, int width, int height, float quality = 0.0f, float sigRatio = 0.0f, int refPoint = 0) {
			setup(data, width, height, quality, sigRatio, refPoint);
		}

		void init(void) {
			this->pData = nullptr;
			this->width = 0;
			this->height = 0;
			this->quality = 0.0f;
			this->sigRatio = 0.0f;
			this->refPoint = 0;
			this->isReversed = false;
			this->imageIndex = 0;
			return;
		}

		void setup(const unsigned char* data, int width, int height, float quality = 0.0f, float sigRatio = 0.0f, int refPoint = 0) {
			this->pData = data;
			this->width = width;
			this->height = height;
			this->quality = quality;
			this->sigRatio = sigRatio;
			this->refPoint = refPoint;
			return;
		}

		bool isEmpty(void) const {
			return pData == nullptr || width <= 0 || height <= 0;
		}
	};


	struct OctFrameImageDescript
	{
		const unsigned char* pData = nullptr;
		int width = 0;
		int height = 0;

	public:
		OctFrameImageDescript() {
		}

		OctFrameImageDescript(const unsigned char* data, int width, int height) {
			setup(data, width, height);
		}

		void setup(const unsigned char* data, int width, int height) {
			this->pData = data;
			this->width = width;
			this->height = height;
		}

		const unsigned char* getBuffer(void) const {
			return pData;
		}
	};

	struct OctDataImageDescript
	{
		const float* pData = nullptr;
		int width = 0;
		int height = 0;

	public:
		OctDataImageDescript() {
		}

		OctDataImageDescript(const float* data, int width, int height) {
			setup(data, width, height);
		}

		void setup(const float* data, int width, int height) {
			this->pData = data;
			this->width = width;
			this->height = height;
		}

		const float* getBuffer(void) const {
			return pData;
		}
	};


	struct OctRetinaImageDescript : public OctFrameImageDescript
	{
	public:
		OctRetinaImageDescript() {
		}

		OctRetinaImageDescript(const unsigned char* data, int width, int height)
			: OctFrameImageDescript(data, width, height)
		{
		}
	};


	struct OctCorneaImageDescript : public OctFrameImageDescript
	{
	public:
		OctCorneaImageDescript() {
		}

		OctCorneaImageDescript(const unsigned char* data, int width, int height)
			: OctFrameImageDescript(data, width, height)
		{
		}
	};


	struct OctEnfaceImageDescript : public OctFrameImageDescript
	{

	public:
		OctEnfaceImageDescript() {
		}

		OctEnfaceImageDescript(const unsigned char* data, int width, int height)
			: OctFrameImageDescript(data, width, height)
		{
		}
	};

	struct OctAngioImageDescript : public OctFrameImageDescript
	{

	public:
		OctAngioImageDescript() {
		}

		OctAngioImageDescript(const unsigned char* data, int width, int height)
			: OctFrameImageDescript(data, width, height)
		{
		}
	};


	// Oct Thickness Map & Chart Descriptor
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct OctThicknessMapDescript : public OctDataImageDescript
	{
	public:
		OctThicknessMapDescript() {
		}

		OctThicknessMapDescript(const float* data, int width, int height)
			: OctDataImageDescript(data, width, height)
		{
		}
	};


	struct OctETDRSChartDescript 
	{
		float centerThick = 0.0f;
		float centerPercent = 0.0f;

		float innerThicks[OCT_ETDRS_CHART_SIDE_SIZE] = { 0.0f };
		float outerThicks[OCT_ETDRS_CHART_SIDE_SIZE] = { 0.0f };
	
		float innerPercents[OCT_ETDRS_CHART_SIDE_SIZE] = { 0.0f };
		float outerPercents[OCT_ETDRS_CHART_SIDE_SIZE] = { 0.0f };

		float superiorThick = 0.0f;
		float inferiorThick = 0.0f;
		float averageThick = 0.0f;

		EyeSide eyeSide = EyeSide::OD;
		float centerX = 0.0f;
		float centerY = 0.0f;

		OctLayerStratumParam stratumParam;
	};

	struct OctGCCThickChartDescript
	{
		float sectionThicks[OCT_GCC_CHART_SECTION_SIZE] = { 0.0f };
		float sectionPercents[OCT_GCC_CHART_SECTION_SIZE] = { 0.0f };

		float superiorThick = 0.0f;
		float inferiorThick = 0.0f;
		float averageThick = 0.0f;

		float superiorPercent = 0.0f;
		float inferiorPercent = 0.0f;

		float innerDiam = OCT_GCC_CHART_INNER_RANGE;
		float outerDiam = OCT_GCC_CHART_OUTER_RANGE;

		EyeSide eyeSide = EyeSide::OD;
		float centerX = 0.0f;
		float centerY = 0.0f;

		OctLayerStratumParam stratumParam;
	};

	struct OctAngioChartDescript
	{
		float centerFlows = 0.0f;
		float centerDensity = 0.0f;

		float innerFlows[OCT_ANGIO_FLOWS_SECTION_SIZE] = { 0.0f };
		float outerFlows[OCT_ANGIO_FLOWS_SECTION_SIZE] = { 0.0f };

		float innerDensity[OCT_ANGIO_FLOWS_SECTION_SIZE] = { 0.0f };
		float outerDensity[OCT_ANGIO_FLOWS_SECTION_SIZE] = { 0.0f };

		float superiorFlows = 0.0f;
		float inferiorFlows = 0.0f;
		float averageFlows = 0.0f;

		float superiorDensity = 0.0f;
		float inferiorDensity = 0.0f;
		float averageDensity = 0.0f;

		EyeSide eyeSide = EyeSide::OD;
		float centerX = 0.0f;
		float centerY = 0.0f;

		OctLayerStratumParam stratumParam;
	};

	struct OctClockChartDescript 
	{
		float clockThicks[OCT_RNFL_CHART_SECTION_SIZE] = { 0.0f };
		float clockPercents[OCT_RNFL_CHART_SECTION_SIZE] = { 0.0f };

		float quadThicks[OCT_RNFL_CHART_QUAD_SIZE] = { 0.0f };
		float quadPercents[OCT_RNFL_CHART_QUAD_SIZE] = { 0.0f };

		float graphTSNIT[OCT_RNFL_TSNIT_GRAPH_SIZE] = { 0.0f };
		float graphNormal[OCT_RNFL_TSNIT_GRAPH_SIZE] = { 0.0f };
		float graphBorder[OCT_RNFL_TSNIT_GRAPH_SIZE] = { 0.0f };
		float graphOutside[OCT_RNFL_TSNIT_GRAPH_SIZE] = { 0.0f };

		EyeSide eyeSide = EyeSide::OD;
		float centerX = 0.0f;
		float centerY = 0.0f;

		OctLayerStratumParam stratumParam;
	};


	// Oct Report Summary Descriptor 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct OctMacularSummaryDescript
	{
		EyeSide eyeSide = EyeSide::OD;
		float diopter = 0.0f;

		float foveaCenterX = 0.0f;
		float foveaCenterY = 0.0f;
		float foveaCenterThick = 0.0f;
		int foveaCenterLine = 0;
		int foveaCenterXpos = 0;
		
		bool isFoveaValid = false;
	};

	struct OctOpticDiscSummaryDescript
	{
		EyeSide eyeSide = EyeSide::OD;
		float diopter = 0.0f;

		int discStartLine;
		int discCloseLine;
		int discCenterLine;
		int discCenterXpos;
		int discXposMin;
		int discXposMax;

		float discArea;
		float discVolume;

		int cupStartLine;
		int cupCloseLine;
		int cupCenterLine;
		int cupCenterXpos;
		int cupXposMin;
		int cupXposMax;

		float cupArea;
		float cupVolume;
		float rimArea;
		float rimVolume;

		int nerveHeadCenterLine;
		int nerveHeadCenterXpos;
		float nerveHeadCenterX;
		float nerveHeadCenterY;

		float cupDiscHorzRatio;
		float cupDiscVertRatio;
		float cupDiscAreaRatio;

		bool isOpticCupValid;
		bool isOpticDiscValid;
		bool isNerveHeadCenterValid;
		bool isCupDiscRatioValid;
	};

	struct OctCorneaSummaryDescript
	{

	};
}