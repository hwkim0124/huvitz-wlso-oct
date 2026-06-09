#include "pch.h"
#include "MacularScanReport.h"
#include "BscanContent.h"
#include "ProtocolSource.h"
#include "OctReport2.h"

using namespace oct_report;


struct MacularScanReport::MacularScanReportImpl
{
	bool isFoveaCenter;
	int foveaCenterLine;
	int foveaCenterXpos;
	float foveaCenterX;
	float foveaCenterY;
	float foveaCenterThickness;

	MacularScanReportImpl() : isFoveaCenter(false)
	{
	}
};


oct_report::MacularScanReport::MacularScanReport() :
	d_ptr(make_unique<MacularScanReportImpl>())
{
}


oct_report::MacularScanReport::~MacularScanReport() = default;
oct_report::MacularScanReport::MacularScanReport(MacularScanReport && rhs) = default;
MacularScanReport & oct_report::MacularScanReport::operator=(MacularScanReport && rhs) = default;


bool oct_report::MacularScanReport::isFoveaCenter(void) const
{
	return d_ptr->isFoveaCenter;
}


float oct_report::MacularScanReport::getFoveaCenterX(void) const
{
	return d_ptr->foveaCenterX;
}


float oct_report::MacularScanReport::getFoveaCenterY(void) const
{
	return d_ptr->foveaCenterY;
}


float oct_report::MacularScanReport::getFoveaCenterThickness(void) const
{
	return d_ptr->foveaCenterThickness;
}


int oct_report::MacularScanReport::getFoveaCenterLineIndex(void) const
{
	return d_ptr->foveaCenterLine;
}


int oct_report::MacularScanReport::getFoveaCenterLateralPos(void) const
{
	return d_ptr->foveaCenterXpos;
}


bool oct_report::MacularScanReport::updateContents(void)
{
	clearContents();

	if (isSourceEmpty()) {
		return false;
	}

	if (getScanPattern()->isEnfacePattern() ||
		getScanPattern()->isRadialPattern() ||
		getScanPattern()->isRasterPattern()) {
	}
	else {
		return false;
	}

	locateFoveaCenter();
	return true;
}


void oct_report::MacularScanReport::clearContents(void)
{
	d_ptr->isFoveaCenter = false;
	d_ptr->foveaCenterX = 0.0f;
	d_ptr->foveaCenterY = 0.0f;
	d_ptr->foveaCenterLine = 0;
	d_ptr->foveaCenterXpos = 0;
	d_ptr->foveaCenterThickness = 0.0f;
	return;
}

