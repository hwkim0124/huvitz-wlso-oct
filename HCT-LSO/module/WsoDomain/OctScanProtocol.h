#pragma once

#include "PatientDefines.h"
#include "OctScanDefines.h"
#include "OctScanHelper.h"
#include "OctDataSetup.h"
#include "OctScanPattern.h"

#include <fstream>
#include <iostream>
#include <string>


namespace wso_domain
{
	struct OctProtocolInitParam
	{
		EyeSide eyeSide = EyeSide::OD;
		float diopter = 0.0f;

		OctPatternCode patternCode = OctPatternCode::MACULAR_LINE;
		OctPatternType previewType = OctPatternType::LINE;

		int scanPoints = 1024;
		int scanLines = 1;
		int scanOverlaps = 1;

		float scanRangeX = 6.0f;
		float scanRangeY = 6.0f;
		float lineSpace = 0.0f;

		OctScanDirection scanDirection = OctScanDirection::X_TO_Y;
		OctScanSpeed scanSpeed = OctScanSpeed::FASTER;

		float scanOffsetX = 0.0f;
		float scanOffsetY = 0.0f;
		float scanScaleX = 1.0f;
		float scanScaleY = 1.0f;
		float scanAngle = 0.0f;

		int enfacePoints = 256;
		int enfaceLines = 64;
		float enfaceRangeX = 6.0f;
		float enfaceRangeY = 6.0f;

		bool isFirstScanOnSide = false;
		bool isPhasingEnface = false;
		bool isNotFocusAlign = false;
		bool isNotImageGrabbing = false;
	};

	struct OctProtocolDescript
	{
		EyeSide eyeSide = EyeSide::OD;
		EyeRegion eyeRegion = EyeRegion::MACULAR;
		float diopter = 0.0f;

		OctPatternCode patternCode = OctPatternCode::MACULAR_LINE;
		OctPatternType patternType = OctPatternType::LINE;
		OctPatternType previewType = OctPatternType::LINE;

		int scanPoints = 1024;
		int scanLines = 1;
		int scanOverlaps = 1;

		float scanRangeX = 6.0f;
		float scanRangeY = 6.0f;

		OctScanDirection scanDirection = OctScanDirection::X_TO_Y;
		OctScanSpeed scanSpeed = OctScanSpeed::FASTER;
		
		char protocolName[128] = { 0, };
	};

	struct OctScanProtocol
	{
		EyeSide eyeSide = EyeSide::OD;
		float diopter = 0.0f;

		OctScanPattern preview;
		OctScanPattern measure;
		OctScanPattern enface;

		bool isInitialScan = false;
		bool isPhasingEnface = false;
		bool isNotFocusAlign = false;
		bool isNotImageGrabbing = false;
		bool isFastRasterScan = false;
		bool isPreviewPattern = false;

	public:
		void initAsDefault()
		{
			preview.initAsDefaultPreview();
			enface.initAsDefaultEnface();
		}

		void setupAsInitParam(const OctProtocolInitParam& param)
		{
			initAsDefault();

			this->eyeSide = param.eyeSide;
			this->diopter = param.diopter;

			float act_rangeX = param.scanRangeX;
			float act_rangeY = param.scanRangeY;
			act_rangeX = act_rangeX == 0.0f ? act_rangeY : act_rangeX;
			act_rangeY = act_rangeY == 0.0f ? act_rangeX : act_rangeY;

			auto& measure = this->measure;
			measure.patternCode = param.patternCode;
			measure.patternType = OctScanHelper::getPatternType(param.patternCode, param.scanDirection);
			measure.previewType = OctScanHelper::getPreviewType(param.previewType, param.scanDirection);
			measure.eyeRegion = OctScanHelper::getPatternRegion(param.patternCode);

			measure.rangeX = act_rangeX;
			measure.rangeY = act_rangeY;
			measure.numAscan = param.scanPoints;
			measure.numBscan = param.scanLines;

			measure.overlaps = param.scanOverlaps;
			measure.lineSpace = param.lineSpace;

			/*
			if (measure.isEnfacePattern() || measure.isRadialPattern() || measure.isRasterPattern()) {
				measure.rangeY = measure.rangeY == 0.0f ? measure.rangeX : measure.rangeY;
			}
			*/

			measure.scanDirection = param.scanDirection;
			measure.scanSpeed = param.scanSpeed;
			measure.scanOffset.set(param.scanOffsetX, param.scanOffsetY, param.scanAngle, param.scanScaleX, param.scanScaleY);
		
			auto& enface = this->enface;
			enface.rangeX = param.enfaceRangeX;
			enface.rangeY = param.enfaceRangeY;
			enface.numAscan = param.enfacePoints;
			enface.numBscan = param.enfaceLines;

			enface.patternCode = param.patternCode;
			enface.patternType = OctScanHelper::getPatternType(param.patternCode, param.scanDirection);
			enface.eyeRegion = OctScanHelper::getPatternRegion(param.patternCode);

			enface.scanDirection = param.scanDirection;
			enface.scanSpeed = param.scanSpeed;
			enface.scanOffset.set(param.scanOffsetX, param.scanOffsetY, param.scanAngle, param.scanScaleX, param.scanScaleY);
		
			auto& preview = this->preview;
			preview.rangeX = act_rangeX;
			preview.rangeY = act_rangeY;
			preview.numAscan = param.scanPoints;
			preview.numBscan = param.scanLines;

			preview.scanDirection = param.scanDirection;
			preview.scanSpeed = param.scanSpeed;
			preview.scanOffset.set(param.scanOffsetX, param.scanOffsetY, param.scanAngle, param.scanScaleX, param.scanScaleY);

			this->isInitialScan = param.isFirstScanOnSide;
			this->isPhasingEnface = param.isPhasingEnface;
			this->isNotFocusAlign = param.isNotFocusAlign;
			this->isNotImageGrabbing = param.isNotImageGrabbing;
		}

