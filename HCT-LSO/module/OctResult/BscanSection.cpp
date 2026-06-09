#include "pch.h"
#include "BscanSection.h"
#include "BscanImage.h"

#include <boost/format.hpp>
#include <format>

using namespace oct_result;


struct BscanSection::BscanSectionImpl
{
	vector<unique_ptr<BscanImage>> images;
	OctScanSection descript;

	BscanSectionImpl()
	{
	}
};


BscanSection::BscanSection() :
	d_ptr(make_unique<BscanSectionImpl>())
{
}

/*
oct_result::BscanSection::BscanSection(OctRoute route) :
	d_ptr(make_unique<BscanSectionImpl>())
{
	d_ptr->descript._route = route;
}
*/


oct_result::BscanSection::BscanSection(const OctScanSection& desc) :
	d_ptr(make_unique<BscanSectionImpl>())
{
	d_ptr->descript = desc;
}


oct_result::BscanSection::~BscanSection() = default;
oct_result::BscanSection::BscanSection(BscanSection && rhs) = default;
BscanSection & oct_result::BscanSection::operator=(BscanSection && rhs) = default;


bool oct_result::BscanSection::setImage(const OctScanImageDescript& desc)
{
	if (getImageCount() > 0) {
		clearAllImages();
	}
	return addImage(desc);
}


bool oct_result::BscanSection::addImage(const OctScanImageDescript& desc)
{
	if (!desc.pData) {
		return false;
	}

	d_ptr->images.push_back(make_unique<BscanImage>(desc));
	return true;
}


bool oct_result::BscanSection::removeImage(int index)
{
	if (index < 0 || index >= getImageCount()) {
		return false;
	}
	d_ptr->images.erase(d_ptr->images.begin() + index);
	return true;
}


BscanImage * oct_result::BscanSection::getImage(int index)
{
	if (index < 0 || index >= getImageCount()) {
		return nullptr;
	}

	BscanImage* p = d_ptr->images[index].get();
	return p;
}


int oct_result::BscanSection::getImageCount(void) const
{
	return (int)d_ptr->images.size();
}


void oct_result::BscanSection::clearAllImages(void)
{
	d_ptr->images.clear();
	return;
}


void oct_result::BscanSection::setDescript(const OctScanSection & desc)
{
	d_ptr->descript = desc;
	return;
}


const OctScanSection & oct_result::BscanSection::getDescript(void) const
{
	return d_ptr->descript;
}


void oct_result::BscanSection::setRouteOfScan(OctRoute route)
{
	d_ptr->descript._route = route;
	return;
}


const OctRoute& oct_result::BscanSection::getRouteOfScan(void) const
{
	return d_ptr->descript._route;
}


const OctPoint & oct_result::BscanSection::getStartPoint(void) const
{
	return d_ptr->descript._route.start();
}


const OctPoint & oct_result::BscanSection::getEndPoint(void) const
{
	return d_ptr->descript._route.close();
}


bool oct_result::BscanSection::isCircle(void)
{
	return d_ptr->descript._route.isCircle();
}


bool oct_result::BscanSection::updateImages(void)
{
	int size = getImageCount();

	for (int i = 0; i < size; i++) {
		if (!updateImage(i)) {
			return false;
		}
	}
	return true;
}


bool oct_result::BscanSection::exportImages(const std::wstring& dirPath, const std::wstring& prefix, bool single)
{
	int size = getImageCount();

	wstring path;
	wstring name;
	for (int i = 0; i < size; i++) {
		if (!prefix.empty()) {
			name = std::format(L"{}{:03d}", prefix, getDescript().getIndex());
		}
		else {
			name = std::format(L"{:03d}", getDescript().getIndex());
		}

		if (size == 1 || single) {
			path = std::format(L"{}\\{}.{}", dirPath, name, BSCAN_IMAGE_FORMAT);
		}
		else {
			path = std::format(L"{}\\{}_{:02d}.{}", dirPath, name, i, BSCAN_IMAGE_FORMAT);
		}

		if (!exportImage(path, i)) {
			return false;
		}
		if (single) {
			break;
		}
	}
	return true;
}


