#include "pch.h"
#include "RetParam2.h"
#include "EnfaceTmap.h"

using namespace ret_param;
using namespace cv;


struct EnfaceTmap::EnfaceTmapImpl
{

	EnfaceTmapImpl() {
	}
};


EnfaceTmap::EnfaceTmap() :
	d_ptr(make_unique<EnfaceTmapImpl>())
{
}


ret_param::EnfaceTmap::~EnfaceTmap() = default;
ret_param::EnfaceTmap::EnfaceTmap(EnfaceTmap && rhs) = default;
EnfaceTmap & ret_param::EnfaceTmap::operator=(EnfaceTmap && rhs) = default;


ret_param::EnfaceTmap::EnfaceTmap(const EnfaceTmap & rhs)
	: d_ptr(make_unique<EnfaceTmapImpl>(*rhs.d_ptr))
{
}


EnfaceTmap & ret_param::EnfaceTmap::operator=(const EnfaceTmap & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool ret_param::EnfaceTmap::copyToImage(cpp_util::CvImage * image, int width, int height)
{
	if (isEmpty()) {
		return false;
	}

	Size dsize;
	// width나 height가 유효하지 않은 값일 경우, 원본 크기를 그대로 사용 함
	if (width < 0 || height < 0) {
		dsize.width = getDataImage()->getCvMatConst().cols;
		dsize.height = getDataImage()->getCvMatConst().rows;
	}
	//
	else {
		dsize.width = width;
		dsize.height = height;
	}

	// Source's type should not be 32SC1, instead 32FC1. 
	cv::resize(getDataImage()->getCvMatConst(), image->getCvMat(), dsize, INTER_CUBIC);
	return true;
}

