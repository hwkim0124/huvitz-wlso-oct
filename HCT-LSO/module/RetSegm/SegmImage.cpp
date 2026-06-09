#include "pch.h"
#include "RetSegm2.h"
#include "SegmImage.h"
#include "CppUtil2.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace ret_segm;
using namespace cv;


struct SegmImage::SegmImageImpl
{
	std::vector<int> colMeans;
	std::vector<float> colStddev;
	std::vector<int> colMaxVals;
	std::vector<int> colMaxLocs;

	float mean;
	float stddev;
	float sampleRatioWidth;
	float sampleRatioHeight;

	SegmImageImpl() :
		mean(0.0f), stddev(0.0f), sampleRatioWidth(1.0f), sampleRatioHeight(1.0f) 
	{
	}
};


SegmImage::SegmImage() :
	d_ptr(make_unique<SegmImageImpl>())
{
}


ret_segm::SegmImage::~SegmImage() = default;
ret_segm::SegmImage::SegmImage(SegmImage && rhs) = default;
SegmImage & ret_segm::SegmImage::operator=(SegmImage && rhs) = default;


ret_segm::SegmImage::SegmImage(const SegmImage & rhs)
	: d_ptr(make_unique<SegmImageImpl>(*rhs.d_ptr))
{
}


SegmImage & ret_segm::SegmImage::operator=(const SegmImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void ret_segm::SegmImage::setSampleRatio(float wRatio, float hRatio)
{
	d_ptr->sampleRatioWidth = wRatio;
	d_ptr->sampleRatioHeight = hRatio;
	return;
}


float ret_segm::SegmImage::getSampleRatioWidth(void) const
{
	return d_ptr->sampleRatioWidth;
}


float ret_segm::SegmImage::getSampleRatioHeight(void) const
{
	return d_ptr->sampleRatioHeight;
}


float ret_segm::SegmImage::getWidthRatioToSample(void) const
{
	float ratio = (float) getWidth() / SAMPLE_IMAGE_WIDTH;
	return ratio;
}


float ret_segm::SegmImage::getHeightRatioToSample(void) const
{
	float ratio = (float)getHeight() / SAMPLE_IMAGE_HEIGHT;
	return ratio;
}


float ret_segm::SegmImage::getMean(void) const
{
	return d_ptr->mean;
}


float ret_segm::SegmImage::getStddev(void) const
{
	return d_ptr->stddev;
}


void ret_segm::SegmImage::setMeanStddev(float mean, float stddev)
{
	d_ptr->mean = mean;
	d_ptr->stddev = stddev;
	return;
}


const std::vector<int>& ret_segm::SegmImage::getColumMaxLocs(void) const
{
	return d_ptr->colMaxLocs;
}


const std::vector<int>& ret_segm::SegmImage::getColumMaxVals(void) const
{
	return d_ptr->colMaxVals;
}


const std::vector<int>& ret_segm::SegmImage::getColumMeans(void) const
{
	return d_ptr->colMeans;
}


void ret_segm::SegmImage::setColumnMax(const std::vector<int>& maxLocs, const std::vector<int>& maxVals)
{
	d_ptr->colMaxLocs = maxLocs;
	d_ptr->colMaxVals = maxVals;
	return;
}


void ret_segm::SegmImage::setColumnMean(const std::vector<int>& means)
{
	d_ptr->colMeans = means;
	return;
}


void ret_segm::SegmImage::setColumnStddev(const std::vector<float>& stddev)
{
	d_ptr->colStddev = stddev;
	return;
}

void ret_segm::SegmImage::updateImageStats(void)
{
	Mat srcMat = getCvMatConst();

	// Mat_<type> can be more convinient to access elements with shorter notation. 
	// http://docs.opencv.org/master/df/dfc/classcv_1_1Mat__.html#details

	// Vec is a template class representing short numerical vectors. 
	// http://docs.opencv.org/master/d6/dcf/classcv_1_1Vec.html#details

	// Scalar is a tuple of 4 doubles. 
	// http://docs.opencv.org/3.0-beta/modules/core/doc/basic_structures.html#Scalar_
	Scalar mean, stddev;
	meanStdDev(srcMat, mean, stddev);

	// These result from the first channel of image. 
	setMeanStddev((float)mean(0), (float)stddev(0));
	return;
}

void ret_segm::SegmImage::updateColumnStats(void)
{
	Mat srcMat = getCvMatConst();

	// Column-wise mean, maximum of all columns.
	// reduce(matSrc, pInMat->m_maxOfCols, 0, CV_REDUCE_MAX);

	Mat means;
	reduce(srcMat, means, 0, REDUCE_AVG);

	auto meanVect = std::vector<int>(means.cols);
	for (int c = 0; c < means.cols; c++) {
		meanVect[c] = means.at<uchar>(0, c);
	}
	setColumnMean(meanVect);

	Mat maxVals = Mat(1, srcMat.cols, CV_32SC1);
	Mat maxIdxs = Mat(1, srcMat.cols, CV_32SC1);

	double maxVal;
	Point maxLoc;
	for (int c = 0; c < srcMat.cols; c++) {
		minMaxLoc(srcMat.col(c), NULL, &maxVal, NULL, &maxLoc);
		maxVals.at<int>(0, c) = (int)maxVal;
		maxIdxs.at<int>(0, c) = (int)maxLoc.y;
	}

	// Don't need deep copy of Mat, since internally reference counted by header copy.
	const int* pVals = maxVals.ptr<int>(0);
	const int* pIdxs = maxIdxs.ptr<int>(0);
	setColumnMax(std::vector<int>(pIdxs, pIdxs + maxIdxs.cols), std::vector<int>(pVals, pVals + maxVals.cols));
	return ;
}
