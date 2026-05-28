#include "pch.h"
#include "CvImage.h"
#include "GuidedFilter.h"
#include "StrFormat.h"

/*
#include <dlib/matrix.h>
#include <dlib/opencv.h>
#include <dlib/image_io.h>
#include <dlib/image_transforms.h>
*/

#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>


using namespace cpp_util;
using namespace cv;
// using namespace dlib;
using namespace std;

// typedef matrix<double, 0, 1> column_vector;


struct CvImage::CvImageImpl
{
	cv::Mat mat;
	std::shared_ptr<Gdiplus::Bitmap> bitmap;
};


CvImage::CvImage() :
	d_ptr(make_unique<CvImageImpl>())
{
}


cpp_util::CvImage::~CvImage() = default;
cpp_util::CvImage::CvImage(CvImage && rhs) = default;
CvImage & cpp_util::CvImage::operator=(CvImage && rhs) = default;


cpp_util::CvImage::CvImage(const CvImage & rhs)
	: d_ptr(make_unique<CvImageImpl>(*rhs.d_ptr))
{
}


CvImage & cpp_util::CvImage::operator=(const CvImage & rhs)
{
	// CvImageImpl struct should be defined before this access code. 
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool cpp_util::CvImage::fromFile(const std::wstring path, bool grayscale)
{
	// Convert a path string in UTF-16 to UTF-8.
	string str = wtoa(path);

	// c++ interface of opencv, IplImage and cvLoadImage has been deprecated. 
	Mat& cvMat = getCvMat();
	cvMat = imread(str.c_str(), (grayscale ? IMREAD_GRAYSCALE : IMREAD_COLOR));
	if (cvMat.empty()) {
		return false;
	}
	return true;
}


bool cpp_util::CvImage::fromBitsData(const unsigned char * pBits, int width, int height, int padding)
{
	// initialize matrix header that points to the bits data. no data is copied. 
	// if padding size is zero, no padding bytes at the end of each row is assumed. 
	// size_t is defined as unsigned long long in 64bit. 
	Mat& cvMat = getCvMat();
	Mat mat = Mat(height, width, CV_8UC1, (void*)pBits, (size_t)(padding == 0 ? Mat::AUTO_STEP : padding));
	// Make a deep copy.
	cvMat = mat.clone();
	return true;
}


bool cpp_util::CvImage::fromBitsDataNoCopy(const unsigned char * pBits, int width, int height, int padding)
{
	// initialize matrix header that points to the bits data. no data is copied. 
	// if padding size is zero, no padding bytes at the end of each row is assumed. 
	// size_t is defined as unsigned long long in 64bit. 
	Mat& cvMat = getCvMat();
	cvMat = Mat(height, width, CV_8UC1, (void*)pBits, (size_t)(padding == 0 ? Mat::AUTO_STEP : padding));

	return true;
}


bool cpp_util::CvImage::fromFloat32(const unsigned char * pData, int width, int height, int padding)
{
	Mat& cvMat = getCvMat();
	Mat raw = Mat(height, width, CV_32FC1, (void*)pData, (size_t)(padding == 0 ? Mat::AUTO_STEP : padding));
	// Make a deep copy.
	cvMat = raw.clone();
	return true;
}


bool cpp_util::CvImage::fromRaw16(const unsigned char * pData, int width, int height, int padding)
{
	Mat& cvMat = getCvMat();
	Mat raw = Mat(height, width, CV_16UC1, (void*)pData, (size_t)(padding == 0 ? Mat::AUTO_STEP : padding));
	// Make a deep copy.
	cvMat = raw.clone();
	return true;
}


bool cpp_util::CvImage::fromRGB24(const unsigned char * pData, int width, int height, int padding)
{
	Mat& cvMat = getCvMat();
	Mat rgb = Mat(height, width, CV_8UC3, (void*)pData, (size_t)(padding == 0 ? Mat::AUTO_STEP : padding));
	// Make a deep copy.
	cvMat = rgb.clone();
	return true;
}


bool cpp_util::CvImage::fromBayer16s(const unsigned char * pData, int width, int height, FlipMode flip, bool useFundusFILR)
{
	/*
	Mat& cvMat = getCvMat();
	Mat bayer16(height, width, CV_16UC1, (void*)pData);
	Mat bayer8 = bayer16.clone();
	bayer16.convertTo(bayer8, CV_8UC1, alpha, beta);

	cvMat = Mat(height, width, CV_8UC3);

	int code = CV_BayerGR2RGB;
	switch (flip) {
	case FlipMode::NONE:
	code = CV_BayerGR2RGB; break;
	case FlipMode::VERTICAL:
	code = CV_BayerBG2RGB; break;
	case FlipMode::HORIZONTAL:
	code = CV_BayerRG2RGB; break;
	case FlipMode::BOTH:
	code = CV_BayerGB2RGB; break;
	}
	cvtColor(bayer8, cvMat, code);
	*/

	Mat bayer16(height, width, CV_16UC1, (void*)pData);
	Mat rgb16; // = Mat(height, width, CV_16UC3);
	int code = COLOR_BayerRG2RGB; 

	if (useFundusFILR) {
		switch (flip) {
		case FlipMode::NONE:				// Sony Camera Original Format BayerRG16
			code = COLOR_BayerRG2RGB; break;
		case FlipMode::VERTICAL:			// Vertical flip = ReverseX = BayerGR16
			code = COLOR_BayerGR2RGB; break;
		case FlipMode::HORIZONTAL:			// Horizontal flip = ReverseY = BayerGB16
			code = COLOR_BayerGB2RGB; break;
		case FlipMode::BOTH:				// Both = ReverseX+ReverseY = BayerBG16
			code = COLOR_BayerBG2RGB; break;
		}
	}
	else {
		switch (flip) {
		case FlipMode::NONE:
			code = COLOR_BayerGR2RGB; break;
		case FlipMode::VERTICAL:
			code = COLOR_BayerBG2RGB; break;
		case FlipMode::HORIZONTAL:
			code = COLOR_BayerRG2RGB; break;
		case FlipMode::BOTH:
			code = COLOR_BayerGB2RGB; break;
		}
	}
	cvtColor(bayer16, rgb16, code);

	getCvMat() = rgb16;
	// Mat& cvMat = getCvMat();
	// cvMat = Mat(height, width, CV_8UC3);
	// rgb16.convertTo(cvMat, CV_8UC3, alpha, beta);
	return true;
}


unsigned char * cpp_util::CvImage::getBitsData(void) const
{
	if (isEmpty()) {
		return nullptr;
	}
	return getCvMat().data;
}

float* cpp_util::CvImage::getFloatData(void) const
{
	if (isEmpty()) {
		return nullptr;
	}
	return getCvMat().ptr<float>(0);
}


bool cpp_util::CvImage::saveFile(const std::string path, bool vflip, unsigned int imgQuality)
{
	std::vector<int> quality_params;
	quality_params.push_back(IMWRITE_JPEG_QUALITY); 
	quality_params.push_back(imgQuality);

	if (vflip) {
		Mat dst;
		cv::flip(d_ptr->mat, dst, 0);
		return imwrite(path, dst, quality_params);
	}

	return imwrite(path, d_ptr->mat, quality_params);
}


bool cpp_util::CvImage::saveFileAsBinary(const std::string path)
{
	if (isEmpty() || getCvMat().type() != CV_16UC1) {
		return false;
	}

	Mat& cvMat = getCvMat();
	std::vector<uint16_t> array;
	if (cvMat.isContinuous()) {
		array.assign((uint16_t*)cvMat.datastart, (uint16_t*)cvMat.dataend);
	}
	else {
		for (int i = 0; i < cvMat.rows; i++) {
			array.insert(array.end(), (uint16_t*)cvMat.ptr<uchar>(i), (uint16_t*)cvMat.ptr<uchar>(i) + cvMat.cols);
		}
	}

	ofstream ofs(path, ios::out | ios::binary);
	ofs.write((char*)&array[0], array.size() * sizeof(uint16_t));
	ofs.close();
	return true;
}

/*
bool cpp_util::CvImage::saveFileAsRaw(const std::string path)
{
	if (isEmpty() || getCvMat().type() != CV_16UC1) {
		return false;
	}

	Mat& cvMat = getCvMat();
	// matrix<rgb_pixel> raw = mat(cv_image<rgb_pixel>(cvMat));
	auto image = cv_image<unsigned short>(cvMat);
	matrix<unsigned short> raw = mat(image);
	save_dng(raw, path);
	return true;
}
*/


bool cpp_util::CvImage::saveFileAsYml(const std::string path, std::string name)
{
	if (isEmpty()) {
		return false;
	}

	try {
		Mat& cvMat = getCvMat();
		FileStorage fs(path, FileStorage::WRITE);
		fs << name << cvMat;
		fs.release();
		return true;
	}
	catch (...) {
		return false;
	}
}

bool cpp_util::CvImage::saveFileAsCV16UC1(const std::string path)
{
	cv::Mat matCovt(d_ptr->mat.size(), CV_16UC1);

	d_ptr->mat.convertTo(matCovt, CV_16UC1);

	return imwrite(path, matCovt);
}

bool cpp_util::CvImage::loadFileAsYml(const std::string path, std::string name)
{
	try {
		Mat& cvMat = getCvMat();
		FileStorage fs(path, FileStorage::READ);
		fs[name] >> cvMat;
		fs.release();
		return true;
	}
	catch (...) {
		return false;
	}
}

void cpp_util::CvImage::flipVertical(void)
{
	if (!isEmpty()) {
		cv::flip(d_ptr->mat, d_ptr->mat, 0);
	}
	return;
}


void cpp_util::CvImage::flipHorizontal(void)
{
	if (!isEmpty()) {
		cv::flip(d_ptr->mat, d_ptr->mat, 1);
	}
	return;
}


void cpp_util::CvImage::rotate90(bool cw)
{
	if (!isEmpty()) {
		if (cw) {
			cv::transpose(d_ptr->mat, d_ptr->mat);
			cv::flip(d_ptr->mat, d_ptr->mat, 1);
		}
		else {
			cv::transpose(d_ptr->mat, d_ptr->mat);
			cv::flip(d_ptr->mat, d_ptr->mat, 0);
		}
	}
	return;
}


unsigned char cpp_util::CvImage::getAtInByte(int x, int y)
{
	if (isEmpty() || getCvMat().type() != CV_8UC1 ||
		x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
		return 0;
	}
	else {
		return getCvMat().at<unsigned char>(y, x);
	}
}


float cpp_util::CvImage::getAtInFloat(int x, int y)
{
	if (isEmpty() || getCvMat().type() != CV_32FC1 ||
		x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
		return 0.0f;
	}
	else {
		return getCvMat().at<float>(y, x);
	}
}


unsigned char cpp_util::CvImage::getRedAt(int x, int y)
{
	if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
		return 0;
	}
	return getCvMat().at<cv::Vec3b>(y, x)[2];
}


