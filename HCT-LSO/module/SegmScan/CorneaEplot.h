#pragma once

#include "SegmScan.h"
#include "CorneaThicknessMap.h"

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

	class SEGMSCAN_DLL_API CorneaEplot : public CorneaThicknessMap
	{
	public:
		CorneaEplot();
		virtual ~CorneaEplot();

		CorneaEplot(CorneaEplot&& rhs);
		CorneaEplot& operator=(CorneaEplot&& rhs);
		CorneaEplot(const CorneaEplot& rhs);
		CorneaEplot& operator=(const CorneaEplot& rhs);

	public:


	private:
		struct CorneaEplotImpl;
		std::unique_ptr<CorneaEplotImpl> d_ptr;
		CorneaEplotImpl& getImpl(void) const;
	};
}

