#include "pch.h"
#include "ProtocolSource.h"
#include "BscanContent.h"
#include "OctReport2.h"

#include <boost/format.hpp>


using namespace oct_report;


struct ProtocolSource::ProtocolSourceImpl
{
	int sourceId = 0;
	std::unique_ptr<ProtocolResult> result;
	std::vector<std::shared_ptr<BscanContent>> patterns;
	std::vector<std::shared_ptr<BscanContent>> previews;
	std::wstring fileDirPath;

	ProtocolSourceImpl() : result(new ProtocolResult())
	{
	}
};


ProtocolSource::ProtocolSource() :
	d_ptr(make_unique<ProtocolSourceImpl>())
{
}


oct_report::ProtocolSource::ProtocolSource(const OctScanProtocol & desc)
{
	setScanProtocol(desc);
}


oct_report::ProtocolSource::ProtocolSource(std::unique_ptr<oct_result::ProtocolResult>&& result)
{
	setupProtocolResult(std::move(result));
}


oct_report::ProtocolSource::~ProtocolSource() = default;
oct_report::ProtocolSource::ProtocolSource(ProtocolSource && rhs) = default;
ProtocolSource & oct_report::ProtocolSource::operator=(ProtocolSource && rhs) = default;


void oct_report::ProtocolSource::setupProtocolResult(std::unique_ptr<oct_result::ProtocolResult>&& result)
{
	if (result != nullptr) {
		d_ptr->result = std::move(result);
		updatePatternContentList();
		updatePreviewContentList();
	}
	return;
}

bool oct_report::ProtocolSource::importProtocolContent(const std::wstring& dir_path)
{
	if (auto result = getProtocolResult(); result) {
		LogD() << "Protocol result importing, path: " << wtoa(dir_path);

		if (!result->importScanProtocol(dir_path)) {
			return false;
		}
		if (!result->importFiles(dir_path)) {
			return false;
		}	
		getImpl().fileDirPath = dir_path;
		updatePatternContentList();
		updatePreviewContentList();
		return true;
	}
	return false;
}

bool oct_report::ProtocolSource::exportProtocolContent(const std::wstring& dir_path, std::wstring& out_path)
{
	if (auto result = getProtocolResult(); result) {
		out_path = getImpl().fileDirPath;
		if (out_path.empty()) {
			if (!result->createExportDirectory(dir_path, out_path)) {
				return false;
			}
		}
		LogD() << "Protocol result exporting, path: " << wtoa(out_path);

		if (!result->exportScanProtocol(out_path)) {
			return false;
		}
		if (!result->exportFiles(out_path)) {
			return false;
		}
		getImpl().fileDirPath = out_path;
		return true;
	}
	return false;
}

int oct_report::ProtocolSource::getSourceId(void) const
{
	return getImpl().sourceId;
}

void oct_report::ProtocolSource::setSourceId(int sourceId)
{
	getImpl().sourceId = sourceId;
	return;
}

bool oct_report::ProtocolSource::isContentEmpty(void) const
{
	if (getPatternContentCount() > 0) {
		return false;
	}
	return true;
}

BscanContent * oct_report::ProtocolSource::getPatternContent(int index) const
{
	if (index >= 0 && index < getPatternContentCount()) {
		return d_ptr->patterns[index].get();
	}
	return nullptr;
}


BscanContent * oct_report::ProtocolSource::getPatternContentOfSection(int sectIdx, int imageIdx) const
{
	// int index = sectIdx * getDescript().getScanOverlaps() + imageIdx;
	int index = sectIdx;
	return getPatternContent(index);
}


int oct_report::ProtocolSource::getPatternContentCount() const
{
	int size = (int) d_ptr->patterns.size();
	return size;
}


int oct_report::ProtocolSource::getPatternSectionCount(void) const
{
	auto& desc = getScanProtocol().getMeasure();
	int size = getPatternContentCount();
	int overlaps = desc.getScanOverlaps();
	int lines = desc.getNumberOfScanLines();

	if (size == overlaps * lines) {
		return lines;
	}
	else {
		return size;
	}
}