unsigned char cpp_util::CvImage::getBlueAt(int x, int y)
{
	if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
		return 0;
	}
	return getCvMat().at<cv::Vec3b>(y, x)[0];
}


unsigned char cpp_util::CvImage::getGreenAt(int x, int y)
{
	if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
		return 0;
	}
	return getCvMat().at<cv::Vec3b>(y, x)[1];
}


std::vector<unsigned char> cpp_util::CvImage::copyDataInBytes(void) const
{
	if (isEmpty() || getCvMat().type() != CV_8UC1) {
		return std::vector<unsigned char>();
	}
	else {
		Mat matImg = getCvMat();
		std::vector<unsigned char> vect;
		vect.assign(matImg.datastart, matImg.dataend);
		return vect;
	}
}


std::vector<float> cpp_util::CvImage::copyDataInFloats(void) const
{
	if (isEmpty() || getCvMat().type() != CV_32FC1) {
		return std::vector<float>();
	}
	else {
		Mat matImg = getCvMat();
		std::vector<float> vect;
		vect.assign((float*)matImg.datastart, (float*)matImg.dataend);
		return vect;
	}
}


void cpp_util::CvImage::convertToFloat(void)
{
	Mat src = getCvMat();
	if (src.type() != CV_32FC1) {
		Mat out;
		src.convertTo(out, CV_32FC1);
		getCvMat() = out;
	}
	return;
}