		void fetchToInitParam(OctProtocolInitParam& param)
		{
			param.eyeSide = this->eyeSide;
			param.diopter = this->diopter;

			param.patternCode = measure.patternCode;

			param.scanRangeX = measure.rangeX;
			param.scanRangeY = measure.rangeY;
			param.scanPoints = measure.numAscan;
			param.scanLines = measure.numBscan;

			param.scanOverlaps = measure.overlaps;
			param.lineSpace = measure.lineSpace;

			param.scanDirection = measure.scanDirection;
			param.scanSpeed = measure.scanSpeed;

			param.scanOffsetX = measure.scanOffset._offsetX;
			param.scanOffsetY = measure.scanOffset._offsetY;
			param.scanScaleX = measure.scanOffset._scaleX;
			param.scanScaleY = measure.scanOffset._scaleY;
			param.scanAngle = measure.scanOffset._angle;

			param.enfaceRangeX = enface.rangeX;
			param.enfaceRangeY = enface.rangeY;
			param.enfacePoints = enface.numAscan;
			param.enfaceLines = enface.numBscan;

			param.isFirstScanOnSide = this->isInitialScan;
			param.isPhasingEnface = this->isPhasingEnface;
			param.isNotFocusAlign = this->isNotFocusAlign;
			param.isNotImageGrabbing = this->isNotImageGrabbing;
		}

		bool isOD(void) const {
			return EyeSide::OD == eyeSide;
		}

		bool isOS(void) const {
			return EyeSide::OS == eyeSide;
		}

		EyeRegion getEyeRegion(void) const {
			return measure.eyeRegion;
		}

		EyeSide getEyeSide(void) const {
			return eyeSide;
		}

		FixationTarget getFixationTarget(void) const {
			if (measure.isOpticDiscScan()) {
				return FixationTarget::OPTIC_DISC;
			}
			if (measure.isFundusScan()) {
				return FixationTarget::FUNDUS;
			}
			return FixationTarget::CENTER;
		}

		OctScanPattern& getPreview(void) {
			return preview;
		}

		OctScanPattern& getMeasure(void) {
			return measure;
		}

		OctScanPattern& getEnface(void) {
			return enface;
		}

		OctPatternCode getMeasurePatternCode(void) {
			return measure.patternCode;
		}

		OctScanSpeed getMeasureScanSpeed(void) {
			return measure.scanSpeed;
		}

		OctScanSpeed getPreviewScanSpeed(void) {
			return preview.scanSpeed;
		}

		bool isPreviewCrossLines(void) const {
			return measure.previewType == OctPatternType::CROSS;
		}

		bool isScanSpeedFastest(void) const {
			return measure.isSpeedFaster();
		}

		bool isScanSpeedFaster(void) const {
			return measure.isSpeedNormal();
		}

		bool isScanSpeedRegular(void) const {
			return measure.isSpeedSlower();
		}

		bool isScanSpeedCustom(void) const {
			return measure.isSpeedCustom();
		}

		bool isMacularScan(void) const {
			return measure.isMarcularScan();
		}

		bool isOpticDiscScan(void) const {
			return measure.isOpticDiscScan();
		}

		bool isCorneaScan(void) const {
			return measure.isCorneaScan();
		}

		bool isAngioScan(void) const {
			return measure.isAngioPattern();
		}

		bool isCubeScan(void) const {
			return measure.isCubePattern();
		}

		bool isLineScan(void) const {
			return measure.isLinePattern();
		}

		bool isCircleScan(void) const {
			return measure.isCirclePattern();
		}

		bool isCrossScan(void) const {
			return measure.isCrossPattern();
		}

		bool isRasterScan(void) const {
			return measure.isRasterPattern();
		}

		bool isRadialScan(void) const {
			return measure.isRadialPattern();
		}

		bool isPreviewOverlapped(void) const {
			return preview.overlaps > 0;
		}

		bool isPatternOverlapped(void) const {
			return measure.overlaps > 0;
		}

		std::string getProtocolTag(void) const {
			std::string label;
			label = (isOD() ? "OD" : "OS");
			label += "_";
			label += OctScanHelper::getPatternName(measure.patternCode);
			return label;
		}

		/*
		bool loadInstance(wstring path) {
			ifstream file(wtoa(path), ios::binary);
			if (file.is_open()) {
				file.read(reinterpret_cast<char*>(this), sizeof(OctScanProtocol));
				file.close();
				return true;
			}
			return false;
		}

		bool saveInstance(wstring path) {
			ofstream file(wtoa(path), ios::binary);
			if (file.is_open()) {
				file.write(reinterpret_cast<char*>(this), sizeof(OctScanProtocol));
				file.close();
				return true;
			}
			return false;
		}
		*/

	};

}