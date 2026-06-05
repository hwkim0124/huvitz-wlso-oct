#include "pch.h"
#include "MeasureScan.h"
#include "PatternFrame.h"
#include "LineTrace.h"

using namespace oct_pattern;


struct MeasureScan::MeasureScanImpl
{
	MeasureScanImpl() {

	}
};


MeasureScan::MeasureScan() :
	d_ptr(make_unique<MeasureScanImpl>())
{
}


oct_pattern::MeasureScan::~MeasureScan() = default;
oct_pattern::MeasureScan::MeasureScan(MeasureScan && rhs) = default;
MeasureScan & oct_pattern::MeasureScan::operator=(MeasureScan && rhs) = default;


oct_pattern::MeasureScan::MeasureScan(const MeasureScan & rhs)
	: d_ptr(make_unique<MeasureScanImpl>(*rhs.d_ptr))
{
}


MeasureScan & oct_pattern::MeasureScan::operator=(const MeasureScan & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool oct_pattern::MeasureScan::buildPattern(bool hidden)
{
	clearPatternFrames();

	// Build line traces for preview and measure scan pattern.
	switch (getPatternType()) {
	using enum OctPatternType;
	case POINT:
		buildPatternPoint();
		break;
	case LINE:
		buildPatternLine(false);
		break;
	case VERT_LINE:
		buildPatternLine(true);
		break;
	case CIRCLE:
		buildPatternCircle();
		break;
	case CROSS:
		buildPatternCross();
		break;
	case RADIAL: 
		buildPatternRadial();
		break;
	case RASTER:
		buildPatternRaster(false);
		break;
	case CUBE:
	case ANGIO:
		buildPatternCube(false);
		break;
	case VERT_RASTER:
		buildPatternRaster(true);
		break;
	case VERT_CUBE:
	case VERT_ANGIO:
		buildPatternCube(true);
		break;
	default:
		return false;
	}
	return true;
}


void oct_pattern::MeasureScan::buildPatternPoint(void)
{
	float xStart = PATTERN_SCAN_CENTER_X;
	float xClose = PATTERN_SCAN_CENTER_X;
	float yStart = PATTERN_SCAN_CENTER_Y;
	float yClose = PATTERN_SCAN_CENTER_Y;

	xStart += getScanRangeOffsetX(true);
	xClose += getScanRangeOffsetX(true);
	yStart += getScanRangeOffsetY(true);
	yClose += getScanRangeOffsetY(true);

	int numPoints = getNumberOfScanPoints();
	int numRepeats = getNumberOfScanOverlaps();

	LineTrace line;
	line.initLine(TRACE_ID_MEASURE_LINE,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints, 
		numRepeats);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);
	return;
}


void oct_pattern::MeasureScan::buildPatternLine(bool isVert, int lineCount)
{
	float radiusX, radiusY;

	if (isVert) {
		radiusX = 0.0f;
		radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	}
	else {
		radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
		radiusY = 0.0f;
	}

	//radiusX *= -1.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY;

	double angle = getScanAngle();
	if (angle > 0.0) {
		double radian = degreeToRadian(angle);
		double radius = (isVert ? radiusY : radiusX);
		xStart = (float)(radius * cos(radian) * -1.0);
		yStart = (float)(radius * sin(radian) * -1.0);
		xClose = (float)(radius * cos(radian) * +1.0);
		yClose = (float)(radius * sin(radian) * +1.0);
	}

	xStart += getScanRangeOffsetX(true);
	xClose += getScanRangeOffsetX(true);
	yStart += getScanRangeOffsetY(true);
	yClose += getScanRangeOffsetY(true);

	int numPoints = getNumberOfScanPoints();
	int numRepeats = getNumberOfScanOverlaps();

	LineTrace line;
	line.initLine(TRACE_ID_MEASURE_LINE + lineCount,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints, 
		numRepeats); // PREVIEW_LINE_NUM_POINTS);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);
	return;
}


void oct_pattern::MeasureScan::buildPatternCircle(void)
{
	float radiusX, radiusY;

	radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	//radiusX *= -1.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY;

	xStart += getScanRangeOffsetX(true);
	xClose += getScanRangeOffsetX(true);
	yStart += getScanRangeOffsetY(true);
	yClose += getScanRangeOffsetY(true);

	int numPoints = getNumberOfScanPoints();
	int numRepeats = getNumberOfScanOverlaps();

	LineTrace line;
	line.initCircle(TRACE_ID_MEASURE_CIRCLE,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints,
		numRepeats);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);
	return;
}