void cpp_util::CvImage::convertToUchar(void)
{
	Mat& src = getCvMat();
	if (src.type() != CV_8UC1) {
		Mat out;
		src.convertTo(out, CV_8UC1);
		getCvMat() = out;
	}
	return;
}


int cpp_util::CvImage::getWidth(void) const
{
	return getCols();
}


int cpp_util::CvImage::getHeight(void) const
{
	return getRows();
}


int cpp_util::CvImage::getRows(void) const
{
	return getCvMat().rows;
}


int cpp_util::CvImage::getCols(void) const
{
	return getCvMat().cols;
}


int cpp_util::CvImage::getWidthStep(void) const
{
	return (int)getCvMat().step;
}


int cpp_util::CvImage::getByteSize(void) const
{
	return getWidthStep()*getHeight();
}


bool cpp_util::CvImage::isEmpty(void) const
{
	return getCvMat().empty();
}


void cpp_util::CvImage::clear(void)
{
	getCvMat() = Mat();
	return;
}

void cpp_util::CvImage::correctGamma(double gamma)
{
	static double g = -1.0;
	static Mat gammaTable(1, 256, CV_8UC1);

	if (g != gamma && gamma > 0.0) {
		g = gamma;
		double invGamma = 1.0 / g;
		uchar * ptr = gammaTable.ptr();
		for (int i = 0; i < 256; i++) {
			ptr[i] = (int)(pow((double)i / 255.0, invGamma) * 255.0);
		}
	}

	if (g > 0.0 && !isEmpty()) {
		LUT(getCvMat(), gammaTable, getCvMat());
	}
}

void cpp_util::CvImage::applyBilateralFilter(int distance, double sigmaColor, double sigmaSpace)
{
	if (isEmpty()) {
		return;
	}

	Mat out;
	cv::bilateralFilter(getCvMat(), out, distance, sigmaColor, sigmaSpace);
	getCvMat() = out;
	return;
}


void cpp_util::CvImage::applyGaborFilter(int ksize1, int ksize2, int kstep, int orients, double divider, double sigma, double lambda, double gamma, double psi)
{
	if (isEmpty()) {
		return;
	}

	Mat src = getCvMat();
	Mat res = cv::Mat::zeros(src.size(), src.type());

	for (int ksz = ksize1; ksz < ksize2; ksz += kstep) {
		for (int i = 0; i < orients; i++) {
			double theta = i * CV_PI / orients;
			auto kernel = cv::getGaborKernel(cv::Size(ksz, ksz), sigma, theta, lambda, gamma, psi);
			kernel /= (divider * cv::sum(kernel)[0]);
			kernel.convertTo(kernel, CV_32F);

			Mat out;
			cv::filter2D(src, out, CV_32F, kernel);
			res = cv::max(res, out);
		}
	}

	getCvMat() = res;
	return;
}


void cpp_util::CvImage::applyGaussianBlur(double sigma, int ksize1, int ksize2)
{
	cv::Mat out;
	cv::Mat src = getCvMat();
	cv::GaussianBlur(src, out, cv::Size(ksize1, ksize2), sigma);
	getCvMat() = out;
	return;
}



void cpp_util::CvImage::applyMedianBlur(int ksize)
{
	cv::Mat out;
	cv::Mat src = getCvMat();
	cv::medianBlur(src, out, ksize);
	getCvMat() = out;
	return;
}


void cpp_util::CvImage::applyGuidedFilter(int r, double eps, int depth)
{
	cv::Mat src = getCvMat();
	cv::Mat out;

	// Because the intensity range of our images is [0, 255]
	double eps2 = eps * 255 * 255;

	out = guidedFilter(src, r, eps2, depth);
	getCvMat() = out;
	return;
}


void cpp_util::CvImage::applyWeighted(CvImage * image, double alpha, double beta, double gamma)
{
	if (!image || image->isEmpty()) {
		return;
	}

	cv::Mat img = image->getCvMat();
	cv::Mat src = getCvMat();
	cv::Mat out;

	cv::addWeighted(src, alpha, img, beta, gamma, out);
	getCvMat() = out;
	return;
}





void cpp_util::CvImage::applyBinarized(int thresh)
{
	cv::Mat src = getCvMat();
	cv::Mat out;
	cv::threshold(src, out, thresh, 255, cv::THRESH_BINARY);
	getCvMat() = out;
	return;
}


void cpp_util::CvImage::applyMorphologyTransform(int ksize)
{
	cv::Mat src = getCvMat();
	cv::Mat out;

	auto kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksize, ksize));
	cv::morphologyEx(src, out, cv::MORPH_OPEN, kernel);
	cv::morphologyEx(out, out, cv::MORPH_CLOSE, kernel);
	getCvMat() = out;
	return;
}


void cpp_util::CvImage::applyGradientConvolution(int kernRows, int kernCols, bool ascent)
{
	Mat kernel = Mat::ones(kernRows, kernCols, CV_32F);

	int rrow1 = ascent ? 0 : (kernel.rows / 2);
	int rrow2 = ascent ? (kernel.rows / 2) : kernel.rows;

	for (int r = rrow1; r < rrow2; r++) {
		for (int c = 0; c < kernel.cols; c++) {
			kernel.at<float>(r, c) *= -1.0f;
		}
	}

	Mat conv;
	filter2D(getCvMat(), conv, CV_32F, kernel, Point(-1, -1), 0, BORDER_REPLICATE);

	// Leave only positive gradients, and normalize in range between [0, 255].
	Mat mask = conv < 0;
	Mat masc;
	Mat resMat;

	conv.copyTo(masc);
	masc.setTo(0.0f, mask);
	cv::normalize(conv, resMat, 0, 255, NORM_MINMAX, CV_8U);

	getCvMat() = resMat;
	return;
}


void cpp_util::CvImage::gradients(int ksize, CvImage * dest)
{
	Mat grad_x, grad_y;
	cv::Sobel(getCvMat(), grad_x, CV_16S, 1, 0, ksize);
	cv::Sobel(getCvMat(), grad_y, CV_16S, 0, 1, ksize);

	Mat abs_gx, abs_gy;
	cv::convertScaleAbs(grad_x, abs_gx);
	cv::convertScaleAbs(grad_y, abs_gy);

	Mat grads;
	cv::addWeighted(abs_gx, 0.5, abs_gy, 0.5, 0.0, grads);

	if (dest) {
		dest->getCvMat() = grads;
	}
	else {
		getCvMat() = grads;
	}
	return;
}


