#include "pch.h"
#include "PatternScan.h"
#include "PatternFrame.h"
#include "LineTrace.h"

using namespace oct_pattern;


struct PatternScan::PatternScanImpl
{
	EyeRegion domain;
	OctPatternType type;
	LineTraceVect lines;
	PatternFrameVect frames;

	int numPoints;
	int numLines;
	int overlaps;
	float rangeX;
	float rangeY;
	float lineSpace;
	float angle;
	float offsetX;
	float offsetY;
	float scaleX;
	float scaleY;
	float posMoveX;
	float posMoveY;

	bool usePattern;
	bool useFaster;
	bool useForeDist;

	PatternScanImpl() : type(OctPatternType::LINE), domain(EyeRegion::MACULAR), 
						numPoints(1024), numLines(1), overlaps(1), 
						rangeX(6.0f), rangeY(6.0f), lineSpace(0.0f), angle(0.0f),
						offsetX(0.0f), offsetY(0.0f), scaleX(0.0f), scaleY(0.0f),
						posMoveX(0.0f), posMoveY(0.0f), 
						usePattern(false), useFaster(false), useForeDist(false)
	{
	}
};


PatternScan::PatternScan() :
	d_ptr(make_unique<PatternScanImpl>())
{
}


oct_pattern::PatternScan::~PatternScan() = default;
oct_pattern::PatternScan::PatternScan(PatternScan && rhs) = default;
PatternScan & oct_pattern::PatternScan::operator=(PatternScan && rhs) = default;


oct_pattern::PatternScan::PatternScan(const PatternScan & rhs)
	: d_ptr(make_unique<PatternScanImpl>(*rhs.d_ptr))
{
}