void oct_pattern::MeasureScan::buildPatternCross(void)
{
	float radiusX, radiusY;

	radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	//radiusX *= -1.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY ;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY ;

	float xInterval = getScanLineSpace() * getScanScaleX();
	float yInterval = getScanLineSpace() * getScanScaleY();

	int numPoints = getNumberOfScanPoints();
	int numRepeats = getNumberOfScanOverlaps();
	int numLines = getNumberOfScanLines() ;
	int numFrames = numLines;
	int halfSize = numLines / 2;

	float x1, y1, x2, y2;
	double angle = getScanAngle();
	int frameIdx = 0;
	int lineIdx = 0;

	// Horizontal lines.
	for (; lineIdx < halfSize; lineIdx++)
	{
		x1 = xStart;
		x2 = xClose;
		y1 = PATTERN_SCAN_CENTER_Y + yInterval * (lineIdx - halfSize/2);
		y2 = PATTERN_SCAN_CENTER_Y + yInterval * (lineIdx - halfSize/2);

		x1 += getScanRangeOffsetX(true);
		x2 += getScanRangeOffsetX(true);
		y1 += getScanRangeOffsetY(true);
		y2 += getScanRangeOffsetY(true);

		LineTrace line;
		line.initLine(TRACE_ID_MEASURE_CROSS + lineIdx,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints,
			numRepeats); 

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}

	// Vertical lines.
	for (; lineIdx < numLines; lineIdx++)
	{
		x1 = PATTERN_SCAN_CENTER_X + xInterval * ((lineIdx - halfSize) - halfSize / 2);
		x2 = PATTERN_SCAN_CENTER_X + xInterval * ((lineIdx - halfSize) - halfSize / 2);
		y1 = yStart;
		y2 = yClose;

		x1 += getScanRangeOffsetX(true);
		x2 += getScanRangeOffsetX(true);
		y1 += getScanRangeOffsetY(true);
		y2 += getScanRangeOffsetY(true);

		LineTrace line;
		line.initLine(TRACE_ID_MEASURE_CROSS + lineIdx,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints,
			numRepeats);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}
	return;
}


void oct_pattern::MeasureScan::buildPatternRadial(void)
{
	float radiusX, radiusY;

	radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	//radiusX *= -1.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY;

	int numPoints = getNumberOfScanPoints();
	int numRepeats = getNumberOfScanOverlaps();
	int numLines = getNumberOfScanLines();

	double angle = 180.0 / numLines;
	float x1, y1, x2, y2;

	for (int lineIdx = 0; lineIdx < numLines; lineIdx++)
	{
		double degree = angle * lineIdx;
		double radian = degreeToRadian(degree);
		x1 = (float)(radiusX * cos(radian) * -1.0);
		y1 = (float)(radiusY * sin(radian) * -1.0);
		x2 = (float)(radiusX * cos(radian) * +1.0);
		y2 = (float)(radiusY * sin(radian) * +1.0);

		x1 += getScanRangeOffsetX(true);
		x2 += getScanRangeOffsetX(true);
		y1 += getScanRangeOffsetY(true);
		y2 += getScanRangeOffsetY(true);

		LineTrace line;
		line.initLine(TRACE_ID_MEASURE_RADIAL + lineIdx,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints,
			numRepeats);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}
	return;
}


void oct_pattern::MeasureScan::buildPatternRaster(bool isVert)
{
	float radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	float radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	//radiusX *= -1.0f;

	// For Angio point profiles.
	// radiusX = radiusY = 0.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY;

	xStart += getScanRangeOffsetX(true);
	xClose += getScanRangeOffsetX(true);
	yStart += getScanRangeOffsetY(true);
	yClose += getScanRangeOffsetY(true);

	int numPoints = getNumberOfScanPoints();
	int numLines = getNumberOfScanLines();
	int numRepeats = getNumberOfScanOverlaps();

	float yInterval = (yClose - yStart) / (numLines - 1);
	float xInterval = (xClose - xStart) / (numLines - 1);

	float x1, y1, x2, y2;
	int traceId;

	int frameIdx = 0;
	int numFrames = numLines;
	resizePatternFrames(numFrames);

	for (int lineIdx = 0; lineIdx < numLines; lineIdx++) {
		if (isVert) {
			x1 = xStart + xInterval*lineIdx;
			y1 = yStart;
			x2 = xStart + xInterval*lineIdx;
			y2 = yClose;
		}
		else {
			x1 = xStart;
			y1 = yStart + yInterval*lineIdx;
			x2 = xClose;
			y2 = yStart + yInterval*lineIdx;
		}

		if (lineIdx != 0) {
			frameIdx++;
		}

		traceId = (TRACE_ID_MEASURE_RASTER + lineIdx) % TRACE_ID_MAX_LINES;

		/*
		if (lineIdx % 2 != 0) {
			if (isVert) {
				temp = y1;
				y1 = y2;
				y2 = temp;
			}
			else {
				temp = x1;
				x1 = x2;
				x2 = temp;
			}
		}
		*/

		LineTrace line;
		/*
		if (useForeDistance()) {
			// Calculate fore paddings corresponding to 0.6mm of galvano accerlation distance prior to measure points.  
			float dist = max(fabs(xClose - xStart), fabs(yClose - yStart));
			float rate = numPoints / dist;
			int numForePadds = (int)(rate * 0.60f);

			line.initLine(traceId,
				OctPoint(x1, y1),
				OctPoint(x2, y2),
				numPoints,
				numRepeats, 
				numForePadds);
		}
		else {
			line.initLine(traceId,
				OctPoint(x1, y1),
				OctPoint(x2, y2),
				numPoints,
				numRepeats);
		}

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
		*/

		if (isVert) {
			line.initVertRaster(TRACE_ID_MEASURE_CUBE + frameIdx * 1, // + frameIdx,
				OctPoint(x1, y1),
				OctPoint(x2, y2),
				numPoints,
				numRepeats);
		}
		else {
			line.initHorzRaster(TRACE_ID_MEASURE_CUBE + frameIdx * 1, // + frameIdx,
				OctPoint(x1, y1),
				OctPoint(x2, y2),
				numPoints,
				numRepeats);
		}

		PatternFrame* pFrame = getPatternFrame(frameIdx);
		pFrame->addLineTrace(line);
	}
	return;
}


