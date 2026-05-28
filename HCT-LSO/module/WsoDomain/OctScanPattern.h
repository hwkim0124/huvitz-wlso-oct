#pragma once

#include "WsoDomain2.h"
#include "PatientDefines.h"
#include "OctScanDefines.h"
#include "OctScanHelper.h"
#include "OctDataSetup.h"

namespace wso_domain
{
	struct OctScanPattern
	{
		OctPatternCode patternCode = OctPatternCode::MACULAR_LINE;
		OctPatternType patternType = OctPatternType::LINE;
		OctPatternType previewType = OctPatternType::LINE;
		EyeRegion eyeRegion = EyeRegion::MACULAR;

		float rangeX = 6.0f;
		float rangeY = 6.0f;
		int numAscan = 1024;
		int numBscan = 1;
		int overlaps = 1;
		float lineSpace = 0.0f;

		OctScanDirection scanDirection = OctScanDirection::X_TO_Y;
		OctScanSpeed scanSpeed = OctScanSpeed::FASTER;
		OctScanOffset scanOffset;

	public:
		void initAsDefaultPreview() {
			patternCode = OctPatternCode::MACULAR_LINE;
			eyeRegion = EyeRegion::MACULAR;
			patternType = OctPatternType::LINE;

			numAscan = PATTERN_PREVIEW_ASCAN_POINTS;
			numBscan = 1;
			rangeX = PATTERN_PREVIEW_RANGE_X;
			rangeY = 0.0f;

			scanOffset = OctScanOffset();
		}

		void initAsDefaultEnface() {
			patternCode = OctPatternCode::MACULAR_CUBE;
			eyeRegion = EyeRegion::MACULAR;
			patternType = OctPatternType::CUBE;

			numAscan = PATTERN_ENFACE_ASCAN_POINTS;
			numBscan = PATTERN_ENFACE_BSCAN_LINES;
			rangeX = PATTERN_ENFACE_RANGE_X;
			rangeY = PATTERN_ENFACE_RANGE_Y;

			scanOffset = OctScanOffset();
		}

		void initAsEmpty() {
			patternCode = OctPatternCode::UNKNOWN;
			eyeRegion = EyeRegion::UNKNOWN;
			patternType = OctPatternType::UNKNOWN;

			numAscan = 0;
			numBscan = 0;
			rangeX = 0.0f;
			rangeY = 0.0f;
			overlaps = 1;
			scanDirection = OctScanDirection::X_TO_Y;
			lineSpace = 0.0f;

			scanOffset = OctScanOffset();
		}

		bool isCorneaScan(void) const {
			return eyeRegion == EyeRegion::CORNEA;
		}

		bool isRetinaScan(void) const {
			return isMarcularScan() || isOpticDiscScan() || isFundusScan();
		}

		bool isMarcularScan(void) const {
			return eyeRegion == EyeRegion::MACULAR;
		}

		bool isOpticDiscScan(void) const {
			return eyeRegion == EyeRegion::OPTIC_DISC;
		}

		bool isFundusScan(void) const {
			return eyeRegion == EyeRegion::FUNDUS;
		}

		bool isEnfacePattern(void) const {
			return isAngioPattern() || isCubePattern();
		}

		bool isAngioPattern(void) const {
			return patternType == OctPatternType::ANGIO || patternType == OctPatternType::VERT_ANGIO;
		}

		bool isLinePattern(void) const {
			return patternType == OctPatternType::LINE || patternType == OctPatternType::VERT_LINE;
		}

		bool isCrossPattern(void) const {
			return patternType == OctPatternType::CROSS;
		}

		bool isCubePattern(void) const {
			return patternType == OctPatternType::CUBE || patternType == OctPatternType::VERT_CUBE;
		}

		bool isRadialPattern(void) const {
			return patternType == OctPatternType::RADIAL;
		}

		bool isRasterPattern(void) const {
			return patternType == OctPatternType::RASTER || patternType == OctPatternType::VERT_RASTER;
		}

		bool isCirclePattern(void) const {
			return patternType == OctPatternType::CIRCLE;
		}

