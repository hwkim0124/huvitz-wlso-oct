#include "pch.h"
#include "MacularSampling.h"

using namespace semt_segm;
using namespace std;


struct MacularSampling::MacularSamplingImpl
{
	unique_ptr<Sampling> sampling;

	MacularSamplingImpl()
	{
	}
};


MacularSampling::MacularSampling() :
	d_ptr(make_unique<MacularSamplingImpl>())
{
}


semt_segm::MacularSampling::~MacularSampling() = default;
semt_segm::MacularSampling::MacularSampling(MacularSampling && rhs) = default;
MacularSampling & semt_segm::MacularSampling::operator=(MacularSampling && rhs) = default;


MacularSampling::MacularSamplingImpl & semt_segm::MacularSampling::getImpl(void) const
{
	return *d_ptr;
}


