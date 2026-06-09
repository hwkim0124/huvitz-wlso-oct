#include "pch.h"
#include "RetSegm2.h"
#include "SegmLayer.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace ret_segm;
using namespace cv;


struct SegmLayer::SegmLayerImpl
{
	LayerPointList points;
	std::vector<int> deltas;
	int regionWidth;
	int regionHeight;

	SegmLayerImpl() : regionWidth(0), regionHeight(0) {
	}

	SegmLayerImpl(int width, int height) {
		regionWidth = width;
		regionHeight = height;
		points.resize(width);
	}
};


SegmLayer::SegmLayer() :
	d_ptr(make_unique<SegmLayerImpl>())
{
}


SegmLayer::SegmLayer(int width, int height) :
	d_ptr(make_unique<SegmLayerImpl>(width, height))
{
}


ret_segm::SegmLayer::~SegmLayer() = default;
ret_segm::SegmLayer::SegmLayer(SegmLayer && rhs) = default;
SegmLayer & ret_segm::SegmLayer::operator=(SegmLayer && rhs) = default;


ret_segm::SegmLayer::SegmLayer(const SegmLayer & rhs)
	: d_ptr(make_unique<SegmLayerImpl>(*rhs.d_ptr))
{
}


SegmLayer & ret_segm::SegmLayer::operator=(const SegmLayer & rhs)
{
	// SegmLayerImpl struct should be defined before this access code. 
	*d_ptr = *rhs.d_ptr;
	return *this;
}


std::vector<int> ret_segm::SegmLayer::getXs(bool pickValids) const
{
	const LayerPointList& points = getPoints();

	if (pickValids == false) {
		std::vector<int> vect(points.size());
		for (int k = 0; k < vect.size(); k++) {
			vect[k] = d_ptr->points[k].x;
		}
		return vect;
	}
	else {
		std::vector<int> vect;
		for (int k = 0; k < points.size(); k++) {
			if (points[k].valid) {
				vect.push_back(points[k].x);
			}
		}
		return vect;
	}
}


std::vector<int> ret_segm::SegmLayer::getYs(bool pickValids) const
{
	const LayerPointList& points = getPoints();

	if (pickValids == false) {
		std::vector<int> vect(points.size());
		for (int k = 0; k < vect.size(); k++) {
			vect[k] = d_ptr->points[k].y;
		}
		return vect;
	}
	else {
		std::vector<int> vect;
		for (int k = 0; k < points.size(); k++) {
			if (points[k].valid) {
				vect.push_back(points[k].y);
			}
		}
		return vect;
	}
}


std::vector<int> ret_segm::SegmLayer::getYsResized(int width, int height) const
{
	LayerPointList vect = getPointsResized(width, height);
	std::vector<int> ys(vect.size(), -1);
	for (int i = 0; i < vect.size(); i++) {
		ys[i] = vect[i].y;
	}
	return ys;
}


LayerPointList ret_segm::SegmLayer::getPointsResized(int width, int height) const
{
	int horzSize = (width == 0 ? getRegionWidth() : width);
	int vertSize = (height == 0 ? getRegionHeight() : height);

	float horzRatio = (float)getRegionWidth() / (float)horzSize;
	float vertRatio = (float)vertSize / (float)getRegionHeight();

	auto dest = LayerPointList(horzSize);
	LayerPointList& points = getPoints();
	if (points.size() != getRegionWidth()) {
		for (int i = 0; i < horzSize; i++) {
			dest[i].set(i, -1);
		}
	}
	else {
		for (int i = 0; i < horzSize; i++) {
			auto p = points[(int)(i*horzRatio)];
			if (p.valid) {
				dest[i].set(i, (int)(p.y*vertRatio));
			}
			// LogD() << dest[i].getX() << ", " << dest[i].getY() << ", " << dest[i].isValid();
		}
	}
	return dest;
}


LayerPointList ret_segm::SegmLayer::getPointsValid(void) const
{
	LayerPointList& points = getPoints();
	LayerPointList vect;
	for (int k = 0; k < points.size(); k++) {
		if (points[k].valid) {
			vect.push_back(points[k]);
		}
	}
	return vect;

}


