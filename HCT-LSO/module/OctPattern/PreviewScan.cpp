#include "pch.h"
#include "PreviewScan.h"
#include "PatternFrame.h"
#include "LineTrace.h"


#include "CppUtil2.h"

using namespace oct_pattern;


struct PreviewScan::PreviewScanImpl
{
	PreviewScanImpl() {
	}
};


PreviewScan::PreviewScan() :
	d_ptr(make_unique<PreviewScanImpl>())
{
}


oct_pattern::PreviewScan::~PreviewScan() = default;
oct_pattern::PreviewScan::PreviewScan(PreviewScan && rhs) = default;
PreviewScan & oct_pattern::PreviewScan::operator=(PreviewScan && rhs) = default;


oct_pattern::PreviewScan::PreviewScan(const PreviewScan & rhs)
	: d_ptr(make_unique<PreviewScanImpl>(*rhs.d_ptr))
{
}


PreviewScan & oct_pattern::PreviewScan::operator=(const PreviewScan & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}



bool oct_pattern::PreviewScan::buildPattern(bool hidden)
{
	clearPatternFrames();

	if (usePreviewPattern())
	{
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
		
		if (hidden) {
			buildPreviewHidden();
		}
	}
	else
	{
		// Build line traces for preview and measure scan pattern.
		switch (getPatternType()) {
		using enum OctPatternType;
		case POINT:
			buildPreviewPoint();
			break;
		case LINE:
			buildPreviewLine(false);
			break;
		case VERT_LINE:
			buildPreviewLine(true);
			break;
		case CROSS:
			buildPreviewCross();
			break;
		case MULTI_LINE:
			buildPreviewMultiLine(false);
			break;
		case VERT_MULTI_LINE:
			buildPreviewMultiLine(true);
			break;
		default:
			buildPreviewLine(false);
			break;
		}

		if (hidden) {
			buildPreviewHidden();
		}
	}
	return true;
}


void oct_pattern::PreviewScan::buildPatternPoint(void)
{
	buildPreviewPoint();
	return;
}


void oct_pattern::PreviewScan::buildPatternLine(bool isVert)
{
	buildPreviewLine(isVert);
	return;
}


void oct_pattern::PreviewScan::buildPatternCircle(void)
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

	LineTrace line;
	line.initCircle(TRACE_ID_MEASURE_CIRCLE,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);
	return;
}


void oct_pattern::PreviewScan::buildPatternCross(void)
{
	float radiusX, radiusY;

	radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	//radiusX *= -1.0f;

	float xStart = PATTERN_SCAN_CENTER_X - radiusX;
	float xClose = PATTERN_SCAN_CENTER_X + radiusX;
	float yStart = PATTERN_SCAN_CENTER_Y - radiusY;
	float yClose = PATTERN_SCAN_CENTER_Y + radiusY;

	float xInterval = getScanLineSpace() * getScanScaleX();
	float yInterval = getScanLineSpace() * getScanScaleY();

	int numPoints = getNumberOfScanPoints();
	int numLines = getNumberOfScanLines();
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
		y1 = PATTERN_SCAN_CENTER_Y + yInterval * (lineIdx - halfSize / 2);
		y2 = PATTERN_SCAN_CENTER_Y + yInterval * (lineIdx - halfSize / 2);

		x1 += getScanRangeOffsetX(true);
		x2 += getScanRangeOffsetX(true);
		y1 += getScanRangeOffsetY(true);
		y2 += getScanRangeOffsetY(true);

		LineTrace line;
		line.initLine(TRACE_ID_MEASURE_CROSS + lineIdx,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints);

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
			numPoints);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}
	return;
}


void oct_pattern::PreviewScan::buildPatternRadial(void)
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
			numPoints);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}
	return;
}


void oct_pattern::PreviewScan::buildPatternRaster(bool isVert)
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

	float yInterval = (yClose - yStart) / (numLines - 1);
	float xInterval = (xClose - xStart) / (numLines - 1);

	float x1, y1, x2, y2;
	int traceId;

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

		traceId = (TRACE_ID_MEASURE_RASTER + lineIdx) % TRACE_ID_MAX_LINES;

		LineTrace line;
		line.initLine(traceId,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}
	return;
}


