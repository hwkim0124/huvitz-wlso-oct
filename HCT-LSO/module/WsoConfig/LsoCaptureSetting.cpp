#include "pch.h"
#include "LsoCaptureSetting.h"


using namespace wso_config;

struct LsoCaptureSetting::LsoCaptureSettingImpl
{
	LsoCaptureFrameROIPreset roiPreset;
	LsoCaptureFrameSeqROIPreset seqPreset;
	LsoCaptureFrameOffsetROIPreset offsPreset;
	LsoCaptureFrameRollSwTrigOverlapPreset rollPreset;

	LsoCaptureSettingImpl() {
		initializeLsoCaptureSettingImpl();
	}

	void initializeLsoCaptureSettingImpl(void) {}
};


LsoCaptureSetting::LsoCaptureSetting() :
	d_ptr(make_unique<LsoCaptureSettingImpl>())
{}


wso_config::LsoCaptureSetting::~LsoCaptureSetting() = default;
wso_config::LsoCaptureSetting::LsoCaptureSetting(LsoCaptureSetting&& rhs) = default;
LsoCaptureSetting& wso_config::LsoCaptureSetting::operator=(LsoCaptureSetting&& rhs) = default;


wso_config::LsoCaptureSetting::LsoCaptureSetting(const LsoCaptureSetting& rhs)
	: d_ptr(make_unique<LsoCaptureSettingImpl>(*rhs.d_ptr))
{}


