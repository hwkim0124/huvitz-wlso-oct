#include "pch.h"
#include "PatternPlan.h"
#include "PreviewScan.h"
#include "MeasureScan.h"
#include "EnfaceScan.h"
#include "LineTrace.h"


using namespace oct_pattern;


struct PatternPlan::PatternPlanImpl
{
	PreviewScan preview;
	MeasureScan measure;
	EnfaceScan enface;
	bool initiated;
	bool useEnface;

	float dispOffsetX;
	float dispOffsetY;
	float dispAngle;
	float dispScaleX;
	float dispScaleY;

	int enfacePoints;
	int enfaceLines;
	float enfaceRangeX;
	float enfaceRangeY;

	PatternPlanImpl() : initiated(false), useEnface(false),
		dispOffsetX(0.0f), dispOffsetY(0.0f), dispAngle(0.0f),
		dispScaleX(1.0f), dispScaleY(1.0f),
		enfacePoints(PATTERN_ENFACE_ASCAN_POINTS),
		enfaceLines(PATTERN_ENFACE_BSCAN_LINES),
		enfaceRangeX(PATTERN_ENFACE_RANGE_X),
		enfaceRangeY(PATTERN_ENFACE_RANGE_Y)
	{
	}
};


PatternPlan::PatternPlan() :
	d_ptr(make_unique<PatternPlanImpl>())
{
}


oct_pattern::PatternPlan::~PatternPlan() = default;
oct_pattern::PatternPlan::PatternPlan(PatternPlan && rhs) = default;
PatternPlan & oct_pattern::PatternPlan::operator=(PatternPlan && rhs) = default;


oct_pattern::PatternPlan::PatternPlan(const PatternPlan & rhs)
	: d_ptr(make_unique<PatternPlanImpl>(*rhs.d_ptr))
{
}