bool oct_report::MacularScanReport::getEnfaceImageDescript(OctLayerStratumParam param, OctEnfaceImageDescript& desc)
{
	MacularEnfaceImage* image;
	if (composeEnfaceImage(param, &image)) {
		if (image->fetchEnfaceImageDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool oct_report::MacularScanReport::getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc)
{
	MacularThicknessMap* tmap;
	if (composeThicknessMap(param, &tmap)) {
		if (tmap->fetchThicknessMapDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool oct_report::MacularScanReport::getETDRSChartDescript(OctLayerStratumParam param, OctETDRSChartDescript& desc)
{
	MacularETDRSChart chart;
	if (composeMacularETDRSChart(param, chart)) {
		if (chart.fetchETDRSChartDescript(desc)) {
			desc.stratumParam = param;
			return true;
		}
	}
	return false;
}

bool oct_report::MacularScanReport::getGCCThickChartDescript(OctLayerStratumParam param, OctGCCThickChartDescript& desc)
{
	GCCThicknessChart chart;
	if (composeGCCThicknessChart(param, chart)) {
		if (chart.fetchGCCThickChartDescript(desc)) {
			desc.stratumParam = param;
			return true;
		}
	}
	return false;
}

bool oct_report::MacularScanReport::getMacularSummaryDescript(OctMacularSummaryDescript& desc)
{
	if (isSourceEmpty()) {
		return false;
	}

	desc.eyeSide = getEyeSide();
	desc.foveaCenterX = getFoveaCenterX();
	desc.foveaCenterY = getFoveaCenterY();
	desc.foveaCenterThick = getFoveaCenterThickness();
	desc.foveaCenterLine = getFoveaCenterLineIndex();
	desc.foveaCenterXpos = getFoveaCenterLateralPos();
	desc.isFoveaValid = isFoveaCenter();
	return true;
}


segm_scan::MacularEnfaceImage * oct_report::MacularScanReport::getEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto shot = prepareEnfaceImage(upper, lower, upperOffset, lowerOffset);
	return static_cast<MacularEnfaceImage*>(shot);
}


segm_scan::MacularThicknessMap * oct_report::MacularScanReport::getThicknessMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto tmap = prepareEnfaceMap(upper, lower, upperOffset, lowerOffset);
	return static_cast<MacularThicknessMap*>(tmap);
}


bool oct_report::MacularScanReport::composeGCCThicknessChart(OctLayerStratumParam param, segm_scan::GCCThicknessChart& chart)
{
	chart = GCCThicknessChart();

	auto tmap = getThicknessMap(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (tmap) {
		chart.setEnfaceData(tmap->getThicknessMap());
		chart.setLocation(param.chartCenterX, param.chartCenterY);
		chart.setEyeSide(getEyeSide());
		chart.updateMetrics();
		return true;
	}
	return false;
}

bool oct_report::MacularScanReport::composeMacularETDRSChart(OctLayerStratumParam param, segm_scan::MacularETDRSChart& chart)
{
	chart = MacularETDRSChart();

	auto tmap = getThicknessMap(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (tmap) {
		chart.setEnfaceData(tmap->getThicknessMap());
		chart.setLocation(param.chartCenterX, param.chartCenterY);
		chart.setEyeSide(getEyeSide());
		chart.updateMetrics();
		return true;
	}
	return false;
}

bool oct_report::MacularScanReport::composeEnfaceImage(OctLayerStratumParam param, segm_scan::MacularEnfaceImage** image)
{
	auto enface = getEnfaceImage(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (enface) {
		*image = enface;
		return true;
	}
	return false;
}

bool oct_report::MacularScanReport::composeThicknessMap(OctLayerStratumParam param, segm_scan::MacularThicknessMap** thick)
{
	auto tmap = getThicknessMap(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (tmap) {
		*thick = tmap;
		return true;
	}
	return false;
}

void oct_report::MacularScanReport::locateFoveaCenter(void)
{
	float rangeX = getScanPattern()->getScanRangeX();
	float rangeY = getScanPattern()->getScanRangeY();
	int numLines = getScanPattern()->getNumberOfScanLines();
	int numPoints = getScanPattern()->getNumberOfScanPoints();

	int numBscans = getProtocolSource()->getPatternContentCount();
	if (numBscans != numLines || numBscans <= 0) {
		return;
	}

	int disc_line = (int)(numLines * (FOVEA_CENTER_DISC_RADIUS_IN_MM / rangeY));
	int line_sidx = disc_line + 1;
	int line_eidx = numLines - line_sidx - 1;
	int disc_side = (int)(numPoints * (FOVEA_CENTER_DISC_RADIUS_IN_MM / rangeX));
	int spot_sidx = disc_side + 1;
	int spot_eidx = numPoints - spot_sidx - 1;

	if (getScanPattern()->isRadialPattern()) {
	}
	else {
		if (rangeX >= FOVEA_OPTIC_DISC_RANGE_X) {
			if (isEyeOD()) {
				spot_eidx = (int)(numPoints * 0.70f);
			}
			else {
				spot_sidx = (int)(numPoints * 0.30f);
			}
		}
	}

	// LogD() << line_sidx << ", " << line_eidx << ", " << disc_side << ", " << spot_sidx << ", " << spot_eidx;

	float axialRes = (float)OctDataSetup::getRetinaScanAxialResolution();
	float slopeDepth = 0.0f, foveaDepth = 0.0f;
	int centerX = 0, centerY = 0;

	for (int i = line_sidx; i <= line_eidx; i++) {
		auto p = getProtocolSource()->getPatternContent(i);
		if (!p) {
			continue;
		}

		auto inner = p->getLayerPoints(OcularLayerType::ILM);
		auto outer = p->getLayerPoints(OcularLayerType::RPE);
		auto nerve = p->getLayerPoints(OcularLayerType::NFL);

		auto thicks = p->getLayerDistance(OcularLayerType::ILM, OcularLayerType::RPE);

		if (all_of(inner.cbegin(), inner.cend(), [](int e) { return e <= 0; }) ||
			all_of(outer.cbegin(), outer.cend(), [](int e) { return e <= 0; }) ||
			all_of(nerve.cbegin(), nerve.cend(), [](int e) { return e <= 0; }) ||
			inner.size() != numPoints ||
			outer.size() != numPoints || 
			nerve.size() != numPoints) {
			continue;
		}

		for (int j = spot_sidx; j <= spot_eidx; j++) {
			if (inner[j] <= 0 || outer[j] <= 0 || nerve[j] <= 0) {
				continue;
			}

			int cent_h = (outer[j] - inner[j]);
			float cent_d = cent_h * axialRes;
			float nerv_d = (nerve[j] - inner[j]) * axialRes;

			if (nerv_d > FOVEA_CENTER_DISC_NERVE_LIMIT) {
				continue;
			}

			int sidx = max((j - disc_side), 0);
			int eidx = min((j + disc_side), numPoints-1);

			nerv_d = (nerve[sidx] - inner[sidx]) * axialRes;
			if (nerv_d > FOVEA_CENTER_DISC_NERVE_LIMIT) {
				continue;
			}

			nerv_d = (nerve[eidx] - inner[eidx]) * axialRes;
			if (nerv_d > FOVEA_CENTER_DISC_NERVE_LIMIT) {
				continue;
			}

			float disc_bh = abs(outer[eidx] - outer[sidx]) * axialRes;
			if (disc_bh > FOVEA_DISC_BOTTOM_DIFF_MAX) {
				continue;
			}

			float disc_bt = min(outer[sidx] - outer[j], outer[eidx] - outer[j]) * axialRes;
			if (disc_bt > FOVEA_DISC_BOTTOM_OFFSET) {
				continue;
			}

			int disc_h1 = 0, disc_h2 = 0;
			int disc_p1 = 0, disc_p2 = 0;

			for (int k = j; k >= sidx; k--) {
				if (outer[k] >= 0 && inner[k] >= 0 && nerve[k] >= 0) {
					if (thicks[k] < cent_h || (inner[k] > (inner[j] + 2))) {
						disc_h1 = disc_p1 = 0;
						break;
					}
					disc_h1 = max(disc_h1, (outer[k] - inner[k]));
					disc_p1 = k;
				}
			}
			for (int k = j; k <= eidx; k++) {
				if (outer[k] >= 0 && inner[k] >= 0 && nerve[k] >= 0) {
					if (thicks[k] < cent_h || (inner[k] > (inner[j] + 2))) {
						disc_h2 = disc_p2 = 0;
						break;
					}
					disc_h2 = max(disc_h2, (outer[k] - inner[k]));
					disc_p2 = k;
				}
			}

			if (disc_h1 > 0 && disc_h2 > 0) {
				float disc_d1 = disc_h1 * axialRes;
				float disc_d2 = disc_h2 * axialRes;
				float disc_d = min(disc_d1, disc_d2) - cent_d;
				
				float disc_a1 = 0, disc_a2 = 0;
				for (int k = disc_p1; k <= j; k++) {
					disc_a1 += thicks[k];
				}

				for (int k = disc_p2; k >= j; k--) {
					disc_a2 += thicks[k];
				}

				float ratio_d = max(disc_d1, disc_d2) / min(disc_d1, disc_d2);
				float ratio_a = max(disc_a1, disc_a2) / min(disc_a1, disc_a2);

				if (ratio_d < FOVEA_DISC_DEPTH_DIFF_MAX && ratio_a < FOVEA_DISC_AREA_DIFF_MAX) {
					if (slopeDepth < disc_d) {
						slopeDepth = disc_d;
						foveaDepth = cent_d;
						centerX = j;
						centerY = i;
						/*
						LogD() << "h1: " << disc_h1 << ", h2: " << disc_h2 << ", disc_d: " << disc_d;
						LogD() << "p1: " << disc_p1 << ", p2: " << disc_p2 << ", disc_a1: " << disc_a1 << ", disc_a2: " << disc_a2;
						LogD() << "ratio_d: " << ratio_d << ", ratio_a: " << ratio_a;
						LogD() << slopeDepth << ", " << foveaDepth << ", " << centerX << ", " << centerY;
						*/
					}
				}
			}
		}
	}

	if (slopeDepth > 0.0f)
	{
		float ratio = (float)slopeDepth / foveaDepth;
		if (ratio >= FOVEA_CENTER_DISC_DIFF_RATIO_MIN) {
			d_ptr->isFoveaCenter = true;
			d_ptr->foveaCenterLine = centerY;
			d_ptr->foveaCenterXpos = centerX;
			d_ptr->foveaCenterThickness = foveaDepth;

			float xPosMM, yPosMM;
			if (getScanPattern()->isRadialPattern()) {
				float offset = (((float)centerX / (float)numPoints) - 0.5f) * rangeX;
				float degree = (180.0f / numLines) * centerY;

				xPosMM = (float)(offset * cos(degreeToRadian(degree)));
				yPosMM = (float)(offset * sin(degreeToRadian(degree)));
			}
			else {
				if (getScanPattern()->isHorizontalScan()) {
					xPosMM = (((float)centerX / (float)numPoints) - 0.5f) * rangeX;
					yPosMM = (((float)centerY / (float)numLines) - 0.5f) * rangeY;
				}
				else {
					yPosMM = (((float)centerX / (float)numPoints) - 0.5f) * rangeX;
					xPosMM = (((float)centerY / (float)numLines) - 0.5f) * rangeY;
				}
			}

			d_ptr->foveaCenterX = xPosMM;
			d_ptr->foveaCenterY = yPosMM;
			// LogD() << "ratio: " << ratio;
		}
	}

	/*
	int line_size = getProtocolSource()->countBscanData();

	int centerXpos;
	int centerLine;
	int curr_d, next_d, prev_d, mean_d;
	int disc_max, fovea_min;

	int line_sidx = (int)(line_size * FOVEA_CENTER_LINE_START);
	int line_eidx = (int)(line_size * FOVEA_CENTER_LINE_CLOSE);

	disc_max = fovea_min = 0;
	for (int i = line_sidx; i < line_eidx; i++) {
		auto p = getProtocolSource()->getBscanData(i);

		auto inner = p->getLayerPoints(OcularLayerType::ILM);
		auto outer = p->getLayerPoints(OcularLayerType::RPE);
		int width = (int)inner.size();

		int spos = (int)(width * FOVEA_CENTER_XPOS_START);
		int epos = (int)(width * FOVEA_CENTER_XPOS_CLOSE);
		int side = (int)(width * FOVEA_CENTER_DISC_WIDTH);

		curr_d = next_d = prev_d = 0;
		for (int j = spos; j <= epos; j++) {
			if (outer[j] >= 0 && inner[j] >= 0) {
				curr_d = outer[j] - inner[j];
				next_d = prev_d = 0;

				int sidx = max((j - side), 0);
				int eidx = min((j + side), width);

				for (int k = j; k >= sidx; k--) {
					if (outer[k] >= 0 && inner[k] >= 0) {
						prev_d = max(prev_d, (outer[k] - inner[k]));
					}
				}

				for (int k = j; k < eidx; k++) {
					if (outer[k] >= 0 && inner[k] >= 0) {
						next_d = max(next_d, (outer[k] - inner[k]));
					}
				}

				prev_d = max(0, prev_d - curr_d);
				next_d = max(0, next_d - curr_d);
				mean_d = min(prev_d, next_d); // (prev_d + next_d) / 2;

				if (disc_max < mean_d) {
					disc_max = mean_d;
					fovea_min = curr_d;
					centerXpos = j;
					centerLine = i;
				}
			}
		}
	}

	if (fovea_min > 0) 
	{
		float axialResol = (float)GlobalSettings::getRetinaScanAxialResolution();

		float ratio = (float) disc_max / fovea_min;
		if (ratio > FOVEA_CENTER_DISC_DIFF_RATIO_MIN) {
			d_ptr->isFoveaCenter = true;
			d_ptr->foveaCenterLine = centerLine;
			d_ptr->foveaCenterXpos = centerXpos;
			d_ptr->foveaCenterThickness = fovea_min * axialResol;

			float rangeX = getDescript()->getScanRangeX();
			float rangeY = getDescript()->getScanRangeY();
			int numLines = getDescript()->getNumberOfScanLines();
			int numPoints = getDescript()->getNumberOfScanPoints();

			float centerX, centerY;
			if (getDescript()->isHorizontalScan()) {
				centerX = (((float)centerXpos / (float)numPoints) - 0.5f) * rangeX;
				centerY = (((float)centerLine / (float)numLines) - 0.5f) * rangeY;
			}
			else {
				centerY = (((float)centerXpos / (float)numPoints) - 0.5f) * rangeX;
				centerX = (((float)centerLine / (float)numLines) - 0.5f) * rangeY;
			}

			d_ptr->foveaCenterX = centerX;
			d_ptr->foveaCenterY = centerY;
		}
	}
	*/
	return;
}


std::unique_ptr<segm_scan::OcularEnfaceImage> oct_report::MacularScanReport::createEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto enface = unique_ptr<MacularEnfaceImage>(new MacularEnfaceImage);

	const auto& desc = getProtocolSource()->getScanPattern();
	const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

	bool result = enface->setupEnfaceImage(desc, upper, lower, upperOffset, lowerOffset, bsegms);

	if (result) {
		return std::move(enface);
	}
	return nullptr;
}


std::unique_ptr<segm_scan::OcularEnfaceMap> oct_report::MacularScanReport::createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto plot = unique_ptr<MacularEnfaceMap>(new MacularEnfaceMap);

	const auto& desc = getProtocolSource()->getScanPattern();
	const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

	bool result = plot->setupThicknessMap(desc, upper, lower, upperOffset, lowerOffset, bsegms);

	if (result) {
		return std::move(plot);
	}
	return nullptr;
}


MacularScanReport::MacularScanReportImpl & oct_report::MacularScanReport::getImpl(void) const
{
	return *d_ptr;
}