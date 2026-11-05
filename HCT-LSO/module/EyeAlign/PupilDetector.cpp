#include "pch.h"
#include "PupilDetector.h"
#include "PupilCenterModel.h"

#include <algorithm>


using namespace eye_align;


PupilDetector::PupilDetector() :
	cameraId_(IrCameraId::CORNEA_LOWER)
{}


PupilDetector::PupilDetector(IrCameraId cameraId) :
	cameraId_(cameraId)
{}


PupilDetector::~PupilDetector()
{}


void eye_align::PupilDetector::setCameraId(IrCameraId cameraId)
{
	cameraId_ = cameraId;
}

IrCameraId eye_align::PupilDetector::getCameraId(void) const
{
	return cameraId_;
}

void eye_align::PupilDetector::setModel(std::shared_ptr<PupilCenterModel> model)
{
	model_ = std::move(model);
}

std::shared_ptr<PupilCenterModel> eye_align::PupilDetector::getModel(void) const
{
	return model_;
}

float eye_align::PupilDetector::getTargetX(int width)
{
	return (PUPIL_TARGET_X < 0) ? (width * 0.5f) : static_cast<float>(PUPIL_TARGET_X);
}

float eye_align::PupilDetector::getTargetY(int height)
{
	return (PUPIL_TARGET_Y < 0) ? (height * 0.5f) : static_cast<float>(PUPIL_TARGET_Y);
}

PupilDetectResult eye_align::PupilDetector::detect(const std::uint8_t* gray, int width, int height, int stride, int frameSequence) const
{
	PupilDetectResult result;
	result.camId = static_cast<int32_t>(cameraId_);
	result.found = 0;
	result.frameSeq = frameSequence;

	if (gray == nullptr || width <= 0 || height <= 0) {
		return result;
	}
	if (stride <= 0) {
		stride = width;
	}
	if (stride < width) {
		return result;
	}

	if (!model_) {
		return result;
	}

	try {
		float normX = 0.0f, normY = 0.0f, presence = 0.0f;
		if (!model_->infer(gray, width, height, stride, normX, normY, presence)) {
			return result;
		}
		result.score = presence;
		result.cx = std::clamp(normX, 0.0f, 1.0f) * static_cast<float>(width);
		result.cy = std::clamp(normY, 0.0f, 1.0f) * static_cast<float>(height);
		result.radius = 0.0f;		// the network has no radius output
		// A NaN presence compares false and reads as "not found".
		result.found = (presence >= model_->getPresenceThreshold()) ? 1 : 0;
	}
	catch (...) {
		result.found = 0;
	}
	return result;
}
