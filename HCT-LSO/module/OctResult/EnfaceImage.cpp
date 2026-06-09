#include "pch.h"
#include "EnfaceImage.h"

#include <boost/format.hpp>

using namespace oct_result;


struct EnfaceImage::EnfaceImageImpl
{
	OctEnfaceImageDescript descript;
	CvImage image;

	EnfaceImageImpl()
	{
	}
};


EnfaceImage::EnfaceImage() :
	d_ptr(make_unique<EnfaceImageImpl>())
{
}


oct_result::EnfaceImage::EnfaceImage(const OctEnfaceImageDescript& desc) :
	d_ptr(make_unique<EnfaceImageImpl>())
{
	setData(desc);
}


oct_result::EnfaceImage::~EnfaceImage() = default;
oct_result::EnfaceImage::EnfaceImage(EnfaceImage && rhs) = default;
EnfaceImage & oct_result::EnfaceImage::operator=(EnfaceImage && rhs) = default;


oct_result::EnfaceImage::EnfaceImage(const EnfaceImage & rhs)
	: d_ptr(make_unique<EnfaceImageImpl>(*rhs.d_ptr))
{
}


EnfaceImage & oct_result::EnfaceImage::operator=(const EnfaceImage & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_result::EnfaceImage::setData(const OctEnfaceImageDescript& desc)
{
	d_ptr->descript = desc;
	d_ptr->image.fromBitsData(desc.pData, desc.width, desc.height);
	return;
}


std::uint8_t * oct_result::EnfaceImage::getBuffer(void) const
{
	return d_ptr->image.getBitsData();
}


const OctEnfaceImageDescript& oct_result::EnfaceImage::getDescript(void) const
{
	d_ptr->descript.setup(getBuffer(), getWidth(), getHeight());
	return d_ptr->descript;
}


bool oct_result::EnfaceImage::isEmpty(void) const
{
	return d_ptr->image.isEmpty();
}


int oct_result::EnfaceImage::getWidth(void) const
{
	return d_ptr->image.getWidth();
}


int oct_result::EnfaceImage::getHeight(void) const
{
	return d_ptr->image.getHeight();
}


bool oct_result::EnfaceImage::exportFile(const std::wstring& dirPath, const std::wstring& imageName,
										int width, int height)
{
	wstring path = (boost::wformat(L"%s//%s.%s") % dirPath % imageName % ENFACE_IMAGE_FORMAT).str();
	
	bool result = false;
	if (!isEmpty()) {
		CvImage resized;
		if (d_ptr->image.resizeTo(&resized, width, height)) {
			result = resized.saveFile(wtoa(path), true);
		}
		LogD() << "Enface image exported, path: " << wtoa(path) << " => " << result;
	}
	else {
		LogD() << "Enface image is empty!, path: " << wtoa(path);
	}
	return result;
}


bool oct_result::EnfaceImage::importFile(const std::wstring& dirPath, const std::wstring& imageName)
{
	bool ret;

	ret = importFileWithExt(dirPath, imageName, ENFACE_IMAGE_FORMAT);
	if (!ret) {
		ret = importFileWithExt(dirPath, imageName, ENFACE_IMAGE_FORMAT_OLD);
	}

	return ret;
}

bool oct_result::EnfaceImage::importFileWithExt(const std::wstring& dirPath,
	const std::wstring& imageName, const std::wstring& imageExt)
{
	wstring path = (boost::wformat(L"%s//%s.%s") % dirPath % imageName % imageExt).str();

	bool result = false;
	result = d_ptr->image.fromFile(path);

	LogD() << "Enface image imported, path: " << wtoa(path) << " => " << result;
	return result;
}

bool oct_result::EnfaceImage::processImage(void)
{
	if (!isEmpty()) {
		d_ptr->image.equalizeHistogram();
		//d_ptr->image.resize(256, 256);
		return true;
	}
	return false;
}


EnfaceImage::EnfaceImageImpl & oct_result::EnfaceImage::getImpl(void) const
{
	return *d_ptr;
}