PatternPlan & oct_pattern::PatternPlan::operator=(const PatternPlan & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void oct_pattern::PatternPlan::setupEnface(int numPoints, int numLines, float rangeX, float rangeY)
{
	getImpl().enfacePoints = numPoints;
	getImpl().enfaceLines = numLines;
	getImpl().enfaceRangeX = rangeX;
	getImpl().enfaceRangeY = rangeY;
	return;
}


void oct_pattern::PatternPlan::setupDisplacement(float offsetX, float offsetY, float angle, float scaleX, float scaleY)
{
	getImpl().dispOffsetX = offsetX;
	getImpl().dispOffsetY = offsetY;
	getImpl().dispAngle = angle;
	getImpl().dispScaleX = scaleX;
	getImpl().dispScaleY = scaleY;
	return;
}



bool oct_pattern::PatternPlan::buildPatternPlan(EyeRegion domain, OctPatternType type,
	int numPoints, int numLines, float rangeX, float rangeY,
	int overlaps, float lineSpace, bool useEnface,
	bool usePattern, bool useFaster)
{
	float offsetX = d_ptr->dispOffsetX;
	float offsetY = d_ptr->dispOffsetY;
	float angle = d_ptr->dispAngle;
	float scaleX = d_ptr->dispScaleX;
	float scaleY = d_ptr->dispScaleY;

	getImpl().preview.initialize(domain, type, numPoints, numLines,
		rangeX, rangeY, overlaps, lineSpace,
		offsetX, offsetY, angle, scaleX, scaleY);
	getImpl().preview.usePreviewPattern(true, usePattern);
	getImpl().preview.useFastRasters(true, useFaster);

	if (!getImpl().preview.buildPattern()) {
		return false;
	}

	getImpl().measure.initialize(domain, type, numPoints, numLines, rangeX, rangeY, overlaps, lineSpace,
		offsetX, offsetY, angle, scaleX, scaleY);
	getImpl().measure.useFastRasters(true, useFaster);

	if (!getImpl().measure.buildPattern()) {
		return false;
	}

	if (usePattern) {
		useEnface = false;
	}

	if (useEnface)
	{
		int enfPoints = d_ptr->enfacePoints;
		int enfLines = d_ptr->enfaceLines;
		float enfRangeX = d_ptr->enfaceRangeX;
		float enfRangeY = d_ptr->enfaceRangeY;

		getImpl().enface.initialize(EyeRegion::MACULAR, OctPatternType::CUBE,
			enfPoints, enfLines, enfRangeX, enfRangeY,
			overlaps, lineSpace,
			offsetX, offsetY, angle, scaleX, scaleY);
		getImpl().enface.useFastRasters(true, useFaster);

		if (!getImpl().enface.buildPattern()) {
			return false;
		}
	}
	else {
		getImpl().enface.clearPatternFrames();
	}

	getImpl().useEnface = useEnface;
	getImpl().initiated = true;
	return true;
}


bool oct_pattern::PatternPlan::buildPatternPlan(OctScanProtocol& protocol)
{
	bool useFaster = protocol.isFastRasterScan;
	bool usePattern = protocol.isPreviewPattern;
	bool useEnface = protocol.isPhasingEnface;

	if (!buildPreviewPattern(protocol)) {
		return false;
	}

	if (!buildMeasurePattern(protocol)) {
		return false;
	}

	if (usePattern) {
		useEnface = false;
	}

	if (useEnface)
	{
		if (!buildEnfacePattern(protocol)) {
			return false;
		}
	}
	else {
		getImpl().enface.clearPatternFrames();
	}

	getImpl().useEnface = useEnface;
	getImpl().initiated = true;
	return true;
}


bool oct_pattern::PatternPlan::buildPreviewPattern(OctScanProtocol& protocol)
{
	bool useFaster = protocol.isFastRasterScan;
	bool usePattern = protocol.isPreviewPattern;
	bool useEnface = protocol.isPhasingEnface;

	OctScanPattern pattern = protocol.measure;
	OctScanOffset offset = pattern.scanOffset;

	float offsetX = (offset._mmAsUnit ? offset._offsetX : 0.0f);
	float offsetY = (offset._mmAsUnit ? offset._offsetY : 0.0f);
	float moveX = (offset._mmAsUnit ? 0.0f : offset._offsetX);
	float moveY = (offset._mmAsUnit ? 0.0f : offset._offsetY);
	float angle = offset._angle;
	float scaleX = offset._scaleX;
	float scaleY = offset._scaleY;

	updatePatternScale(pattern, scaleX, scaleY);
	updatePatternOffset(pattern, offsetX, offsetY);

	EyeRegion domain = pattern.eyeRegion;
	OctPatternType type = pattern.patternType;

	int numPoints = PATTERN_PREVIEW_LINE_NUM_POINTS; // pattern._numPoints;
	int numLines = pattern.numBscan;
	float rangeX = pattern.rangeX;
	float rangeY = pattern.rangeY;
	int overlaps = pattern.overlaps;
	float lineSpace = pattern.lineSpace;

	if (!usePattern) {
		if (type != OctPatternType::POINT) {
			type = pattern.previewType;
			if (type == OctPatternType::MULTI_LINE || type == OctPatternType::VERT_MULTI_LINE) {
				numLines = 4;
				numPoints = PATTERN_PREVIEW_MULTI_NUM_POINTS;
			}
			else if (type == OctPatternType::CROSS) {
				numLines = 2;
			}
			else {
				numLines = 1;
			}
		}
		if (pattern.isAngioPattern()) {
			numPoints = pattern.numAscan;
		}
	}

	getImpl().preview.initialize(domain, type, numPoints, numLines,
		rangeX, rangeY, overlaps, lineSpace,
		offsetX, offsetY, angle, scaleX, scaleY, moveX, moveY);
	getImpl().preview.usePreviewPattern(true, usePattern);
	getImpl().preview.useFastRasters(true, useFaster);

	if (!getImpl().preview.buildPattern(true)) {
		LogD() << "Failed to build preview pattern";
		return false;
	}
	else {
		PreviewScan& scan = getPreviewScan();
		LogD() << "[ Preview Scan ]";
		LogD() << "X-Y range: " << scan.getScanRangeX() << ", " << scan.getScanRangeY();
		LogD() << "X-Y scale: " << scan.getScanScaleX() << ", " << scan.getScanScaleY();
		LogD() << "X-Y offset: " << scan.getScanRangeOffsetX() << ", " << scan.getScanRangeOffsetY() << " angle: " << scan.getScanAngle();
		LogD() << "Points/lines: " << scan.getNumberOfScanPoints() << ", " << scan.getNumberOfScanLines();
		LogD() << "Pattern type: " << static_cast<int>(type) << ", horiz.: " << pattern.isHorizontalScan();
		LogD() << "Line space: " << scan.getScanLineSpace() << " overlaps: " << scan.getNumberOfScanOverlaps();
		LogD() << "Pattern frames: " << scan.getNumberOfPatternFrames();
	}
	
	return true;
}


bool oct_pattern::PatternPlan::buildMeasurePattern(OctScanProtocol& protocol)
{
	bool useFaster = protocol.isFastRasterScan;
	bool usePattern = protocol.isPreviewPattern;
	bool useEnface = protocol.isPhasingEnface;

	OctScanPattern pattern = protocol.measure;
	OctScanOffset offset = pattern.scanOffset;

	float offsetX = (offset._mmAsUnit ? offset._offsetX : 0.0f);
	float offsetY = (offset._mmAsUnit ? offset._offsetY : 0.0f);
	float moveX = (offset._mmAsUnit ? 0.0f : offset._offsetX);
	float moveY = (offset._mmAsUnit ? 0.0f : offset._offsetY);
	float angle = offset._angle;
	float scaleX = offset._scaleX;
	float scaleY = offset._scaleY;

	updatePatternScale(pattern, scaleX, scaleY);
	updatePatternOffset(pattern, offsetX, offsetY);

	EyeRegion domain = pattern.eyeRegion;
	OctPatternType type = pattern.patternType;

	int numPoints = pattern.numAscan;
	int numLines = pattern.numBscan;
	float rangeX = pattern.rangeX;
	float rangeY = pattern.rangeY;
	int overlaps = pattern.overlaps;
	float lineSpace = pattern.lineSpace;

	getImpl().measure.initialize(domain, type, numPoints, numLines, rangeX, rangeY, overlaps, lineSpace,
		offsetX, offsetY, angle, scaleX, scaleY, moveX, moveY);
	getImpl().measure.useFastRasters(true, useFaster);

	if (!getImpl().measure.buildPattern()) {
		LogD() << "Failed to build measure pattern";
		return false;
	}
	else {
		MeasureScan& scan = getMeasureScan();
		LogD() << "[ Measure Scan ]";
		LogD() << "X-Y range: " << scan.getScanRangeX() << ", " << scan.getScanRangeY();
		LogD() << "X-Y scale: " << scan.getScanScaleX() << ", " << scan.getScanScaleY();
		LogD() << "X-Y offset: " << scan.getScanRangeOffsetX() << ", " << scan.getScanRangeOffsetY() << " angle: " << scan.getScanAngle();
		LogD() << "Points/lines: " << scan.getNumberOfScanPoints() << ", " << scan.getNumberOfScanLines();
		LogD() << "Pattern type: " << static_cast<int>(type) << ", horiz.: " << pattern.isHorizontalScan();
		LogD() << "Line space: " << scan.getScanLineSpace() << " overlaps: " << scan.getNumberOfScanOverlaps();
		LogD() << "Pattern frames: " << scan.getNumberOfPatternFrames() << ", enface: " << useEnface << ", faster: " << useFaster;
		LogD() << "Angio scan: " << pattern.isAngioPattern();
	}
	return true;
}


bool oct_pattern::PatternPlan::buildEnfacePattern(OctScanProtocol& protocol)
{
	bool useFaster = protocol.isFastRasterScan;
	bool usePattern = protocol.isPreviewPattern;
	bool useEnface = protocol.isPhasingEnface;

	OctScanPattern enface = protocol.enface;
	OctScanOffset offset = enface.scanOffset;

	float offsetX = (offset._mmAsUnit ? offset._offsetX : 0.0f);
	float offsetY = (offset._mmAsUnit ? offset._offsetY : 0.0f);
	float moveX = (offset._mmAsUnit ? 0.0f : offset._offsetX);
	float moveY = (offset._mmAsUnit ? 0.0f : offset._offsetY);
	float angle = offset._angle;
	float scaleX = offset._scaleX;
	float scaleY = offset._scaleY;

	updatePatternScale(enface, scaleX, scaleY);
	updatePatternOffset(enface, offsetX, offsetY);

	EyeRegion domain = protocol.getEyeRegion();// enface._domain;
	OctPatternType type = enface.patternType;

	int enfPoints = enface.numAscan;
	int enfLines = enface.numBscan;
	float enfRangeX = enface.rangeX;
	float enfRangeY = enface.rangeY;
	int overlaps = 1;
	float lineSpace = 0.0f;

	// auto type = (measure.getPattern().isPreviewVertLine() ? PatternType::VertCube : PatternType::HorzCube);
	auto enfType = OctPatternType::CUBE;
	getImpl().enface.initialize(domain, enfType,
		enfPoints, enfLines, enfRangeX, enfRangeY,
		overlaps, lineSpace,
		offsetX, offsetY, angle, scaleX, scaleY, moveX, moveY);
	getImpl().enface.useFastRasters(true, useFaster);

	if (!getImpl().enface.buildPattern()) {
		LogD() << "Failed to build enface pattern";
		return false;
	}
	else {
		EnfaceScan& scan = getEnfaceScan();
		LogD() << "[ Enface Scan ]";
		LogD() << "X-Y range: " << scan.getScanRangeX() << ", " << scan.getScanRangeY();
		LogD() << "X-Y scale: " << scan.getScanScaleX() << ", " << scan.getScanScaleY();
		LogD() << "X-Y offset: " << scan.getScanRangeOffsetX() << ", " << scan.getScanRangeOffsetY() << " angle: " << scan.getScanAngle();
		LogD() << "Pattern type: " << static_cast<int>(type) << ", horiz.: " << enface.isHorizontalScan();
		LogD() << "Points/lines: " << scan.getNumberOfScanPoints() << ", " << scan.getNumberOfScanLines();
		LogD() << "Line space: " << scan.getScanLineSpace() << " overlaps: " << scan.getNumberOfScanOverlaps();
		LogD() << "Pattern frames: " << scan.getNumberOfPatternFrames();
	}

	return true;
}


void oct_pattern::PatternPlan::updatePatternScale(const OctScanPattern& pattern, float & scaleX, float & scaleY)
{
	int index = 0;
	int speed = 0;
	float rangeX = pattern.rangeX;
	float rangeY = pattern.rangeY;

	if (scaleX != 1.0f || scaleY != 1.0f) {
		return;
	}

	int xIndex = rangeX <= 3.0f ? 0 : (rangeX <= 6.0f ? 1 : (rangeX <= 9.0f ? 2 : (rangeX <= 12.0f ? 3 : 4)));
	int yIndex = rangeY <= 3.0f ? 0 : (rangeY <= 6.0f ? 1 : (rangeY <= 9.0f ? 2 : (rangeY <= 12.0f ? 3 : 4)));

	if (pattern.isCorneaScan())
	{
		if (pattern.isSpeedSlower()) {
			speed = 0;
		}
		else if (pattern.isSpeedNormal()) {
			speed = 1;
		}
		else {
			speed = 2;
		}
		/*
		if (pattern.getPatternName() == PatternName::Topography) {
			scaleX = GlobalSettings::topographyPatternScale(index).first;
			scaleY = GlobalSettings::topographyPatternScale(index).second;
		}
		else {
			if (GlobalSettings::useTopoCalibration(false)) {
				scaleX = GlobalSettings::topographyPatternScale(index).first;
				scaleY = GlobalSettings::topographyPatternScale(index).second;
			}
			else {
				scaleX = GlobalSettings::corneaPatternScale(index).first;
				scaleY = GlobalSettings::corneaPatternScale(index).second;
			}
		}
		*/
		if (auto* p = OctScanOptions::getInstance(); p) {
			// tie(scaleX, scaleY) = p->getCorneaPatternRangeScale(speed, xIndex);
			scaleX = p->getCorneaPatternRangeScaleX(speed, xIndex);
			scaleY = p->getCorneaPatternRangeScaleY(speed, yIndex);
		}
	}
	else 
	{
		if (pattern.isSpeedSlower()) {
			speed = 0;
		}
		else if (pattern.isSpeedNormal()) {
			speed = 1;
		}
		else {
			speed = 2;
		}
		if (auto* p = OctScanOptions::getInstance(); p) {
			// tie(scaleX, scaleY) = p->getRetinaPatternRangeScale(speed, index);
			scaleX = p->getRetinaPatternRangeScaleX(speed, xIndex);
			scaleY = p->getRetinaPatternRangeScaleY(speed, yIndex);
		}
	}
	return;
}


void oct_pattern::PatternPlan::updatePatternOffset(const OctScanPattern & pattern, float & offsetX, float & offsetY)
{
	int speed = 0;

	if (offsetX != 0.0f || offsetY != 0.0f) {
		return;
	}

	if (pattern.isCorneaScan())
	{
		if (pattern.isSpeedSlower()) {
			speed = 0;
		}
		else if (pattern.isSpeedNormal()) {
			speed = 1;
		}
		else {
			speed = 2;
		}
		/*
		if (pattern.getPatternName() == PatternName::Topography) {
			offsetX = GlobalSettings::topographyPatternOffset(index).first;
			offsetY = GlobalSettings::topographyPatternOffset(index).second;
		}
		else {
			if (GlobalSettings::useTopoCalibration(false)) {
				offsetX = GlobalSettings::topographyPatternOffset(index).first;
				offsetY = GlobalSettings::topographyPatternOffset(index).second;
			}
			else {
				offsetX = GlobalSettings::corneaPatternOffset(index).first;
				offsetY = GlobalSettings::corneaPatternOffset(index).second;
			}
		}
		*/
		if (auto* p = OctScanOptions::getInstance(); p) {
			tie(offsetX, offsetY) = p->getCorneaPatternRangeOffset(speed);
		}
	}
	else
	{
		if (pattern.isSpeedSlower()) {
			speed = 0;
		}
		else if (pattern.isSpeedNormal()) {
			speed = 1;
		}
		else {
			speed = 2;
		}
		if (auto* p = OctScanOptions::getInstance(); p) {
			tie(offsetX, offsetY) = p->getRetinaPatternRangeOffset(speed);
		}
	}
	return;
}


int oct_pattern::PatternPlan::getPreviewScanPoints(void) const
{
	return getImpl().preview.getNumberOfScanPoints();
}


int oct_pattern::PatternPlan::getMeasureScanPoints(void) const
{
	return getImpl().measure.getNumberOfScanPoints();
}


int oct_pattern::PatternPlan::getEnfaceScanPoints(void) const
{
	return getImpl().enface.getNumberOfScanPoints();
}


int oct_pattern::PatternPlan::getPreviewScanLines(void) const
{
	return getImpl().preview.getNumberOfScanLines();
}


int oct_pattern::PatternPlan::getMeasureScanLines(void) const
{
	return getImpl().measure.getNumberOfScanLines();
}


int oct_pattern::PatternPlan::getEnfaceScanLines(void) const
{
	return getImpl().enface.getNumberOfScanLines();
}


bool oct_pattern::PatternPlan::isInitiated(void) const
{
	return getImpl().initiated;
}


void oct_pattern::PatternPlan::clear(void)
{
	getImpl().initiated = false;
	return;
}


bool oct_pattern::PatternPlan::isCornea(void) const
{
	return getImpl().preview.isCornea();
}


bool oct_pattern::PatternPlan::isEnfaceScan(void) const
{
	return getImpl().useEnface;
}


PreviewScan & oct_pattern::PatternPlan::getPreviewScan(void) const
{
	return getImpl().preview;
}


MeasureScan & oct_pattern::PatternPlan::getMeasureScan(void) const
{
	return getImpl().measure;
}


EnfaceScan & oct_pattern::PatternPlan::getEnfaceScan(void) const
{
	return getImpl().enface;
}


PatternPlan::PatternPlanImpl & oct_pattern::PatternPlan::getImpl(void) const
{
	return *d_ptr;
}
