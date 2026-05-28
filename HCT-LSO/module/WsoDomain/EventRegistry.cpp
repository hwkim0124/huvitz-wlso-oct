#include "pch.h"
#include "EventRegistry.h"


using namespace wso_domain;

std::mutex EventRegistry::singleMutex_;


struct EventRegistry::EventRegistryImpl
{
	optional<OctPreviewBufferGrabbedEvent> octPreviewGrabbed;
	optional<OctMeasureBufferGrabbedEvent> octMeasureGrabbed;
	optional<OctEnfaceBufferGrabbedEvent> octEnfaceGrabbed;

	optional<OctEnfaceImageProcessedEvent> octEnfaceImageProcessed;
	optional<OctPreviewImageProcessedEvent> octPreviewImageProcessed;
	optional<OctMeasureImageProcessedEvent> octMeasureImageProcessed;
	optional<OctProtocolPatternAcquiredEvent> octPatternAcquired;
	optional<OctProtocolMeasureCompletedEvent> octMeasureCompleted;

	EventRegistryImpl() {
		octPreviewGrabbed = nullopt;
		octMeasureGrabbed = nullopt;
		octEnfaceGrabbed = nullopt;

		octEnfaceImageProcessed = nullopt;
		octPreviewImageProcessed = nullopt;
		octMeasureImageProcessed = nullopt;
		octPatternAcquired = nullopt;
		octMeasureCompleted = nullopt;
	}
};



wso_domain::EventRegistry::EventRegistry() :
	d_ptr(std::make_unique<EventRegistryImpl>())
{
}


wso_domain::EventRegistry::~EventRegistry()
{
}


EventRegistry* wso_domain::EventRegistry::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static EventRegistry instance;
	return &instance;
}

void wso_domain::EventRegistry::setOctPreviewBufferGrabbed(OctPreviewBufferGrabbedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octPreviewGrabbed = clb;
}

void wso_domain::EventRegistry::setOctMeasureBufferGrabbed(OctMeasureBufferGrabbedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octMeasureGrabbed = clb;
}

void wso_domain::EventRegistry::setOctEnfaceBufferGrabbed(OctEnfaceBufferGrabbedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octEnfaceGrabbed = clb;
}

void wso_domain::EventRegistry::setOctEnfaceImageProcessed(OctEnfaceImageProcessedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octEnfaceImageProcessed = clb;
}

void wso_domain::EventRegistry::setOctPreviewImageProcessed(OctPreviewImageProcessedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octPreviewImageProcessed = clb;
}

void wso_domain::EventRegistry::setOctMeasureImageProcessed(OctMeasureImageProcessedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octMeasureImageProcessed = clb;
}

void wso_domain::EventRegistry::setOctProtocolPatternAcquired(OctProtocolPatternAcquiredEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octPatternAcquired = clb;
}

void wso_domain::EventRegistry::setOctProtocolMeasureCompleted(OctProtocolMeasureCompletedEvent clb)
{
	lock_guard<mutex> lock(singleMutex_);
	impl().octMeasureCompleted = clb;
}

void wso_domain::EventRegistry::runOctPreviewBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octPreviewGrabbed) {
		if (auto func = *impl().octPreviewGrabbed; func) {
			func(buff, size_x, size_y, index);
		}
	}
}

void wso_domain::EventRegistry::runOctMeasureBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octMeasureGrabbed) {
		if (auto func = *impl().octMeasureGrabbed; func) {
			func(buff, size_x, size_y, index);
		}
	}
}

void wso_domain::EventRegistry::runOctEnfaceBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octEnfaceGrabbed) {
		if (auto func = *impl().octEnfaceGrabbed; func) {
			func(buff, size_x, size_y, index);
		}
	}
}


void wso_domain::EventRegistry::runOctEnfaceImageProcessed(unsigned char* bits, int width, int height)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octEnfaceImageProcessed) {
		if (auto func = *impl().octEnfaceImageProcessed; func) {
			func(bits, width, height);
		}
	}
}

void wso_domain::EventRegistry::runOctPreviewImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octPreviewImageProcessed) {
		if (auto func = *impl().octPreviewImageProcessed; func) {
			func(bits, width, height, qindex, sn_ratio, ref_point, image_index);
		}
	}
}

void wso_domain::EventRegistry::runOctMeasureImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index, unsigned char* lateral)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octMeasureImageProcessed) {
		if (auto func = *impl().octMeasureImageProcessed; func) {
			func(bits, width, height, qindex, sn_ratio, ref_point, image_index, lateral);
		}
	}
}

void wso_domain::EventRegistry::runOctProtocolPatternAcquired(bool result)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octPatternAcquired) {
		if (auto func = *impl().octPatternAcquired; func) {
			func(result);
		}
	}
}

void wso_domain::EventRegistry::runOctProtocolMeasureCompleted(bool result)
{
	lock_guard<mutex> lock(singleMutex_);
	if (impl().octMeasureCompleted) {
		if (auto func = *impl().octMeasureCompleted; func) {
			func(result);
		}
	}
}


EventRegistry::EventRegistryImpl& wso_domain::EventRegistry::impl(void) const
{
	return *d_ptr;
}
