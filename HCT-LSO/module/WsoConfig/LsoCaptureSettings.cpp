#include "pch.h"
#include "LsoCaptureSettings.h"


using namespace wso_config;

struct LsoCaptureSettings::LsoCaptureSettingsImpl
{
	LsoCaptureFrameROIPreset roiPreset;
	LsoCaptureFrameSeqROIPreset seqPreset;
	LsoCaptureFrameOffsetROIPreset offsPreset;
	LsoCaptureFrameRollSwTrigOverlapPreset rollPreset;

	LsoCaptureSettingsImpl() {
		initializeLsoCaptureSettingsImpl();
	}

	void initializeLsoCaptureSettingsImpl(void) {}
};


LsoCaptureSettings::LsoCaptureSettings() :
	d_ptr(make_unique<LsoCaptureSettingsImpl>())
{}


wso_config::LsoCaptureSettings::~LsoCaptureSettings() = default;
wso_config::LsoCaptureSettings::LsoCaptureSettings(LsoCaptureSettings&& rhs) = default;
LsoCaptureSettings& wso_config::LsoCaptureSettings::operator=(LsoCaptureSettings&& rhs) = default;


wso_config::LsoCaptureSettings::LsoCaptureSettings(const LsoCaptureSettings& rhs)
	: d_ptr(make_unique<LsoCaptureSettingsImpl>(*rhs.d_ptr))
{}


