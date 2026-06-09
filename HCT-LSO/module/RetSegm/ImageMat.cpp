#include "pch.h"
#include "RetSegm2.h"
#include "ImageMat.h"

using namespace cv;

#include <iostream>
using namespace std;
using namespace ret_segm;

struct ImageMat::ImageMatImpl
{
	// cv::Mat_<uchar> m_imageMat;
	std::unique_ptr<cv::Mat> m_pCvMat;
	std::vector<int> m_meanCols;
	std::vector<int> m_maxValCols;
	std::vector<int> m_maxIdxCols;

	float m_mean;
	float m_stddev;
	float m_sampleRatioWidth;
	float m_sampleRatioHeight;

	ImageMatImpl() {
		m_pCvMat = make_unique<cv::Mat>();
		m_sampleRatioWidth = 1.0f;
		m_sampleRatioHeight = 1.0f;
	};
};


ImageMat::ImageMat()
	: d_ptr(make_unique<ImageMatImpl>())
{

}

ret_segm::ImageMat::~ImageMat()
{
}

ret_segm::ImageMat::ImageMat(ImageMat&& rhs) = default;
ImageMat& ret_segm::ImageMat::operator=(ImageMat&& rhs) = default;


bool ImageMat::fromFile(std::wstring path)
{
	// Convert a path string in UTF-16 to UTF-8.
	string str = wtoa(path);

#ifdef _DEBUG
	LogD() << "Image file loaded : " << str;
#endif

	// c++ interface of opencv, IplImage and cvLoadImage has been deprecated. 
	Mat& cvMat = getCvMat();
	cvMat = imread(str.c_str(), IMREAD_GRAYSCALE);
	if (cvMat.empty()) {
#ifdef _DEBUG
		LogD() << "Can't open the file";
#endif
		return false;
	}


	/*
	// resize(m_imageMat, m_imageMat, cv::Size(), 1.0, 0.5, INTER_AREA);
	// GaussianBlur(m_imageMat, m_imageMat, Size(), 5.0);
	Mat matResult;
	m_imageMat.copyTo(matResult);
	bilateralFilter(m_imageMat, matResult, 15, 60.0, 20.0);
	m_imageMat = matResult;
	*/
	return true;
}

bool ImageMat::fromBitmap(int width, int height, const unsigned char* pBits, int paddingSize)
{
	// initialize matrix header that points to the bits data. no data is copied. 
	// if padding size is zero, no padding bytes at the end of each row is assumed. 
	// size_t is defined as unsigned long long in 64bit. 
	Mat& cvMat = getCvMat();
	cvMat = Mat(width, height, CV_8UC1, (void*)pBits, (size_t)(paddingSize == 0 ? Mat::AUTO_STEP : paddingSize));
	return true;
}

const unsigned char* ImageMat::getBitsData(void) const
{
	return impl().m_pCvMat->data;
}

int ret_segm::ImageMat::getWidth(void) const
{
	return getCols();
}

int ret_segm::ImageMat::getHeight(void) const
{
	return getRows();
}

int ret_segm::ImageMat::getCols(void) const
{
	return impl().m_pCvMat->cols;
}

int ret_segm::ImageMat::getRows(void) const
{
	return impl().m_pCvMat->rows;
}

int ImageMat::getWidthStep(void) const
{
	return (int)impl().m_pCvMat->step;
}

int ret_segm::ImageMat::getByteSize(void) const
{
	return getWidthStep()*getHeight();
}

void ImageMat::convertToRGB(ImageMat* pRGBMat) const
{
	cvtColor(*(impl().m_pCvMat), pRGBMat->getCvMat(), COLOR_GRAY2RGBA);

	// byteSize = (int)(matRGB.total()*matRGB.elemSize());

	// Passed by a reference to pointer, it's C++ style!
	// refer: http://stackoverflow.com/questions/8494909/c-why-double-pointer-for-out-return-function-parameter
	// pBuff = new BYTE[byteSize];
	// memcpy(pBuff, matRGB.data, byteSize);
	return;
}
/*
Gdiplus::Bitmap * ret_segm::ImageMat::createBitmap(void) const
{
	Mat matRgb;
	cvtColor(*(m_pCvMat), matRgb, COLOR_GRAY2RGBA);
	return CGdiPlus::CopyMatToBmp(matRgb);
}
*/
cv::Mat& ret_segm::ImageMat::getCvMat(void)
{
	return *(impl().m_pCvMat);
}

const cv::Mat & ret_segm::ImageMat::getCvMatConst(void) const
{
	// TODO: insert return statement here
	return *(impl().m_pCvMat);
}

void ret_segm::ImageMat::setMeanStddev(float mean, float stddev)
{
	impl().m_mean = mean;
	impl().m_stddev = stddev;
	return;
}

float ret_segm::ImageMat::getMean(void) const
{
	return impl().m_mean;
}

float ret_segm::ImageMat::getStddev(void) const
{
	return impl().m_stddev;
}

float ret_segm::ImageMat::getSampleRatioWidth(void) const
{
	return impl().m_sampleRatioWidth;
}

float ret_segm::ImageMat::getSampleRatioHeight(void) const
{
	return impl().m_sampleRatioHeight;
}

void ret_segm::ImageMat::setSampleRatio(float widthRatio, float heightRatio)
{
	impl().m_sampleRatioWidth = widthRatio;
	impl().m_sampleRatioHeight = heightRatio;
	return;
}

std::vector<int>& ret_segm::ImageMat::getMeanOfColumns(void) const
{
	return impl().m_meanCols;
}

std::vector<int>& ret_segm::ImageMat::getMaxValOfColumns(void) const
{
	return impl().m_maxValCols;
}

std::vector<int>& ret_segm::ImageMat::getMaxIdxOfColumns(void) const
{
	return impl().m_maxIdxCols;
}

void ret_segm::ImageMat::setMeanOfColumns(const cv::Mat & mean)
{
	// const int* p = mean.ptr<uchar>(0);
	// m_meanCols = std::vector<int>(p, p + mean.cols);
	impl().m_meanCols = std::vector<int>(mean.cols);
	for (int c = 0; c < mean.cols; c++) {
		impl().m_meanCols[c] = mean.at<uchar>(0, c);
	}
	return;
}

void ret_segm::ImageMat::setMaxValOfColumns(const cv::Mat & vals)
{
	const int* p = vals.ptr<int>(0);
	impl().m_maxValCols = std::vector<int>(p, p + vals.cols);
	return;
}

void ret_segm::ImageMat::setMaxIdxOfColumns(const cv::Mat & idxs)
{
	const int* p = idxs.ptr<int>(0);
	impl().m_maxIdxCols = std::vector<int>(p, p + idxs.cols);
	return;
}

ret_segm::ImageMat::ImageMatImpl& ret_segm::ImageMat::impl(void) const
{
	return *d_ptr;
}

