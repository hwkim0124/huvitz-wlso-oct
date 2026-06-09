#include "pch.h"
#include "OctArchive.h"
#include "SloArchive.h"
#include "SloScanning.h"


using namespace wso_system;
using namespace std;


std::mutex OctArchive::singleMutex_;


struct OctArchive::OctArchiveImpl
{
	OctScanProtocol* protocol;
	PatternPlan* pattern;
	bool initialized;

	vector<unique_ptr<ProtocolResult>> results;

	OctArchiveImpl() {
		initializeOctArchiveImpl();
	}

	void initializeOctArchiveImpl(void) {
		protocol = nullptr;
		pattern = nullptr;
		initialized = false;
	}
};



wso_system::OctArchive::OctArchive() :
	d_ptr(std::make_unique<OctArchiveImpl>())
{
}


wso_system::OctArchive::~OctArchive()
{
}


OctArchive* wso_system::OctArchive::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static OctArchive instance;
	return &instance;
}

bool wso_system::OctArchive::initializeOctArchive(void)
{
	if (auto regist = EventRegistry::getInstance(); regist) {
		auto func1 = std::bind(&OctArchive::onOctMeasureImageProcessed, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
			std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
			std::placeholders::_7, std::placeholders::_8);
		regist->setOctMeasureImageProcessed(func1);
		auto func2 = std::bind(&OctArchive::onOctPreviewImageProcessed, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
			std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
			std::placeholders::_7);
		regist->setOctPreviewImageProcessed(func2);
		auto func3 = std::bind(&OctArchive::onOctEnfaceImageProcessed, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		regist->setOctEnfaceImageProcessed(func3);

		auto func4 = std::bind(&OctArchive::onOctProtocolPatternAcquired, this,
			std::placeholders::_1);
		regist->setOctProtocolPatternAcquired(func4);

		auto func5 = std::bind(&OctArchive::onOctProtocolMeasureCompleted, this,
			std::placeholders::_1);
		regist->setOctProtocolMeasureCompleted(func5);
	}

	impl().initialized = true;
	LogD() << "OctArchive initialized: " << isInitialized();
	return isInitialized();
}

void wso_system::OctArchive::releaseOctArchive(void)
{
	if (auto regist = EventRegistry::getInstance(); regist) {
		regist->setOctMeasureImageProcessed(nullptr);
		regist->setOctPreviewImageProcessed(nullptr);
		regist->setOctEnfaceImageProcessed(nullptr);

		regist->setOctProtocolPatternAcquired(nullptr);
		regist->setOctProtocolMeasureCompleted(nullptr);
	}
	impl().initialized = false;
	LogD() << "OctArchive released.";
	return;
}

bool wso_system::OctArchive::isInitialized(void) const
{
	return impl().initialized;
}

bool wso_system::OctArchive::initiateProtocolResult(OctScanProtocol* protocol, PatternPlan* plan)
{
	impl().protocol = protocol;
	impl().pattern = plan;

	impl().results.clear();
	if (protocol) {
		auto result = make_unique<ProtocolResult>(*protocol);
		impl().results.push_back(std::move(result));
	}
	return true;
}

void wso_system::OctArchive::clearProtocolResult(void)
{
	impl().results.clear();
	impl().protocol = nullptr;
	impl().pattern = nullptr;
	return;
}

bool wso_system::OctArchive::assignPreviewImage(const OctScanSection& section, const OctScanImageDescript& image)
{
	if (auto result = getProtocolResult(); result) {
		auto flag = result->getPreviewOutput().addSectionImage(section, image);
		return flag;
	}
	return false;
}

bool wso_system::OctArchive::assignMeasureImage(const OctScanSection& section, const OctScanImageDescript& image, int overlap_index)
{
	if (auto result = getProtocolResult(); result) {
		if (auto sect = result->getPatternOutput().getSection(section.getIndex()); sect) {
			if (!overlap_index) {
				sect->clearAllImages();
			}
		}
		auto flag = result->getPatternOutput().addSectionImage(section, image);
		return flag;
	}
	return false;
}

bool wso_system::OctArchive::assignEnfaceImage(const OctEnfaceImageDescript& image)
{
	if (auto result = getProtocolResult(); result) {
		auto enhance = OctScanOptions::getInstance()->isEnfaceImageEnhanceEnabled();
		auto flag = result->getEnfaceOutput().setEnfaceImage(image, enhance);
		return flag;
	}
	return false;
}

bool wso_system::OctArchive::assignCorneaImage(const OctCorneaImageDescript& image)
{
	if (auto result = getProtocolResult(); result) {
		auto flag = result->setCorneaImage(image);
		return flag;
	}
	return false;
}

bool wso_system::OctArchive::assignRetinaImage(const OctRetinaImageDescript& image)
{
	if (auto result = getProtocolResult(); result) {
		auto flag = result->setRetinaImage(image);
		return flag;
	}
	return false;
}

int wso_system::OctArchive::getMeasureSectionCount(void) const
{
	if (auto result = getProtocolResult(); result) {
		return result->getPatternOutput().getSectionCount();
	}
	return 0;
}

int wso_system::OctArchive::getPreviewSectionCount(void) const
{
	if (auto result = getProtocolResult(); result) {
		return result->getPreviewOutput().getSectionCount();
	}
	return 0;
}

bool wso_system::OctArchive::getMeasureSectionImage(int sect_index, int image_index, OctScanImageDescript& image) const
{
	if (auto result = getProtocolResult(); result) {
		if (auto p = result->getPatternOutput().getSectionImageDescript(sect_index, image_index); p) {
			image = *p;
			return true;
		}
	}
	return false;
}

bool wso_system::OctArchive::getPreviewSectionImage(int sect_index, int image_index, OctScanImageDescript& image) const
{
	if (auto result = getProtocolResult(); result) {
		if (auto p = result->getPreviewOutput().getSectionImageDescript(sect_index, image_index); p) {
			image = *p;
			return true;
		}
	}
	return false;
}

bool wso_system::OctArchive::getMeasureEnfaceImage(OctEnfaceImageDescript& image) const
{
	if (auto result = getProtocolResult(); result) {
		if (auto p = result->getEnfaceOutput().getEnfaceImageDescript(); p) {
			image = *p;
			return true;
		}
	}
	return false;
}

bool wso_system::OctArchive::getCorneaCameraImage(OctCorneaImageDescript& image) const
{
	if (auto result = getProtocolResult(); result) {
		if (auto p = result->getCorneaImageDescript(); p) {
			image = *p;
			return true;
		}
	}
	return false;
}

bool wso_system::OctArchive::isProtocolResultValid(void) const
{
	if (getMeasureSectionCount() > 0 && getPreviewSectionCount() > 0) {
		return true;
	}
	return false;
}

ProtocolResult* wso_system::OctArchive::getProtocolResult(void) const
{
	if (!impl().results.empty()) {
		return impl().results.back().get();
	}
	return nullptr;
}

unique_ptr<oct_result::ProtocolResult> wso_system::OctArchive::fetchProtocolResultLast(void)
{
	if (!impl().results.empty()) {
		auto result = std::move(impl().results.back());
		impl().results.pop_back();
		return result;
	}
	return unique_ptr<oct_result::ProtocolResult>();
}

void wso_system::OctArchive::onOctEnfaceImageProcessed(unsigned char* bits, int width, int height)
{
	LogD() << "Oct enface image processed" << ", w: " << width << ", h: " << height;
}

void wso_system::OctArchive::onOctPreviewImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index)
{
	bool repeated = true;
	if (auto p = OctScanStatus::getInstance(); p) {
		if (p->isCapturingToMeasure()) {
			repeated = p->isPreviewAveragingEnabled();
			LogD() << "Oct preview image processed, index: " << image_index << ", w: " << width << ", h: " << height;

			if (auto* plan = impl().pattern; plan) {
				auto& preview = plan->getPreviewScan();
				if (auto line = preview.getLineTraceFromImageIndex(image_index, repeated, true); line) {
					if (auto line_index = preview.getLineIndexFromImageIndex(image_index, repeated, true); line_index >= 0) {
						if (repeated) {
							OctRoute route = line->getRouteOfScan();
							OctScanSection section(line_index, route);
							OctScanImageDescript image(bits, width, height, qindex, sn_ratio, ref_point);

							if (assignPreviewImage(section, image)) {
								LogD() << "Preview image assigned, line_idx: " << line_index << ", image_idx: " << image_index;
							}
						}
					}
					else {
						LogD() << "Failed to get line index from image index: " << image_index;
					}
				}
				else {
					LogD() << "Failed to get line trace from image index: " << image_index;
				}
			}
		}
	}

	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->runOctPreviewImageCaptured(bits, width, height, qindex, sn_ratio, ref_point, image_index);
	}
}

