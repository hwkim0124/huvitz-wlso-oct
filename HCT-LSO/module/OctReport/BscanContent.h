#pragma once

#include "OctReport.h"

#include <string>
#include <vector>
#include <memory>


namespace oct_result {
	class BscanImage;
}


namespace segm_scan {
	class OcularBsegm;
}


namespace ret_segm {
	class SegmImage;
}


namespace oct_report
{
	class OCTREPORT_DLL_API BscanContent 
	{
	public:
		BscanContent();
		BscanContent(oct_result::BscanImage* image);
		virtual ~BscanContent();

		BscanContent(BscanContent&& rhs);
		BscanContent& operator=(BscanContent&& rhs);
		BscanContent(const BscanContent& rhs) = delete;
		BscanContent& operator=(const BscanContent& rhs) = delete;

	public:
		const OctScanImageDescript* getImageDescript(void) const;
		oct_result::BscanImage* getImageObject(void) const;
		std::uint8_t* getImageBuffer(void) const;
		std::wstring getImageName(bool path = false) const;

		int getImageWidth(void) const;
		int getImageHeight(void) const;
		int getSectionIndex(void) const;
		int getOverlapIndex(void) const;

		void setSectionIndex(int index);
		void setOverlapIndex(int index);
		void setOcularBsegm(std::unique_ptr<segm_scan::OcularBsegm> bsegm);
		bool employBsegmImage(void);

		segm_scan::OcularBsegm* getOcularBsegm(void) const;
		bool isBsegmResult(void) const;
		bool isBsegmEmpty(void) const;

		ret_segm::SegmImage* getBsegmSource(void);
		ret_segm::SegmImage* getBsegmSample(void);
		ret_segm::SegmImage* getBsegmDenoised(void);
		ret_segm::SegmImage* getBsegmGradients(void);
		ret_segm::SegmImage* getBsegmAscends(void);
		ret_segm::SegmImage* getBsegmDescends(void);

		std::vector<int> getLayerPoints(OcularLayerType layer, int width = 0, int height = 0) const;
		std::vector<float> getLayerThickness(OcularLayerType upper, OcularLayerType lower, int width = 0, int height = 0) const;
		std::vector<float> getLayerDistance(OcularLayerType upper, OcularLayerType lower, int width = 0, int height = 0) const;
		std::vector<float> getCurvatureRadius(void) const;
		std::vector<float> getCorneaCurvature(int mapType) const;
		bool isLayerPoints(OcularLayerType layer) const;

		void setLayerPoints(OcularLayerType layer, std::vector<int> points) const;
		void setCurvatureRadius(const std::vector<float>& rads);

		bool isOpticNerveDisc(void) const;
		bool getOpticNerveDiscRange(int& x1, int& x2) const;
		bool getOpticNerveDiscPixels(int& pixels) const;
		bool isOpticNerveCup(void) const;
		bool getOpticNerveCupRange(int& x1, int& x2) const;
		bool getOpticNerveCupPixels(int& pixels) const;

		void setOpticNerveDiscRange(int x1, int x2);
		void setOpticNerveDiscPixels(int pixels);
		void setOpticNerveCupRange(int x1, int x2);
		void setOpticNerveCupPixels(int pixels);

		bool isEmpty(void) const;

		bool fetchRetinaBsegmResult(const OctScanPattern& pattern, OctRetinaBsegmDescriptor& desc) const;
		bool fetchCorneaBsegmResult(const OctScanPattern& pattern, OctCorneaBsegmDescriptor& desc) const;
		bool fetchRetinaBsegmTraits(const OctScanPattern& pattern, OctRetinaBsegmTraits& desc) const;
		bool fetchCorneaBsegmTraits(const OctScanPattern& pattern, OctCorneaBsegmTraits& desc) const;
		bool fetchBsegmLayerPoints(OcularLayerType type, const OctScanPattern& pattern, OctBsegmLayerPoints& layer) const;

		bool exportBsegmResult(std::wstring path, const OctScanPattern desc);
		bool importBsegmResult(std::wstring path, const OctScanPattern desc);

	private:
		struct BscanDataImpl;
		std::unique_ptr<BscanDataImpl> d_ptr;
		BscanDataImpl& getImpl(void) const;
	};
}
