#include "pch.h"
#include "CorneaImage.h"

#include <boost/format.hpp>

using namespace oct_result;


struct CorneaImage::CorneaImageImpl
{
	OctCorneaImageDescript descript;
	CvImage image;

	CorneaImageImpl()
	{
	}
};


CorneaImage::CorneaImage() :
	d_ptr(make_unique<CorneaImageImpl>())
{
}


oct_result::CorneaImage::CorneaImage(const OctCorneaImageDescript& desc) :
	d_ptr(make_unique<CorneaImageImpl>())
{
	setData(desc);
}

oct_result::CorneaImage::~CorneaImage() = default;
oct_result::CorneaImage::CorneaImage(CorneaImage && rhs) = default;
CorneaImage & oct_result::CorneaImage::operator=(CorneaImage && rhs) = default;


oct_result::CorneaImage::CorneaImage(const CorneaImage & rhs)
	: d_ptr(make_unique<CorneaImageImpl>(*rhs.d_ptr))
{
}


CorneaImage & oct_result::CorneaImage::operator=(const CorneaImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_result::CorneaImage::setData(const OctCorneaImageDescript& desc)
{
	d_ptr->descript = desc;

	d_ptr->image.fromBitsData(desc.pData, desc.width, desc.height);
	getImpl().descript.pData = d_ptr->image.getBitsData();
	return;
}


const OctCorneaImageDescript& oct_result::CorneaImage::getDescript(void) const
{
	auto descCopy = d_ptr->descript;
	descCopy.pData = getBuffer();
	descCopy.width = getWidth();
	descCopy.height = getHeight();
	d_ptr->descript = descCopy;
	return d_ptr->descript;
}


std::uint8_t * oct_result::CorneaImage::getBuffer(void) const
{
	return d_ptr->image.getBitsData();
}


bool oct_result::CorneaImage::isEmpty(void) const
{
	return getImpl().image.isEmpty();
}


int oct_result::CorneaImage::getWidth(void) const
{
	return getImpl().image.getWidth();
}


int oct_result::CorneaImage::getHeight(void) const
{
	return getImpl().image.getHeight();
}


bool oct_result::CorneaImage::exportFile(std::wstring dirPath, std::wstring imageName)
{
	wstring path = (boost::wformat(L"%s//%s.%s") % dirPath % imageName % FRAME_IMAGE_FORMAT).str();

	bool result = false;
	if (!isEmpty()) {
		result = d_ptr->image.saveFile(wtoa(path), false);
		LogD() << "Cornea image exported, path: " << wtoa(path);
	}
	else {
		LogD() << "Cornea image is empty, path: " << wtoa(path);
	}
	return false;
}


bool oct_result::CorneaImage::importFile(std::wstring dirPath, std::wstring imageName)
{
	bool ret;

	ret = importFileWithExt(dirPath, imageName, FRAME_IMAGE_FORMAT);
	if (!ret) {
		ret = importFileWithExt(dirPath, imageName, FRAME_IMAGE_FORMAT_OLD);
	}

	return ret;
}

bool oct_result::CorneaImage::importFileWithExt(std::wstring dirPath, std::wstring imageName,
	std::wstring imageExt)
{
	wstring path = (boost::wformat(L"%s//%s.%s") % dirPath % imageName % imageExt).str();

	bool result = false;
	result = d_ptr->image.fromFile(path);

	LogD() << "Cornea image imported, path: " << wtoa(path) << " => " << result;
	return result;
}

CorneaImage::CorneaImageImpl & oct_result::CorneaImage::getImpl(void) const
{
	return *d_ptr;
}

