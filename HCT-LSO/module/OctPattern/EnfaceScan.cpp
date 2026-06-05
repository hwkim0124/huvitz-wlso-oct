#include "pch.h"
#include "EnfaceScan.h"
#include "PatternFrame.h"
#include "LineTrace.h"


using namespace oct_pattern;


struct EnfaceScan::EnfaceScanImpl
{
	EnfaceScanImpl() {

	}
};


EnfaceScan::EnfaceScan() :
	d_ptr(make_unique<EnfaceScanImpl>())
{
}


oct_pattern::EnfaceScan::~EnfaceScan() = default;
oct_pattern::EnfaceScan::EnfaceScan(EnfaceScan && rhs) = default;
EnfaceScan & oct_pattern::EnfaceScan::operator=(EnfaceScan && rhs) = default;


oct_pattern::EnfaceScan::EnfaceScan(const EnfaceScan & rhs)
	: d_ptr(make_unique<EnfaceScanImpl>(*rhs.d_ptr))
{
}


EnfaceScan & oct_pattern::EnfaceScan::operator=(const EnfaceScan & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool oct_pattern::EnfaceScan::buildPattern(void)
{
	clearPatternFrames();

	switch (getPatternType()) {
	using enum OctPatternType;
	case VERT_RASTER:
	case VERT_CUBE:
	case VERT_ANGIO:
		buildEnfaceCube(true);
		break;

	case RASTER:
	case CUBE:
	case ANGIO:
	default:
		buildEnfaceCube(false);
		break;
	}
	return true;
}


void oct_pattern::EnfaceScan::buildEnfaceCube(bool isVert)
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

	int numFrames = getNumberOfFramesFromPoints(numPoints, numLines, true);
	int maxFrames = (useFastRasters() ? (PATTERN_FRAMES_ENFACE_MAX/2) : PATTERN_FRAMES_ENFACE_MAX);
	if (numFrames <= 0 || numFrames > maxFrames) {
		return;
	}
	int frameSize = getFrameCapacityFromPoints(numPoints, numLines, true);

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

		// Increase trace id corresponding to frame as galvano profile index.
		LineTrace line;
		if (isVert) {
			if (useFastRasters()) {
				line.initVertRasterFast(TRACE_ID_ENFACE_CUBE + frameIdx * 2, // + frameIdx * 2,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
			else {
				line.initVertRaster(TRACE_ID_ENFACE_CUBE + frameIdx, // + frameIdx,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
		}
		else {
			if (useFastRasters()) {
				line.initHorzRasterFast(TRACE_ID_ENFACE_CUBE + frameIdx * 2, // + frameIdx * 2,
					OctPoint(x1, y1),
					OctPoint(x2, y2),
					numPoints);
			}
			else {
				line.initHorzRaster(TRACE_ID_ENFACE_CUBE + frameIdx, // + frameIdx,
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


EnfaceScan::EnfaceScanImpl & oct_pattern::EnfaceScan::getImpl(void) const
{
	return *d_ptr;
}