void wso_system::OctArchive::onOctMeasureImageProcessed(unsigned char* bits, int width, int height, float qindex, float sn_ratio, int ref_point, int image_index, unsigned char* lateral)
{
	if (auto p = OctScanStatus::getInstance(); p) {
		if (!p->isCapturingToMeasure()) {
			return;
		}
	}

	LogD() << "Oct measure image processed, index: " << image_index << ", w: " << width << ", h: " << height;

	if (auto* plan = impl().pattern; plan) {
		auto& measure = plan->getMeasureScan();
		if (auto line = measure.getLineTraceFromImageIndex(image_index, true); line) {
			if (auto line_index = measure.getLineIndexFromImageIndex(image_index, true); line_index >= 0) {
				auto over_index = measure.getOverlapIndexFromImageIndex(image_index, true);

				OctRoute route = line->getRouteOfScan();
				OctScanSection section(line_index, route);
				OctScanImageDescript image(bits, width, height, qindex, sn_ratio, ref_point);

				if (auto* proto = impl().protocol; proto) {
					int num_lines = measure.getNumberOfScanLines();
					int num_overs = measure.getNumberOfScanOverlaps();
					bool is_verts = measure.isVertical();
					bool is_close = (num_overs == over_index + 1);
					bool reversed = false;

					if (over_index > 0 && proto->isAngioScan()) {
						// Skip overlapping images for angio scan. 
					}
					else {
						if (assignMeasureImage(section, image, over_index)) {
							LogD() << "Measure image assigned, line_idx: " << line_index << ", image_idx: " << image_index << ", overlap_idx: " << over_index;
							if (!over_index) {
								ChainOutput::setLateralLineOfEnface(lateral, line_index, width, num_lines, is_verts, reversed);
							}
						}
					}

					if (getMeasureSectionCount() == num_lines && is_close) {
						if (proto->isCubeScan()) {
							int enface_w = (is_verts ? num_lines : width);
							int enface_h = (is_verts ? width : num_lines);

							ChainOutput::setEnfaceImageResult(enface_w, enface_h);
							auto data = ChainOutput::getEnfaceImageBuffer();
							OctEnfaceImageDescript enface(data, enface_w, enface_h);
							if (assignEnfaceImage(enface)) {
								LogD() << "Enface image assigned, w: " << enface_w << ", h: " << enface_h;
							}
						}
					}

					if (over_index == 0) {
						if (auto* p = CallbackRegistry::getInstance(); p) {
							p->runOctPreviewImageCaptured(bits, width, height, qindex, sn_ratio, ref_point, image_index);
						}
					}
				}
			}
			else {
				LogD() << "Failed to get line index from image index: " << image_index;
			}
		}
		else {
			LogD() << "Failed to get line trace from image index: " << image_index;
		}
	}
}

