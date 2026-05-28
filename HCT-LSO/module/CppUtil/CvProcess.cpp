#include "pch.h"
#include "CvProcess.h"
#include "CvImage.h"
#include "Logger.h"

#include "CppUtil2.h"

using namespace cpp_util;
using namespace cv;
using namespace std;


struct CvProcess::CvProcessImpl
{
	cv::Mat matSum;
	cv::Mat matBase;
	cv::Mat matMask;
	int countOfSum;

	CvProcessImpl()
	{
	}
};


CvProcess::CvProcess() :
	d_ptr(make_unique<CvProcessImpl>())
{
}


CvProcess::~CvProcess() = default;
cpp_util::CvProcess::CvProcess(CvProcess && rhs) = default;
CvProcess & cpp_util::CvProcess::operator=(CvProcess && rhs) = default;


cpp_util::CvProcess::CvProcess(const CvProcess & rhs)
	: d_ptr(make_unique<CvProcessImpl>(*rhs.d_ptr))
{
}


CvProcess & cpp_util::CvProcess::operator=(const CvProcess & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool cpp_util::CvProcess::startAveraging(CvImage & image)
{
	if (image.isEmpty()) {
		getImpl().countOfSum = 0;
		return false;
	}

	getImpl().matBase = image.getCvMat().clone();
	getImpl().matMask = Mat(image.getHeight(), image.getWidth(), CV_32SC1, Scalar(1));

	getImpl().matSum = Mat(image.getHeight(), image.getWidth(), CV_32SC1, Scalar(0));
	
	Mat matSrc;
	getImpl().matBase.convertTo(matSrc, CV_32SC1);
	getImpl().matSum += matSrc;

	getImpl().countOfSum = 1;
	return true;
}


bool cpp_util::CvProcess::insertAveraging(CvImage & image, double ccLimit, int numIters, double termEps)
{
	if (image.isEmpty()) {
		return false;
	}

	Mat matWarp = Mat::eye(2, 3, CV_32F);
	const int motionType = MOTION_EUCLIDEAN; // MOTION_AFFINE; // MOTION_TRANSLATION;// MOTION_EUCLIDEAN;

	TermCriteria criteria(TermCriteria::COUNT + TermCriteria::EPS, numIters, termEps);

	try {
		Mat matSrc = image.getCvMat();
		Mat matDst;
		double cc = findTransformECC(getImpl().matBase, matSrc, matWarp, motionType, criteria);

		LogD() << "Transform ecc: " << cc << ", limit: " << ccLimit << ", countOfSum: " << getImpl().countOfSum;

		if (cc > ccLimit) {
			warpAffine(matSrc, matDst, matWarp, matSrc.size(), INTER_LINEAR | WARP_INVERSE_MAP);
			Mat mat32s;
			matDst.convertTo(mat32s, CV_32SC1);
			//cv::add(getImpl().matSum, mat32s, getImpl().matSum);
			getImpl().matSum += mat32s;

			Mat matCnt;
			matCnt = cv::min(mat32s, 1);
			//matCnt.convertTo(mat32s, CV_32SC1);
			//getImpl().matMask += matCnt;
			//cv::add(getImpl().matMask, matCnt, getImpl().matMask);
			getImpl().matMask += matCnt;
			getImpl().countOfSum++;
			return true;
		}
	}
	catch (...) {
	}
	return false;
}


int cpp_util::CvProcess::closeAveraging(CvImage & image)
{
	if (getImpl().countOfSum > 1) {
		getImpl().matSum /= getImpl().matMask;
	}
	getImpl().matSum.convertTo(image.getCvMat(), CV_8UC1);
	return getImpl().countOfSum;
}


bool cpp_util::CvProcess::registImage(CvImage & imgBase, CvImage & imgRegist, bool lateral, int numIters, double termEps)
{
	Mat matWarp = Mat::eye(2, 3, CV_32F);
	const int motionType = MOTION_EUCLIDEAN;// MOTION_EUCLIDEAN;

	TermCriteria criteria(TermCriteria::COUNT + TermCriteria::EPS, numIters, termEps);

	Mat matRef = imgBase.getCvMat();
	Mat matSrc = imgRegist.getCvMat();
	Mat matDst;
	
	try {
		double cc = findTransformECC(matRef, matSrc, matWarp, motionType, criteria);
		// LogD() << matWarp.at<float>(0, 0) << ", " << matWarp.at<float>(0, 1) << ", " << matWarp.at<float>(0, 2);
		LogD() << matWarp.at<float>(1, 0) << ", " << matWarp.at<float>(1, 1) << ", " << matWarp.at<float>(1, 2);

		// Ignore lateral translation.
		if (!lateral) {
			matWarp.at<float>(0, 2) = 0.0f;
		}

		warpAffine(matSrc, matDst, matWarp, matSrc.size(), INTER_LINEAR | WARP_INVERSE_MAP);
		imgRegist.getCvMat() = matDst.clone();
		return true;
	}
	catch (const std::exception &ex) {
		LogD() << ex.what() << std::endl;
	}
	return false;
}


bool cpp_util::CvProcess::registImage2(CvImage & imgBase, CvImage & imgRegist, CvImage& imgRegist2, CvImage& imgRegist3, bool lateral, int numIters, double termEps)
{
	Mat matWarp = Mat::eye(2, 3, CV_32F);
	const int motionType = MOTION_TRANSLATION;// MOTION_EUCLIDEAN;

	TermCriteria criteria(TermCriteria::COUNT + TermCriteria::EPS, numIters, termEps);

	Mat matRef = imgBase.getCvMat();
	Mat matSrc = imgRegist.getCvMat();
	Mat matDst;

	try {
		double cc = findTransformECC(matRef, matSrc, matWarp, motionType, criteria);

		// Ignore lateral translation.
		if (!lateral) {
			matWarp.at<float>(0, 2) = 0.0f;
		}

		// LogD() << matWarp.at<float>(0, 0) << ", " << matWarp.at<float>(0, 1) << ", " << matWarp.at<float>(0, 2);
		// LogD() << matWarp.at<float>(1, 0) << ", " << matWarp.at<float>(1, 1) << ", " << matWarp.at<float>(1, 2);

		warpAffine(matSrc, matDst, matWarp, matSrc.size(), INTER_LINEAR | WARP_INVERSE_MAP);
		imgRegist.getCvMat() = matDst.clone();

		Mat matDst2;
		Mat matSrc2 = imgRegist.getCvMat();
		warpAffine(matSrc2, matDst2, matWarp, matSrc2.size(), INTER_LINEAR | WARP_INVERSE_MAP);
		imgRegist2.getCvMat() = matDst2.clone();

		Mat matDst3;
		Mat matSrc3 = imgRegist.getCvMat();
		warpAffine(matSrc3, matDst3, matWarp, matSrc3.size(), INTER_LINEAR | WARP_INVERSE_MAP);
		imgRegist3.getCvMat() = matDst3.clone();
		return true;
	}
	catch (const std::exception &ex) {
		LogD() << ex.what() << std::endl;
	}
	return false;
}


CvProcess::CvProcessImpl & cpp_util::CvProcess::getImpl(void)
{
	return *d_ptr;
}
