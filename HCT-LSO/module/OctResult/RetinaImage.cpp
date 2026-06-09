#include "pch.h"
#include "RetinaImage.h"


#include <boost/format.hpp>

using namespace oct_result;


struct RetinaImage::RetinaImageImpl
{
	OctRetinaImageDescript descript;
	CvImage image;

	RetinaImageImpl()
	{
	}
};


RetinaImage::RetinaImage() :
	d_ptr(make_unique<RetinaImageImpl>())
{
}


oct_result::RetinaImage::RetinaImage(const OctRetinaImageDescript& desc) :
	d_ptr(make_unique<RetinaImageImpl>())
{
	setData(desc);
}

oct_result::RetinaImage::~RetinaImage() = default;
oct_result::RetinaImage::RetinaImage(RetinaImage&& rhs) = default;
RetinaImage& oct_result::RetinaImage::operator=(RetinaImage&& rhs) = default;


oct_result::RetinaImage::RetinaImage(const RetinaImage& rhs)
	: d_ptr(make_unique<RetinaImageImpl>(*rhs.d_ptr))
{
}


RetinaImage& oct_result::RetinaImage::operator=(const RetinaImage& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_result::RetinaImage::setData(const OctRetinaImageDescript& desc)
{
	d_ptr->descript = desc;

	d_ptr->image.fromBitsData(desc.pData, desc.width, desc.height);
	getImpl().descript.pData = d_ptr->image.getBitsData();
	return;
}


const OctRetinaImageDescript& oct_result::RetinaImage::getDescript(void) const
{
	auto descCopy = d_ptr->descript;
	descCopy.pData = getBuffer();
	descCopy.width = getWidth();
	descCopy.height = getHeight();
	d_ptr->descript = descCopy;
	return d_ptr->descript;
}


std::uint8_t* oct_result::RetinaImage::getBuffer(void) const
{
	return d_ptr->image.getBitsData();
}


bool oct_result::RetinaImage::isEmpty(void) const
{
	return getImpl().image.isEmpty();
}


int oct_result::RetinaImage::getWidth(void) const
{
	return getImpl().image.getWidth();
}


int oct_result::RetinaImage::getHeight(void) const
{
	return getImpl().image.getHeight();
}


bool oct_result::RetinaImage::exportFile(std::wstring dirPath, std::wstring imageName)
{
	wstring path = (boost::wformat(L"%s//%s.%s") % dirPath % imageName % FRAME_IMAGE_FORMAT).str();

	bool result = false;
	if (!isEmpty()) {
		result = d_ptr->image.saveFile(wtoa(path), false);
		LogD() << "Retina image exported, path: " << wtoa(path);
	}
	else {
		LogD() << "Retina image is empty, path: " << wtoa(path);
	}
	return false;
}


bool oct_result::RetinaImage::importFile(std::wstring dirPath, std::wstring imageName)
{
	bool ret;

	ret = importFileWithExt(dirPath, imageName, FRAME_IMAGE_FORMAT);
	if (!ret) {
		ret = importFileWithExt(dirPath, imageName, FRAME_IMAGE_FORMAT_OLD);
	}

	return ret;
}

bool oct_result::RetinaImage::importFileWithExt(std::wstring dirPath, std::wstring imageName,
	std::wstring imageExt)
{
	wstring path = (boost::wformat(L"%s//%s.%s") % dirPath % imageName % imageExt).str();

	bool result = false;
	result = d_ptr->image.fromFile(path);

	LogD() << "Retina image imported, path: " << wtoa(path) << " => " << result;
	return result;
}

RetinaImage::RetinaImageImpl& oct_result::RetinaImage::getImpl(void) const
{
	return *d_ptr;
}