LayerPointList& ret_segm::SegmLayer::getPoints(void) const
{
	return d_ptr->points;
}


std::vector<int>& ret_segm::SegmLayer::getDeltas(void) const
{
	return d_ptr->deltas;
}


void ret_segm::SegmLayer::setValid(int index, bool flag)
{
	d_ptr->points[index].valid = flag;
	return;
}


bool ret_segm::SegmLayer::isValid(int index)
{
	return d_ptr->points[index].valid;
}


int ret_segm::SegmLayer::getY(int index) const
{
	return d_ptr->points[index].y;
}


void ret_segm::SegmLayer::setY(int index, int y)
{
	d_ptr->points[index].y = y;
	return;
}


void ret_segm::SegmLayer::setPoint(int index, int x, int y)
{
	d_ptr->points[index] = LayerPoint(x, y);
	return;
}


void ret_segm::SegmLayer::setPoint(int index, int x, int y, bool valid)
{
	d_ptr->points[index] = LayerPoint(x, y, valid);
	return;
}


void ret_segm::SegmLayer::setPoint(int index, const LayerPoint & point)
{
	d_ptr->points[index] = point;
	return;
}

void ret_segm::SegmLayer::setPoints(const std::vector<int>& ys)
{
	clear((int)ys.size());
	LayerPointList& points = getPoints();
	for (int x = 0; x < ys.size(); x++) {
		points[x].set(x, ys[x]);
	}
	return;
}

void ret_segm::SegmLayer::initialize(const std::vector<int>& vect, int width, int height)
{
	clear((int)vect.size());
	LayerPointList& points = getPoints();
	for (int x = 0; x < vect.size(); x++) {
		points[x].set(x, vect[x]);
	}
	setRegionSize(width, height);
	return;
}


void ret_segm::SegmLayer::initialize(const LayerPointList& vect, int width, int height)
{
	getPoints() = LayerPointList(vect);
	setRegionSize(width, height);
	return;
}


void ret_segm::SegmLayer::initialize(const cv::Mat & cvMat, int width, int height)
{
	clear(cvMat.cols);
	LayerPointList& points = getPoints();
	for (int c = 0; c < cvMat.cols; c++) {
		points[c].set( c, cvMat.at<int>(0, c) );
	}
	setRegionSize(width, height);
	return;
}


void ret_segm::SegmLayer::setRegionSize(int width, int height)
{
	d_ptr->regionWidth = width;
	d_ptr->regionHeight = height;
	return;
}


int ret_segm::SegmLayer::getRegionWidth(void) const
{
	return d_ptr->regionWidth;
}


int ret_segm::SegmLayer::getRegionHeight(void) const
{
	return d_ptr->regionHeight;
}


bool ret_segm::SegmLayer::resize(int targetWidth, int targetHeight)
{
	if (targetWidth <= 0 || targetHeight <= 0) {
		return false;
	}

	if (targetWidth != getRegionWidth() || targetHeight != getRegionHeight()) {
		/*
		auto dest = vector<SegmPoint>(targetWidth);
		float horzRatio = (float)getRegionWidth() / (float)targetWidth;
		float vertRatio = (float)targetHeight / (float)getRegionHeight();

		vector<SegmPoint>& points = getPoints();
		for (int i = 0; i < targetWidth; i++) {
			SegmPoint p = points[(int)(i*horzRatio)];
			if (p.valid) {
				dest[i].set(i, (int)(p.y*vertRatio));
			}
			// LogD() << dest[i].getX() << ", " << dest[i].getY() << ", " << dest[i].isValid();
		}
		*/
		LayerPointList& points = getPoints();
		points = getPointsResized(targetWidth, targetHeight);
		setRegionSize(targetWidth, targetHeight);
	}
	return true;
}


int ret_segm::SegmLayer::getSize(void) const
{
	return (int)getPoints().size();
}


bool ret_segm::SegmLayer::isEmpty(void) const
{
	return getPoints().empty();
}


void ret_segm::SegmLayer::clear(int size)
{
	LayerPointList& points = getPoints();
	if (size <= 0) {
		points.clear();
	}
	else {
		points = LayerPointList(size);
	}
	return;
}
