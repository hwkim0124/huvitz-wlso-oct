#include "pch.h"
#include "OctAngio2.h"
#include "AngioEtdrsChart.h"


using namespace cpp_util;
using namespace oct_angio;
using namespace std;
using namespace cv;

#include <iterator>
#include <algorithm>
#include <numeric>


struct AngioEtdrsChart::AngioEtdrsChartImpl
{
	float innerDiam = 1.0f;
	float outerDiam = 2.5f;

	float flowsCenter = 0.0f;
	float flowsInner[4] = { 0.0f };
	float flowsOuter[4] = { 0.0f };
	float flowsTotal = 0.0f;
	float flowsSuperior = 0.0f;
	float flowsInferior = 0.0f;

	float ratioCenter = 0.0f;
	float ratioInner[4] = { 0.0f };
	float ratioOuter[4] = { 0.0f };
	float ratioTotal = 0.0f;
	float ratioSuperior = 0.0f;
	float ratioInferior = 0.0f;

	AngioEtdrsChartImpl() {

	};
};

AngioEtdrsChart::AngioEtdrsChart()
	: d_ptr(make_unique<AngioEtdrsChartImpl>())
{
}


AngioEtdrsChart::~AngioEtdrsChart()
{
	// Destructor should be defined for unique_ptr to delete AngioEtdrsChartImpl as an incomplete type.
}


oct_angio::AngioEtdrsChart::AngioEtdrsChart(AngioEtdrsChart && rhs) = default;
AngioEtdrsChart & oct_angio::AngioEtdrsChart::operator=(AngioEtdrsChart && rhs) = default;


oct_angio::AngioEtdrsChart::AngioEtdrsChart(const AngioEtdrsChart & rhs)
	: d_ptr(make_unique<AngioEtdrsChartImpl>(*rhs.d_ptr))
{
}