std::vector<segm_scan::OcularBsegm*> oct_report::ProtocolSource::getPatternBscanSegmList(void) const
{
	auto list = std::vector<segm_scan::OcularBsegm*>();

	int sections = getPatternContentCount();
	for (int i = 0; i < sections; i++) {
		auto data = getPatternContent(i);
		if (data) {
			list.push_back(data->getOcularBsegm());
		}
	}
	return list;
}


BscanContent * oct_report::ProtocolSource::getPreviewContent(int index) const
{
	if (index >= 0 && index < getPreviewContentCount()) {
		return d_ptr->previews[index].get();
	}
	return nullptr;
}


int oct_report::ProtocolSource::getPreviewContentCount(void) const
{
	return (int)d_ptr->previews.size();
}


int oct_report::ProtocolSource::updatePreviewContentList(bool vflip)
{
	auto list = getPreviewOutput()->getSectionIndexList();
	int count = 0;

	d_ptr->previews.clear();
	for (auto index : list) {
		auto image = getPreviewOutput()->getSectionImage(index);
		if (image) {
			if (vflip) {
				image->flipVert();
			}
			auto data = make_shared<BscanContent>(image);
			data->setSectionIndex(index);
			d_ptr->previews.push_back(data);
			count++;
		}
	}
	return count;
}


oct_result::ProtocolResult * oct_report::ProtocolSource::getProtocolResult(void) const
{
	return d_ptr->result.get();
}


oct_result::PreviewOutput * oct_report::ProtocolSource::getPreviewOutput(void) const
{
	return &getProtocolResult()->getPreviewOutput();;
}


oct_result::PatternOutput * oct_report::ProtocolSource::getPatternOutput(void) const
{
	return &getProtocolResult()->getPatternOutput();
}

oct_result::AngioOutput* oct_report::ProtocolSource::getAngioOutput(void) const
{
	return &getProtocolResult()->getAngioOutput();
}


OctScanProtocol& oct_report::ProtocolSource::getScanProtocol(void) const
{
	return getProtocolResult()->getScanProtocol();
}


void oct_report::ProtocolSource::setScanProtocol(const OctScanProtocol& desc)
{
	getProtocolResult()->setScanProtocol(desc);
	return;
}

OctScanPattern& oct_report::ProtocolSource::getScanPattern(void) const
{
	return getScanProtocol().getMeasure();
}


const OctScanImageDescript * oct_report::ProtocolSource::getPreviewImageDescript(int sectIdx, int imageIdx)
{
	if (getProtocolResult()) {
		return getProtocolResult()->getPreviewOutput().getSectionImageDescript(sectIdx, imageIdx);
	}
	return nullptr;
}


const OctScanImageDescript * oct_report::ProtocolSource::getPatternImageDescript(int sectIdx, int imageIdx)
{
	if (getProtocolResult()) {
		return getProtocolResult()->getPatternOutput().getSectionImageDescript(sectIdx, imageIdx);
	}
	return nullptr;
}


bool oct_report::ProtocolSource::getRetinaBsegmDescriptor(int sectIdx, int imageIdx, OctRetinaBsegmDescriptor& desc)
{
	if (auto p = getPatternContentOfSection(sectIdx, imageIdx); p) {
		auto& pattern = getScanProtocol().getMeasure();
		return p->fetchRetinaBsegmResult(pattern, desc);
	}
	return false;
}

bool oct_report::ProtocolSource::getCorneaBsegmDescriptor(int sectIdx, int imageIdx, OctCorneaBsegmDescriptor& desc)
{
	if (auto p = getPatternContentOfSection(sectIdx, imageIdx); p) {
		auto& pattern = getScanProtocol().getMeasure();
		return p->fetchCorneaBsegmResult(pattern, desc);
	}
	return false;
}


bool oct_report::ProtocolSource::getRetinaBsegmTraits(int sectIdx, int imageIdx, OctRetinaBsegmTraits& traits)
{
	if (auto p = getPatternContentOfSection(sectIdx, imageIdx); p) {
		auto& pattern = getScanProtocol().getMeasure();
		return p->fetchRetinaBsegmTraits(pattern, traits);
	}
	return false;
}

