#include "pch.h"
#include "EnfaceOutput.h"
#include "EnfaceImage.h"

#include <boost/format.hpp>
#include <thread>
#include <vector>
#include <algorithm>

using namespace oct_result;


struct EnfaceOutput::EnfaceOutputImpl
{
	unique_ptr<EnfaceImage> enface;

	EnfaceOutputImpl() : enface(make_unique<EnfaceImage>())
	{
	}
};


EnfaceOutput::EnfaceOutput() :
	d_ptr(make_unique<EnfaceOutputImpl>())
{
}


oct_result::EnfaceOutput::~EnfaceOutput() = default;
oct_result::EnfaceOutput::EnfaceOutput(EnfaceOutput && rhs) = default;
EnfaceOutput & oct_result::EnfaceOutput::operator=(EnfaceOutput && rhs) = default;


const EnfaceImage* oct_result::EnfaceOutput::getEnfaceImage(void)
{
	return d_ptr->enface.get();
}


const OctEnfaceImageDescript* oct_result::EnfaceOutput::getEnfaceImageDescript(void)
{
	auto image = getEnfaceImage();
	if (image != nullptr) {
		return &image->getDescript();
	}
	return nullptr;
}


bool oct_result::EnfaceOutput::setEnfaceImage(const OctEnfaceImageDescript& enface, bool process)
{
	d_ptr->enface = make_unique<EnfaceImage>(enface);
	if (process) {
		d_ptr->enface->processImage();
	}
	return true;
}


bool oct_result::EnfaceOutput::exportImage(const std::wstring & dirName,
	const std::wstring & fileName)
{
	return getImpl().enface->exportFile(dirName, fileName);
}


bool oct_result::EnfaceOutput::importImage(const std::wstring& dirName,
	const std::wstring& fileName)
{
	return getImpl().enface->importFile(dirName, fileName);
}


EnfaceOutput::EnfaceOutputImpl & oct_result::EnfaceOutput::getImpl(void) const
{
	return *d_ptr;
}