void cpp_util::CvImage::applyRemoveReflectionLight(int lightRadius, int shadowRadius, int peripheryRadius)
{

	Mat src, dst;
	Mat splitBGR[3];

	if (isEmpty()) {
		return;
	}

	if (getCvMat().type() != CV_8UC3 && getCvMat().type() != CV_16UC3) {
		return;
	}

	if (lightRadius == 0 || shadowRadius == 0 || peripheryRadius == 0) {
		return;
	}

	if (lightRadius == shadowRadius ||
		shadowRadius == peripheryRadius ||
		peripheryRadius == lightRadius) {
		return;
	}

	getCvMat().copyTo(src);
	split(src, splitBGR);

	for (int i = 0; i < 3; i++)
	{
		// removal reflection light
		Mat light, shadow, periphery;
		splitBGR[i].copyTo(light);
		splitBGR[i].copyTo(shadow);
		splitBGR[i].copyTo(periphery);

		Mat lightMask = Mat::ones(light.rows, light.cols, light.type());
		Mat shadowMask = Mat::ones(shadow.rows, shadow.cols, shadow.type());
		Mat peripheryMask = Mat::ones(periphery.rows, periphery.cols, periphery.type());
		cv::Point centerPoint(src.cols / 2, src.rows / 2);
		std::vector<int> peripheryVector;

		cv::circle(lightMask, centerPoint, lightRadius, 0, -1);
		cv::circle(shadowMask, centerPoint, shadowRadius, 0, -1);
		cv::circle(peripheryMask, centerPoint, peripheryRadius, 0, -1);

		light.setTo(0, lightMask);
		shadow.setTo(0, shadowMask);
		periphery.setTo(0, peripheryMask);
		cv::multiply(lightMask, shadow, shadow);
		cv::multiply(shadowMask, periphery, periphery);

		int cnt = 0;
		int shadowMean = 0;
		int peripheryMean = 0;
		for (int y = 0; y < shadow.rows; y++)
		{
			for (int x = 0; x < shadow.cols; x++)
				if (shadow.at<uchar>(y, x) != 0) {
					shadowMean += shadow.at<uchar>(y, x);
					cnt++;
				}
		}

		if (shadowMean != 0 && cnt != 0)
		{
			shadowMean /= cnt;
		}

		for (int y = 0; y < periphery.rows; y++)
		{
			for (int x = 0; x < periphery.cols; x++)
				if (periphery.at<uchar>(y, x) != 0) {
					peripheryVector.push_back(periphery.at<uchar>(y, x));
				}
		}

		std::vector<int>::iterator start = peripheryVector.begin();
		std::vector<int>::iterator end = peripheryVector.end();

		std::vector<int>::iterator med = start;
		std::advance(med, peripheryVector.size() / 2);

		// This makes the 2nd position hold the median.
		std::nth_element(start, med, end);

		if (peripheryVector.size() > 3) {
			peripheryMean = peripheryVector[2];
		}

		light -= shadowMean;

		cv::blur(light, light, Size(47, 47));
		splitBGR[i] -= light;

		// Correction shadow
		Mat shadowCorrection = Mat::zeros(shadow.rows, shadow.cols, shadow.type());
		auto angle = 0.25;
		for (int j = 0; j < 1440; j++) {
			cv::Point innerPoint;
			cv::Point outterPoint;

			innerPoint.x = (int)round(centerPoint.x + shadowRadius * cos(angle * j * CV_PI / 180.0));
			innerPoint.y = (int)round(centerPoint.y + shadowRadius * sin(angle * j * CV_PI / 180.0));

			outterPoint.x = (int)round(centerPoint.x + peripheryRadius * cos(angle * j * CV_PI / 180.0));
			outterPoint.y = (int)round(centerPoint.y + peripheryRadius * sin(angle * j * CV_PI / 180.0));

			LineIterator innerIt(splitBGR[i], centerPoint, innerPoint, 4);
			LineIterator outterIt(splitBGR[i], innerPoint, outterPoint, 4);

			double outterData = 0.0f;

			for (int k = 0; k < outterIt.count; k++, ++outterIt)
			{
				outterData += splitBGR[i].at<uchar>(outterIt.pos());
			}

			if (outterData != 0.0 && outterIt.count != 0) {
				outterData /= (double)outterIt.count;
			}

			if (outterData > peripheryMean)
			{
				outterData = peripheryMean;
			}

			for (int k = 0; k < innerIt.count; k++, ++innerIt)
			{
				if (splitBGR[i].at<uchar>(innerIt.pos()) > outterData)
				{
					shadowCorrection.at<uchar>(innerIt.pos()) = 0;
				}
				else
				{
					double differ = abs(splitBGR[i].at<uchar>(innerIt.pos()) - outterData);
					shadowCorrection.at<uchar>(innerIt.pos()) = (uchar)differ;
				}
			}
		}

		cv::blur(shadowCorrection, shadowCorrection, Size(66, 66));
		addWeighted(splitBGR[i], 1.0, shadowCorrection, 1.0, 0.0, splitBGR[i]);
	}

	merge(splitBGR, 3, dst);
	getCvMat() = dst;

	return;
}