void oct_pattern::MeasureScan::buildPatternCube(bool isVert)
{
	float radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	float radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	//radiusX *= -1.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY;

	xStart += getScanRangeOffsetX(true);
	xClose += getScanRangeOffsetX(true);
	yStart += getScanRangeOffsetY(true);
	yClose += getScanRangeOffsetY(true);

	int numPoints = getNumberOfScanPoints();
	int numLines = getNumberOfScanLines();
	int numRepeats = getNumberOfScanOverlaps();

	int numFrames = getNumberOfFramesFromPoints(numPoints, numLines, numRepeats, false);
	int maxFrames = (useFastRasters() ? (PATTERN_FRAMES_MEASURE_MAX / 2) : PATTERN_FRAMES_MEASURE_MAX);
	if (numFrames <= 0 || numFrames > maxFrames) {
		// return;
	}
	int frameSize = getFrameCapacityFromPoints(numPoints, numLines, numRepeats, false);

	LogD() << "Pattern Cube, numFrames: " << numFrames << ", frameSize: " << frameSize << ", numLines: " << numLines;

	float yInterval = (yClose - yStart) / (numLines - 1);
	float xInterval = (xClose - xStart) / (numLines - 1);

	float x1, y1, x2, y2;
	int frameIdx = 0;

	resizePatternFrames(numFrames);

	for (int lineIdx = 0; lineIdx < numLines; lineIdx++) {
		if (isVert) {
			x1 = xStart + xInterval*lineIdx;
			y1 = yStart;
			x2 = xStart + xInterval*lineIdx;
			y2 = yClose;
		}
		else {
			x1 = xStart;
			y1 = yStart + yInterval*lineIdx;
			x2 = xClose;
			y2 = yStart + yInterval*lineIdx;
		}

		if (lineIdx != 0 && (lineIdx % frameSize) == 0) {
			frameIdx++;
		}

		int traceId = 0;
		if (useFastRasters()) {
			traceId = TRACE_ID_MEASURE_CUBE + frameIdx * 2;
		}
		else {
			traceId = TRACE_ID_MEASURE_CUBE + frameIdx * 1;
		}

		if (traceId >= TRACE_ID_MAX_LINES) {
			traceId = traceId % TRACE_ID_MAX_LINES;
		}

		// Increase trace id corresponding to frame as galvano profile index.
		LineTrace line;
		if (isVert) {
			if (useFastRasters()) {
				line.initVertRasterFast(traceId, // + frameIdx * 2,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints, 
					numRepeats);
			}
			else {
				line.initVertRaster(traceId, // + frameIdx,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints,
					numRepeats);
			}
		}
		else {
			if (useFastRasters()) {
				line.initHorzRasterFast(traceId, // + frameIdx * 2,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints, 
					numRepeats);
			}
			else {
				line.initHorzRaster(traceId, // + frameIdx,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints, 
					numRepeats);
			}
		}

		PatternFrame* pFrame = getPatternFrame(frameIdx);
		pFrame->addLineTrace(line);
	}
	return;
}


MeasureScan::MeasureScanImpl & oct_pattern::MeasureScan::getImpl(void) const
{
	return *d_ptr;
}


