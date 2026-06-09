#include "pch.h"
#include "RetParam2.h"
#include "EnfaceShot.h"

using namespace ret_param;
using namespace cv;



struct EnfaceShot::EnfaceShotImpl
{
	EnfaceShotImpl() {
	}
};


EnfaceShot::EnfaceShot() :
	d_ptr(make_unique<EnfaceShotImpl>())
{
}


ret_param::EnfaceShot::~EnfaceShot() = default;
ret_param::EnfaceShot::EnfaceShot(EnfaceShot && rhs) = default;
EnfaceShot & ret_param::EnfaceShot::operator=(EnfaceShot && rhs) = default;


ret_param::EnfaceShot::EnfaceShot(const EnfaceShot & rhs)
	: d_ptr(make_unique<EnfaceShotImpl>(*rhs.d_ptr))
{
}


EnfaceShot & ret_param::EnfaceShot::operator=(const EnfaceShot & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool ret_param::EnfaceShot::copyToImage(cpp_util::CvImage * image, int width, int height)
{
	if (isEmpty()) {
		return false;
	}

	Size dsize(width, height);
	cv::resize(getDataImage()->getCvMatConst(), image->getCvMat(), dsize, INTER_CUBIC);
	return true;
}

