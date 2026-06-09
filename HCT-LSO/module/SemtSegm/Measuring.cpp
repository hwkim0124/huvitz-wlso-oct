#include "pch.h"
#include "SemtSegm2.h"
#include "Measuring.h"
#include "Bordering.h"
#include "Sampling.h"
#include "BscanSegmentator.h"

using namespace semt_segm;
using namespace cv;

#include <algorithm>


struct Measuring::MeasuringImpl
{
	const BscanSegmentator* pSegm = nullptr;

	int discX1 = -1;
	int discX2 = -1;
	int cupX1 = -1;
	int cupX2 = -1;

	int discSidePixels = 0;
	int discCupPixels = 0;

	MeasuringImpl()
	{
	}
};


Measuring::Measuring(BscanSegmentator* pSegm) :
	d_ptr(make_unique<MeasuringImpl>())
{
	getImpl().pSegm = pSegm;
}


semt_segm::Measuring::~Measuring() = default;
semt_segm::Measuring::Measuring(Measuring && rhs) = default;
Measuring & semt_segm::Measuring::operator=(Measuring && rhs) = default;

bool semt_segm::Measuring::findOpticDiscSideParams() const
{
	auto* pSegm = getImpl().pSegm;
	auto* pBorder = pSegm->getBodering();

	if (!pBorder->isOpticDiscRegion()) {
		return false;
	}

	int disc_x1 = pBorder->getOpticDiscX1();
	int disc_x2 = pBorder->getOpticDiscX2();
	int pixels = 0;
	int dsize = (disc_x2 - disc_x1 + 1);
	int index = pSegm->getImageIndex();

	const auto& inner = pSegm->getLayerILM()->getOptimalPath();
	const auto& outer = pSegm->getLayerRPE()->getOptimalPath();

	int out_x1 = disc_x1;
	int out_x2 = disc_x2;
	int out_y1 = outer[out_x1];
	int out_y2 = outer[out_x2];

	if (out_y1 < 0 || out_y2 < 0 || out_x1 == out_x2) {
		return false;
	}

	const float FEAT_DISK_CUP_LINE_DEPTH = 150.0f;
	const float axialResol = (float)OctDataSetup::getRetinaScanAxialResolution();

	int upp_y1 = (int)(out_y1 - FEAT_DISK_CUP_LINE_DEPTH / axialResol);
	int upp_y2 = (int)(out_y2 - FEAT_DISK_CUP_LINE_DEPTH / axialResol);
	upp_y1 = max(upp_y1, 0);
	upp_y2 = max(upp_y2, 0);

	float slope = (float)(upp_y2 - upp_y1) / (float)(out_x2 - out_x1);
	int line;

	int count = 0;
	for (int k = out_x1, dist = 0; k <= out_x2; k++, dist++) {
		line = (int)(upp_y1 + dist * slope);
		if (inner[k] < line) {
			count += (line - inner[k]);
		}
	}

	pixels = count;
	LogD() << "Optic disc, range: " << disc_x1 << ", " << disc_x2 << ", size: " << dsize << ", pixels: " << pixels << ", index: " << index;

	getImpl().discX1 = disc_x1;
	getImpl().discX2 = disc_x2;
	getImpl().discSidePixels = pixels;
	return true;
}

bool semt_segm::Measuring::findOpticDiscCupParams() const
{
	auto* pSegm = getImpl().pSegm;
	auto* pBorder = pSegm->getBodering();

	if (!pBorder->isOpticDiscRegion()) {
		return false;
	}

	int disc_x1 = pBorder->getOpticDiscX1();
	int disc_x2 = pBorder->getOpticDiscX2();
	int pixels = 0;
	int dsize = (disc_x2 - disc_x1 + 1);
	int index = pSegm->getImageIndex();

	const auto& inner = pSegm->getLayerILM()->getOptimalPath();
	const auto& outer = pSegm->getLayerRPE()->getOptimalPath();

	int out_x1 = disc_x1;
	int out_x2 = disc_x2;
	int out_y1 = outer[out_x1];
	int out_y2 = outer[out_x2];

	if (out_y1 < 0 || out_y2 < 0 || out_x1 == out_x2) {
		return false;
	}

	const float FEAT_DISK_CUP_LINE_DEPTH = 150.0f;
	const float axialResol = (float)OctDataSetup::getRetinaScanAxialResolution();

	int upp_y1 = (int)(out_y1 - FEAT_DISK_CUP_LINE_DEPTH / axialResol);
	int upp_y2 = (int)(out_y2 - FEAT_DISK_CUP_LINE_DEPTH / axialResol);
	upp_y1 = max(upp_y1, 0);
	upp_y2 = max(upp_y2, 0);

	float slope = (float)(upp_y2 - upp_y1) / (float)(out_x2 - out_x1);
	int line;

	int cup_x1 = -1;
	int cup_x2 = -1;

	for (int k = out_x1, dist = 0; k <= out_x2; k++, dist++) {
		line = max((int)(upp_y1 + dist * slope), 0);
		if (inner[k] >= line) {
			cup_x1 = k;
			break;
		}
	}

	if (cup_x1 >= 0)
	{
		for (int k = out_x2, dist = 0; k >= out_x1; k--, dist++) {
			line = max((int)(upp_y2 - dist * slope), 0);
			if (inner[k] >= line) {
				cup_x2 = k;
				break;
			}
		}
	}

	if (cup_x1 < 0 || cup_x2 < 0 || cup_x1 == cup_x2) {
		return false;
	}

	int count = 0;
	for (int k = cup_x1, dist = (cup_x1 - out_x1); k <= cup_x2; k++, dist++) {
		line = max((int)(upp_y1 + dist * slope), 0);
		if (inner[k] >= line) {
			count += (inner[k] - line + 1);
		}
	}

	pixels = count;
	LogD() << "Optic cup, range: " << cup_x1 << ", " << cup_x2 << ", size: " << (cup_x2 - cup_x1 + 1) << ", pixels: " << pixels << ", index: " << index;

	getImpl().cupX1 = cup_x1;
	getImpl().cupX2 = cup_x2;
	getImpl().discCupPixels = pixels;
	return true;
}



int semt_segm::Measuring::getOpticDiscX1() const
{
	return getImpl().discX1;
}

int semt_segm::Measuring::getOpticDiscX2() const
{
	return getImpl().discX2;
}

int semt_segm::Measuring::getOpticDiscPixels() const
{
	return getImpl().discSidePixels;
}

int semt_segm::Measuring::getOpticCupX1() const
{
	return getImpl().cupX1;
}

int semt_segm::Measuring::getOpticCupX2() const
{
	return getImpl().cupX2;
}

int semt_segm::Measuring::getOpticCupPixels() const
{
	return getImpl().discCupPixels;
}

bool semt_segm::Measuring::isOpticDiscRegion() const
{
	return (getImpl().discX1 >= 0 && getImpl().discX2 >= 0 && getImpl().discX1 < getImpl().discX2);
}

bool semt_segm::Measuring::isOpticCupRegion() const
{
	return (getImpl().cupX1 >= 0 && getImpl().cupX2 >= 0 && getImpl().cupX1 < getImpl().cupX2);
}

Measuring::MeasuringImpl & semt_segm::Measuring::getImpl(void) const
{
	return *d_ptr;
}


