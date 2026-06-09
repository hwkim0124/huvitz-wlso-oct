#include "pch.h"
#include "AngioChart.h"

#include "OctAngio2.h"

using namespace oct_angio;


struct AngioChart::AngioChartImpl
{
	float rangeX = 3.0f;
	float rangeY = 3.0f;
	int centerPosX = 0;
	int centerPosY = 0;
	int imageW = 384;
	int imageH = 384;
	
	float centerX = 0.0f;
	float centerY = 0.0f;

	EyeSide eyeSide = EyeSide::OD;
	float threshold = 0.15f;
	CvImage image;

	AngioChartImpl() {

	};
};

AngioChart::AngioChart()
	: d_ptr(make_unique<AngioChartImpl>())
{
}


AngioChart::~AngioChart()
{
	// Destructor should be defined for unique_ptr to delete AngioChartImpl as an incomplete type.
}


oct_angio::AngioChart::AngioChart(AngioChart && rhs) = default;
AngioChart & oct_angio::AngioChart::operator=(AngioChart && rhs) = default;


oct_angio::AngioChart::AngioChart(const AngioChart & rhs)
	: d_ptr(make_unique<AngioChartImpl>(*rhs.d_ptr))
{
}


AngioChart & oct_angio::AngioChart::operator=(const AngioChart & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_angio::AngioChart::setupAngioChart(cpp_util::CvImage& image, float rangeX, float rangeY, int imageW, int imageH)
{
	getImpl().rangeX = rangeX;
	getImpl().rangeY = rangeY;
	getImpl().imageW = imageW;
	getImpl().imageH = imageH;

	getImpl().centerPosX = imageW / 2;
	getImpl().centerPosY = imageH / 2;

	image.copyTo(&getImpl().image);
	return;
}


void oct_angio::AngioChart::setCenterPosition(float xmm, float ymm)
{
	int xpos = (int)(getImpl().imageW / 2 + xmm * pixelsPerXmm());
	int ypos = (int)(getImpl().imageH / 2 + ymm * pixelsPerYmm());
	xpos = min(max(xpos, 0), getImpl().imageW - 1);
	ypos = min(max(ypos, 0), getImpl().imageH - 1);
	getImpl().centerPosX = xpos;
	getImpl().centerPosY = ypos;
	getImpl().centerX = xmm;
	getImpl().centerY = ymm;
	return;
}


float oct_angio::AngioChart::getThreshold(void) const
{
	return getImpl().threshold;
}


void oct_angio::AngioChart::setEyeSide(EyeSide side)
{
	getImpl().eyeSide = side;
	return;
}

void oct_angio::AngioChart::setThreshold(float threshold)
{
	getImpl().threshold = max(min(threshold, 1.0f), 0.0f);
	return;
}


bool oct_angio::AngioChart::updateAngioChart(void)
{
	return false;
}


EyeSide oct_angio::AngioChart::eyeSide(void) const
{
	return getImpl().eyeSide;
}

int oct_angio::AngioChart::centerPosX(void) const
{
	return getImpl().centerPosX;
}

int oct_angio::AngioChart::centerPosY(void) const
{
	return getImpl().centerPosY;
}

float oct_angio::AngioChart::centerX(void) const
{
	return getImpl().centerX;
}

float oct_angio::AngioChart::centerY(void) const
{
	return getImpl().centerY;
}

float oct_angio::AngioChart::pixelsPerXmm(void) const
{
	return (getImpl().rangeX <= 0.0f ? 0.0f : getImpl().imageW / getImpl().rangeX);
}

float oct_angio::AngioChart::pixelsPerYmm(void) const
{
	return (getImpl().rangeY <= 0.0f ? 0.0f : getImpl().imageH / getImpl().rangeY);
}

cpp_util::CvImage & oct_angio::AngioChart::getAngioImage(void)
{
	return getImpl().image;
}


AngioChart::AngioChartImpl & oct_angio::AngioChart::getImpl(void) const
{
	return *d_ptr;
}
