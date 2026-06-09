#include "pch.h"
#include "OpticDiscSampling.h"

using namespace semt_segm;
using namespace std;


struct OpticDiscSampling::OpticDiscSamplingImpl
{
	unique_ptr<Sampling> sampling;

	OpticDiscSamplingImpl()
	{
	}
};


OpticDiscSampling::OpticDiscSampling() :
	d_ptr(make_unique<OpticDiscSamplingImpl>())
{
}


semt_segm::OpticDiscSampling::~OpticDiscSampling() = default;
semt_segm::OpticDiscSampling::OpticDiscSampling(OpticDiscSampling && rhs) = default;
OpticDiscSampling & semt_segm::OpticDiscSampling::operator=(OpticDiscSampling && rhs) = default;


OpticDiscSampling::OpticDiscSamplingImpl & semt_segm::OpticDiscSampling::getImpl(void) const
{
	return *d_ptr;
}