bool oct_report::ProtocolSource::getCorneaBsegmTraits(int sectIdx, int imageIdx, OctCorneaBsegmTraits& traits)
{
	if (auto p = getPatternContentOfSection(sectIdx, imageIdx); p) {
		auto& pattern = getScanProtocol().getMeasure();
		return p->fetchCorneaBsegmTraits(pattern, traits);
	}
	return false;
}

bool oct_report::ProtocolSource::getBsegmLayerPoints(int sectIdx, int imageIdx, OcularLayerType type, OctBsegmLayerPoints& layer)
{
	if (auto p = getPatternContentOfSection(sectIdx, imageIdx); p) {
		auto& pattern = getScanProtocol().getMeasure();
		return p->fetchBsegmLayerPoints(type, pattern, layer);
	}
	return false;
}

std::vector<int> oct_report::ProtocolSource::getLayerPoints(int sectIdx, int imageIdx, OcularLayerType layer) const
{
	if (auto p = getPatternContentOfSection(sectIdx, imageIdx); p) {
		return p->getLayerPoints(layer);
	}
	return std::vector<int>();
}

bool oct_report::ProtocolSource::getRetinaLayerMapArrays(LayerMapArrays& layers) const
{
	int count = getPatternContentCount();
	layers.clear();

	for (int i = 0; i < count; i++) {
		if (auto p = getPatternContent(i); p) {
			for (int j = 0; j < NUMBER_OF_RETINA_LAYERS; j++) {
				auto k = static_cast<int>(OcularLayerType::ILM) + j;
				auto t = static_cast<OcularLayerType>(k);

				auto points = p->getLayerPoints(t);
				layers[t].push_back(points);
			}
		}
	}
	return true;
}


const OctRetinaImageDescript * oct_report::ProtocolSource::getRetinaImageDescript(void) const
{
	if (getProtocolResult()) {
		return getProtocolResult()->getRetinaImageDescript();
	}
	return nullptr;
}


const OctCorneaImageDescript * oct_report::ProtocolSource::getCorneaImageDescript(void) const
{
	if (getProtocolResult()) {
		return getProtocolResult()->getCorneaImageDescript();
	}
	return nullptr;
}


int oct_report::ProtocolSource::importPatternImages(const std::wstring & dirPath, int numImages)
{
	int count = getPatternOutput()->importImages(numImages, dirPath);
	updatePatternContentList();
	return count;
}


int oct_report::ProtocolSource::importPatternImages(const std::vector<std::wstring>& fileList)
{
	int count = getPatternOutput()->importImages(fileList);
	updatePatternContentList();
	return count;
}


int oct_report::ProtocolSource::importPreviewImages(const std::wstring & dirPath, int numImages)
{
	int count = getPreviewOutput()->importImages(numImages, dirPath);
	updatePreviewContentList();
	return count;
}


int oct_report::ProtocolSource::updatePatternImages(void)
{
	int count = getPatternOutput()->updateImages();
	return count;
}


int oct_report::ProtocolSource::updatePreviewImages(void)
{
	int count = getPreviewOutput()->updateImages();
	return count;
}


bool oct_report::ProtocolSource::exportBsegmResults(const std::wstring dirPath)
{
	wstring path, name;

	auto& desc = getScanProtocol().getMeasure();
	int lines = desc.getNumberOfScanLines();
	int overlaps = desc.getScanOverlaps();
	bool isAngio = desc.isAngioPattern();
	int count = (int)getPatternContentCount();

	for (int i = 0; i < count; i++) {
		if (false) { // isAngio || (count == lines * overlaps)) {
			int major = i / overlaps;
			int minor = i % overlaps;

			auto bscan = getPatternContentOfSection(major, minor);
			if (bscan) {
				name = (boost::wformat(L"%03d_%02d") % major % minor).str();
				path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
				if (!bscan->exportBsegmResult(path, desc)) {
					return false;
				}
			}
		}
		else {
			auto bscan = getPatternContent(i);
			if (bscan) {
				int index = bscan->getSectionIndex();
				name = (boost::wformat(L"%03d") % index).str();
				path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
				if (!bscan->exportBsegmResult(path, desc)) {
					return false;
				}
			}
		}


		/*
		int major = i / overlaps;
		int minor = i % overlaps;

		auto bscan = getBscanData(major, minor);
		if (bscan) {
			*//*
			name = bscan->getImageName();
			if (name.empty()) {
				if (overlaps > 1) {
					name = (boost::wformat(L"%03d_%02d") % (i / overlaps) % (i % overlaps)).str();
				}
				else {
					name = (boost::wformat(L"%03d") % i).str();
				}
			}
			else {
				name = name.substr(0, std::max<int>((int)(name.size() - 4), 0));
			}
			*/
			/*
			if (angio) {
				if (minor > 0) {
					name = (boost::wformat(L"%03d_%02d") % major % minor).str();
				}
				else {
					name = (boost::wformat(L"%03d") % major).str();
				}
			}
			else {
				name = (boost::wformat(L"%03d") % major).str();
			}

			path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
			if (!bscan->exportBsegmResult(path, getDescript())) {
				return false;
			}
		}
		*/
	}

	for (int i = 0; i < getPreviewContentCount(); i++) {
		auto bscan = getPreviewContent(i);
		if (bscan) {
			name = L"preview";
			if (i > 0) {
				name += (boost::wformat(L"_%1d") % i).str();
			}

			path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
			if (!bscan->exportBsegmResult(path, desc)) {
				return false;
			}
		}
	}
	return true;
}


