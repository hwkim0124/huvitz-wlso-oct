#include "pch.h"
#include "OpticDiscBscan.h"
#include "OpticDiscLayerSegm.h"

using namespace semt_segm;
using namespace std;


struct OpticDiscBscan::OpticDiscBscanImpl
{
	// unique_ptr<OpticDiscLayerSegm> bSegm;

	OpticDiscBscanImpl()
	{
	}
};


OpticDiscBscan::OpticDiscBscan() :
	d_ptr(make_unique<OpticDiscBscanImpl>())
{
}


semt_segm::OpticDiscBscan::~OpticDiscBscan() = default;
semt_segm::OpticDiscBscan::OpticDiscBscan(OpticDiscBscan && rhs) = default;
OpticDiscBscan & semt_segm::OpticDiscBscan::operator=(OpticDiscBscan && rhs) = default;


bool semt_segm::OpticDiscBscan::performSegmentation()
{
	resetBscanSegmentator();

	if (!getBscanSegmentator()->doSegmentation()) {
		return false;
	}

	return true;
}

int semt_segm::OpticDiscBscan::getOpticDiscX1() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->getOpticDiscX1();
}

int semt_segm::OpticDiscBscan::getOpticDiscX2() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->getOpticDiscX2();
}

int semt_segm::OpticDiscBscan::getOpticDiscPixels() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->getOpticDiscPixels();
}

int semt_segm::OpticDiscBscan::getOpticCupX1() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->getOpticCupX1();
}

int semt_segm::OpticDiscBscan::getOpticCupX2() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->getOpticCupX2();
}

int semt_segm::OpticDiscBscan::getOpticCupPixels() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->getOpticCupPixels();
}

bool semt_segm::OpticDiscBscan::isOpticDiscRegion() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->isOpticDiscRegion();
}

bool semt_segm::OpticDiscBscan::isOpticCupRegion() const
{
	auto* pSegm = getBscanSegmentator();
	return pSegm->getMeasuring()->isOpticCupRegion();
}


void semt_segm::OpticDiscBscan::resetBscanSegmentator()
{
	// std::make_unique already returns a pure rvalue.
	// getImpl().bSegm = make_unique<OpticDiscLayerSegm>(this);
	setBscanSegmentator(new OpticDiscLayerSegm(this));
	return;
}


OpticDiscBscan::OpticDiscBscanImpl & semt_segm::OpticDiscBscan::getImpl(void) const
{
	return *d_ptr;
}