PatternScan & oct_pattern::PatternScan::operator=(const PatternScan & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_pattern::PatternScan::initialize(EyeRegion domain, OctPatternType type, int numPoints, int numLines, 
										 float rangeX, float rangeY, int overlaps, float lineSpace, 
										 float offsetX, float offsetY, float angle, float scaleX, float scaleY, 
										 float posMoveX, float posMoveY)
{
	getImpl().type = type;
	getImpl().domain = domain;

	setScanRange(rangeX, rangeY);
	setScanOverlaps(overlaps);
	setScanLineSpace(lineSpace);
	setScanAngle(angle);
	setScanOffset(offsetX, offsetY);
	setScanScale(scaleX, scaleY);
	setNumberOfScanPoints(numPoints);
	setNumberOfScanLines(numLines);

	d_ptr->posMoveX = posMoveX;
	d_ptr->posMoveY = posMoveY;

	// initializeAsPattern();
	return;
}


bool oct_pattern::PatternScan::buildPattern(bool hidden)
{
	clearPatternFrames();

	// Build line traces for preview and measure scan pattern.
	switch (getImpl().type) {
	case OctPatternType::POINT:
		buildPatternPoint();
		break;
	case OctPatternType::LINE:
		buildPatternLine(false);
		break;
	case OctPatternType::VERT_LINE:
		buildPatternLine(true);
		break;
	case OctPatternType::CIRCLE:
		buildPatternCircle();
		break;
	case OctPatternType::CROSS:
		buildPatternCross();
		break;
	case OctPatternType::RADIAL:
		buildPatternRadial();
		break;
	case OctPatternType::RASTER:
	case OctPatternType::CUBE:
	case OctPatternType::ANGIO:
		buildPatternCube(false);
		break;
	case OctPatternType::VERT_RASTER:
	case OctPatternType::VERT_CUBE:
	case OctPatternType::VERT_ANGIO:
		buildPatternCube(true);
		break;
	default:
		return false;
	}
	return true;
}


bool oct_pattern::PatternScan::usePreviewPattern(bool isset, bool flag)
{
	if (isset) {
		getImpl().usePattern = flag;
	}
	return getImpl().usePattern;
}


bool oct_pattern::PatternScan::useFastRasters(bool isset, bool flag)
{
	if (isset) {
		getImpl().useFaster = flag;
	}
	return getImpl().useFaster;
}


bool oct_pattern::PatternScan::useForeDistance(bool isset, bool flag)
{
	if (isset) {
		getImpl().useForeDist = flag;
	}
	return getImpl().useForeDist;
}


void oct_pattern::PatternScan::setScanRange(float rangeX, float rangeY)
{
	float value = rangeX;
	value = (value < PATTERN_SCAN_RANGE_X_MIN ? PATTERN_SCAN_RANGE_X_MIN : value);
	value = (value > PATTERN_SCAN_RANGE_X_MAX ? PATTERN_SCAN_RANGE_X_MAX : value);
	getImpl().rangeX = value;
	
	value = rangeY;
	value = (value < PATTERN_SCAN_RANGE_Y_MIN ? PATTERN_SCAN_RANGE_Y_MIN : value);
	value = (value > PATTERN_SCAN_RANGE_Y_MAX ? PATTERN_SCAN_RANGE_Y_MAX : value);
	getImpl().rangeY = value;
	return;
}


void oct_pattern::PatternScan::setScanAngle(float angle)
{
	float value = angle;
	value = (value < PATTERN_SCAN_ANGLE_MIN ? PATTERN_SCAN_ANGLE_MIN : value);
	value = (value > PATTERN_SCAN_ANGLE_MAX ? PATTERN_SCAN_ANGLE_MAX : value);
	getImpl().angle = value;
	return;
}


void oct_pattern::PatternScan::setScanOffset(float offsetX, float offsetY)
{
	getImpl().offsetX = offsetX;
	getImpl().offsetY = offsetY;
	return;
}


void oct_pattern::PatternScan::setScanScale(float scaleX, float scaleY)
{
	getImpl().scaleX = scaleX;
	getImpl().scaleY = scaleY;
	return;
}


void oct_pattern::PatternScan::setScanOverlaps(int overlaps)
{
	getImpl().overlaps = overlaps;
	return;
}


void oct_pattern::PatternScan::setScanLineSpace(float space)
{
	getImpl().lineSpace = space;
	return;
}


void oct_pattern::PatternScan::setNumberOfScanPoints(int numPoints)
{
	int value = numPoints;
	value = (value < PATTERN_SCAN_POINTS_MIN ? PATTERN_SCAN_POINTS_MIN : value);
	value = (value > PATTERN_SCAN_POINTS_MAX ? PATTERN_SCAN_POINTS_MAX : value);
	getImpl().numPoints = value;
	return;
}


void oct_pattern::PatternScan::setNumberOfScanLines(int numLines)
{
	int value = numLines;
	value = (value < PATTERN_SCAN_LINES_MIN ? PATTERN_SCAN_LINES_MIN : value);
	value = (value > PATTERN_SCAN_LINES_MAX ? PATTERN_SCAN_LINES_MAX : value);
	getImpl().numLines = value;
	return;
}


float oct_pattern::PatternScan::getScanRangeX(void) const
{
	return getImpl().rangeX;
}


float oct_pattern::PatternScan::getScanRangeY(void) const
{
	return getImpl().rangeY;
}


int oct_pattern::PatternScan::getNumberOfScanOverlaps(void) const
{
	return getImpl().overlaps;
}


float oct_pattern::PatternScan::getScanLineSpace(void) const
{
	return getImpl().lineSpace;
}


float oct_pattern::PatternScan::getScanAngle(void) const
{
	return getImpl().angle;
}


float oct_pattern::PatternScan::getScanRangeOffsetX(bool valid) const
{
	if (!valid) {
		return getImpl().offsetX;
	}
	else {
		float avail = (PATTERN_SCAN_RANGE_X_MAX - getScanRangeX()) / 2.0f;
		float offset = getImpl().offsetX;
		if (fabs(offset) > avail) {
			// offset = (offset > 0.0f ? avail : (avail*-1));
		}
		return offset;
	}
}


float oct_pattern::PatternScan::getScanRangeOffsetY(bool valid) const
{
	if (!valid) {
		return getImpl().offsetY;
	}
	else {
		float avail = (PATTERN_SCAN_RANGE_Y_MAX - getScanRangeY()) / 2.0f;
		float offset = getImpl().offsetY;
		if (fabs(offset) > avail) {
			// offset = (offset > 0.0f ? avail : (avail*-1));
		}
		return offset;
	}
}


float oct_pattern::PatternScan::getScanScaleX(void) const
{
	return getImpl().scaleX;
}


float oct_pattern::PatternScan::getScanScaleY(void) const
{
	return getImpl().scaleY;
}


float oct_pattern::PatternScan::getScanMoveX(void) const
{
	return getImpl().posMoveX;
}


float oct_pattern::PatternScan::getScanMoveY(void) const
{
	return getImpl().posMoveY;
}


int oct_pattern::PatternScan::getNumberOfScanPoints(void) const
{
	return getImpl().numPoints;
}


int oct_pattern::PatternScan::getNumberOfScanLines(void) const
{
	return getImpl().numLines;
}


int oct_pattern::PatternScan::getFrameCapacityFromPoints(int numPoints, int numLines, int numOverlaps, bool enface) const
{
	int frameSize;
	if (enface)
	{
		if (numPoints >= (int)OctAscanSize::ASCAN_1024) {
			frameSize = 4; // 8;
		}
		else if (numPoints >= (int)OctAscanSize::ASCAN_512) {
			frameSize = 8;  16; // 8;
		}
		else {
			frameSize = 8;  16; // 32;// 64; // 16;
		}
	}
	else
	{
		if (numPoints >= (int)OctAscanSize::ASCAN_1024) {
			frameSize = 4; // 8;
		}
		else if (numPoints >= (int)OctAscanSize::ASCAN_512) {
			// frameSize = (numOverlaps > 1 ? 4 : 8); // 16; 24; //  32; // 8;
			frameSize = (numOverlaps > 1 ? 2 : 8); ;
		}
		else {
			// frameSize = (numOverlaps > 1 ? 8 : 16); // 16; 32; // 64; // 64; // 16;
			frameSize = (numOverlaps > 1 ? 4 : 16); // 16; 32; // 64; // 64; // 16;
		}
	}

	frameSize = (frameSize > numLines ? numLines : frameSize);
	return frameSize;
}


int oct_pattern::PatternScan::getNumberOfFramesFromPoints(int numPoints, int numLines, int numOverlaps, bool enface) const
{
	int frameSize = getFrameCapacityFromPoints(numPoints, numLines, numOverlaps, enface);

	int numFrames = numLines / frameSize + (numLines % frameSize != 0 ? 1 : 0);
	return numFrames;
}


bool oct_pattern::PatternScan::isVertical(void) const
{
	switch (getImpl().type) {
	case OctPatternType::VERT_LINE:
	case OctPatternType::VERT_CUBE:
	case OctPatternType::VERT_RASTER:
	case OctPatternType::VERT_ANGIO:
		return true;
	}
	return false;
}


bool oct_pattern::PatternScan::isCornea(void) const
{
	if (getImpl().domain == EyeRegion::CORNEA) {
		return true;
	}
	return false;
}


bool oct_pattern::PatternScan::isScan3D(void) const
{
	switch (getPatternType()) {
	case OctPatternType::RASTER:
	case OctPatternType::VERT_RASTER:
	case OctPatternType::CUBE:
	case OctPatternType::VERT_CUBE:
	case OctPatternType::ANGIO:
	case OctPatternType::VERT_ANGIO:
		return true;
	}
	return false;
}


bool oct_pattern::PatternScan::isRaster(void) const
{
	switch (getPatternType()) {
	case OctPatternType::RASTER:
	case OctPatternType::VERT_RASTER:
		return true;
	}
	return false;
}


OctPatternType oct_pattern::PatternScan::getPatternType(void) const
{
	return getImpl().type;
}


std::vector<PatternFrame> & oct_pattern::PatternScan::getPatternFrames(void) const
{
	return getImpl().frames;
}


void oct_pattern::PatternScan::clearPatternFrames(void)
{
	getImpl().frames.clear();
	return;
}


int oct_pattern::PatternScan::resizePatternFrames(int size)
{
	clearPatternFrames();
	for (int i = 0; i < size; i++) {
		getImpl().frames.emplace_back();
	}
	return getNumberOfPatternFrames();
}


int oct_pattern::PatternScan::getNumberOfPatternFrames(void) const
{
	return (int)getImpl().frames.size();
}


PatternFrame * oct_pattern::PatternScan::getPatternFrame(int index) const
{
	if (index >= 0 && index < getNumberOfPatternFrames()) {
		return &getImpl().frames[index];
	}
	return nullptr;
}


void oct_pattern::PatternScan::addPatternFrame(PatternFrame & frame)
{
	getImpl().frames.push_back(move(frame));
	return;
}


std::vector<int> oct_pattern::PatternScan::getLateralSizeListOfFrame(int index, bool repeat) const
{
	PatternFrame* p = getPatternFrame(index);
	if (p != nullptr) {
		return p->getLateralSizeList(repeat);
	}
	return std::vector<int>();
}


LineTrace * oct_pattern::PatternScan::getLineTraceFromImageIndex(int index, bool repeat, bool preview) const
{
	int rsize = index;
	int lsize = 0;
	for (const auto& frame : d_ptr->frames) {
		for (auto& line : frame.getLineTraceList()) {
			if (preview) {
				if (repeat && !line.isLineHD()) {
					continue;
				}
			}
			lsize = (repeat ? line.getNumberOfRepeats() : 1);
			rsize -= lsize;
			if (rsize < 0) {
				return &line;
			}
		}
	}
	return nullptr;
}


int oct_pattern::PatternScan::getLineIndexFromImageIndex(int index, bool repeat, bool preview) const
{
	int rsize = index;
	int lsize = 0;
	int nline = 0;
	for (const auto& frame : d_ptr->frames) {
		for (auto& line : frame.getLineTraceList()) {
			if (preview) {
				if (repeat && !line.isLineHD()) {
					continue;
				}
			}
			lsize = (repeat ? line.getNumberOfRepeats() : 1);
			rsize -= lsize;
			if (rsize < 0) {
				return nline;
			}
			nline++;
		}
	}
	return -1;
}


int oct_pattern::PatternScan::getOverlapIndexFromImageIndex(int index, bool repeat, bool preview) const
{
	if (!repeat) {
		return 0;
	}

	int rsize = index;
	int lsize = 0;
	int nline = 0;
	for (const auto& frame : d_ptr->frames) {
		for (auto& line : frame.getLineTraceList()) {
			if (preview) {
				if (!line.isLineHD()) {
					continue;
				}
			}
			lsize = line.getNumberOfRepeats();
			rsize -= lsize;
			if (rsize < 0) {
				return (rsize + lsize);
			}
			nline++;
		}
	}
	return 0;
}


void oct_pattern::PatternScan::buildPatternPoint(void)
{
	return;
}


void oct_pattern::PatternScan::buildPatternLine(bool isVert, int lineCount)
{
	return;
}


void oct_pattern::PatternScan::buildPatternCircle(void)
{
	return;
}


void oct_pattern::PatternScan::buildPatternCross(void)
{
	return;
}


void oct_pattern::PatternScan::buildPatternRadial(void)
{
	return;
}


void oct_pattern::PatternScan::buildPatternRaster(bool isVert)
{
	return;
}


void oct_pattern::PatternScan::buildPatternCube(bool isVert)
{
	return;
}



PatternScan::PatternScanImpl & oct_pattern::PatternScan::getImpl(void) const
{
	return *d_ptr;
}