void cpp_util::CvImage::applyPreprocessingFAZ(float fazThreshold)
{
	Mat src;
	Mat FazImg;
	Mat res = Mat::zeros(src.size(), CV_32FC1);
	Mat res2 = Mat::zeros(src.size(), CV_32FC1);
	Mat sumImage = Mat::zeros(src.size(), src.type());

	getCvMat().copyTo(src);
	int x1 = src.cols / 5, y1 = src.rows / 5;
	Mat Roi = src(Rect(x1, y1, x1 * 3, y1 * 3));
	threshold(src, src, cv::mean(Roi).val[0] * 1.5, 255, THRESH_BINARY); 
	src.copyTo(FazImg);

	GaussianBlur(src, src, cv::Size(33, 33), 0);
	medianBlur(src, src, 67);

	cv::adaptiveThreshold(src, src, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 67, 0);

	Mat ker = getStructuringElement(MORPH_ELLIPSE, Size(5, 5), Point(1, 1));
	cv::morphologyEx(src, src, MORPH_OPEN, ker, Point(-1, -1), 2);

	distanceTransform(src, src, DIST_L2, 3);

	Mat circleMask = cv::Mat::ones(src.size(), CV_8UC1);
	circle(circleMask, Point(src.rows / 2, src.cols / 2), src.cols / 4, Scalar(0, 0, 0), -1);
	src.setTo(0, circleMask);

	double radius;
	int maxIdx[2];
	minMaxIdx(src, NULL, &radius, NULL, maxIdx);
	Point center(maxIdx[1], maxIdx[0]);

	if (radius > src.cols || radius < 5) {
		return;
	}
	equalizeHist(FazImg, FazImg);

	auto clahe = cv::createCLAHE(1.0, cv::Size(5, 5));
	clahe->apply(FazImg, FazImg);

	GaussianBlur(FazImg, FazImg, Size(9, 9), 0);
	cv::adaptiveThreshold(FazImg, FazImg, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 55, 0);

	distanceTransform(FazImg, FazImg, DIST_L2, 3);

	normalize(FazImg, FazImg, 0, 1.0, NORM_MINMAX);
	threshold(FazImg, FazImg, fazThreshold, 255, THRESH_BINARY); // fazThreshold Range 0.05 ~ 0.15, default = 0.1.
	FazImg.convertTo(FazImg, CV_8UC1);

	Mat cicleMask = Mat::ones(FazImg.size(), CV_8U);
	cv::circle(cicleMask, center, (int)(radius*2.0f), Scalar(0, 0, 0), -1);
	FazImg.setTo(0, cicleMask);

	getCvMat() = FazImg;

	return;
}

int cpp_util::CvImage::calculateCenterMean(int lightRadius)
{
	Mat src;

	getCvMat().copyTo(src);
	Point centerPoint(src.cols / 2, src.rows / 2);

	Mat circleMask = cv::Mat::zeros(src.size(), CV_8UC1);
	circle(circleMask, centerPoint, lightRadius, Scalar::all(1), -1);

	auto circleMean = cv::mean(src, circleMask);
	auto r = circleMean.val[2];
	auto g = circleMean.val[1];
	auto b = circleMean.val[0];

	int centerMean = (int)(r*0.2126 + g*0.7152 + b*0.0722);

	return centerMean;
}

int cpp_util::CvImage::calculateOverBrightPixel(int lightRadius, int threshold)
{
	Mat src;

	getCvMat().copyTo(src);
	Point centerPoint(src.cols / 2, src.rows / 2);

	Mat circleMask = cv::Mat::zeros(src.size(), CV_8UC1);
	circle(circleMask, centerPoint, lightRadius, 0, -1);
	src.setTo(0, circleMask);

	int pixelCnt = 0;
	int centerX = src.cols / 2;
	int centerY = src.rows / 2;
	for (int i = centerX - lightRadius; i < centerX + lightRadius; ++i) {
		for (int j = centerY - lightRadius; j < centerY + lightRadius; ++j) {

			auto r = src.at<Vec3b>(j, i)[2];
			auto g = src.at<Vec3b>(j, i)[1];
			auto b = src.at<Vec3b>(j, i)[0];

			int centerMean = (int)(r*0.2126 + g*0.7152 + b*0.0722);

			if (centerMean > threshold) {
				pixelCnt++;
			}
		}
	}

	return pixelCnt;
}

void cpp_util::CvImage::equalizeHistogram(double clipLimit, int addScalar, bool nocopy)
{
	// Refer
	// http://docs.opencv.org/3.1.0/d5/daf/tutorial_py_histogram_equalization.html#gsc.tab=0
	if (isEmpty()) {
		return;
	}

	if (addScalar <= 0) {
		addScalar = 8;
	}

	if (nocopy) {
		Mat dest = getCvMat();
		cv::Ptr<cv::CLAHE> p = cv::createCLAHE(clipLimit, cv::Size(addScalar, addScalar));
		p->apply(getCvMat(), dest);
		//cv::equalizeHist(getImage()->getCvMat(), dest);
		// cv::add(dest, Scalar(addScalar), dest);
	}
	else {
		Mat src = getCvMat();
		Mat res1;
		cv::Ptr<cv::CLAHE> p = cv::createCLAHE(clipLimit, cv::Size(addScalar, addScalar));
		p->apply(src, res1);
		//cv::equalizeHist(getImage()->getCvMat(), dest);
		// cv::add(res1, Scalar(addScalar), res2);
		getCvMat() = res1;
	}
	return;
}


bool cpp_util::CvImage::processGrayWorld(float radius, float * corrR, float * corrB)
{
	if (isEmpty() || getCvMat().type() != CV_8UC3) {
		return false;
	}

	Mat srcImg = getCvMat();
	Mat mask = Mat::zeros(srcImg.rows, srcImg.cols, CV_8UC1);

	Point center(srcImg.rows / 2, srcImg.cols / 2);
	int sizeRad = (int)(((center.x + center.y) * 0.5) * radius);
	cv::circle(mask, center, sizeRad, 255, -1);

	std::vector<Mat> channels;
	cv::split(srcImg, channels);

	double bm = cv::mean(channels[0], mask)(0);
	double gm = cv::mean(channels[1], mask)(0);
	double rm = cv::mean(channels[2], mask)(0);

	if (rm > 0.0) {
		*corrR = (float)(gm / rm);
	}
	else {
		*corrR = 1.0f;
	}

	if (bm > 0.0) {
		*corrB = (float)(gm / bm);
	}
	else {
		*corrB = 1.0f;
	}

	return true;
}


void cpp_util::CvImage::balanceColorByGrayWorld(float corrR, float corrB)
{
	if (isEmpty()) {
		return;
	}

	if (getCvMat().type() != CV_8UC3 && getCvMat().type() != CV_16UC3) {
		return;
	}

	Mat& srcImg = getCvMat();
	std::vector<Mat> channels;
	cv::split(srcImg, channels);

	channels[0] = corrB * channels[0];
	channels[2] = corrR * channels[2];

	cv::merge(channels, srcImg);
	return;
}


