#include "pch.h"
#include "OctScanStatus.h"


using namespace wso_domain;


std::mutex OctScanStatus::singleMutex_;


struct OctScanStatus::OctScanStatusImpl
{
	bool captureToMeasure;
	bool previewAveraging;

	OctScanStatusImpl()
	{
		captureToMeasure = false;
		previewAveraging = false;
	}
};


wso_domain::OctScanStatus::OctScanStatus() :
	d_ptr(std::make_unique<OctScanStatusImpl>())
{
}


wso_domain::OctScanStatus::~OctScanStatus()
{
}


OctScanStatus* wso_domain::OctScanStatus::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static OctScanStatus instance;
	return &instance;
}

bool wso_domain::OctScanStatus::isCapturingToMeasure(optional<bool> flag) const
{
	if (flag.has_value()) {
		impl().captureToMeasure = flag.value();
	}
	return impl().captureToMeasure;
}

bool wso_domain::OctScanStatus::isPreviewAveragingEnabled(optional<bool> flag) const
{
	if (flag.has_value()) {
		impl().previewAveraging = flag.value();
	}
	return impl().previewAveraging;
}


OctScanStatus::OctScanStatusImpl& wso_domain::OctScanStatus::impl(void) const
{
	return *d_ptr;
}
