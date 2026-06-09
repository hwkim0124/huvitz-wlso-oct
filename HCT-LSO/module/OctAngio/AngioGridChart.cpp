#include "pch.h"
#include "AngioGridChart.h"

#include "OctAngio2.h"


using namespace oct_angio;



struct AngioGridChart::AngioGridChartImpl
{
	float gridSize = 1.0f;
	float flowsSection[9] = { 0.0f };
	float ratioSection[9] = { 0.0f };

	AngioGridChartImpl() {

	};
};



AngioGridChart::AngioGridChart()
	: d_ptr(make_unique<AngioGridChartImpl>())
{
}


AngioGridChart::~AngioGridChart()
{
}


oct_angio::AngioGridChart::AngioGridChart(AngioGridChart && rhs) = default;
AngioGridChart & oct_angio::AngioGridChart::operator=(AngioGridChart && rhs) = default;


oct_angio::AngioGridChart::AngioGridChart(const AngioGridChart & rhs)
	: d_ptr(make_unique<AngioGridChartImpl>(*rhs.d_ptr))
{
}


AngioGridChart & oct_angio::AngioGridChart::operator=(const AngioGridChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_angio::AngioGridChart::setSectionSize(float size)
{
	getImpl().gridSize = size;
	return;
}


bool oct_angio::AngioGridChart::updateAngioChart(void)
{
	auto& image = getAngioImage();
	if (image.isEmpty()) {
		return false;
	}

	Mat matSrc = image.getCvMatConst();

	Point center = Point(centerPosX(), centerPosY());
	float wPixMM = pixelsPerXmm();
	float hPixMM = pixelsPerYmm();
	float pixSize = (1.0f / wPixMM) * (1.0f / hPixMM);

	Mat matRes;
	int thresh = (int)(getThreshold() * 255);
	cv::threshold(matSrc, matRes, thresh, 1.0, THRESH_TOZERO);

	for (int i = 0; i < 9; i++) {
		Mat matMask = Mat::zeros(matRes.rows, matRes.cols, CV_8UC1);

		Point pt1 = Point((int)(center.x + ((i % 3 - 1) - 0.5f)*wPixMM), (int)(center.y + ((i / 3 - 1) - 0.5f)*hPixMM));
		Point pt2 = Point((int)(center.x + ((i % 3 - 1) + 0.5f)*wPixMM), (int)(center.y + ((i / 3 - 1) + 0.5f)*hPixMM));
		cv::rectangle(matMask, pt1, pt2, 1, -1);

		Mat matSect = matRes.mul(matMask);
		double flows = cv::sum(matSect)[0];
		double total = cv::sum(matMask)[0];

		getImpl().flowsSection[i] = (float)(flows * pixSize);
		getImpl().ratioSection[i] = (float)(flows / total);
	}
	return true;
}


float oct_angio::AngioGridChart::flowsSection(int index) const
{
	if (index >= 0 && index < 9) {
		return getImpl().flowsSection[index];
	}
	return 0.0f;
}


float oct_angio::AngioGridChart::densitySection(int index) const
{
	if (index >= 0 && index < 9) {
		return getImpl().ratioSection[index];
	}
	return 0.0f;
}


AngioGridChart::AngioGridChartImpl & oct_angio::AngioGridChart::getImpl(void) const
{
	return *d_ptr;
}

