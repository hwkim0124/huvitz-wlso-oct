#pragma once

#include "CppUtil2.h"

#include <string>
#include <memory>
#include <vector>


namespace cv {
	class Mat;
}


namespace cpp_util
{
	class GuidedFilterImpl;

	class GuidedFilter
	{
	public:
		GuidedFilter(const cv::Mat &I, int r, double eps);
		~GuidedFilter();

		cv::Mat filter(const cv::Mat &p, int depth = -1) const;

	private:
		cpp_util::GuidedFilterImpl *impl_;
	};


	cv::Mat CPPUTIL_DLL_API guidedFilter(const cv::Mat &I, const cv::Mat &p, int r, double eps, int depth = -1);
	cv::Mat CPPUTIL_DLL_API guidedFilter(const cv::Mat &I, int r, double eps, int depth = -1);
}