void wso_system::OctArchive::onOctProtocolPatternAcquired(bool result)
{
	if (auto result = getProtocolResult(); result) {
		if (auto slo = SloArchive::getInstance(); slo) {
			SloFrameImageDescript desc;
			if (slo->getPreviewImageDescript(desc)) {
				OctRetinaImageDescript desc2(desc.pData, desc.width, desc.height);
				result->setRetinaImage(desc2);
			}
		}
	}
	return;
}

void wso_system::OctArchive::onOctProtocolMeasureCompleted(bool result)
{
	if (auto result = getProtocolResult(); result) {
		result->getPreviewOutput().applyAveraging();
		LogD() << "Preview image averaging applied.";

		// Overlapping section images are averaged to the single image. (including Angio pattern). 
		if (result->getScanProtocol().isPatternOverlapped()) {
			result->getPatternOutput().applyAveraging();
			LogD() << "Pattern image averaging applied.";
		}

		if (result->getScanProtocol().isAngioScan()) {
			auto& ampls = AngioSetup::getAmplitudes();
			result->getAngioOutput().setAmplitudes(std::move(ampls));
			LogD() << "Angio amplitudes assigned.";
		}
	}
	return;
}


OctArchive::OctArchiveImpl& wso_system::OctArchive::impl(void) const
{
	return *d_ptr;
}