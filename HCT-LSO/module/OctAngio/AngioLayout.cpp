#include "pch.h"
#include "AngioLayout.h"


using namespace cpp_util;
using namespace oct_angio;
using namespace std;
using namespace cv;


struct AngioLayout::AngioLayoutImpl
{
	int lines = 0;
	int repeats = 0;
	int points = 0;
	bool vertical = false;
	bool isDisc = false;

	float rangeX = 3.0f;
	float rangeY = 3.0f;
	float foveaX = 0.0f;
	float foveaY = 0.0f;

	AngioLayoutImpl() {

	};
};



AngioLayout::AngioLayout()
	: d_ptr(make_unique<AngioLayoutImpl>())
{
}


oct_angio::AngioLayout::AngioLayout(int lines, int points, int repeats, bool vertical)
	: d_ptr(make_unique<AngioLayoutImpl>())
{
	getImpl().lines = lines;
	getImpl().points = points;
	getImpl().repeats = repeats;
	getImpl().vertical = vertical;
}

AngioLayout::~AngioLayout()
{
	// Destructor should be defined for unique_ptr to delete AngioLayoutImpl as an incomplete type.
}


oct_angio::AngioLayout::AngioLayout(AngioLayout && rhs) = default;
AngioLayout & oct_angio::AngioLayout::operator=(AngioLayout && rhs) = default;

oct_angio::AngioLayout::AngioLayout(const AngioLayout & rhs)
	: d_ptr(make_unique<AngioLayoutImpl>(*rhs.d_ptr))
{
}


AngioLayout & oct_angio::AngioLayout::operator=(const AngioLayout & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_angio::AngioLayout::setupLayout(int lines, int points, int repeats, bool vertical)
{
	getImpl().lines = lines;
	getImpl().points = points;
	getImpl().repeats = repeats;
	getImpl().vertical = vertical;
}

void oct_angio::AngioLayout::setupRange(float rangeX, float rangeY, float centerX, float centerY, bool isDisc)
{
	getImpl().rangeX = rangeX;
	getImpl().rangeY = rangeY;
	getImpl().foveaX = centerX;
	getImpl().foveaY = centerY;
	getImpl().isDisc = isDisc;
}

int oct_angio::AngioLayout::numberOfLines(void) const
{
	return getImpl().lines;
}

int oct_angio::AngioLayout::numberOfPoints(void) const
{
	return getImpl().points;
}

int oct_angio::AngioLayout::numberOfRepeats(void) const
{
	return getImpl().repeats;
}

bool oct_angio::AngioLayout::isVerticalScan(void) const
{
	return getImpl().vertical;
}

bool oct_angio::AngioLayout::isDiscScan(void) const
{
	return getImpl().isDisc;
}

bool oct_angio::AngioLayout::isMacularScan(void) const
{
	return !isDiscScan();
}

bool oct_angio::AngioLayout::isFoveaCenter(void) const
{
	if (!isMacularScan()) {
		return false;
	}

	if (scanRangeX() > 6.0f || scanRangeY() > 6.0f) {
		return false;
	}

	if (foveaCenterX() <= 0.0f && foveaCenterY() <= 0.0f) {
		return false;
	}

	return true;
}

float oct_angio::AngioLayout::scanRangeX(void) const
{
	return getImpl().rangeX;
}

float oct_angio::AngioLayout::scanRangeY(void) const
{
	return getImpl().rangeY;
}

float oct_angio::AngioLayout::foveaCenterX(void) const
{
	return getImpl().foveaX;
}

float oct_angio::AngioLayout::foveaCenterY(void) const
{
	return getImpl().foveaY;
}

bool oct_angio::AngioLayout::getFoveaCenterInPixel(int& cx, int& cy) const
{
	if (isMacularScan()) {
		float rangeX = scanRangeX();
		float rangeY = scanRangeY();
		float fov_cx = foveaCenterX();
		float fov_cy = foveaCenterY();

		if (rangeX <= 0.0f || rangeY <= 0.0f || fov_cx <= 0.0f || fov_cy <= 0.0f) {
			cx = (int)(getWidth() * 0.5f);
			cy = (int)(getHeight() * 0.5f);
		}
		else {
			cx = (int)(getWidth() * (fov_cx / rangeX));
			cy = (int)(getHeight() * (fov_cy / rangeY));
		}
		return true;
	}
	return false;
}

bool oct_angio::AngioLayout::getFoveaRadiusInPixel(int& size1, int& size2) const
{
	if (isMacularScan()) {
		const float FOVEA_RADIUS_IN_MM = 0.25f;
		const float SCAN_RANGE_X = 4.5f;
		const float SCAN_RANGE_Y = 4.5f;

		float rangeX = scanRangeX();
		float rangeY = scanRangeY();
		float fov_cx = foveaCenterX();
		float fov_cy = foveaCenterY();

		if (rangeX <= 0.0f || rangeY <= 0.0f || fov_cx <= 0.0f || fov_cy <= 0.0f) {
			size1 = (int)((getWidth() / SCAN_RANGE_X) * FOVEA_RADIUS_IN_MM);
			size2 = (int)((getHeight() / SCAN_RANGE_Y) * FOVEA_RADIUS_IN_MM);
		}
		else {
			size1 = (int)((getWidth()/rangeX) * FOVEA_RADIUS_IN_MM);
			size2 = (int)((getHeight()/rangeY) * FOVEA_RADIUS_IN_MM);
		}
		return true;
	}
	return false;
}

int oct_angio::AngioLayout::getWidth(void) const
{
	return (isVerticalScan() ? numberOfLines() : numberOfPoints());
}

int oct_angio::AngioLayout::getHeight(void) const
{
	return (isVerticalScan() ? numberOfPoints() : numberOfLines());
}

int oct_angio::AngioLayout::getSize(void) const
{
	return getImpl().lines * getImpl().points;
}

AngioLayout::AngioLayoutImpl & oct_angio::AngioLayout::getImpl(void) const
{
	return *d_ptr;
}
