#pragma once
#include "CorneaEnfaceMap.h"

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

	class SEGMSCAN_DLL_API CorneaThicknessMap : public CorneaEnfaceMap
	{
	public:
		CorneaThicknessMap();
		virtual ~CorneaThicknessMap();

		CorneaThicknessMap(CorneaThicknessMap&& rhs);
		CorneaThicknessMap& operator=(CorneaThicknessMap&& rhs);
		CorneaThicknessMap(const CorneaThicknessMap& rhs);
		CorneaThicknessMap& operator=(const CorneaThicknessMap& rhs);

	public:


	private:
		struct CorneaThicknessMapImpl;
		std::unique_ptr<CorneaThicknessMapImpl> d_ptr;
		CorneaThicknessMapImpl& getImpl(void) const;
	};
}