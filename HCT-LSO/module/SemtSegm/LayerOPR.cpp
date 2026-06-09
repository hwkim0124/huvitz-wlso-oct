#include "pch.h"
#include "SemtSegm2.h"
#include "LayerOPR.h"
#include "BscanSegmentator.h"

using namespace semt_segm;
using namespace cv;

#include <algorithm>


struct LayerOPR::LayerOPRImpl
{
	LayerOPRImpl()
	{
	}
};


LayerOPR::LayerOPR(BscanSegmentator* pSegm) :
	d_ptr(make_unique<LayerOPRImpl>()), OptimalLayer(pSegm)
{
}


semt_segm::LayerOPR::~LayerOPR() = default;
semt_segm::LayerOPR::LayerOPR(LayerOPR && rhs) = default;
LayerOPR & semt_segm::LayerOPR::operator=(LayerOPR && rhs) = default;


bool semt_segm::LayerOPR::buildFlattenedPath()
{
	auto* pSegm = getSegmentator();

	const auto& layer = pSegm->getLayerBRM()->getOptimalPath();

	getOptimalPath() = layer;
	return true;
}

bool semt_segm::LayerOPR::buildBoundaryLayer()
{
	auto* pSegm = getSegmentator();
	auto* pSample = pSegm->getSampling();

	int width = pSample->getSourceWidth();
	int height = pSample->getSourceHeight();
	float rangeX = pSegm->getImageRangeX();

	const int FILTER_SIZE = 13;
	const int CURVE_DEGREE = 1;

	auto& path = getOptimalPath();
	int size = (int)path.size();
	if (size <= 0) {
		return false;
	}

	const auto& inner = pSegm->getLayerRPE()->getOptimalPath();

	float scale = (1.0f / pSample->getSampleWidthRatio());
	int filter = (int)(scale * FILTER_SIZE);
	auto outs = path;

	outs = cpp_util::SgFilter::smoothInts(outs, filter, CURVE_DEGREE);
	std::transform(cbegin(outs), cend(outs), cbegin(inner), begin(outs), [=](int elem1, int elem2) { return max(elem1, elem2); });
	transform(begin(outs), end(outs), begin(outs), [=](int elm) { return min(max(elm, 0), height - 1); });

	path = outs;
	return true;
}

LayerOPR::LayerOPRImpl & semt_segm::LayerOPR::getImpl(void) const
{
	return *d_ptr;
}
