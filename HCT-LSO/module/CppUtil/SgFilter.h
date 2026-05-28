#pragma once

#include "CppUtil2.h"


#include <vector>

namespace cpp_util
{
	class CPPUTIL_DLL_API SgFilter
	{
	public:
		SgFilter();
		~SgFilter();

	public:
		static std::vector<double> smoothDoubles(std::vector<double> data, int window, int degree);
		static std::vector<int> smoothInts(std::vector<int> data, int window, int degree);
	};
}

