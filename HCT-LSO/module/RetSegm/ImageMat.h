#pragma once

#include "RetSegm.h"

#include <string>
#include <memory>
#include <vector>

namespace cv {
	class Mat;
}

namespace Gdiplus {
	class Bitmap;
}

namespace ret_segm
{
	class RETSEGM_DLL_API ImageMat
	{
	public:
		ImageMat();
		virtual ~ImageMat();

		ImageMat(ImageMat&& rhs);
		ImageMat& operator=(ImageMat&& rhs);

	public:
		bool fromFile(std::wstring path);
		bool fromBitmap(int width, int height, const unsigned char* pBits, int paddingSize);
		const unsigned char* getBitsData(void) const;
		int getWidth(void) const;
		int getHeight(void) const;
		int getRows(void) const;
		int getCols(void) const;
		int getWidthStep(void) const;
		int getByteSize(void) const;

		void convertToRGB(ImageMat* pRGBMat) const;
		cv::Mat& getCvMat(void);
		const cv::Mat& getCvMatConst(void) const;

		void setMeanStddev(float mean, float stddev);
		float getMean(void) const;
		float getStddev(void) const;
		float getSampleRatioWidth(void) const;
		float getSampleRatioHeight(void) const;
		void setSampleRatio(float widthRatio, float heightRatio);

		std::vector<int>& getMeanOfColumns(void) const;
		std::vector<int>& getMaxValOfColumns(void) const;
		std::vector<int>& getMaxIdxOfColumns(void) const;

		void setMeanOfColumns(const cv::Mat& mean);
		void setMaxValOfColumns(const cv::Mat& vals);
		void setMaxIdxOfColumns(const cv::Mat& idxs);


	private:
		struct ImageMatImpl;
		std::unique_ptr<ImageMatImpl> d_ptr;
		ImageMatImpl& impl(void) const;

		// Even though friend declarations violates ImageMat's encapsulation, 
		// theses classes implement algorithms processing ImageMat's internal data as input, 
		// and its output could be got back and stored within ImageMat for the further uses
		// without through a little cumbersome interfaces. 

		// This is restricted to the classes only for the purpose of data processing to ImageMat. 
		friend class Preprocess;
		friend class Coarse;
	};
}