LsoCaptureSetting& wso_config::LsoCaptureSetting::operator=(const LsoCaptureSetting& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void wso_config::LsoCaptureSetting::initializeLsoCaptureSetting(void)
{
	resetToDefaultValues();
	return;
}

void wso_config::LsoCaptureSetting::resetToDefaultValues(void)
{
	impl().roiPreset = {};
	impl().seqPreset = {};
	impl().offsPreset = {};
	impl().rollPreset = {};
	return;
}

bool wso_config::LsoCaptureSetting::importFromBoardProfile(const HbsConfiguration* config)
{
	return true;
}

bool wso_config::LsoCaptureSetting::exportToBoardProfile(HbsConfiguration* config) const
{
	return true;
}

LsoCaptureFrameROIPreset* wso_config::LsoCaptureSetting::getLsoCaptureFrameROIPreset(void) const
{
	return &impl().roiPreset;
}

void wso_config::LsoCaptureSetting::setLsoCaptureFrameROIPreset(const LsoCaptureFrameROIPreset& param)
{
	impl().roiPreset = param;
	return;
}

int wso_config::LsoCaptureSetting::getCaptureFrameCount(void) const
{
	return impl().roiPreset.frameCount;
}

LsoCaptureFrameROI wso_config::LsoCaptureSetting::getCaptureFrameROI(int index) const
{
	if (index < 0 || index >= impl().roiPreset.frameCount) {
		return {};
	}
	return impl().roiPreset.frameRois[index];
}

void wso_config::LsoCaptureSetting::setCaptureFrameCount(int count)
{
	impl().roiPreset.frameCount = count;
	return;
}

void wso_config::LsoCaptureSetting::setCaptureFrameROI(int index, LsoCaptureFrameROI param)
{
	if (index < 0 || index >= impl().roiPreset.frameCount) {
		return;
	}
	impl().roiPreset.frameRois[index] = param;
	return;
}

LsoCaptureFrameSeqROIPreset* wso_config::LsoCaptureSetting::getLsoCaptureFrameSeqROIPreset(void) const
{
	return &impl().seqPreset;
}

void wso_config::LsoCaptureSetting::setLsoCaptureFrameSeqROIPreset(const LsoCaptureFrameSeqROIPreset& param)
{
	impl().seqPreset = param;
	return;
}

int wso_config::LsoCaptureSetting::getSequencerFrameCount(void) const
{
	return impl().seqPreset.frameCount;
}

LsoCaptureFrameROI wso_config::LsoCaptureSetting::getSequencerFrameROI(int index) const
{
	if (index < 0 || index >= impl().seqPreset.frameCount) {
		return {};
	}
	return impl().seqPreset.frameRois[index];
}

void wso_config::LsoCaptureSetting::setSequencerFrameCount(int nCount)
{
	impl().seqPreset.frameCount = nCount;
	return;
}

void wso_config::LsoCaptureSetting::setSequencerFrameROI(int index, LsoCaptureFrameROI param)
{
	if (index < 0 || index >= impl().seqPreset.frameCount) {
		return;
	}
	impl().seqPreset.frameRois[index] = param;
	return;
}

LsoCaptureFrameOffsetROIPreset* wso_config::LsoCaptureSetting::getLsoCaptureFrameOffsetROIPreset(void) const
{
	return &impl().offsPreset;
}

void wso_config::LsoCaptureSetting::setLsoCaptureFrameOffsetROIPreset(const LsoCaptureFrameOffsetROIPreset& param)
{
	impl().offsPreset = param;
	return;
}

int wso_config::LsoCaptureSetting::getOffsetRoiWidth(void) const
{
	return impl().offsPreset.roiWidth;
}

int wso_config::LsoCaptureSetting::getOffsetRoiHeight(void) const
{
	return impl().offsPreset.roiHeight;
}

void wso_config::LsoCaptureSetting::setOffsetRoiWidth(int width)
{
	impl().offsPreset.roiWidth = width;
	return;
}

void wso_config::LsoCaptureSetting::setOffsetRoiHeight(int height)
{
	impl().offsPreset.roiHeight = height;
	return;
}

int wso_config::LsoCaptureSetting::getOffsetFrameCount(void) const
{
	return impl().offsPreset.frameCount;
}

LsoCaptureFrameROI wso_config::LsoCaptureSetting::getOffsetFrameROI(int index) const
{
	if (index < 0 || index >= impl().offsPreset.frameCount) {
		return {};
	}
	return impl().offsPreset.frameRois[index];
}

void wso_config::LsoCaptureSetting::setOffsetFrameCount(int count)
{
	impl().offsPreset.frameCount = count;
	return;
}

void wso_config::LsoCaptureSetting::setOffsetFrameROI(int index, LsoCaptureFrameROI param)
{
	if (index < 0 || index >= impl().offsPreset.frameCount) {
		return;
	}
	impl().offsPreset.frameRois[index] = param;
	return;
}

LsoCaptureFrameRollSwTrigOverlapPreset* wso_config::LsoCaptureSetting::getLsoCaptureFrameRollSwTrigOverlapPreset(void) const
{
	return &impl().rollPreset;
}

void wso_config::LsoCaptureSetting::setLsoCaptureFrameRollSwTrigOverlapPreset(const LsoCaptureFrameRollSwTrigOverlapPreset& param)
{
	impl().rollPreset = param;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapCount(int count)
{
	impl().rollPreset.overlap.overlapFrameCount = count;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapRoiWidth(int roiWidth)
{
	impl().rollPreset.overlap.frameWidth = roiWidth;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapRoiHeight(int roiHeight)
{
	impl().rollPreset.overlap.frameHeight = roiHeight;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapOffsetX(int offsetX)
{
	impl().rollPreset.overlap.offsetX = offsetX;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapOffsetY(int offsetY)
{
	impl().rollPreset.overlap.offsetY = offsetY;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapTriggerInterval(int interval)
{
	impl().rollPreset.overlap.triggerIntervalMs = interval;
	return;
}

void wso_config::LsoCaptureSetting::setFrameRollSwTrigOverlapBrightness(int bright)
{
	impl().rollPreset.overlap.brightness = bright;
	return;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapCount(void) const
{
	return impl().rollPreset.overlap.overlapFrameCount;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapRoiWidth(void) const
{
	return impl().rollPreset.overlap.frameWidth;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapRoiHeight(void) const
{
	return impl().rollPreset.overlap.frameHeight;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapOffsetX(void) const
{
	return impl().rollPreset.overlap.offsetX;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapOffsetY(void) const
{
	return impl().rollPreset.overlap.offsetY;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapTriggerInterval(void) const
{
	return impl().rollPreset.overlap.triggerIntervalMs;
}

int wso_config::LsoCaptureSetting::getFrameRollSwTrigOverlapBrightness(void) const
{
	return impl().rollPreset.overlap.brightness;
}


LsoCaptureSetting::LsoCaptureSettingImpl& wso_config::LsoCaptureSetting::impl(void) const
{
	return *d_ptr;
}