float radial_corr_factor(float r, float R)
{
	float p4 = 1.56f;
	float p3 = -4.02f;
	float p2 = 3.11f;
	float p1 = -1.00f;
	float p0 = 0.99f;
	float x = r / R;

	float f = p4*x*x*x*x
		+ p3*x*x*x
		+ p2*x*x
		+ p1*x
		+ p0;

	if (f == 0.0f)
		return 1.0f;
	else
		return 1.0f / f;
}


std::vector<float> level_corr_factor()
{
	const int N = 256;
	int x1 = 32, y1 = 96;
	int x2 = 64, y2 = 128;
	std::vector<float> level(N);
	float y;
	level[0] = 0.0;
	for (int i = 1; i < x1; i++) {
		y = 2.0f * i;
		level[i] = 2.0f;
	}
	for (int i = x1; i < x2; i++) {
		y = i + 64.0f;
		level[i] = y / i;
	}
	for (int i = x2; i < N; i++) {
		y = (float)(0.666667 * i + 85);
		level[i] = y / i;
	}

	return level;
}



void cpp_util::CvImage::denoise()
{
	cv::Mat dst;
	cv::Mat &m = getCvMat();
	cv::medianBlur(m, dst, 3);

	m = dst.clone();
}


void cpp_util::CvImage::blur(int xsize, int ysize, double xsigma, double ysigma)
{
	cv::Mat dst;
	cv::Mat &m = getCvMat();
	cv::GaussianBlur(m, dst, cv::Size(xsize, ysize), xsigma, ysigma);

	m = dst.clone();
}


bool cpp_util::CvImage::convertToRGB(CvImage * pRGB) const
{
	if (isEmpty()) {
		return false;
	}

	Mat matSrc;
	if (this->getCvMat().type() != CV_8U) {
		this->getCvMat().convertTo(matSrc, CV_8U);
	}
	else {
		matSrc = this->getCvMat();
	}

	cvtColor(matSrc, pRGB->getCvMat(), COLOR_GRAY2RGBA);

	// Passed by a reference to pointer, it's C++ style!
	// refer: http://stackoverflow.com/questions/8494909/c-why-double-pointer-for-out-return-function-parameter
	// pBuff = new BYTE[byteSize];
	// memcpy(pBuff, matRGB.data, byteSize);
	return true;
}


bool cpp_util::CvImage::convertRGB16ToRGB8(double alpha, double beta) const
{
	if (isEmpty()) {
		return false;
	}

	Mat rgb8s;
	getCvMat().convertTo(rgb8s, CV_8UC3, alpha, beta);
	getCvMat() = rgb8s;
	return true;
}


void cpp_util::CvImage::applyMaskROI(std::pair<int, int> center, int radius, bool resize)
{
	if (isEmpty()) {
		return;
	}

	if (getCvMat().cols <= (center.first + radius) ||
		getCvMat().rows <= (center.second + radius)) {
		return;
	}

	int x1 = center.first - radius;
	int y1 = center.second - radius;
	int x2 = center.first + radius;
	int y2 = center.second + radius;
	int mask_dia = radius * 2;
	int mask_rad = radius;

	if (x1 < 0 || y1 <= 0 || x1 >= x2 || y1 >= y2 || mask_rad <= 0) {
		return;
	}

	if (resize) {
		crop(x1, y1, mask_dia, mask_dia);
		drawCircleMask(mask_rad, mask_rad, mask_rad);
	}
	else {
		cv::Mat srci = Mat::zeros(getCvMat().size(), getCvMat().type());
		crop(x1, y1, mask_dia, mask_dia);
		drawCircleMask(mask_rad, mask_rad, mask_rad);

		int sx = (srci.cols - mask_dia) / 2;
		int sy = (srci.rows - mask_dia) / 2;
		int rows = getCvMat().rows;
		int cols = getCvMat().cols;
		getCvMat().copyTo(srci(cv::Rect(sx, sy, cols, rows)));
		//srci.setTo(255);

		cv::Mat& mat = getCvMat();
		mat = srci.clone();
	}
	return;
}


void cpp_util::CvImage::drawCircleMask(int x1, int y1, int x2, int y2)
{
	if (isEmpty() || getCvMat().type() != CV_8U) {
		return;
	}

	if (x1 < 0 || x2 >= getCols() || y1 < 0 || y2 >= getCols() ||
		x1 >= x2 || y1 >= y2) {
		return;
	}

	int cx = (x2 + x1) / 2;
	int cy = (y2 + y1) / 2;
	int rad = (y2 - y1) / 2;

	Mat mask = Mat::ones(getRows(), getCols(), CV_8UC1);
	Point center(cx, cy);
	cv::circle(mask, center, rad, 0, -1);

	cv::bitwise_and(getCvMat(), mask, getCvMat());
	return;
}


void cpp_util::CvImage::drawCircleMask(int cx, int cy, int rad, bool invert)
{
	if (isEmpty()) {
		return;
	}

	if (invert) {
		Mat mask = Mat::zeros(getRows(), getCols(), CV_8UC1);
		Point center(cx, cy);
		cv::circle(mask, center, rad, 1, -1);

		getCvMat().setTo(0, mask);
	}
	else {
		Mat mask = Mat::ones(getRows(), getCols(), CV_8UC1);
		Point center(cx, cy);
		cv::circle(mask, center, rad, 0, -1);

		getCvMat().setTo(0, mask);
	}
	return;
}