		bool isSpeedFaster(void) const {
			return scanSpeed == OctScanSpeed::FASTER;
		}

		bool isSpeedNormal(void) const {
			return scanSpeed == OctScanSpeed::NORMAL;
		}

		bool isSpeedSlower(void) const {
			return scanSpeed == OctScanSpeed::SLOWER;
		}

		bool isSpeedCustom(void) const {
			return scanSpeed == OctScanSpeed::CUSTOM;
		}

		bool isHorizontalScan(void) const {
			return scanDirection == OctScanDirection::X_TO_Y;
		}

		bool isVerticalScan(void) const {
			return !isHorizontalScan();
		}

		float getScanRangeX(void) const {
			return rangeX;
		}

		float getScanRangeY(void) const {
			return rangeY;
		}

		float getScanDistance(void) const {
			return (isHorizontalScan() ? getScanRangeX() : getScanRangeY());
		}

		float getScanLength(void) const {
			if (isCirclePattern()) {
				return getScanRangeX() * 3.14159265358979f;
			}
			else {
				return getScanDistance();
			}
		}

		float getStartX(bool offset = true) const {
			float pos = (rangeX / 2.0f) * -1.0f + (offset ? scanOffset._offsetX : 0.0f);
			return pos;
		}

		float getCloseX(bool offset = true) const {
			float pos = (rangeX / 2.0f) * +1.0f + (offset ? scanOffset._offsetX : 0.0f);
			return pos;
		}

		float getStartY(bool offset = true) const {
			float pos = (rangeY / 2.0f) * -1.0f + (offset ? scanOffset._offsetY : 0.0f);
			return pos;
		}

		float getCloseY(bool offset = true) const {
			float pos = (rangeY / 2.0f) * +1.0f + (offset ? scanOffset._offsetY : 0.0f);
			return pos;
		}

		OctPoint startPoint(bool offset = true) const {
			return OctPoint(getStartX(offset), getStartY(offset));
		}

		OctPoint closePoint(bool offset = true) const {
			return OctPoint(getCloseX(offset), getCloseY(offset));
		}

		OctPoint centerPoint(bool offset = true) const {
			return (offset ? OctPoint(scanOffset._offsetX, scanOffset._offsetY) : OctPoint());
		}

		float startX(bool offset = true) const {
			return getStartX(offset);
		}

		float closeX(bool offset = true) const {
			return getCloseX(offset);
		}

		float startY(bool offset = true) const {
			return getStartY(offset);
		}

		float closeY(bool offset = true) const {
			return getCloseY(offset);
		}

		float centerX(bool offset = true) const {
			return centerPoint(offset)._x;
		}

		float centerY(bool offset = true) const {
			return centerPoint(offset)._y;
		}

		float width(void) const {
			return (float)fabs(closeX() - startX());
		}

		float height(void) const {
			return (float)fabs(closeY() - startY());
		}

		float radiusX(void) const {
			return width() * 2.0f;
		}

		float radiusY(void) const {
			return height() * 2.0f;
		}

		int getNumberOfScanPoints(void) const {
			return numAscan;
		}

		int getNumberOfScanLines(void) const {
			return numBscan;
		}

		float getPixelWidth(void) const {
			int points = getNumberOfScanPoints();
			float range = (isHorizontalScan() ? getScanRangeX() : getScanRangeY());
			if (points <= 0) {
				return 0.0f;
			}
			else {
				return (range / points);
			}
		}

		float getPixelHeight(void) const {
			if (isCorneaScan()) {
				auto resol = OctDataSetup::getCorneaScanAxialResolution();
				return (float)resol;
			}
			else {
				auto resol = OctDataSetup::getRetinaScanAxialResolution();
				return (float)resol;
			}
		}

		float getLineDistance(void) const {
			int lines = getNumberOfScanLines();
			float range = (isHorizontalScan() ? getScanRangeX() : getScanRangeY());

			if (lines <= 0) {
				return 0.0f;
			}
			else {
				return (range / lines);
			}
		}

