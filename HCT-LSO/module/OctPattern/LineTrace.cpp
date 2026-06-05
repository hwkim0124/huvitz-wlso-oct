#include "pch.h"
#include "LineTrace.h"

using namespace oct_pattern;


struct LineTrace::LineTraceImpl
{
	int traceId;
	LineTraceType type;
	OctRoute path;
	OctScanSpeed speed;

	int numScanPoints;
	int numForePaddings;
	int numPostPaddings;
	int numRepeats;

	int16_t  galvPosX1[8192];
	int16_t  galvPosY1[8192];
	int16_t  galvPosX2[8192];
	int16_t  galvPosY2[8192];

	int countOfPosX;
	int countOfPosY;
	bool hidden;
	bool lineHD;
	

	LineTraceImpl() : traceId(0), type(LineTraceType::Line),
		numScanPoints(0), numRepeats(1),
		numForePaddings(TRIGGER_FORE_PADDING_POINTS), numPostPaddings(TRIGGER_POST_PADDING_POINTS),
		galvPosX1{ 0 }, galvPosY1{ 0 }, galvPosX2{ 0 }, galvPosY2{ 0 },
		countOfPosX(0), countOfPosY(0), speed(OctScanSpeed::FASTER), hidden(false), lineHD(false)
	{
	}

};


LineTrace::LineTrace() :
	d_ptr(make_unique<LineTraceImpl>())
{
}


oct_pattern::LineTrace::~LineTrace() = default;
oct_pattern::LineTrace::LineTrace(LineTrace && rhs) = default;
LineTrace & oct_pattern::LineTrace::operator=(LineTrace && rhs) = default;


oct_pattern::LineTrace::LineTrace(const LineTrace & rhs)
	: d_ptr(make_unique<LineTraceImpl>(*rhs.d_ptr))
{
}