bool oct_report::ProtocolSource::importBsegmResults(const std::wstring dirPath)
{
	wstring path, name;

	auto& desc = getScanProtocol().getMeasure();
	int lines = desc.getNumberOfScanLines();
	int overlaps = desc.getScanOverlaps();
	bool isAngio = desc.isAngioPattern();
	int count = (int)getPatternContentCount();

	for (int i = 0; i < count; i++) {
		if (false) { // isAngio || (count == lines * overlaps)) {
			int major = i / overlaps;
			int minor = i % overlaps;

			auto bscan = getPatternContentOfSection(major, minor);
			if (bscan) {
				name = (boost::wformat(L"%03d_%02d") % major % minor).str();
				path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
				if (!bscan->importBsegmResult(path, desc)) {
					return false;
				}
			}
		}
		else {
			auto bscan = getPatternContent(i);
			if (bscan) {
				name = (boost::wformat(L"%03d") % i).str();
				path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
				if (!bscan->importBsegmResult(path, desc)) {
					return false;
				}
			}
		}

		/*
		int major = i / overlaps;
		int minor = i % overlaps;

		auto bscan = getBscanData(major, minor);
		if (bscan) {
			if (angio) {
				if (minor > 0) {
					name = (boost::wformat(L"%03d_%02d") % major % minor).str();
				}
				else {
					name = (boost::wformat(L"%03d") % major).str();
				}
			}
			else {
				name = (boost::wformat(L"%03d") % major).str();
			}

			path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
			if (!bscan->importBsegmResult(path, getDescript())) {
				return false;
			}
		}
		*/
	}

	for (int i = 0; i < getPreviewContentCount(); i++) {
		auto bscan = getPreviewContent(i);
		if (bscan) {
			name = L"preview";
			if (i > 0) {
				name += (boost::wformat(L"_%1d") % i).str();
			}

			path = (boost::wformat(L"%s//%s.json") % dirPath % name).str();
			if (!bscan->importBsegmResult(path, desc)) {
				return false;
			}
		}
	}
	return true;
}


void oct_report::ProtocolSource::clear(void)
{
	d_ptr->result.reset(new ProtocolResult());
	d_ptr->patterns.clear();
	return;
}


int oct_report::ProtocolSource::updatePatternContentList(bool vflip)
{
	auto list = getPatternOutput()->getSectionIndexList();
	int count = 0;

	d_ptr->patterns.clear();
	for (auto index : list) {
		for (int i = 0; i < getPatternOutput()->getSectionImageCount(index); i++) {
			auto image = getPatternOutput()->getSectionImage(index, i);
			if (image) {
				if (vflip) {
					image->flipVert();
				}
				auto data = make_shared<BscanContent>(image);
				data->setSectionIndex(index);
				data->setOverlapIndex(i);
				d_ptr->patterns.push_back(data);
				count++;
			}
		}
	}
	return count;
}



ProtocolSource::ProtocolSourceImpl & oct_report::ProtocolSource::getImpl(void) const
{
	return *d_ptr;
}
