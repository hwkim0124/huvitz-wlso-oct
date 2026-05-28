#pragma once

#include "CppUtil2.h"

#include <string>
#include <memory>


namespace cv {
	class Mat;
}


namespace cpp_util
{
	class CvImage;

	class CPPUTIL_DLL_API CvProcess
	{
	public:
		CvProcess();
		virtual ~CvProcess();

		CvProcess(CvProcess&& rhs);
		CvProcess& operator=(CvProcess&& rhs);
		CvProcess(const CvProcess& rhs);
		CvProcess& operator=(const CvProcess& rhs);

	public:
		bool startAveraging(CvImage& image);
		// bool insertAveraging(CvImage& image, double ccLimit = 0.90, int numIters = 1000, double termEps = 1e-3);
		bool insertAveraging(CvImage& image, double ccLimit = 0.90, int numIters = 100, double termEps = 1e-2);
		int closeAveraging(CvImage& image);

		bool registImage(CvImage& imgBase, CvImage& imgRegist, bool lateral=true, int numIters = 500, double termEps = 1e-4);
		bool registImage2(CvImage& imgBase, CvImage& imgRegist, CvImage& imgRegist2, CvImage& imgRegist3, bool lateral = true, int numIters = 500, double termEps = 1e-4);
	
	private:
		struct CvProcessImpl;
		std::unique_ptr<CvProcessImpl> d_ptr;
		CvProcess::CvProcessImpl& getImpl(void);
	};
}

