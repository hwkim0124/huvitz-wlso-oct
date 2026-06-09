#include "pch.h"
#include "BscanImage.h"

using namespace oct_result;


struct BscanImage::BscanImageImpl
{
	CvImage image;

	OctScanImageDescript descript;
	wstring filename;

	// bool vertReversed = true; 

	BscanImageImpl()
	{
	}
};


BscanImage::BscanImage() :
	d_ptr(make_unique<BscanImageImpl>())
{
}


oct_result::BscanImage::BscanImage(const OctScanImageDescript& desc) :
	d_ptr(make_unique<BscanImageImpl>())
{
	setData(desc);
}


oct_result::BscanImage::BscanImage(std::uint8_t * data, int width, int height, float quality, float sigRatio, int refPoint) :
	d_ptr(make_unique<BscanImageImpl>())
{
	setData(data, width, height, quality, sigRatio, refPoint);
}


oct_result::BscanImage::~BscanImage() = default;
oct_result::BscanImage::BscanImage(BscanImage && rhs) = default;
BscanImage & oct_result::BscanImage::operator=(BscanImage && rhs) = default;


oct_result::BscanImage::BscanImage(const BscanImage & rhs)
	: d_ptr(make_unique<BscanImageImpl>(*rhs.d_ptr))
{
}


BscanImage & oct_result::BscanImage::operator=(const BscanImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_result::BscanImage::setData(const std::uint8_t * data, int width, int height, float quality, float sigRatio, int refPoint)
{
	d_ptr->image.fromBitsData(data, width, height);
	// d_ptr->image.flipHorizontal();

	/*
	if (reverse) {
		d_ptr->image.flipHorizontal();
	}
	*/

	std::uint8_t* buff = d_ptr->image.getBitsData();

	d_ptr->descript.setup(buff, width, height, quality, sigRatio, refPoint);
	return;
}


void oct_result::BscanImage::setData(const OctScanImageDescript& desc)
{
	d_ptr->image.fromBitsData(desc.pData, desc.width, desc.height);

	/*
	if (desc.isReversed) {
		d_ptr->image.flipHorizontal();
	}
	*/

	d_ptr->descript = desc;
	d_ptr->descript.isReversed = false;

	getImpl().descript.pData = d_ptr->image.getBitsData();
	return;
}


bool oct_result::BscanImage::setImage(cpp_util::CvImage * image)
{
	if (image && !image->isEmpty()) {
		int width = image->getWidth();
		int height = image->getHeight();

		if (getWidth() == width && getHeight() == height) {
			d_ptr->image.fromBitsData(image->getBitsData(), width, height);
			return true;
		}
	}
	return false;
}


std::uint8_t * oct_result::BscanImage::getBuffer(void) const
{
	return d_ptr->image.getBitsData();
}


const OctScanImageDescript & oct_result::BscanImage::getDescript(void) const
{
	auto descCopy = d_ptr->descript;
	descCopy.pData = getBuffer();
	descCopy.width = getWidth();
	descCopy.height = getHeight();
	d_ptr->descript = descCopy;
	return d_ptr->descript;
}


cpp_util::CvImage & oct_result::BscanImage::getImage(void) const
{
	return d_ptr->image;
}


std::wstring oct_result::BscanImage::getFileName(bool path) const
{
	if (!path) {
		auto path = d_ptr->filename;
		auto npos = path.find_last_of('/');
		if (npos != wstring::npos) {
			return path.substr(npos + 1);
		}
		npos = path.find_last_of('\\');
		if (npos != wstring::npos) {
			return path.substr(npos + 1);
		}
	}
	return d_ptr->filename;
}


bool oct_result::BscanImage::isEmpty(void) const
{
	return d_ptr->image.isEmpty();
}


int oct_result::BscanImage::getWidth(void) const
{
	return d_ptr->image.getWidth();
}


int oct_result::BscanImage::getHeight(void) const
{
	return d_ptr->image.getHeight();
}


float oct_result::BscanImage::getQualityIndex(void) const
{
	return d_ptr->descript.quality;
}


float oct_result::BscanImage::getSignalRatio(void) const
{
	return d_ptr->descript.sigRatio;
}


int oct_result::BscanImage::getReferencePoint(void) const
{
	return d_ptr->descript.refPoint;
}


void oct_result::BscanImage::setQualityIndex(float value)
{
	d_ptr->descript.quality = value;
	return;
}


void oct_result::BscanImage::setSignalRatio(float value)
{
	d_ptr->descript.sigRatio = value;
	return;
}


void oct_result::BscanImage::setReferencePoint(int value)
{
	d_ptr->descript.refPoint = value;
	return;
}


bool oct_result::BscanImage::updateFile(void)
{
	if (!d_ptr->filename.empty()) {
		auto path = d_ptr->filename;
		if (d_ptr->image.saveFile(wtoa(path), false)) {
			return true;
		}
	}
	return false;
}


bool oct_result::BscanImage::exportFile(const std::wstring& path)
{
	if (!isEmpty() && !path.empty()) {
		// if (d_ptr->image.saveFile(wtoa(path), getImpl().vertReversed)) {
		if (d_ptr->image.saveFile(wtoa(path), false)) {
			d_ptr->filename = path;
			return true;
		}
	}
	return false;
}


bool oct_result::BscanImage::importFile(const std::wstring & path)
{
	if (d_ptr->image.fromFile(path)) {
		d_ptr->filename = path;
		return true;
	}
	return false;
}

void oct_result::BscanImage::flipVert(void)
{
	if (!isEmpty()) {
		d_ptr->image.flipVertical();
		// d_ptr->vertReversed = !d_ptr->vertReversed;
	}
	return;
}


BscanImage::BscanImageImpl & oct_result::BscanImage::getImpl(void) const
{
	return *d_ptr;
}
