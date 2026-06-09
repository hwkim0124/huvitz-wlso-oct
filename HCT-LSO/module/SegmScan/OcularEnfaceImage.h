#pragma once

#include "SegmScan.h"

#include <memory>
#include <vector>


namespace cpp_util {
	class CvImage;
}

namespace segm_scan
{
	class OcularBsegm;

	class SEGMSCAN_DLL_API OcularEnfaceImage 
	{
	public:
		OcularEnfaceImage();
		virtual ~OcularEnfaceImage();

		OcularEnfaceImage(OcularEnfaceImage&& rhs);
		OcularEnfaceImage& operator=(OcularEnfaceImage&& rhs);
		OcularEnfaceImage(const OcularEnfaceImage& rhs);
		OcularEnfaceImage& operator=(const OcularEnfaceImage& rhs);

	public:
		virtual bool setupEnfaceImage(const OctScanPattern& pattern,
									OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset,
									const std::vector<OcularBsegm*>& bsegms);
		virtual bool isIdentical(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset) const;

		bool isEmpty(void) const;
		bool fetchEnfaceImageDescript(OctEnfaceImageDescript& desc) const;

		cpp_util::CvImage buildImage(int width, int height) const;

	private:
		struct OcularEnfaceImageImpl;
		std::unique_ptr<OcularEnfaceImageImpl> d_ptr;
		OcularEnfaceImageImpl& getImpl(void) const;
	};
}