bool oct_result::BscanSection::importImages(const std::wstring & dirPath, const std::wstring & prefix, int size)
{
	wstring path;
	wstring name;
	bool ret;

	clearAllImages();
	for (int i = 0; i < size; i++) {
		// get name
		if (!prefix.empty()) {
			name = (boost::wformat(L"%s%03d") % prefix % getDescript().getIndex()).str();
		}
		else {
			name = (boost::wformat(L"%03d") % getDescript().getIndex()).str();
		}

		// import with kImageExt
		ret = importImageWithExt(dirPath, name, BSCAN_IMAGE_FORMAT, size, i);
		if (!ret) {
			ret = importImageWithExt(dirPath, name, BSCAN_IMAGE_FORMAT_OLD, size, i);
		}
	}
	return true;
}


bool oct_result::BscanSection::updateImage(int imageIdx)
{
	BscanImage* image = getImage(imageIdx);
	bool result = false;
	if (image != nullptr) {
		result = image->updateFile();
	}
	return result;
}


bool oct_result::BscanSection::exportImage(const std::wstring & path, int imageIdx)
{
	BscanImage* image = getImage(imageIdx);
	bool result = false;
	if (image != nullptr) {
		result = image->exportFile(path);
		if (!result) {
			LogD() << "Failed exporting section image, path: " << wtoa(path) << " => " << result;
		}
		else {
			LogD() << "Section image saved, path: " << wtoa(path);
		}
	}
	else {
		LogD() << "Section image empty!, path: " << wtoa(path);
	}
	return result;
}


bool oct_result::BscanSection::importImage(const std::wstring& path, bool append)
{
	if (!append) {
		clearAllImages();
	}

	auto image = make_unique<BscanImage>();
	bool result = false;
	if (image->importFile(path)) {
		d_ptr->images.push_back(std::move(image));
		result = true;
	}
	else {
		LogD() << "Failed importing section image, path: " << wtoa(path);
	}
	return result;
}

bool oct_result::BscanSection::importImageWithExt(const std::wstring& dirPath,
	const std::wstring& name, const std::wstring& imageExt, int size, int idx)
{
	wstring path;

	if (size == 1) {
		path = (boost::wformat(L"%s//%s.%s") % dirPath % name % imageExt).str();
	}
	else {
		path = (boost::wformat(L"%s//%s_%02d.%s") % dirPath % name % idx % imageExt).str();
	}

	if (!importImage(path, true)) {
		return false;
	}
	return true;
}

bool oct_result::BscanSection::applyAveraging(bool forward)
{
	int size = (int) getImpl().images.size();
	if (size <= 1) {
		return true;
	}

	int baseIdx;
	BscanImage* p; // = getImage(baseIdx);
	CvProcess cvProc;

	if (forward) {
		baseIdx = 0;
	}
	else {
		baseIdx = 0;
		float qualityMax = 0.0f;
		float qualityStd = 4.0f;
		for (int i = (size - 1); i >= 0; i--) {
			if ((p = getImage(i)) != nullptr) {
				if (p->getSignalRatio() > qualityMax) {
					qualityMax = p->getSignalRatio();
					baseIdx = i;
				}
				if (qualityMax >= qualityStd) {
					break;
				}
			}
		}
		if (baseIdx == 0) {
			baseIdx = size - 1;
		}
	}

	p = getImage(baseIdx);
	if (p == nullptr || !cvProc.startAveraging(p->getImage())) {
		return false;
	}

	if (forward) {
		for (int i = baseIdx + 1; i < size; i++) {
			if ((p = getImage(i)) != nullptr) {
				cvProc.insertAveraging(p->getImage());
				// DebugOut2() << "Averaging image, idx: " << i;
			}
		}
	}
	else {
		int count = 1;
		for (int i = baseIdx - 1; i >= 0; i--) {
			if (i != baseIdx && (p = getImage(i)) != nullptr) {
				if (cvProc.insertAveraging(p->getImage())) {
					// DebugOut2() << "Averaging image, idx: " << i;
					if (++count >= PATTERN_PREVIEW_AVERAGE_SIZE) {
						break;
					}
				}
			}
		}
	}

	CvImage output;
	bool result = false; 
	int count = cvProc.closeAveraging(output);
	if (count > 0) {
		p = getImage(0);
		OctScanImageDescript desc = p->getDescript();
		desc.pData = output.getBitsData();
		p->setData(desc);
		result = true;
		LogD() << "Averaging images, result: " << count << " / " << size;
	}

	getImpl().images.erase(getImpl().images.cbegin() + 1, getImpl().images.cend());
	return true;
}


BscanSection::BscanSectionImpl & oct_result::BscanSection::getImpl(void) const
{
	return *d_ptr;
}
