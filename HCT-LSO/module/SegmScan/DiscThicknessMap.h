#pragma once

#include "SegmScan.h"
#include "DiscEnfaceMap.h"

#include <memory>
#include <vector>

namespace segm_scan
{
	class SEGMSCAN_DLL_API DiscThicknessMap : public DiscEnfaceMap
	{
	public:
		DiscThicknessMap();
		virtual ~DiscThicknessMap();

		DiscThicknessMap(DiscThicknessMap&& rhs);
		DiscThicknessMap& operator=(DiscThicknessMap&& rhs);
		DiscThicknessMap(const DiscThicknessMap& rhs);
		DiscThicknessMap& operator=(const DiscThicknessMap& rhs);

	private:
		struct DiscThicknessMapImpl;
		std::unique_ptr<DiscThicknessMapImpl> d_ptr;
		DiscThicknessMapImpl& getImpl(void) const;
	};
}