LsoCaptureSettings& wso_config::LsoCaptureSettings::operator=(const LsoCaptureSettings& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void wso_config::LsoCaptureSettings::initializeLsoCaptureSettings(void)
{
	resetToDefaultValues();
	return;
}

void wso_config::LsoCaptureSettings::resetToDefaultValues(void)
{
	impl().roiPreset = {};
	impl().seqPreset = {};
	impl().offsPreset = {};
	impl().rollPreset = {};
	return;
}

bool wso_config::LsoCaptureSettings::importFromBoardProfile(const HbsConfiguration* config)
{
	return true;
}

bool wso_config::LsoCaptureSettings::exportToBoardProfile(HbsConfiguration* config) const
{
	return true;
}

LsoCaptureFrameROIPreset* wso_config::LsoCaptureSettings::getLsoCaptureFrameROIPreset(void) const
{
	return &impl().roiPreset;
}

void wso_config::LsoCaptureSettings::setLsoCaptureFrameROIPreset(const LsoCaptureFrameROIPreset& param)
{
	impl().roiPreset = param;
	return;
}

int wso_config::LsoCaptureSettings::getCaptureFrameCount(void) const
{
	return impl().roiPreset.frameCount;
}

LsoCaptureFrameROI wso_config::LsoCaptureSettings::getCaptureFrameROI(int index) const
{
	if (index < 0 || index >= impl().roiPreset.frameCount) {
		return {};
	}
	return impl().roiPreset.frameRois[index];
}

void wso_config::LsoCaptureSettings::setCaptureFrameCount(int count)
{
	impl().roiPreset.frameCount = count;
	return;
}

void wso_config::LsoCaptureSettings::setCaptureFrameROI(int index, LsoCaptureFrameROI param)
{
	if (index < 0 || index >= impl().roiPreset.frameCount) {
		return;
	}
	impl().roiPreset.frameRois[index] = param;
	return;
}

LsoCaptureFrameSeqROIPreset* wso_config::LsoCaptureSettings::getLsoCaptureFrameSeqROIPreset(void) const
{
	return &impl().seqPreset;
}

void wso_config::LsoCaptureSettings::setLsoCaptureFrameSeqROIPreset(const LsoCaptureFrameSeqROIPreset& param)
{
	impl().seqPreset = param;
	return;
}

int wso_config::LsoCaptureSettings::getSequencerFrameCount(void) const
{
	return impl().seqPreset.frameCount;
}

LsoCaptureFrameROI wso_config::LsoCaptureSettings::getSequencerFrameROI(int index) const
{
	if (index < 0 || index >= impl().seqPreset.frameCount) {
		return {};
	}
	return impl().seqPreset.frameRois[index];
}

void wso_config::LsoCaptureSettings::setSequencerFrameCount(int nCount)
{
	impl().seqPreset.frameCount = nCount;
	return;
}

void wso_config::LsoCaptureSettings::setSequencerFrameROI(int index, LsoCaptureFrameROI param)
{
	if (index < 0 || index >= impl().seqPreset.frameCount) {
		return;
	}
	impl().seqPreset.frameRois[index] = param;
	return;
}

LsoCaptureFrameOffsetROIPreset* wso_config::LsoCaptureSettings::getLsoCaptureFrameOffsetROIPreset(void) const
{
	return &impl().offsPreset;
}

void wso_config::LsoCaptureSettings::setLsoCaptureFrameOffsetROIPreset(const LsoCaptureFrameOffsetROIPreset& param)
{
	impl().offsPreset = param;
	return;
}

int wso_config::LsoCaptureSettings::getOffsetRoiWidth(void) const
{
	return impl().offsPreset.roiWidth;
}

int wso_config::LsoCaptureSettings::getOffsetRoiHeight(void) const
{
	return impl().offsPreset.roiHeight;
}

void wso_config::LsoCaptureSettings::setOffsetRoiWidth(int width)
{
	impl().offsPreset.roiWidth = width;
	return;
}

void wso_config::LsoCaptureSettings::setOffsetRoiHeight(int height)
{
	impl().offsPreset.roiHeight = height;
	return;
}

int wso_config::LsoCaptureSettings::getOffsetFrameCount(void) const
{
	return impl().offsPreset.frameCount;
}

LsoCaptureFrameROI wso_config::LsoCaptureSettings::getOffsetFrameROI(int index) const
{
	if (index < 0 || index >= impl().offsPreset.frameCount) {
		return {};
	}
	return impl().offsPreset.frameRois[index];
}

void wso_config::LsoCaptureSettings::setOffsetFrameCount(int count)
{
	impl().offsPreset.frameCount = count;
	return;
}

void wso_config::LsoCaptureSettings::setOffsetFrameROI(int index, LsoCaptureFrameROI param)
{
	if (index < 0 || index >= impl().offsPreset.frameCount) {
		return;
	}
	impl().offsPreset.frameRois[index] = param;
	return;
}

LsoCaptureFrameRollSwTrigOverlapPreset* wso_config::LsoCaptureSettings::getLsoCaptureFrameRollSwTrigOverlapPreset(void) const
{
	return &impl().rollPreset;
}

void wso_config::LsoCaptureSettings::setLsoCaptureFrameRollSwTrigOverlapPreset(const LsoCaptureFrameRollSwTrigOverlapPreset& param)
{
	impl().rollPreset = param;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapCount(int count)
{
	impl().rollPreset.overlap.overlapFrameCount = count;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapRoiWidth(int roiWidth)
{
	impl().rollPreset.overlap.frameWidth = roiWidth;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapRoiHeight(int roiHeight)
{
	impl().rollPreset.overlap.frameHeight = roiHeight;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapOffsetX(int offsetX)
{
	impl().rollPreset.overlap.offsetX = offsetX;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapOffsetY(int offsetY)
{
	impl().rollPreset.overlap.offsetY = offsetY;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapTriggerInterval(int interval)
{
	impl().rollPreset.overlap.triggerIntervalMs = interval;
	return;
}

void wso_config::LsoCaptureSettings::setFrameRollSwTrigOverlapBrightness(int bright)
{
	impl().rollPreset.overlap.brightness = bright;
	return;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapCount(void) const
{
	return impl().rollPreset.overlap.overlapFrameCount;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapRoiWidth(void) const
{
	return impl().rollPreset.overlap.frameWidth;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapRoiHeight(void) const
{
	return impl().rollPreset.overlap.frameHeight;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapOffsetX(void) const
{
	return impl().rollPreset.overlap.offsetX;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapOffsetY(void) const
{
	return impl().rollPreset.overlap.offsetY;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapTriggerInterval(void) const
{
	return impl().rollPreset.overlap.triggerIntervalMs;
}

int wso_config::LsoCaptureSettings::getFrameRollSwTrigOverlapBrightness(void) const
{
	return impl().rollPreset.overlap.brightness;
}


LsoCaptureSettings::LsoCaptureSettingsImpl& wso_config::LsoCaptureSettings::impl(void) const
{
	return *d_ptr;
}
