#include "pch.h"
#include "BscanBundle.h"
#include "BscanSection.h"
#include "BscanImage.h"

using namespace oct_result;


struct BscanBundle::BscanBundleImpl
{
	std::map<int, unique_ptr<BscanSection>> sections;
	OctScanPattern descript;

	BscanBundleImpl()
	{
	}
};


BscanBundle::BscanBundle() :
	d_ptr(make_unique<BscanBundleImpl>())
{
}


oct_result::BscanBundle::BscanBundle(const OctScanPattern & desc) :
	d_ptr(make_unique<BscanBundleImpl>())
{
	d_ptr->descript = desc;
}


oct_result::BscanBundle::~BscanBundle() = default;
oct_result::BscanBundle::BscanBundle(BscanBundle && rhs) = default;
BscanBundle & oct_result::BscanBundle::operator=(BscanBundle && rhs) = default;

/*
oct_result::BscanBundle::BscanBundle(const BscanBundle & rhs) :
	d_ptr(make_unique<BscanBundleImpl>())
{
}


BscanBundle & oct_result::BscanBundle::operator=(const BscanBundle & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}
*/


void oct_result::BscanBundle::setDescript(const OctScanPattern & desc)
{
	getImpl().descript = desc;
	return;
}


OctScanPattern & oct_result::BscanBundle::getDescript(void) const
{
	return getImpl().descript;
}


BscanImage * oct_result::BscanBundle::getSectionImage(int sectIdx, int imageIdx)
{
	BscanSection* pSect = getSection(sectIdx);
	if (pSect != nullptr) {
		return pSect->getImage(imageIdx);
	}
	return nullptr;
}


int oct_result::BscanBundle::getSectionImageCount(int sectIdx)
{
	BscanSection* pSect = getSection(sectIdx);
	if (pSect != nullptr) {
		return pSect->getImageCount();
	}
	return 0;
}


const OctScanImageDescript* oct_result::BscanBundle::getSectionImageDescript(int sectIdx, int imageIdx)
{
	BscanImage* pImage = getSectionImage(sectIdx, imageIdx);
	if (pImage != nullptr) {
		return &pImage->getDescript();
	}
	return nullptr;
}


bool oct_result::BscanBundle::addSectionImage(const OctScanSection & section, const OctScanImageDescript& image, int sectSize)
{
	BscanSection* pSect = makeSection(section);
	if (pSect == nullptr) {
		return false;
	}

	if (pSect->getImageCount() >= sectSize) {
		pSect->removeImage(0);
	}
	return pSect->addImage(image);
}


bool oct_result::BscanBundle::addSectionImage(const OctScanSection & section, const OctScanImageDescript& image)
{
	BscanSection* pSect = makeSection(section);
	if (pSect == nullptr) {
		return false;
	}

	return pSect->addImage(image);
}


bool oct_result::BscanBundle::addSectionImage(const OctScanSection & section, const std::wstring& path)
{
	BscanSection* pSect = makeSection(section);
	if (pSect == nullptr) {
		return false;
	}

	return pSect->importImage(path, true);
}


bool oct_result::BscanBundle::setSectionImage(const OctScanSection & section, const OctScanImageDescript& image)
{
	BscanSection* pSect = makeSection(section);
	if (pSect == nullptr) {
		return false;
	}

	return pSect->setImage(image);
}


bool oct_result::BscanBundle::setSectionImage(const OctScanSection & section, const std::wstring & path)
{
	BscanSection* pSect = makeSection(section);
	if (pSect == nullptr) {
		return false;
	}

	return pSect->importImage(path, false);
}


bool oct_result::BscanBundle::addSection(const OctScanSection & sect)
{
	int index = sect.getIndex();
	if (index >= 0) {
		d_ptr->sections[index] = make_unique<BscanSection>(sect);
		return true;
	}
	return false;
}


BscanSection * oct_result::BscanBundle::getSection(int index)
{
	auto iter = d_ptr->sections.find(index);
	if (iter != end(d_ptr->sections)) {
		return iter->second.get();
	}
	return nullptr;
}


BscanSection * oct_result::BscanBundle::getSectionLast(void)
{
	auto iter = rbegin(d_ptr->sections);
	if (iter != rend(d_ptr->sections)) {
		return iter->second.get();
	}
	return nullptr;
}


BscanSection * oct_result::BscanBundle::makeSection(const OctScanSection & desc)
{
	int index = desc.getIndex();
	BscanSection* p = getSection(index);
	if (p == nullptr) {
		if (addSection(desc)) {
			p = getSection(index);
		}
	}
	return p;
}


int oct_result::BscanBundle::getSectionCount(void) const
{
	return (int)d_ptr->sections.size();
}


std::vector<int> oct_result::BscanBundle::getSectionIndexList(void) const
{
	vector<int> list;
	for (auto iter = d_ptr->sections.begin(); iter != d_ptr->sections.end(); iter++) {
		list.push_back(iter->first);
	}
	return list;
}


void oct_result::BscanBundle::clearAllSections(void)
{
	d_ptr->sections.clear();
	return;
}


BscanBundle::BscanBundleImpl & oct_result::BscanBundle::getImpl(void) const
{
	return *d_ptr;
}