void oct_pattern::PreviewScan::buildPatternCube(bool isVert)
{
	float radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	float radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();

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

	int numFrames = getNumberOfFramesFromPoints(numPoints, numLines, numRepeats);
	int maxFrames = (useFastRasters() ? TRACE_ID_MAX_REPEATS : TRACE_ID_MAX_LINES);
	if (numFrames <= 0 || numFrames > maxFrames) {
		return;
	}
	int frameSize = getFrameCapacityFromPoints(numPoints, numLines, numRepeats);

	float yInterval = (yClose - yStart) / (numLines - 1);
	float xInterval = (xClose - xStart) / (numLines - 1);

	float x1, y1, x2, y2;
	double angle = getScanAngle();
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

		if (angle > 0.0) {
			double radian = degreeToRadian(angle);
			double radius = (isVert ? radiusY : radiusX);
			xStart = (float)(x1 * cos(radian) * -1.0);
			yStart = (float)(y1 * sin(radian) * -1.0);
			xClose = (float)(x2 * cos(radian) * +1.0);
			yClose = (float)(y2 * sin(radian) * +1.0);
		}

		if (lineIdx != 0 && (lineIdx % frameSize) == 0) {
			frameIdx++;
		}

		// Increase trace id corresponding to frame as galvano profile index.
		LineTrace line;
		if (isVert) {
			if (useFastRasters()) {
				line.initVertRasterFast(TRACE_ID_PREVIEW_CUBE, // + frameIdx * 2,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
			else {
				line.initVertRaster(TRACE_ID_PREVIEW_CUBE, // + frameIdx,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
		}
		else {
			if (useFastRasters()) {
				line.initHorzRasterFast(TRACE_ID_PREVIEW_CUBE, // + frameIdx * 2,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
			else {
				line.initHorzRaster(TRACE_ID_PREVIEW_CUBE, // + frameIdx,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
		}

		PatternFrame* pFrame = getPatternFrame(frameIdx);
		pFrame->addLineTrace(line);
	}
	return;
}


void oct_pattern::PreviewScan::buildPreviewPoint(void)
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

	LineTrace line;
	line.initLine(TRACE_ID_PREVIEW_POINT,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints); // PREVIEW_LINE_NUM_POINTS);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);
	return;
}


void oct_pattern::PreviewScan::buildPreviewHidden(void)
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

	LineTrace line;
	line.initLine(TRACE_ID_PREVIEW_HIDDEN,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints); // PREVIEW_LINE_NUM_POINTS);

	line.setHidden(true);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);
	return;
}


void oct_pattern::PreviewScan::buildPreviewLine(bool isVert, int lineCount)
{
	float radiusX, radiusY;

	if (isVert) {
		radiusX = 0.0f ;
		radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();
	}
	else {
		radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
		radiusY = 0.0f ;
	}

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

	int numPoints = getNumberOfScanPoints(); // PATTERN_PREVIEW_LINE_NUM_POINTS; // getNumberOfScanPoints();
	setNumberOfScanPoints(numPoints);

	LineTrace line;
	line.initLine(TRACE_ID_PREVIEW_LINE + lineCount,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints); // PREVIEW_LINE_NUM_POINTS);

	PatternFrame frame;
	frame.addLineTrace(line);
	addPatternFrame(frame);

	// Scan speed for preview is fixed as the fastest. 
	const int NUM_LINES_HD = 5; 

	LineTrace line2;
	line2.initLine(TRACE_ID_PREVIEW_LINE_HD + lineCount,
		OctPoint(xStart, yStart),
		OctPoint(xClose, yClose),
		numPoints, 
		NUM_LINES_HD); // PREVIEW_LINE_NUM_POINTS);
	line2.setLineHD(true);

	PatternFrame frame2;
	frame2.addLineTrace(line2);
	addPatternFrame(frame2);
	return;
}


void oct_pattern::PreviewScan::buildPreviewCircle(void)
{
	buildPreviewLine(false, 0);
	return;
}


void oct_pattern::PreviewScan::buildPreviewCross(void)
{
	buildPreviewLine(false, 0);
	buildPreviewLine(true, 1);
	return;
}


void oct_pattern::PreviewScan::buildPreviewRadial(void)
{
	buildPreviewCross();
	return;
}


void oct_pattern::PreviewScan::buildPreviewRaster(bool isVert)
{
	buildPreviewLine(isVert);
	return;
}


void oct_pattern::PreviewScan::buildPreviewCube(bool isVert)
{
	buildPreviewLine(isVert);
	return;
}

void oct_pattern::PreviewScan::buildPreviewMultiLine(bool isVert)
{
	float radiusX, radiusY;

	radiusX = (getScanRangeX() / 2.0f) * getScanScaleX();
	radiusY = (getScanRangeY() / 2.0f) * getScanScaleY();

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

	float xCenter = (xStart + xClose) / 2.0f;
	float yCenter = (yStart + yClose) / 2.0f;

	int numPoints = getNumberOfScanPoints(); 
	setNumberOfScanPoints(numPoints);

	// Scan speed for preview is fixed as the fastest. 
	const int NUM_REPEATS_HD = 5;
	const int NUM_LINES_MULTI = 4;

	float x1, x2, y1, y2;
	for (int lidx = 0; lidx < NUM_LINES_MULTI; lidx++)
	{
		if (lidx == 0) {
			x1 = (isVert ? xStart : xStart);
			x2 = (isVert ? xStart : xClose);
			y1 = (isVert ? yStart : yStart);
			y2 = (isVert ? yClose : yStart);
		}
		else if (lidx == 1) {
			x1 = (isVert ? xCenter : xStart);
			x2 = (isVert ? xCenter : xClose);
			y1 = (isVert ? yStart : yCenter);
			y2 = (isVert ? yClose : yCenter);
		}
		else if (lidx == 2) {
			x1 = (isVert ? xClose : xStart);
			x2 = (isVert ? xClose : xClose);
			y1 = (isVert ? yStart : yClose);
			y2 = (isVert ? yClose : yClose);
		}
		else {
			x1 = (isVert ? xStart : xCenter);
			x2 = (isVert ? xClose : xCenter);
			y1 = (isVert ? yCenter : yStart);
			y2 = (isVert ? yCenter : yClose);
		}

		LineTrace line;
		line.initLine(TRACE_ID_PREVIEW_MULTI + lidx,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}

	for (int lidx = 0; lidx < NUM_LINES_MULTI; lidx++)
	{
		if (lidx == 0) {
			x1 = (isVert ? xStart : xStart);
			x2 = (isVert ? xStart : xClose);
			y1 = (isVert ? yStart : yStart);
			y2 = (isVert ? yClose : yStart);
		}
		else if (lidx == 1) {
			x1 = (isVert ? xCenter : xStart);
			x2 = (isVert ? xCenter : xClose);
			y1 = (isVert ? yStart : yCenter);
			y2 = (isVert ? yClose : yCenter);
		}
		else if (lidx == 2) {
			x1 = (isVert ? xClose : xStart);
			x2 = (isVert ? xClose : xClose);
			y1 = (isVert ? yStart : yClose);
			y2 = (isVert ? yClose : yClose);
		}
		else {
			x1 = (isVert ? xStart : xCenter);
			x2 = (isVert ? xClose : xCenter);
			y1 = (isVert ? yCenter : yStart);
			y2 = (isVert ? yCenter : yClose);
		}

		LineTrace line;
		line.initLine(TRACE_ID_PREVIEW_MULTI_HD + lidx,
			OctPoint(x1, y1),
			OctPoint(x2, y2),
			numPoints, 
			NUM_REPEATS_HD);
		line.setLineHD(true);

		PatternFrame frame;
		frame.addLineTrace(line);
		addPatternFrame(frame);
	}
	return;
}

PreviewScan::PreviewScanImpl & oct_pattern::PreviewScan::getImpl(void) const
{
	return *d_ptr;
}


