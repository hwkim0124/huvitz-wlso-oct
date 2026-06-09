#include "pch.h"
#include "AngioFAZ.h"

#include "OctAngio2.h"

using namespace oct_angio;


struct AngioFAZ::AngioFAZImpl
{
	CvImage image;
	pair<int, int> fazCenter;
	pair<int, int> cicleCenter;
	vector<pair<int, int>> fazPoint;
	
	double fazArea;
	double fazPerimeter;
	double cicleRadius;
	float fazThreshold;

	AngioFAZImpl() {

	};
};

AngioFAZ::AngioFAZ()
	: d_ptr(make_unique<AngioFAZImpl>())
{
}


AngioFAZ::~AngioFAZ()
{
}


AngioFAZ::AngioFAZImpl & oct_angio::AngioFAZ::getImpl(void) const
{
	return *d_ptr;
}

CvImage & oct_angio::AngioFAZ::getFazImage(void)
{
	return getImpl().image;
}

const vector<pair<int, int>> oct_angio::AngioFAZ::getFazPoint(void) 
{
	return d_ptr->fazPoint;
}

const pair<int, int> oct_angio::AngioFAZ::getFazCenter(void)
{
	return d_ptr->fazCenter;
}

const pair<int, int> oct_angio::AngioFAZ::getCicleCenter(void)
{
	return d_ptr->cicleCenter;
}

const double oct_angio::AngioFAZ::getCicleRadius(void)
{
	return d_ptr->cicleRadius;
}

const double oct_angio::AngioFAZ::getFazArea(void)
{
	return d_ptr->fazArea;
}

const double oct_angio::AngioFAZ::getFazPerimeter(void)
{
	return d_ptr->fazPerimeter;
}

const float oct_angio::AngioFAZ::getThreshold(void)
{
	return d_ptr->fazThreshold;
}

void oct_angio::AngioFAZ::setupAngioFAZ(cpp_util::CvImage& image, float fazThreshold)
{
	setThreshold(fazThreshold);

	image.copyTo(&getImpl().image);
	return;
}

void oct_angio::AngioFAZ::setFazPoint(vector<pair<int, int>> point)
{
	d_ptr->fazPoint = point;
	return;
}

void oct_angio::AngioFAZ::setFazCenter(pair<int, int> center)
{
	d_ptr->fazCenter = center;
	return;
}

void oct_angio::AngioFAZ::setApproxCicle(pair<int, int> center, double radius)
{
	d_ptr->cicleCenter = center;
	d_ptr->cicleRadius = radius;
	return;
}

void oct_angio::AngioFAZ::setFazArea(double area)
{
	d_ptr->fazArea = area;
	return;
}

void oct_angio::AngioFAZ::setFazPerimeter(double perimeter)
{
	d_ptr->fazPerimeter = perimeter;
	return;
}

void oct_angio::AngioFAZ::setThreshold(float Threshold)
{
	d_ptr->fazThreshold = Threshold;
	return;
}

bool oct_angio::AngioFAZ::preprocessingImage(void)
{
	if (getFazImage().isEmpty())
	{
		return false;
	}
	getFazImage().applyPreprocessingFAZ(getThreshold());
	return true;
}

bool oct_angio::AngioFAZ::calculateFAZ(void)
{
	vector<pair<int, int>> point;
	pair<int, int> center;
	int width = getFazImage().getCols();
	int height = getFazImage().getRows();
	int	mode = cv::RETR_TREE;
	int	count = 0;

	if (width == 0 || height == 0) {
		return false;
	}

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(d_ptr->image.getCvMat(), contours, hierarchy, 
		cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, Point(0, 0));

	if (contours.size() == 0) {
		return false;
	}

	vector<vector<Point> >::iterator iter;
	vector<Point> longContour = contours[0];
	for (iter = contours.begin(); iter != contours.end(); ++iter) {
		if (iter->size() > longContour.size()) {
			longContour = *iter;
		}
	}

	vector<Point>::iterator longIter;
	for (longIter = longContour.begin(); longIter != longContour.end(); ++longIter) {

		pair<int, int> pt;
		pt.first = longIter->x; 
		pt.second = longIter->y;
		point.push_back(pt);
	}

	auto mmt = moments(longContour);
	center.first = int(mmt.m10 / mmt.m00);
	center.second = int(mmt.m01 / mmt.m00);

	Point2f cicleCenter; float radius;
	minEnclosingCircle(longContour, cicleCenter, radius);
	pair<int, int> approxCenter;
	approxCenter.first = (int)cicleCenter.x; 
	approxCenter.second = (int)cicleCenter.y;

	setFazPoint(point);
	setFazCenter(center);
	setFazArea(mmt.m00);
	setFazPerimeter(arcLength(longContour,true));
	setApproxCicle(approxCenter, radius);

	return true;
}