/*
Gdiplus::Bitmap * cpp_util::CvImage::createBitmap(void) const
{
	if (isEmpty()) {
		return nullptr;
	}

	Mat matSrc;
	if (this->getCvMat().type() != CV_8U) {
		this->getCvMat().convertTo(matSrc, CV_8U);
	}
	else {
		matSrc = this->getCvMat();
	}
	*/

	/*
	// This must be caused by OpenCL optimization or necessary to rebuild OpenCV library than using static version.
	// When disable OpenCL via environment variable ('OPENCV_OPENCL_RUNTIME=qqq'),
	// the performance is much improved, but throws an exception from cvtColor inside.
	// Refer : http://developer.amd.com/community/blog/2014/10/15/opencv-3-0-transparent-api-opencl-acceleration/

	// Color conversion by cvtColor is too slow!!
	// It takes hundreds of milliseconds for just 640x480 gray image.
	Mat matRgb;
	cvtColor(matSrc, matRgb, CV_GRAY2RGBA);
	Gdiplus::Bitmap* p =  CGdiPlus::CopyMatToBmp(matRgb);
	*/

	// 8 bit indexed bitmap.
	// Gdiplus::Bitmap* p = CGdiPlus::CopyMatToBmp(matSrc);

	/*
	Mat matRgb;
	cvtColor(matSrc, matRgb, COLOR_GRAY2RGBA);
	Gdiplus::Bitmap* p = CGdiPlus::CopyMatToBmp(matRgb);
	d_ptr->bitmap.reset(p);
	return p;
	*/
	/*
	return nullptr;
}
*/

/*
Gdiplus::Bitmap * cpp_util::CvImage::getBitmap(bool redraw)
{
	if (d_ptr->bitmap == nullptr || redraw == true) {
		createBitmap();
	}
	return d_ptr->bitmap.get();
}
*/


bool cpp_util::CvImage::resize(int width, int height)
{
	CvImage resize;
	if (resizeTo(&resize, width, height)) {
		getCvMat() = resize.getCvMat().clone();
		return true;
	}
	return false;
}


bool cpp_util::CvImage::resizeTo(CvImage * image, int width, int height)
{
	if (isEmpty()) {
		return false;
	}
	Size size(width, height);
	cv::resize(getCvMatConst(), image->getCvMat(), size, INTER_LANCZOS4);
	return true;
}


bool cpp_util::CvImage::resizeFastTo(CvImage * image, int width, int height)
{
	if (isEmpty()) {
		return false;
	}
	Size size(width, height);
	cv::resize(getCvMatConst(), image->getCvMat(), size, INTER_NEAREST);
	return true;
}


bool cpp_util::CvImage::crop(int x0, int y0, int w, int h)
{
	if (isEmpty()) {
		return false;
	}

	cv::Mat& m = getCvMat();

	cv::Rect myROI(x0, y0, w, h);
	cv::Mat matROI = m(myROI);
	getCvMat() = matROI.clone();

	return true;
}


void cpp_util::CvImage::copyTo(CvImage * image) const
{
	if (isEmpty() || image == nullptr) {
		return;
	}

	image->getCvMat() = getCvMat().clone();
	return;
}

/*
HBITMAP cpp_util::CvImage::createDIBitmap(HDC hDC, bool vflip) const
{
	// Creating bitmaps from the data in memory are coming out fuzzy when doing something like below?
	// http://stackoverflow.com/questions/4598872/creating-hbitmap-from-memory-buffer
	// HBITMAP hbmp = CreateBitmap(getImpl().width, getImpl().height, 1, 8, getImpl().image.getBitsData());
	HBITMAP hbmp;

	if (getCvMat().type() == CV_8UC3)
	{
		size_t size = sizeof(BITMAPINFOHEADER);
		BITMAPINFO* pbmi = (BITMAPINFO*)alloca(size);
		ZeroMemory(pbmi, size);
		pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pbmi->bmiHeader.biBitCount = 24;
		pbmi->bmiHeader.biWidth = getWidth();
		pbmi->bmiHeader.biHeight = getHeight() * (vflip ? -1 : 1);// *-1; // Windows bitmaps are stored with the bottom line first.
		pbmi->bmiHeader.biPlanes = 1;
		pbmi->bmiHeader.biCompression = BI_RGB; // Non-compression image
		pbmi->bmiHeader.biSizeImage = 0; // it's zero if non-compression image

										 // Create HBITMAP should be deleted in the caller. 
		int fdwInit = (isEmpty() ? 0 : CBM_INIT);
		hbmp = CreateDIBitmap(hDC, (BITMAPINFOHEADER *)&pbmi->bmiHeader, fdwInit, getBitsData(), (BITMAPINFO*)pbmi, DIB_RGB_COLORS);
	}
	else // if (getCvMat().type() == CV_8UC1)
	{
		// alloca() allocates memory that is automatically freed in the stack frame of the caller.
		size_t size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256;
		BITMAPINFO* pbmi = (BITMAPINFO*)alloca(size);
		ZeroMemory(pbmi, size);
		pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pbmi->bmiHeader.biBitCount = 8;
		pbmi->bmiHeader.biWidth = getWidth();
		pbmi->bmiHeader.biHeight = getHeight() * (vflip ? -1 : 1);// *-1; // Windows bitmaps are stored with the bottom line first.
		pbmi->bmiHeader.biPlanes = 1;
		pbmi->bmiHeader.biCompression = BI_RGB; // Non-compression image
		pbmi->bmiHeader.biSizeImage = 0; // it's zero if non-compression image

		for (int i = 0; i < 256; i++) {
			pbmi->bmiColors[i].rgbRed = i;
			pbmi->bmiColors[i].rgbGreen = i;
			pbmi->bmiColors[i].rgbBlue = i;
			pbmi->bmiColors[i].rgbReserved = 0;
		}

		// Create HBITMAP should be deleted in the caller. 
		int fdwInit = (isEmpty() ? 0 : CBM_INIT);
		hbmp = CreateDIBitmap(hDC, (BITMAPINFOHEADER *)&pbmi->bmiHeader, fdwInit, getBitsData(), (BITMAPINFO*)pbmi, DIB_RGB_COLORS);
	}

	return hbmp;
}
*/


cv::Mat & cpp_util::CvImage::getCvMat(void) const
{
	return d_ptr->mat;
}


cv::Mat* cpp_util::CvImage::getCvMatPtr(void) const
{
	return &(d_ptr->mat);
}


const cv::Mat & cpp_util::CvImage::getCvMatConst(void) const
{
	return d_ptr->mat;
}


void cpp_util::CvImage::getMeanStddev(float * mean, float * stddev)
{
	if (isEmpty()) {
		return;
	}

	Scalar mean1, stddev1;
	meanStdDev(getCvMat(), mean1, stddev1);
	*mean = (float)mean1(0);
	*stddev = (float)stddev1(0);
	return;
}


