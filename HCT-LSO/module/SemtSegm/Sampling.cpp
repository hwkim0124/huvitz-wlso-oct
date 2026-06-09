#include "pch.h"
#include "Sampling.h"
#include "SemtSegm2.h"

using namespace semt_segm;
using namespace cv;


struct Sampling::SamplingImpl
{
	OcularImage imgSource;
	OcularImage imgSample;
	OcularImage imgAverage;
	OcularImage imgAscent;
	OcularImage imgDescent;

	SamplingImpl()
	{
	}
};


Sampling::Sampling() :
	d_ptr(make_unique<SamplingImpl>())
{
}


semt_segm::Sampling::~Sampling() = default;
semt_segm::Sampling::Sampling(Sampling && rhs) = default;
Sampling & semt_segm::Sampling::operator=(Sampling && rhs) = default;


bool semt_segm::Sampling::produceSample(OctScanImageDescript image, int width, int height)
{
	if (image.isEmpty()) {
		return false;
	}

	auto& sample = getImpl().imgSample;
	auto& source = getImpl().imgSource;

	if (source.fromBitsData(image.pData, image.width, image.height)) {
		if (width != image.width || height != image.height) {
			// Source image is shirinked to sampling dimension without aliasing effect. 
			Size size(width, height);
			cv::resize(source.getCvMatConst(), sample.getCvMat(), size, 0.0, 0.0, INTER_AREA);
		}
		else {
			sample.getCvMat() = source.getCvMatConst();
		}

		// sample.applyGuidedFilter(1);
		sample.updateImageStats();
		sample.updateColumnStats();
		return true;
	}
	return false;
}


bool semt_segm::Sampling::checkIfValidSample(int index) const
{
	const auto& sample = getImpl().imgSample;

	float mean = sample.getMean();
	float stddev = sample.getStddev();

	if (mean < VALID_IMAGE_MEAN_MIN || mean > VALID_IMAGE_MEAN_MAX) {
		LogD() << "Sample index: " << index << ", invalid mean: " << mean;
		return false;
	}

	auto peaks = sample.getColumMaxVals();
	auto means = sample.getColumMeans();

	int under = 0;
	int sidx = (int)(peaks.size() * 0.20f);
	int eidx = (int)(peaks.size() * 0.80f);
	int tcnt = (eidx - sidx + 1);
	for (int c = sidx; c < eidx; c++) {
		float ratio = peaks[c] / (means[c] + 1.0f);
		if (ratio < VALID_PEAK_TO_MEAN_RATIO_MIN) {
			under++;
		}
	}

	float poor = (float)under / tcnt; // peaks.size();
	if (poor >= VALID_POOR_SNR_RATIO_MIN) {
		LogD() << "Sample index: " << index << ", poor SNR ratio: " << poor;
		return false;
	}

	LogD() << "Sample index: " << index << ", mean: " << mean << ", good: " << (1.0f - poor);
	return true;
}

bool semt_segm::Sampling::createGradientMaps(void)
{
	if (!createAscentGradients()) {
		return false;
	}
	/*
	if (!createDescentGraidents()) {
		return false;
	}
	*/
	if (!createSampleAveraging()) {
		return false;
	}

	getImpl().imgAscent.updateColumnStats();
	getImpl().imgAscent.updateImageStats();
	// getImpl().imgDescent.updateColumnStats();
	// getImpl().imgDescent.updateImageStats();
	getImpl().imgAverage.updateColumnStats();
	getImpl().imgAverage.updateImageStats();
	return true;
}

int semt_segm::Sampling::getSampleWidth(void) const
{
	return sample()->getWidth();
}

int semt_segm::Sampling::getSampleHeight(void) const
{
	return sample()->getHeight();
}

int semt_segm::Sampling::getSourceWidth(void) const
{
	return source()->getWidth();
}

int semt_segm::Sampling::getSourceHeight(void) const
{
	return source()->getHeight();
}

float semt_segm::Sampling::getSampleWidthRatio(void) const
{
	return ((float)getSampleWidth())/((float)getSourceWidth());
}

float semt_segm::Sampling::getSampleHeightRatio(void) const
{
	return ((float)getSampleHeight()) / ((float)getSourceHeight());
}