LineTrace & oct_pattern::LineTrace::operator=(const LineTrace & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_pattern::LineTrace::initLine(int traceId, OctPoint start, OctPoint end, int numPoints, int numRepeats, int forePadds, int postPadds)
{
	getImpl().traceId = traceId;
	getImpl().type = LineTraceType::Line;
	getImpl().path.setLine(start, end);
	getImpl().numScanPoints = numPoints;
	getImpl().numRepeats = numRepeats;
	setPaddings(forePadds, postPadds);
	return;
}


void oct_pattern::LineTrace::initCircle(int traceId, OctPoint start, OctPoint end, int numPoints, int numRepeats, int forePadds, int postPadds)
{
	getImpl().traceId = traceId;
	getImpl().type = LineTraceType::Circle;
	getImpl().path.setLine(start, end);
	getImpl().numScanPoints = numPoints;
	getImpl().numRepeats = numRepeats;
	setPaddings(forePadds, postPadds);
	return;
}


void oct_pattern::LineTrace::initHorzRaster(int traceId, OctPoint start, OctPoint end, int numPoints, int numRepeats, int forePadds, int postPadds)
{
	getImpl().traceId = traceId;
	getImpl().type = LineTraceType::HorzRaster;
	getImpl().path.setLine(start, end);
	getImpl().numScanPoints = numPoints;
	getImpl().numRepeats = numRepeats;
	setPaddings(forePadds, postPadds);
	return;
}


void oct_pattern::LineTrace::initVertRaster(int traceId, OctPoint start, OctPoint end, int numPoints, int numRepeats, int forePadds, int postPadds)
{
	getImpl().traceId = traceId;
	getImpl().type = LineTraceType::VertRaster;
	getImpl().path.setLine(start, end);
	getImpl().numScanPoints = numPoints;
	getImpl().numRepeats = numRepeats;
	setPaddings(forePadds, postPadds);
	return;
}


void oct_pattern::LineTrace::initHorzRasterFast(int traceId, OctPoint start, OctPoint end, int numPoints, int numRepeats, int forePadds, int postPadds)
{
	getImpl().traceId = traceId;
	getImpl().type = LineTraceType::HorzRasterFast;
	getImpl().path.setLine(start, end);
	getImpl().numScanPoints = numPoints;
	getImpl().numRepeats = numRepeats;
	setPaddings(forePadds, postPadds);
	return;
}


void oct_pattern::LineTrace::initVertRasterFast(int traceId, OctPoint start, OctPoint end, int numPoints, int numRepeats, int forePadds, int postPadds)
{
	getImpl().traceId = traceId;
	getImpl().type = LineTraceType::VertRasterFast;
	getImpl().path.setLine(start, end);
	getImpl().numScanPoints = numPoints;
	getImpl().numRepeats = numRepeats;
	setPaddings(forePadds, postPadds);
	return;
}


void oct_pattern::LineTrace::setPaddings(int forePadds, int postPadds, int numPoints)
{
	if (numPoints != 0) {
		forePadds = (int)(numPoints * TRIGGER_FORE_PADDING_RATIO);
		postPadds = (int)(numPoints * TRIGGER_POST_PADDING_RATIO);
		// forePadds = max(forePadds, TRIGGER_FORE_PADDING_POINTS);
		// postPadds = max(postPadds, TRIGGER_POST_PADDING_POINTS);
	}

	getImpl().numForePaddings = forePadds;
	getImpl().numPostPaddings = postPadds;
	// LogD() << "Set paddings: " << forePadds << ", " << postPadds << ", " << numPoints;
	return;
}


OctRoute oct_pattern::LineTrace::getRouteOfScan(void) const
{
	OctRoute route;
	if (isCircle()) {
		// route.setCircle()
		route.setLine(getStartX(), getStartY(), getCloseX(), getCloseY());
	}
	else {
		route.setLine(getStartX(), getStartY(), getCloseX(), getCloseY());
	}
	return route;
}


LineTraceType oct_pattern::LineTrace::getTraceType(void) const
{
	return getImpl().type;
}


std::string oct_pattern::LineTrace::getTraceTypeStr(void) const
{
	if (isLine()) {
		return string("line");
	}
	if (isCircle()) {
		return string("circle");
	}
	if (isRasterX()) {
		return string("rasterX");
	}
	if (isRasterY()) {
		return string("rasterY");
	}
	if (isRasterFastX()) {
		return string("rasterFastX");
	}
	if (isRasterFastY()) {
		return string("rasterFastY");
	}
	return string("unknown");
}


int oct_pattern::LineTrace::getTraceId(void) const
{
	return getImpl().traceId;
}


int oct_pattern::LineTrace::getForePaddings(void) const
{
	return getImpl().numForePaddings;
}


int oct_pattern::LineTrace::getPostPaddings(void) const
{
	return getImpl().numPostPaddings;
}


int oct_pattern::LineTrace::getNumberOfScanPoints(bool repeats) const
{
	if (!repeats) {
		return getImpl().numScanPoints;
	}
	else {
		return (getImpl().numScanPoints * getNumberOfRepeats());
	}
}


int oct_pattern::LineTrace::getNumberOfRepeats(void) const
{
	return getImpl().numRepeats;
}


void oct_pattern::LineTrace::setScanSpeed(OctScanSpeed speed)
{
	d_ptr->speed = speed;
	return;
}


OctScanSpeed oct_pattern::LineTrace::getScanSpeed(void)
{
	return d_ptr->speed;
}


float oct_pattern::LineTrace::getStartX(void) const
{
	return getImpl().path.start()._x;
}


float oct_pattern::LineTrace::getStartY(void) const
{
	return getImpl().path.start()._y;
}


float oct_pattern::LineTrace::getCloseX(void) const
{
	return getImpl().path.close()._x;
}


float oct_pattern::LineTrace::getCloseY(void) const
{
	return getImpl().path.close()._y;
}


short oct_pattern::LineTrace::getPositionX(int index) const
{
	if (index < 0 || index >= getImpl().countOfPosX) {
		return 0;
	}
	return getImpl().galvPosX1[index];
}


short oct_pattern::LineTrace::getPositionY(int index) const
{
	if (index < 0 || index >= getImpl().countOfPosY) {
		return 0;
	}
	return getImpl().galvPosY1[index];
}

short oct_pattern::LineTrace::getPositionFirstX(void) const
{
	auto index = 0;
	auto pos = getPositionX(index);
	return pos;
}

short oct_pattern::LineTrace::getPositionFirstY(void) const
{
	auto index = 0;
	auto pos = getPositionY(index);
	return pos;
}

short oct_pattern::LineTrace::getPositionLastX(void) const
{
	auto index = getImpl().countOfPosX - 1;
	auto pos = getPositionX(index);
	return pos;
}

short oct_pattern::LineTrace::getPositionLastY(void) const
{
	auto index = getImpl().countOfPosY - 1;
	auto pos = getPositionY(index);
	return pos;
}


short * oct_pattern::LineTrace::getGalvanoPositionsX(bool reversed) const
{
	if (reversed) {
		getImpl().galvPosX2;
	}
	return getImpl().galvPosX1;
}


short * oct_pattern::LineTrace::getGalvanoPositionsY(bool reversed) const
{
	if (reversed) {
		getImpl().galvPosY2;
	}
	return getImpl().galvPosY1;
}


void oct_pattern::LineTrace::setGalvanoPositionsX(short * posXs, short count)
{
	/*
	if (count > 0) {
		memcpy(getImpl().galvPosXs, posXs, sizeof(short)*count);
	}
	*/
	for (int i = 0; i < count; i++) {
		d_ptr->galvPosX1[i] = posXs[i];
		d_ptr->galvPosX2[i] = posXs[count - i - 1];
	}

	getImpl().countOfPosX = count;
	return;
}


void oct_pattern::LineTrace::setGalvanoPositionsY(short * posYs, short count)
{
	/*
	if (count > 0) {
		memcpy(getImpl().galvPosYs, posYs, sizeof(short)*count);
	}
	*/
	for (int i = 0; i < count; i++) {
		d_ptr->galvPosY1[i] = posYs[i];
		d_ptr->galvPosY2[i] = posYs[count - i - 1];
	}

	getImpl().countOfPosY = count;
	return;
}


int oct_pattern::LineTrace::getCountOfPositionsX(void) const
{
	return getImpl().countOfPosX;
}


int oct_pattern::LineTrace::getCountOfPositionsY(void) const
{
	return getImpl().countOfPosY;
}


bool oct_pattern::LineTrace::isLine(void) const
{
	return (getImpl().type == LineTraceType::Line);
}


bool oct_pattern::LineTrace::isCircle(void) const
{
	return (getImpl().type == LineTraceType::Circle);
}


bool oct_pattern::LineTrace::isRasterX(void) const
{
	return (getImpl().type == LineTraceType::HorzRaster);
}


bool oct_pattern::LineTrace::isRasterY(void) const
{
	return (getImpl().type == LineTraceType::VertRaster);
}


bool oct_pattern::LineTrace::isRasterFastX(void) const
{
	return (getImpl().type == LineTraceType::HorzRasterFast);
}


bool oct_pattern::LineTrace::isRasterFastY(void) const
{
	return (getImpl().type == LineTraceType::VertRasterFast);
}

bool oct_pattern::LineTrace::isHidden(void) const
{
	return getImpl().hidden;
}

bool oct_pattern::LineTrace::isLineHD(void) const
{
	return getImpl().lineHD;
}

void oct_pattern::LineTrace::setHidden(bool flag)
{
	getImpl().hidden = flag;
	return;
}

void oct_pattern::LineTrace::setLineHD(bool flag)
{
	getImpl().lineHD = flag;
	return;
}


LineTrace::LineTraceImpl & oct_pattern::LineTrace::getImpl(void) const
{
	return *d_ptr;
}

