#pragma once

#include "NormData.h"

#include <memory>
#include <vector>


namespace norm_data
{
	class NORMDATA_DLL_API DiscTsnitGraph 
	{
	public:
		DiscTsnitGraph();
		virtual ~DiscTsnitGraph();

	public:
		static std::vector<float> getNormalLine(int size);
		static std::vector<float> getBorderLine(int size);
		static std::vector<float> getOutsideLine(int size);

	protected:
		static std::vector<float> resizeData(const std::vector<float>& data, int size);
		static std::vector<float> getNormalLineData(void);
		static std::vector<float> getBorderLineData(void);
		static std::vector<float> getOutsideLineData(void);
	};
}