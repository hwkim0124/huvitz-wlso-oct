#include "pch.h"
#include "PreviewOutput.h"
#include "BscanBundle.h"
#include "BscanSection.h"

#include <boost/format.hpp>
#include <thread>
#include <vector>
#include <algorithm>

using namespace oct_result;


struct PreviewOutput::PreviewOutputImpl
{
	PreviewOutputImpl()
	{
	}
};


PreviewOutput::PreviewOutput() :
	d_ptr(make_unique<PreviewOutputImpl>())
{
}


oct_result::PreviewOutput::~PreviewOutput() = default;
oct_result::PreviewOutput::PreviewOutput(PreviewOutput && rhs) = default;
PreviewOutput & oct_result::PreviewOutput::operator=(PreviewOutput && rhs) = default;


int oct_result::PreviewOutput::updateImages(void)
{
	auto list = getSectionIndexList();
	int count = 0;

	for (auto index : list) {
		if (updateImage(index)) {
			count++;
		}
	}
	LogD() << "Preview images updated, count: " << count;
	return count;
}


int oct_result::PreviewOutput::exportImages(const std::wstring & dirName, const std::wstring & prefix)
{
	if (!SystemUtil::isDirectory(dirName)) {
		LogE() << "Export preview directory not found!, path: " << wtoa(dirName);
		return false;
	}

	auto list = getSectionIndexList();
	int count = 0;
	for (auto index : list) {
		if (exportImage(index, dirName, prefix)) {
			count++;
		}
		// break;
	}
	LogD() << "Preview images exported, count: " << count;
	return count;
}


int oct_result::PreviewOutput::importImages(int numImages, const std::wstring & dirName, const std::wstring & prefix)
{
	if (!SystemUtil::isDirectory(dirName)) {
		LogE() << "Import preview directory not found!, path: " << wtoa(dirName);
		return false;
	}

	clearAllSections();

	int count = 0;

	/*
	int total = (numImages <= 0 ? (getDescript().isPreviewCross() ? 2 : 1) : numImages);
	// for (int index = 0; index < getDescript().getNumberOfScanLines(); index++) {
	for (int index = 0; index < total; index++) {
		if (importImage(index, dirName, prefix, fileExt)) {
			count++;
		}
	}
	*/

	if (importImage(0, dirName, prefix)) {
		count++;
	}
	if (importImage(1, dirName, prefix)) {
		count++;
	}

	if (count >= 2) {
		getDescript().previewType = OctPatternType::CROSS;
	}

	LogD() << "Preview images imported, count: " << count;
	return count;
}


bool oct_result::PreviewOutput::updateImage(int sectIdx)
{
	auto pSect = getSection(sectIdx);
	bool result = false;
	if (pSect) {
		result = pSect->updateImage();
	}
	return result;
}


bool oct_result::PreviewOutput::exportImage(int sectIdx, const std::wstring & dirPath, const std::wstring & prefix)
{
	auto pSect = getSection(sectIdx);
	bool result = false;
	if (pSect) {
		// result = pSect->exportImages(dirPath, prefix, fileExt, true);
		wstring path;

		/*
		if (sectIdx == 0) {
			if (getDescript().isPreviewVertLine()) {
				path = (boost::wformat(L"%s//%s_v.%s") % dirPath % prefix % fileExt).str();
			}
			else {
				path = (boost::wformat(L"%s//%s_h.%s") % dirPath % prefix % fileExt).str();
			}
		}
		else {
			path = (boost::wformat(L"%s//%s_v.%s") % dirPath % prefix % fileExt).str();
		}
		*/

		if (sectIdx > 0) {
			path = (boost::wformat(L"%s//%s_%1d.%s") % dirPath % prefix % sectIdx % BSCAN_IMAGE_FORMAT).str();
		}
		else {
			path = (boost::wformat(L"%s//%s.%s") % dirPath % prefix % BSCAN_IMAGE_FORMAT).str();
		}

		result = pSect->exportImage(path);
	}
	return result;
}


bool oct_result::PreviewOutput::importImage(int sectIdx, const std::wstring dirPath, const std::wstring & prefix)
{
	bool ret;

	ret = importImageWithExt(sectIdx, dirPath, prefix, BSCAN_IMAGE_FORMAT);
	if (!ret) {
		ret = importImageWithExt(sectIdx, dirPath, prefix, BSCAN_IMAGE_FORMAT_OLD);
	}

	return ret;
}

bool oct_result::PreviewOutput::importImageWithExt(int sectIdx, const std::wstring dirPath, const std::wstring& prefix,
	const std::wstring& imageExt)
{
	auto desc = getDescript().createScanSection(sectIdx);
	auto pSect = makeSection(desc);
	bool result = false;
	if (pSect) {
		// result = pSect->importImages(dirPath, prefix, fileExt);
		wstring path;

		if (sectIdx > 0) {
			path = (boost::wformat(L"%s//%s_%1d.%s") % dirPath % prefix % sectIdx % imageExt).str();
		}
		else {
			path = (boost::wformat(L"%s//%s.%s") % dirPath % prefix % imageExt).str();
		}

		result = pSect->importImage(path);

		/*
		if (sectIdx == 0) {
		path = (boost::wformat(L"%s//%s.%s") % dirPath % prefix % fileExt).str();
		result = pSect->importImage(path);

		if (result) {
		getDescript().setPreviewType(PatternType::HorzLine);
		}
		else {
		path = (boost::wformat(L"%s//%s_h.%s") % dirPath % prefix % fileExt).str();
		result = pSect->importImage(path);
		if (result) {
		getDescript().setPreviewType(PatternType::HorzLine);
		}
		else {
		path = (boost::wformat(L"%s//%s_v.%s") % dirPath % prefix % fileExt).str();
		result = pSect->importImage(path);
		if (result) {
		getDescript().setPreviewType(PatternType::VertLine);
		}
		}
		}
		}
		else {
		path = (boost::wformat(L"%s//%s_v.%s") % dirPath % prefix % fileExt).str();
		result = pSect->importImage(path);
		if (result) {
		getDescript().setPreviewType(PatternType::Cross);
		}
		}
		*/
	}
	return result;
}

bool oct_result::PreviewOutput::applyAveraging(void)
{
	auto n_workers = std::thread::hardware_concurrency();

	std::vector<std::vector<BscanSection*>> tasks(n_workers);
	auto list = getSectionIndexList();

	for (int i = 0; i < list.size(); i++) {
		int index = list[i];
		BscanSection* p = getSection(index);
		if (p != nullptr && p->getImageCount() > 1) {
			int k = i % n_workers;
			tasks[k].push_back(p);
		}
	}

	std::vector<std::thread> workers;

	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k]() {
				for (auto p : tasks[k]) {
					bool res = p->applyAveraging();
					if (res) {
						LogD() << "Image averaging success, index: " << p->getDescript().getIndex();
					}
					else {
						LogD() << "Image averaging failed, index: " << p->getDescript().getIndex();
					}
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t)
	{
		t.join();
	});
	return true;
}


PreviewOutput::PreviewOutputImpl & oct_result::PreviewOutput::getImpl(void) const
{
	return *d_ptr;
}
