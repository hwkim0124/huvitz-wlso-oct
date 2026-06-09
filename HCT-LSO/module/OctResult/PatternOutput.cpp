#include "pch.h"
#include "PatternOutput.h"
#include "BscanSection.h"
#include "BscanImage.h"
#include "EnfaceImage.h"

#include <boost/format.hpp>
#include <thread>
#include <vector>
#include <algorithm>

using namespace oct_result;



struct PatternOutput::PatternOutputImpl
{
	OctScanPattern descript;
	unique_ptr<EnfaceImage> enface;

	PatternOutputImpl() : enface(make_unique<EnfaceImage>())
	{
	}
};


PatternOutput::PatternOutput() :
	d_ptr(make_unique<PatternOutputImpl>())
{
}


oct_result::PatternOutput::~PatternOutput() = default;
oct_result::PatternOutput::PatternOutput(PatternOutput && rhs) = default;
PatternOutput & oct_result::PatternOutput::operator=(PatternOutput && rhs) = default;

/*
oct_result::PatternOutput::PatternOutput(const PatternOutput & rhs)
	: d_ptr(make_unique<PatternOutputImpl>(*rhs.d_ptr))
{
}


PatternOutput & oct_result::PatternOutput::operator=(const PatternOutput & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}
*/


int oct_result::PatternOutput::updateImages(void)
{
	auto list = getSectionIndexList();
	int count = 0;

	for (auto index : list) {
		if (updateImage(index)) {
			count++;
		}
	}
	LogD() << "Pattern images updated, count: " << count;
	return count;
}


int oct_result::PatternOutput::exportImages(const std::wstring dirPath, const std::wstring & prefix)
{
	if (!SystemUtil::isDirectory(dirPath)) {
		LogE() << "Export pattern directory not found!, path: " << wtoa(dirPath);
		return false;
	}

	auto list = getSectionIndexList();
	int count = 0;
	for (auto index : list) {
		if (exportImage(index, dirPath, prefix)) {
			count++;
		}
	}
	LogD() << "Pattern images exported, count: " << count;
	return count;
}


int oct_result::PatternOutput::importImages(int numImages, const std::wstring dirPath, const std::wstring & prefix)
{
	if (!SystemUtil::isDirectory(dirPath)) {
		LogE() << "Import pattern directory not found!, path: " << wtoa(dirPath);
		return false;
	}

	clearAllSections();

	int count = 0;
	int total = (numImages <= 0 ? getDescript().getNumberOfScanLines() : numImages);
	for (int index = 0; index < total; index++) {
		if (importImage(index, dirPath, prefix)) {
			count++;
		}
	}
	LogD() << "Pattern images imported, count: " << count;
	return count;
}


int oct_result::PatternOutput::importImages(const std::vector<std::wstring>& fileList)
{
	clearAllSections();

	int count = 0;
	for (int index = 0; index < fileList.size(); index++) {
		if (importImage(index, fileList[index])) {
			count++;
		}
	}
	LogD() << "Pattern images imported, count: " << count;
	return count;
}


bool oct_result::PatternOutput::updateImage(int sectIdx)
{
	auto pSect = getSection(sectIdx);
	bool result = false;
	if (pSect) {
		result = pSect->updateImages();
	}
	return result;
}


bool oct_result::PatternOutput::exportImage(int sectIdx, const std::wstring & dirPath, const std::wstring & prefix)
{
	auto pSect = getSection(sectIdx);
	bool result = false;
	if (pSect) {
		result = pSect->exportImages(dirPath, prefix, false);
	}
	return result;
}


bool oct_result::PatternOutput::importImage(int sectIdx, const std::wstring& dirPath, const std::wstring & prefix)
{
	auto desc = getDescript().createScanSection(sectIdx);
	auto pSect = makeSection(desc);
	bool result = false;
	if (pSect) {
		result = pSect->importImages(dirPath, prefix);
	}
	// LogD() << "Section: " << desc.text();
	return result;
}


bool oct_result::PatternOutput::importImage(int sectIdx, const std::wstring & filePath)
{
	auto desc = getDescript().createScanSection(sectIdx);
	auto pSect = makeSection(desc);
	bool result = false;
	if (pSect) {
		result = pSect->importImage(filePath);
	}
	// LogD() << "Section: " << desc.text();
	return result;
}


float oct_result::PatternOutput::getAverageOfQualityIndex(void)
{
	float qsum = 0.0f;
	int count = 0;

	for (int i = 0; i < getSectionCount(); i++) {
		auto p = getSection(i)->getImage();
		if (p != nullptr) {
			qsum += p->getQualityIndex();
			count += 1;
		}
	}

	float mean = (count > 0 ? (qsum / count) : 0.0f);
	return mean;
}


bool oct_result::PatternOutput::applyAveraging(void)
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


PatternOutput::PatternOutputImpl & oct_result::PatternOutput::getImpl(void) const
{
	return *d_ptr;
}