void cpp_util::CvImage::getMeanStddevInCircle(float * mean, float * stddev, int radius)
{
	if (isEmpty()) {
		return;
	}

	cv::Mat src = getCvMat();
	cv::Mat mask = Mat::zeros(getRows(), getCols(), CV_8UC1);

	int size = min(getRows(), getCols());
	int rads = min(size / 2, radius);
	int cx = getRows() / 2;
	int cy = getCols() / 2;

	cv::Point center(cx, cy);
	cv::circle(mask, center, rads, 1, -1);

	Scalar mean1, stddev1;
	meanStdDev(src, mean1, stddev1, mask);
	*mean = (float)mean1(0);
	*stddev = (float)stddev1(0);
	return;
}


bool cpp_util::CvImage::reduceMeanStddev(int dimen, std::vector<float>& mean, std::vector<float>& stddev)
{
	cv::Mat meanVal, stdVal;
	cv::Mat src = getCvMat();

	int size = (dimen == 0 ? src.cols : src.rows);
	mean = std::vector<float>(size);
	stddev = std::vector<float>(size);

	if (dimen == 0) {
		for (int i = 0; i < src.cols; i++) {
			meanStdDev(src.col(i), meanVal, stdVal);
			mean[i] = (float)meanVal.at<double>(0);
			stddev[i] = (float)stdVal.at<double>(0);
		}
	}
	else {
		for (int i = 0; i < src.rows; i++) {
			meanStdDev(src.row(i), meanVal, stdVal);
			mean[i] = (float)meanVal.at<double>(0);
			stddev[i] = (float)stdVal.at<double>(0);
		}
	}

	return true;
}


int cpp_util::CvImage::getPercentile(int pval, int step, int radius = 0)
{
	if (isEmpty() || getCvMat().type() != CV_8UC1) {
		return 0;
	}

	auto hist = std::vector<int>(256, 0);

	cv::Mat src = getCvMat();
	int size = 0;

	if (radius == 0) {
		for (int y = 0; y < src.rows; y += step) {
			auto ptr = src.ptr(y);
			for (int x = 0; x < src.cols; x += step) {
				hist[ptr[x]] += 1;
				size++;
			}
		}
	}
	else {
		int cx = src.cols / 2;
		int cy = src.rows / 2;
		int rad = min(min(cx, cy), radius);
		int radsqr = rad * rad;

		for (int y = 0; y < src.rows; y += step) {
			auto ptr = src.ptr(y);
			for (int x = 0; x < src.cols; x += step) {
				int dx = x - cx;
				int dy = y - cy;
				if ((dx*dx + dy*dy) <= radsqr) {
					hist[ptr[x]] += 1;
					size++;
				}
			}
		}
	}

	int bound = size * pval / 100;
	int count = 0;
	for (int i = 0; i <= 255; i++) {
		count += hist[i];
		if (count >= bound) {
			return i;
		}
	}
	return 0;
}

std::vector<double> cpp_util::CvImage::applyDewarping(std::vector<int> layerEPI, float resX, float resY, double reflactiveIndex)
{
	Mat& img = getCvMat();
	int size = (int)layerEPI.size();
	int width = img.cols;
	int height = img.rows;

	auto uppX = std::vector<double>(size, -1);
	auto uppY = std::vector<double>(size, -1);
	auto theta = std::vector<double>(size, -1);

	Mat remapX = cv::Mat::zeros(height, width, CV_32FC1);
	Mat remapY = cv::Mat::zeros(height, width, CV_32FC1);

	Mat A = Mat(size, 3, CV_64FC1);
	Mat X = Mat(3, 1, CV_64FC1);
	Mat B = Mat(size, 1, CV_64FC1);

	for (int i = 0; i < size; i++) {
		uppX[i] = i;
		uppY[i] = layerEPI[i];

		if (uppY[i] >= height) {
			uppY[i] = height - 1;
		}

		double x = i * resX;
		double y = layerEPI[i] * resY * reflactiveIndex;

		A.at<double>(i, 0) = pow(x, 2);
		A.at<double>(i, 1) = x;
		A.at<double>(i, 2) = 1;

		B.at<double>(i, 0) = y;
	}

	Mat Apinv;
	invert(A, Apinv, DECOMP_SVD);

	X = Apinv * B;

	double a = X.at<double>(0, 0);
	double b = X.at<double>(1, 0);
	double c = X.at<double>(2, 0);

	//Backward Mapping
	for (int i = 0; i < size; i++) {
		double slope = 2 * a * (i * resX) + b;
		theta[i] = atan(slope);
		double theta2 = asin(sin(theta[i]) / reflactiveIndex);
		double phi = (theta[i] - theta2);

		for (int j = 0; j <= uppY[i]; j++)
		{
			remapX.at<float>(j, i) = (float)i;
			remapY.at<float>(j, i) = (float)j;
		}
		for (int j = (int)(uppY[i] + 1); j < height; j++)
		{
			float x, y;
			double OPL = abs(j - uppY[i]) * reflactiveIndex;
			x = (float)(uppX[i] + OPL * sin(phi));
			y = (float)(uppY[i] + OPL * cos(phi));

			if (x < 0 || x > width)
				x = -1;
			if (y < 0 || y > height)
				y = -1;

			remapX.at<float>(j, i) = x;
			remapY.at<float>(j, i) = y;
		}
	}

	//Forward Mapping.
	/*for (int i = 0; i < size; i++) {
	double slope = 2 * a * (i * resX) + b;
	theta[i] = atan(slope);
	double theta2 = asin(sin(theta[i]) / reflactiveIndex);
	double phi = (theta[i] - theta2);

	for (int j = 0; j <= uppY[i]; j++)
	{
	remapX.at<float>(j, i) = i;
	remapY.at<float>(j, i) = j;
	}
	for (int j = uppY[i] + 1; j < height; j++)
	{
	float x, y;
	double OPL = abs(j - uppY[i]) / reflactiveIndex;
	x = uppX[i] - OPL * sin(phi);
	y = uppY[i] + OPL * cos(phi);
	remapX.at<float>(y, x) = i;
	remapY.at<float>(y, x) = j;
	}
	}*/

	Mat img_output;

	remap(img, img_output, remapX, remapY, cv::INTER_CUBIC);
	getCvMat() = img_output.clone();

	return theta;
}