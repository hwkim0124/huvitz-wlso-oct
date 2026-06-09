#pragma once

#include "SegmScan.h"

#include <memory>
#include <vector>


namespace cpp_util {
	class CvImage;
}

namespace ret_param {
	class EnfaceTmap;
}


namespace segm_scan
{
	class OcularBsegm;

	class SEGMSCAN_DLL_API OcularEnfaceMap
	{
	public:
		OcularEnfaceMap();
		virtual ~OcularEnfaceMap();

		OcularEnfaceMap(OcularEnfaceMap&& rhs);
		OcularEnfaceMap& operator=(OcularEnfaceMap&& rhs);
		OcularEnfaceMap(const OcularEnfaceMap& rhs);
		OcularEnfaceMap& operator=(const OcularEnfaceMap& rhs);

	public:
		virtual bool setupThicknessMap(const OctScanPattern& pattern, 
										OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset,
										const std::vector<OcularBsegm*>& bsegms);
		virtual bool isIdentical(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset) const;
		virtual bool isEmpty(void) const;

		bool fetchThicknessMapDescript(OctThicknessMapDescript& desc) const;

		cpp_util::CvImage buildImage(int width, int height) const;
		ret_param::EnfaceTmap* getThicknessMap(void) const;

	private:
		struct OcularEnfaceMapImpl;
		std::unique_ptr<OcularEnfaceMapImpl> d_ptr;
		OcularEnfaceMapImpl& getImpl(void) const;
	};
}