AngioEtdrsChart & oct_angio::AngioEtdrsChart::operator=(const AngioEtdrsChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_angio::AngioEtdrsChart::setSectionDiameters(float inner, float outer)
{
	getImpl().innerDiam = inner;
	getImpl().outerDiam = outer;
	return;
}


bool oct_angio::AngioEtdrsChart::updateAngioChart(void)
{
	auto& image = getAngioImage();
	if (image.isEmpty()) {
		return false;
	}

	Mat msrc = image.getCvMatConst();
	Mat mask = Mat::zeros(msrc.rows, msrc.cols, CV_8UC1);

	Point center = Point(centerPosX(), centerPosY());
	float wPixMM = pixelsPerXmm();
	float hPixMM = pixelsPerYmm();
	float pixSize = (1.0f / wPixMM) * (1.0f / hPixMM);

	Mat mres;
	int thresh = (int)(getThreshold() * 255);
	cv::threshold(msrc, mres, thresh, 1.0, THRESH_TOZERO);

	float radius;
	Size size1, size2, size3;

	radius = d_ptr->innerDiam * 0.5f;
	size1 = Size((int)(radius*wPixMM), (int)(radius*hPixMM));
	cv::ellipse(mask, center, size1, 0.0, 0.0, 360.0, 1, -1);

	Mat sect = mres.mul(mask);
	double flows = cv::sum(sect)[0];
	double total = cv::sum(mask)[0];
	getImpl().flowsCenter = (float)(flows * pixSize);
	getImpl().ratioCenter = (float)(flows / total);

	radius = d_ptr->outerDiam * 0.5f;
	size2 = Size((int)(radius*wPixMM), (int)(radius*hPixMM));
	
	for (int i = 0; i < 4; i++) {
		mask = cv::Scalar::all(0);
		switch (i) {
		case 0:
			cv::ellipse(mask, center, size2, 0.0, 225.0, 315.0, 1, -1);
			break;
		case 1:
			cv::ellipse(mask, center, size2, 0.0, 315, 405, 1, -1);
			break;
		case 2:
			cv::ellipse(mask, center, size2, 0.0, 45, 135, 1, -1);
			break;
		case 3:
			cv::ellipse(mask, center, size2, 0.0, 135, 225, 1, -1);
			break;
		}
		cv::ellipse(mask, center, size1, 0.0, 0.0, 360.0, 0, -1);

		Mat sect = mres.mul(mask);
		double flows = cv::sum(sect)[0];
		double total = cv::sum(mask)[0];
		flows = isfinite(flows) ? flows : 0.0;
		total = isfinite(total) ? total : 0.0;
		getImpl().flowsInner[i] = (float)(flows * pixSize);
		getImpl().ratioInner[i] = (float)(flows / total);
	}

	/*
	radius = d_ptr->outerDiam * 0.5f;
	size3 = Size((int)(radius*wPixMM), (int)(radius*hPixMM));

	for (int i = 0; i < 4; i++) {
		mask = cv::Scalar::all(0);
		switch (i) {
		case 0:
			cv::ellipse(mask, center, size3, 0.0, 225.0, 315.0, 1, -1);
			break;
		case 1:
			cv::ellipse(mask, center, size3, 0.0, 315, 405, 1, -1);
			break;
		case 2:
			cv::ellipse(mask, center, size3, 0.0, 45, 135, 1, -1);
			break;
		case 3:
			cv::ellipse(mask, center, size3, 0.0, 135, 225, 1, -1);
			break;
		}
		cv::ellipse(mask, center, size2, 0.0, 0.0, 360.0, 0, -1);

		Mat sect = mres.mul(mask);
		double flows = cv::sum(sect)[0];
		double total = cv::sum(mask)[0];
		getImpl().flowsOuter[i] = (float)(flows * pixSize);
		getImpl().ratioOuter[i] = (float)(flows / total);
	}
	*/

	mask = cv::Scalar::all(0);
	cv::ellipse(mask, center, size2, 0.0, 0.0, 360.0, 1, -1);
	sect = mres.mul(mask);
	flows = cv::sum(sect)[0];
	total = cv::sum(mask)[0];
	flows = isfinite(flows) ? flows : 0.0;
	total = isfinite(total) ? total : 0.0;
	getImpl().flowsTotal = (float)(flows * pixSize);
	getImpl().ratioTotal = (float)(flows / total);

	mask = cv::Scalar::all(0);
	cv::ellipse(mask, center, size2, 0.0, 180.0, 360.0, 1, -1);
	sect = mres.mul(mask);
	flows = cv::sum(sect)[0];
	total = cv::sum(mask)[0];
	flows = isfinite(flows) ? flows : 0.0;
	total = isfinite(total) ? total : 0.0;
	getImpl().flowsSuperior = (float)(flows * pixSize);
	getImpl().ratioSuperior = (float)(flows / total);

	mask = cv::Scalar::all(0);
	cv::ellipse(mask, center, size2, 0.0, 0.0, 180.0, 1, -1);
	sect = mres.mul(mask);
	flows = cv::sum(sect)[0];
	total = cv::sum(mask)[0];
	flows = isfinite(flows) ? flows : 0.0;
	total = isfinite(total) ? total : 0.0;
	getImpl().flowsInferior = (float)(flows * pixSize);
	getImpl().ratioInferior = (float)(flows / total);
	return true;
}

float oct_angio::AngioEtdrsChart::flowsCenter(void) const
{
	return getImpl().flowsCenter;
}

float oct_angio::AngioEtdrsChart::flowsSection(int index) const
{
	if (index >= 0 && index < 4) {
		return getImpl().flowsInner[index];
	}
	return 0.0f;
}

float oct_angio::AngioEtdrsChart::flowsTotal(void) const
{
	return getImpl().flowsTotal;
}

float oct_angio::AngioEtdrsChart::flowsSuperior(void) const
{
	return getImpl().flowsSuperior;
}

float oct_angio::AngioEtdrsChart::flowsInferior(void) const
{
	return getImpl().flowsInferior;
}

float oct_angio::AngioEtdrsChart::densityCenter(void) const
{
	return getImpl().ratioCenter;
}

float oct_angio::AngioEtdrsChart::densitySection(int index) const
{
	if (index >= 0 && index < 4) {
		return getImpl().ratioInner[index];
	}
	return 0.0f;
}

float oct_angio::AngioEtdrsChart::densityTotal(void) const
{
	return getImpl().ratioTotal;
}

float oct_angio::AngioEtdrsChart::densitySuperior(void) const
{
	return getImpl().ratioSuperior;
}

float oct_angio::AngioEtdrsChart::densityInferior(void) const
{
	return getImpl().ratioInferior;
}


bool oct_angio::AngioEtdrsChart::fetchAngioChartDescript(OctAngioChartDescript& desc) const
{
	desc.centerFlows = flowsCenter();
	desc.centerDensity = densityCenter();

	for (int i = 0; i < 4; i++) {
		desc.innerFlows[i] = flowsSection(i);
		desc.innerDensity[i] = densitySection(i);
	}

	desc.superiorFlows = flowsSuperior();
	desc.inferiorFlows = flowsInferior();
	desc.averageFlows = flowsTotal();

	desc.superiorDensity = densitySuperior();
	desc.inferiorDensity = densityInferior();
	desc.averageDensity = densityTotal();

	desc.eyeSide = eyeSide();
	desc.centerX = centerX();
	desc.centerY = centerY();
	return true;
}


AngioEtdrsChart::AngioEtdrsChartImpl & oct_angio::AngioEtdrsChart::getImpl(void) const
{
	return *d_ptr;
}
