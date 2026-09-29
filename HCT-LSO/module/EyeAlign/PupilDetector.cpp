#include "pch.h"
#include "PupilDetector.h"


using namespace eye_align;


PupilDetector::PupilDetector() :
	camId_(IrCameraId::CORNEA_LOWER), param_()
{}


PupilDetector::PupilDetector(IrCameraId camId, const PupilDetectParam& param) :
	camId_(camId), param_(param)
{}


PupilDetector::~PupilDetector()
{}


void eye_align::PupilDetector::setCameraId(IrCameraId camId)
{
	camId_ = camId;
}

IrCameraId eye_align::PupilDetector::getCameraId(void) const
{
	return camId_;
}

void eye_align::PupilDetector::setParam(const PupilDetectParam& param)
{
	param_ = param;
}

const PupilDetectParam& eye_align::PupilDetector::getParam(void) const
{
	return param_;
}

float eye_align::PupilDetector::getTargetX(int width) const
{
	return (param_.targetX < 0) ? (width * 0.5f) : static_cast<float>(param_.targetX);
}

float eye_align::PupilDetector::getTargetY(int height) const
{
	return (param_.targetY < 0) ? (height * 0.5f) : static_cast<float>(param_.targetY);
}

PupilDetectResult eye_align::PupilDetector::detect(const std::uint8_t* gray, int width, int height, int stride, int frameSeq) const
{
	PupilDetectResult result;
	result.camId = static_cast<int32_t>(camId_);
	result.found = 0;
	result.frameSeq = frameSeq;

	if (gray == nullptr || width <= 0 || height <= 0) {
		return result;
	}
	if (stride <= 0) {
		stride = width;
	}
	if (stride < width) {
		return result;
	}

	// TODO: pupil detection algorithm.
	// Expected output: result.found = 1, result.cx / result.cy = pupil centre in image
	// pixels, result.radius = pupil radius in pixels, result.score = confidence 0..1.
	// Until then the detector reports "not found"; the aligner's override hook
	// (EyeAligner::setPupilOverride) stands in for it on the bench.
	return result;
}