		OctRange getScanRange(bool offset = true) const {
			float x1 = getStartX(offset);
			float y1 = getStartY(offset);
			float x2 = getCloseX(offset);
			float y2 = getCloseY(offset);
			return OctRange(x1, y1, x2, y2);
		}

		int getScanOverlaps(void) const {
			return overlaps;
		}

		float getScanLineSpace(void) const {
			return lineSpace;
		}

		OctRoute getRouteOfScanLine(int index) const
		{
			OctRoute route;

			switch (patternCode) {
			case OctPatternCode::MACULAR_CUBE:
			case OctPatternCode::DISC_CUBE:
			case OctPatternCode::FUNDUS_CUBE:
			case OctPatternCode::MACULAR_ANGIO:
			case OctPatternCode::DISC_ANGIO:
			case OctPatternCode::FUNDUS_ANGIO:
				return getRouteOfCube(index);
			case OctPatternCode::DISC_RADIAL:
				return getRouteOfRadial(index);
			case OctPatternCode::MACULAR_RASTER:
			case OctPatternCode::FUNDUS_RASTER:
				return getRouteOfRaster(index);
			case OctPatternCode::MACULAR_CROSS:
				return getRouteOfCross(index);
			case OctPatternCode::DISC_CIRCLE:
				return getRouteOfCircle();
			case OctPatternCode::MACULAR_LINE:
			case OctPatternCode::FUNDUS_LINE:
			default:
				return getRouteOfLine();
			}
		}

		OctScanSection createScanSection(int index) const
		{
			OctScanSection sect(index, getRouteOfScanLine(index));
			return sect;
		}


	private:
		OctRoute getRouteOfLine(void) const {
			if (isHorizontalScan()) {
				return OctRoute(startX(), centerY(), closeX(), centerY());
			}
			else {
				return OctRoute(centerX(), startY(), centerX(), closeY());
			}
		}

		OctRoute getRouteOfCircle(void) const {
			return OctRoute(startPoint(), closePoint(), true);
		}

		OctRoute getRouteOfCross(int index) const {
			int half = getNumberOfScanLines() / 2;
			float space = getScanLineSpace();
			if (index < half) {
				float y = centerY() - space * (index - half / 2);
				return OctRoute(startX(), y, closeX(), y);
			}
			else {
				float x = centerX() - space * ((index - half) - half / 2);
				return OctRoute(x, startY(), x, closeY());
			}
		}

		OctRoute getRouteOfRadial(int index) const {
			int lines = getNumberOfScanLines();
			double angle = 180.0 / lines;
			double degree = angle * index;
			double radian = ((degree * 3.14159265358979323) / 180.0); // NumericFunc::degreeToRadian(degree);

			float x1 = (float)(radiusX() * cos(radian) * -1.0 + scanOffset.getScanOffsetX());
			float y1 = (float)(radiusY() * sin(radian) * -1.0 + scanOffset.getScanOffsetY());
			float x2 = (float)(radiusX() * cos(radian) * +1.0 + scanOffset.getScanOffsetX());
			float y2 = (float)(radiusY() * sin(radian) * +1.0 + scanOffset.getScanOffsetY());

			return OctRoute(x1, y1, x2, y2);
		}

		OctRoute getRouteOfRaster(int index) const {
			if (isHorizontalScan()) {
				float space = height() / (getNumberOfScanLines() - 1);
				float y = startY() + space * index;
				return OctRoute(startX(), y, closeX(), y);
			}
			else {
				float space = width() / (getNumberOfScanLines() - 1);
				float x = startX() + space * index;
				return OctRoute(x, startY(), x, closeY());
			}
		}

		OctRoute getRouteOfCube(int index) const {
			if (isHorizontalScan()) {
				float space = height() / (getNumberOfScanLines() - 1);
				float y = startY() + space * index;
				return OctRoute(startX(), y, closeX(), y);
			}
			else {
				float space = width() / (getNumberOfScanLines() - 1);
				float x = startX() + space * index;
				return OctRoute(x, startY(), x, closeY());
			}
		}
	};

}