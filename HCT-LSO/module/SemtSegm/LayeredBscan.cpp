#include "pch.h"
#include "LayeredBscan.h"
#include "BscanSegmentator.h"
#include "SemtSegm2.h"

using namespace semt_segm;
using namespace std;


struct LayeredBscan::LayeredBscanImpl
{
	OctScanImageDescript image;
	unique_ptr<BscanSegmentator> bSegm;

	LayeredBscanImpl()
	{
	}
};


LayeredBscan::LayeredBscan() :
	d_ptr(make_unique<LayeredBscanImpl>())
{
}


semt_segm::LayeredBscan::~LayeredBscan() = default;
semt_segm::LayeredBscan::LayeredBscan(LayeredBscan && rhs) = default;
LayeredBscan & semt_segm::LayeredBscan::operator=(LayeredBscan && rhs) = default;


void semt_segm::LayeredBscan::setImageSource(const unsigned char * bits, int width, int height, float range, int index)
{
	getImpl().image.setup(bits, width, height);
	getImpl().image.scanRange = range;
	getImpl().image.imageIndex = index;
}

void semt_segm::LayeredBscan::setImageSource(OctScanImageDescript image)
{
	getImpl().image = image;
}

void semt_segm::LayeredBscan::setImageIndex(int index)
{
	getImpl().image.imageIndex = index;
}


bool semt_segm::LayeredBscan::isImageSource(void) const
{
	return !getImpl().image.isEmpty();
}

int semt_segm::LayeredBscan::getImageIndex(void) const
{
	return getImpl().image.imageIndex;
}

float semt_segm::LayeredBscan::getImageRangeX(void) const
{
	return getImpl().image.scanRange;
}

const OctScanImageDescript& semt_segm::LayeredBscan::getImageSource(void) const
{
	return getImpl().image;
}

const OcularImage * semt_segm::LayeredBscan::getImageSample(void) const
{
	return getBscanSegmentator()->getSampling()->sample();
}

const OcularImage * semt_segm::LayeredBscan::getImageAscent(void) const
{
	return getBscanSegmentator()->getSampling()->ascent();
}

const OcularImage * semt_segm::LayeredBscan::getImageDescent(void) const
{
	return getBscanSegmentator()->getSampling()->descent();
}

const OcularImage * semt_segm::LayeredBscan::getImageAverage(void) const
{
	return getBscanSegmentator()->getSampling()->average();;
}

const OcularImage * semt_segm::LayeredBscan::getInputFlattened(void) const
{
	return getBscanSegmentator()->getClassifier()->getInputImage();
}

const OcularImage * semt_segm::LayeredBscan::getInputDenoised(void) const
{
	return getBscanSegmentator()->getClassifier()->getInputDenoised();
}

const std::vector<int>& semt_segm::LayeredBscan::getInnerEdges() const
{
	return getBscanSegmentator()->getBodering()->getInnerEdges();
}

const std::vector<int>& semt_segm::LayeredBscan::getOuterEdges() const
{
	return getBscanSegmentator()->getBodering()->getOuterEdges();
}

const std::vector<int>& semt_segm::LayeredBscan::getInnerBound() const
{
	return getBscanSegmentator()->getBodering()->getInnerBound();
}

const std::vector<int>& semt_segm::LayeredBscan::getOuterBound() const
{
	return getBscanSegmentator()->getBodering()->getOuterBound();
}

const std::vector<int>& semt_segm::LayeredBscan::getCenterBound() const
{
	return getBscanSegmentator()->getBodering()->getCenterBound();
}

const std::vector<int>& semt_segm::LayeredBscan::getInnerBorder() const
{
	return getBscanSegmentator()->getBodering()->getInnerBorder();
}

const std::vector<int>& semt_segm::LayeredBscan::getOuterBorder() const
{
	return getBscanSegmentator()->getBodering()->getOuterBorder();
}

const std::vector<int>& semt_segm::LayeredBscan::getInnerLayer() const
{
	return getBscanSegmentator()->getLayerInner()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getOuterLayer() const
{
	return getBscanSegmentator()->getLayerOuter()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryILM() const
{
	return getBscanSegmentator()->getLayerILM()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryNFL() const
{
	return getBscanSegmentator()->getLayerNFL()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryOPL() const
{
	return getBscanSegmentator()->getLayerOPL()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryIPL() const
{
	return getBscanSegmentator()->getLayerIPL()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryIOS() const
{
	return getBscanSegmentator()->getLayerIOS()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryRPE() const
{
	return getBscanSegmentator()->getLayerRPE()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryBRM() const
{
	return getBscanSegmentator()->getLayerBRM()->getOptimalPath();
}

const std::vector<int>& semt_segm::LayeredBscan::getBoundaryOPR() const
{
	return getBscanSegmentator()->getLayerOPR()->getOptimalPath();
}

void semt_segm::LayeredBscan::setBscanSegmentator(BscanSegmentator * segm)
{
	getImpl().bSegm = unique_ptr<BscanSegmentator>(segm);
}

BscanSegmentator * semt_segm::LayeredBscan::getBscanSegmentator() const
{
	return getImpl().bSegm.get();
}


LayeredBscan::LayeredBscanImpl & semt_segm::LayeredBscan::getImpl(void) const
{
	return *d_ptr;
}


