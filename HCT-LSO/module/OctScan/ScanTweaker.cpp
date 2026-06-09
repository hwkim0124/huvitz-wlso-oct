#include "pch.h"
#include "ScanTweaker.h"

#include "sig_chain.h"

using namespace oct_scan;
using namespace sig_chain;


struct ScanTweaker::ScanTweakerImpl
{
	unsigned long splitCount;

	ScanTweakerImpl() : splitCount(0) {
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<ScanTweaker::ScanTweakerImpl> ScanTweaker::d_ptr(new ScanTweakerImpl());


ScanTweaker::ScanTweaker()
{
}


ScanTweaker::~ScanTweaker()
{
}


bool oct_scan::ScanTweaker::obtainQualityIndexFromPreview(float& qidx, float& sig_ratio, bool next)
{
	unsigned long prevCount, nextCount;
	float prevIndex, nextIndex;
	float prevRatio, nextRatio;

	// Ignore the preview image right after motor moving.
	ChainOutput::getPreviewImageResult(&prevIndex, &prevRatio, nullptr, &prevCount);
	if (!next) {
		qidx = prevIndex;
		sig_ratio = prevRatio;
		return true;
	}

	int waits = 0;

	while (true) {
		this_thread::sleep_for(chrono::milliseconds(OBTAIN_NEXT_PREVIEW_WAIT_DELAY));
		if (++waits > OBTAIN_NEXT_PREVIEW_RETRY_MAX) {
			break;
		}

		ChainOutput::getPreviewImageResult(&nextIndex, &nextRatio, nullptr, &nextCount);
		if (prevCount != nextCount) {
			qidx = nextIndex;
			sig_ratio = nextRatio;
			return true;
		}
	}
	return false;
}


bool oct_scan::ScanTweaker::obtainReferencePointFromPreview(int& refPoint, bool next)
{
	unsigned long prevCount, nextCount;
	int prevPoint, nextPoint;

	// Ignore the preview image right after motor moving.
	ChainOutput::getPreviewImageResult(nullptr, nullptr, &prevPoint, &prevCount);
	if (!next) {
		refPoint = prevPoint;
		return true;
	}

	int waits = 0;

	while (true) {
		this_thread::sleep_for(chrono::milliseconds(OBTAIN_NEXT_PREVIEW_WAIT_DELAY));
		if (++waits > OBTAIN_NEXT_PREVIEW_RETRY_MAX) {
			break;
		}

		ChainOutput::getPreviewImageResult(nullptr, nullptr, &nextPoint, &nextCount);
		if (prevCount != nextCount) {
			refPoint = nextPoint;
			return true;
		}
	}
	return false;
}


bool oct_scan::ScanTweaker::isQualityToSignal(float qidx)
{
	return (qidx >= SCAN_AUTO_QINDEX_TO_SIGNAL ? true : false);
}


bool oct_scan::ScanTweaker::isQualityToComplete(float qidx)
{
	return (qidx >= SCAN_AUTO_QINDEX_TO_COMPLETE ? true : false);
}


bool oct_scan::ScanTweaker::isQualityToTarget(float qidx, bool isLensBack)
{
	if (isLensBack) {
		return (qidx >= SCAN_AUTO_QINDEX_TO_TARGET_LENS_BACK ? true : false);
	}
	else {
		return (qidx >= SCAN_AUTO_QINDEX_TO_TARGET ? true : false);
	}
}


bool oct_scan::ScanTweaker::isQualityToConfirm(float qidx)
{
	return (qidx >= SCAN_AUTO_QINDEX_TO_CONFIRM ? true : false);
}


void oct_scan::ScanTweaker::onScanOptimizingStarted(void) 
{
	ChainSetup::pauseDynamicDispersionCorrection(true);
}


void oct_scan::ScanTweaker::onScanOptimizingClosed(void)
{
	ChainSetup::pauseDynamicDispersionCorrection(false);
}


ScanTweaker::ScanTweakerImpl & oct_scan::ScanTweaker::getImpl(void)
{
	return *d_ptr;
}