bool semt_segm::Sampling::createAscentGradients(void)
{
	const int KERNEL_ROWS = 17; // 9; // 19;
	const int KERNEL_COLS = 5; // 5;

	Mat srcMat = getImpl().imgSample.getCvMatConst();
	Mat kernel = Mat::ones(KERNEL_ROWS, KERNEL_COLS, CV_32F);

	for (int r = 0; r < kernel.rows; r++) {
		for (int c = kernel.cols / 3; c < (kernel.cols * 2) / 3; c++) {
			kernel.at<float>(r, c) = 1.0f; // 2.0f;
		}
	}
	for (int r = 0; r < kernel.rows / 2; r++) {
		for (int c = 0; c < kernel.cols; c++) {
			kernel.at<float>(r, c) *= -1.0f;
		}
	}
	/*
	for (int c = 0; c < kernel.cols; c++) {
	kernel.at<float>(kernel.rows / 2, c) = 0.0f;
	}
	*/

	Mat conv;
	Mat mask;
	filter2D(srcMat, conv, CV_32F, kernel, Point(-1, -1), 0, BORDER_REFLECT);

	// Leave only positive gradients, and normalize in range between [0, 255].
	mask = conv < 0;
	Mat masc;
	Mat ascMat;

	conv.copyTo(masc);
	masc.setTo(0.0f, mask);
	cv::normalize(masc, ascMat, 0, 255, NORM_MINMAX, CV_8U);
	getImpl().imgAscent.getCvMat() = ascMat;
	return true;
}

bool semt_segm::Sampling::createDescentGraidents(void)
{
	const int KERNEL_ROWS = 11;
	const int KERNEL_COLS = 5;

	Mat srcMat = getImpl().imgSample.getCvMatConst();
	Mat kernel = Mat::ones(KERNEL_ROWS, KERNEL_COLS, CV_32F);

	for (int r = 0; r < kernel.rows; r++) {
		for (int c = kernel.cols / 3; c < (kernel.cols * 2) / 3; c++) {
			kernel.at<float>(r, c) = 2.0f;
		}
	}
	for (int r = (kernel.rows / 2 + 1); r < kernel.rows; r++) {
		for (int c = 0; c < kernel.cols; c++) {
			kernel.at<float>(r, c) *= -1.0f;
		}
	}

	Mat conv;
	Mat mask;
	filter2D(srcMat, conv, CV_32F, kernel, Point(-1, -1), 0, BORDER_REFLECT);

	// Leave only positive gradients, and normalize in range between [0, 255].
	mask = conv < 0;
	Mat mdes;
	Mat desMat;

	conv.copyTo(mdes);
	mdes.setTo(0.0f, mask);
	cv::normalize(mdes, desMat, 0, 255, NORM_MINMAX, CV_8U);
	getImpl().imgDescent.getCvMat() = desMat;
	return true;
}

bool semt_segm::Sampling::createSampleAveraging(void)
{
	const int RADIUS = 2;

	Mat srcMat = getImpl().imgSample.getCvMatConst();

	getImpl().imgAverage.getCvMat() = srcMat;
	getImpl().imgAverage.applyGuidedFilter(RADIUS);
	return true;

	/*
	const int KERNEL_ROWS = 27; // 27; // 19;
	const int KERNEL_COLS = 27; // 27; // 27; // 19;

	Mat kernel = Mat::ones(KERNEL_ROWS, KERNEL_COLS, CV_32F);

	Mat conv;
	Mat mask;
	filter2D(srcMat, conv, CV_32F, kernel, Point(-1, -1), 0, BORDER_CONSTANT);

	// Leave only positive gradients, and normalize in range between [0, 255].
	mask = conv < 0;
	Mat mdes;
	Mat desMat;

	conv.copyTo(mdes);
	mdes.setTo(0.0f, mask);
	cv::normalize(mdes, desMat, 0, 255, NORM_MINMAX, CV_8U);
	getImpl().imgAverage.getCvMat() = desMat;
	return true;
	*/
}

OcularImage * semt_segm::Sampling::source(void) const
{
	return &(getImpl().imgSource);
}

OcularImage * semt_segm::Sampling::sample(void) const
{
	return &(getImpl().imgSample);
}

OcularImage * semt_segm::Sampling::ascent(void) const
{
	return &(getImpl().imgAscent);
}

OcularImage * semt_segm::Sampling::descent(void) const
{
	return &(getImpl().imgDescent);
}

OcularImage * semt_segm::Sampling::average(void) const
{
	return &(getImpl().imgAverage);
}


Sampling::SamplingImpl & semt_segm::Sampling::getImpl(void) const
{
	return *d_ptr;
}


