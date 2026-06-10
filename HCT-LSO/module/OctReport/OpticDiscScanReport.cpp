#include "pch.h"
#include "OpticDiscScanReport.h"
#include "BscanContent.h"
#include "ProtocolSource.h"
#include "OctReport2.h"

using namespace oct_report;
using namespace std;


struct OpticDiscScanReport::OpticDiscScanReportImpl
{
	bool isOpticCup;
	bool isOpticDisc;

	int discStartLine;
	int discCloseLine;
	int discCenterLine;
	int discCenterXpos;
	int discXposMin;
	int discXposMax;

	float discArea;
	float discVolume;

	int cupStartLine;
	int cupCloseLine;
	int cupCenterLine;
	int cupCenterXpos;
	int cupXposMin;
	int cupXposMax;

	float cupArea;
	float cupVolume;

	float rimArea;
	float rimVolume;

	bool isNerveHeadCenter;
	int nerveHeadCenterLine;
	int nerveHeadCenterXpos;
	float nerveHeadCenterX;
	float nerveHeadCenterY;

	bool isCupDiscRatio;
	float cupDiscHorzRatio;
	float cupDiscVertRatio;
	float cupDiscAreaRatio;

	OpticDiscScanReportImpl() : isOpticCup(false), isOpticDisc(false), isNerveHeadCenter(false), 
		cupDiscHorzRatio(0.0f), cupDiscVertRatio(0.0f), isCupDiscRatio(false)
	{
	}
};


oct_report::OpticDiscScanReport::OpticDiscScanReport() :
	d_ptr(make_unique<OpticDiscScanReportImpl>())
{
}


oct_report::OpticDiscScanReport::~OpticDiscScanReport() = default;
oct_report::OpticDiscScanReport::OpticDiscScanReport(OpticDiscScanReport && rhs) = default;
OpticDiscScanReport & oct_report::OpticDiscScanReport::operator=(OpticDiscScanReport && rhs) = default;

bool oct_report::OpticDiscScanReport::isNerveHeadCup(void) const
{
	return d_ptr->isOpticCup;
}


bool oct_report::OpticDiscScanReport::isNerveHeadDisc(void) const
{
	return d_ptr->isOpticDisc;
}


bool oct_report::OpticDiscScanReport::isNerveHeadCenter(void) const
{
	return d_ptr->isNerveHeadCenter;
}


int oct_report::OpticDiscScanReport::getNerveHeadCenterLineIndex(void) const
{
	return d_ptr->nerveHeadCenterLine;
}


int oct_report::OpticDiscScanReport::getNerveHeadCenterLateralPos(void) const
{
	return d_ptr->nerveHeadCenterXpos;
}


float oct_report::OpticDiscScanReport::getNerveHeadCenterX(void) const
{
	return d_ptr->nerveHeadCenterX;
}


float oct_report::OpticDiscScanReport::getNerveHeadCenterY(void) const
{
	return d_ptr->nerveHeadCenterY;
}


float oct_report::OpticDiscScanReport::getDiscArea(void) const
{
	return d_ptr->discArea;
}


float oct_report::OpticDiscScanReport::getDiscVolume(void) const
{
	return d_ptr->discVolume;
}


float oct_report::OpticDiscScanReport::getCupArea(void) const
{
	return d_ptr->cupArea;
}


float oct_report::OpticDiscScanReport::getCupVolume(void) const
{
	return d_ptr->cupVolume;
}


float oct_report::OpticDiscScanReport::getRimArea(void) const
{
	return d_ptr->rimArea;
}


float oct_report::OpticDiscScanReport::getCupDiscHorzRatio(void) const
{
	return d_ptr->cupDiscHorzRatio;
}


float oct_report::OpticDiscScanReport::getCupDiscVertRatio(void) const
{
	return d_ptr->cupDiscVertRatio;
}


float oct_report::OpticDiscScanReport::getCupDiscAreaRatio(void) const
{
	return d_ptr->cupDiscAreaRatio;
}


bool oct_report::OpticDiscScanReport::updateContents(void)
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

	locateCupDiscCenter();
	calculateCupDiscRatio();
	calculateCupDiscArea();
	calculateCupDiscVolume();
	return true;
}


void oct_report::OpticDiscScanReport::clearContents(void)
{
	d_ptr->isNerveHeadCenter = false;
	d_ptr->isCupDiscRatio = false;
	d_ptr->isOpticCup = false;
	d_ptr->isOpticDisc = false;

	d_ptr->nerveHeadCenterLine = 0;
	d_ptr->nerveHeadCenterXpos = 0;
	d_ptr->nerveHeadCenterX = 0.0f;
	d_ptr->nerveHeadCenterY = 0.0f;

	d_ptr->discArea = 0.0f;
	d_ptr->discVolume = 0.0f;
	d_ptr->cupArea = 0.0f;
	d_ptr->cupVolume = 0.0f;
	d_ptr->cupDiscHorzRatio = 0.0f;
	d_ptr->cupDiscVertRatio = 0.0f;
	d_ptr->cupDiscAreaRatio = 0.0f;
	d_ptr->rimArea = 0.0f;
	d_ptr->rimVolume = 0.0f;
	return;
}

bool oct_report::OpticDiscScanReport::getEnfaceImageDescript(OctLayerStratumParam param, OctEnfaceImageDescript& desc)
{
	DiscEnfaceImage* image;
	if (composeEnfaceImage(param, &image)) {
		if (image->fetchEnfaceImageDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool oct_report::OpticDiscScanReport::getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc)
{
	DiscThicknessMap* tmap;
	if (composeThicknessMap(param, &tmap)) {
		if (tmap->fetchThicknessMapDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool oct_report::OpticDiscScanReport::getClockChartDescript(OctLayerStratumParam param, OctClockChartDescript& desc)
{
	DiscClockChart chart;
	if (composeDiscClockChart(param, chart)) {
		if (chart.fetchClockChartDescript(desc)) {
			desc.stratumParam = param;
			return true;
		}
	}
	return false;
}

bool oct_report::OpticDiscScanReport::getOpticDiscSummaryDescript(OctOpticDiscSummaryDescript& desc)
{
	if (isSourceEmpty()) {
		return false;
	}

	desc.eyeSide = getEyeSide();
	desc.discStartLine = d_ptr->discStartLine;
	desc.discCloseLine = d_ptr->discCloseLine;
	desc.discCenterLine = d_ptr->discCenterLine;
	desc.discCenterXpos = d_ptr->discCenterXpos;
	desc.discXposMin = d_ptr->discXposMin;
	desc.discXposMax = d_ptr->discXposMax;

	desc.discArea = d_ptr->discArea;
	desc.discVolume = d_ptr->discVolume;

	desc.cupStartLine = d_ptr->cupStartLine;
	desc.cupCloseLine = d_ptr->cupCloseLine;
	desc.cupCenterLine = d_ptr->cupCenterLine;
	desc.cupCenterXpos = d_ptr->cupCenterXpos;
	desc.cupXposMin = d_ptr->cupXposMin;
	desc.cupXposMax = d_ptr->cupXposMax;

	desc.cupArea = d_ptr->cupArea;
	desc.cupVolume = d_ptr->cupVolume;
	desc.rimArea = d_ptr->rimArea;
	desc.rimVolume = d_ptr->rimVolume;

	desc.nerveHeadCenterLine = d_ptr->nerveHeadCenterLine;
	desc.nerveHeadCenterXpos = d_ptr->nerveHeadCenterXpos;
	desc.nerveHeadCenterX = d_ptr->nerveHeadCenterX;
	desc.nerveHeadCenterY = d_ptr->nerveHeadCenterY;

	desc.cupDiscHorzRatio = d_ptr->cupDiscHorzRatio;
	desc.cupDiscVertRatio = d_ptr->cupDiscVertRatio;
	desc.cupDiscAreaRatio = d_ptr->cupDiscAreaRatio;

	desc.isOpticCupValid = d_ptr->isOpticCup;
	desc.isOpticDiscValid = d_ptr->isOpticDisc;
	desc.isNerveHeadCenterValid = d_ptr->isNerveHeadCenter;
	desc.isCupDiscRatioValid = d_ptr->isCupDiscRatio;

	return true;
}

bool oct_report::OpticDiscScanReport::composeDiscClockChart(OctLayerStratumParam param, segm_scan::DiscClockChart& chart, float circleSize)
{
	chart = DiscClockChart();
	chart.setCircleSize(circleSize);

	auto upper = param.upperType;
	auto lower = param.lowerType;
	auto upperOffset = param.upperOffset;
	auto lowerOffset = param.lowerOffset;
	auto centerX = param.chartCenterX;
	auto centerY = param.chartCenterY;

	if (getProtocolSource()->getScanPattern().isCirclePattern()) {
		if (getProtocolSource()->getPatternBscanSegmList().size() > 0) {
			auto bsegm = getProtocolSource()->getPatternBscanSegmList()[0];
			if (bsegm) {
				vector<float> thicks;
				if (bsegm->getThicknessLine(upper, lower, thicks, 0, 0, upperOffset, lowerOffset)) {
					chart.setEyeSide(getEyeSide());
					chart.updateMetrics(thicks);
					return true;
				}
			}
		}
	}
	else {
		auto tmap = getThicknessMap(upper, lower, upperOffset, lowerOffset);
		if (tmap) {
			chart.setEnfaceData(tmap->getThicknessMap());
			chart.setEyeSide(getEyeSide());
			chart.setLocation(centerX, centerY);
			chart.updateMetrics();
			return true;
		}
	}
	return false;
}

bool oct_report::OpticDiscScanReport::composeEnfaceImage(OctLayerStratumParam param, segm_scan::DiscEnfaceImage** image)
{
	auto enface = getEnfaceImage(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (enface) {
		*image = enface;
		return true;
	}
	return false;
}

bool oct_report::OpticDiscScanReport::composeThicknessMap(OctLayerStratumParam param, segm_scan::DiscThicknessMap** thick)
{
	auto tmap = getThicknessMap(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (tmap) {
		*thick = tmap;
		return true;
	}
	return false;
}


segm_scan::DiscEnfaceImage * oct_report::OpticDiscScanReport::getEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto shot = prepareEnfaceImage(upper, lower, upperOffset, lowerOffset);
	return static_cast<DiscEnfaceImage*>(shot);
}


segm_scan::DiscThicknessMap * oct_report::OpticDiscScanReport::getThicknessMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto tmap = prepareEnfaceMap(upper, lower, upperOffset, lowerOffset);
	return static_cast<DiscThicknessMap*>(tmap);
}

void oct_report::OpticDiscScanReport::locateCupDiscCenter(void)
{
	int size = getProtocolSource()->getPatternContentCount();

	auto disc_lines = std::vector<int>();
	auto disc_cents = std::vector<int>();
	auto disc_xmins = std::vector<int>();
	auto disc_xmaxs = std::vector<int>();

	int x1, x2;
	for (int i = 0; i < size; i++) {
		auto p = getProtocolSource()->getPatternContent(i);
		if (p->getOpticNerveDiscRange(x1, x2)) {
			disc_lines.push_back(i);
			disc_cents.push_back((x1 + x2) / 2);
			disc_xmins.push_back(x1);
			disc_xmaxs.push_back(x2);
		}
	}

	if (!disc_lines.empty()) {
		d_ptr->isOpticDisc = true;
		d_ptr->discStartLine = disc_lines.front();
		d_ptr->discCloseLine = disc_lines.back();

		sort(disc_cents.begin(), disc_cents.end());
		sort(disc_xmins.begin(), disc_xmins.end());
		sort(disc_xmaxs.begin(), disc_xmaxs.end());

		d_ptr->discCenterLine = disc_lines[disc_lines.size() / 2];
		d_ptr->discCenterXpos = disc_cents[disc_cents.size() / 2];
		d_ptr->discXposMin = disc_xmins.front();
		d_ptr->discXposMax = disc_xmaxs.back();

		auto p = getProtocolSource()->getPatternContent(d_ptr->discCenterLine);
		if (p->getOpticNerveDiscRange(x1, x2)) {
			d_ptr->discXposMin = x1;
			d_ptr->discXposMax = x2;
		}
	}
	else {
		d_ptr->isOpticDisc = false;
	}

	auto cup_lines = std::vector<int>();
	auto cup_cents = std::vector<int>();
	auto cup_xmins = std::vector<int>();
	auto cup_xmaxs = std::vector<int>();

	for (int i = 0; i < size; i++) {
		auto p = getProtocolSource()->getPatternContent(i);
		if (p->getOpticNerveCupRange(x1, x2)) {
			cup_lines.push_back(i);
			cup_cents.push_back((x1 + x2) / 2);
			cup_xmins.push_back(x1);
			cup_xmaxs.push_back(x2);
		}
	}

	if (!cup_lines.empty()) {
		d_ptr->isOpticCup = true;
		d_ptr->cupStartLine = cup_lines.front();
		d_ptr->cupCloseLine = cup_lines.back();

		sort(cup_cents.begin(), cup_cents.end());
		sort(cup_xmins.begin(), cup_xmins.end());
		sort(cup_xmaxs.begin(), cup_xmaxs.end());

		d_ptr->cupCenterLine = cup_lines[cup_lines.size() / 2];
		d_ptr->cupCenterXpos = cup_cents[cup_cents.size() / 2];
		d_ptr->cupXposMin = cup_xmins.front();
		d_ptr->cupXposMax = cup_xmaxs.back();

		auto p = getProtocolSource()->getPatternContent(d_ptr->cupCenterLine);
		if (p->getOpticNerveCupRange(x1, x2)) {
			d_ptr->cupXposMin = x1;
			d_ptr->cupXposMax = x2;
		}
	}
	else {
		d_ptr->isOpticCup = false;
	}

	/*
	if (d_ptr->isOpticCup) {
		if (cup_lines.size() >= (int)(disc_lines.size() * 0.25f)) {
			d_ptr->nerveHeadCenterLine = d_ptr->cupCenterLine;
			d_ptr->nerveHeadCenterXpos = d_ptr->cupCenterXpos;
		}
		else {
			d_ptr->nerveHeadCenterLine = d_ptr->discCenterLine;
			d_ptr->nerveHeadCenterXpos = d_ptr->discCenterXpos;
		}
		d_ptr->isNerveHeadCenter = true;
	}
	else */ if (d_ptr->isOpticDisc) {
		d_ptr->nerveHeadCenterLine = d_ptr->discCenterLine;
		d_ptr->nerveHeadCenterXpos = d_ptr->discCenterXpos;
		d_ptr->isNerveHeadCenter = true;
	}
	else {
		d_ptr->isNerveHeadCenter = false;
	}

	if (d_ptr->isNerveHeadCenter) {
		float rangeX = getScanPattern()->getScanRangeX();
		float rangeY = getScanPattern()->getScanRangeY();
		int numLines = getScanPattern()->getNumberOfScanLines();
		int numPoints = getScanPattern()->getNumberOfScanPoints();

		int centerLine = d_ptr->nerveHeadCenterLine;
		int centerXpos = d_ptr->nerveHeadCenterXpos;

		float centerX, centerY;
		if (getScanPattern()->isHorizontalScan()) {
			centerX = (((float)centerXpos / (float)numPoints) - 0.5f) * rangeX;
			centerY = (((float)centerLine / (float)numLines) - 0.5f) * rangeY;
		}
		else {
			centerY = (((float)centerXpos / (float)numPoints) - 0.5f) * rangeX;
			centerX = (((float)centerLine / (float)numLines) - 0.5f) * rangeY;
		}

		d_ptr->nerveHeadCenterX = centerX;
		d_ptr->nerveHeadCenterY = centerY;

		LogD() << "Nerve head center, line: " << centerLine << ", xpos: " << centerXpos;
	}
	return;
}


void oct_report::OpticDiscScanReport::calculateCupDiscArea(void)
{
	if (!d_ptr->isNerveHeadCenter) {
		return;
	}

	float unit_w = getProtocolSource()->getScanPattern().getPixelWidth();
	float unit_h = getProtocolSource()->getScanPattern().getLineDistance();
	float unit_a = unit_w * unit_h;

	float cupArea = 0.0f;
	float discArea = 0.0f;

	int x1, x2;
	int pixels = 0;

	if (d_ptr->isOpticDisc) {
		for (int i = d_ptr->discStartLine; i <= d_ptr->discCloseLine; i++) {
			auto p = getProtocolSource()->getPatternContent(i);

			if (p->getOpticNerveDiscRange(x1, x2)) {
				// LogD() << "Area line: " << i << ", disc range: " << x1 << ", " << x2;
				pixels += (x2 - x1 + 1);
			}
		}

		discArea = pixels * unit_a;
	}

	if (d_ptr->isOpticCup) {
		pixels = 0;
		for (int i = d_ptr->cupStartLine; i <= d_ptr->cupCloseLine; i++) {
			auto p = getProtocolSource()->getPatternContent(i);

			if (p->getOpticNerveCupRange(x1, x2)) {
				// LogD() << "Area line: " << i << ", cup range: " << x1 << ", " << x2;
				pixels += (x2 - x1 + 1);
			}
		}

		cupArea = pixels * unit_a;
	}

	d_ptr->discArea = discArea;
	d_ptr->cupArea = cupArea;
	d_ptr->rimArea = discArea - cupArea;
	d_ptr->cupDiscAreaRatio = cupArea / discArea;

	LogD() << "Disc area: " << discArea << ", cup area: " << cupArea << ", ratio: " << d_ptr->cupDiscAreaRatio;
	return;
}


void oct_report::OpticDiscScanReport::calculateCupDiscVolume(void)
{
	if (!d_ptr->isNerveHeadCenter) {
		return;
	}

	float unit_w = getProtocolSource()->getScanPattern().getPixelWidth();
	float unit_d = getProtocolSource()->getScanPattern().getPixelHeight() * 0.001f;
	float unit_h = getProtocolSource()->getScanPattern().getLineDistance();
	float unit_v = unit_w * unit_h * unit_d;

	float cupVolume = 0.0f;
	float discVolume = 0.0f;

	int pixels = 0;
	int voxels = 0;

	if (d_ptr->isOpticDisc) {
		for (int i = d_ptr->discStartLine; i <= d_ptr->discCloseLine; i++) {
			auto p = getProtocolSource()->getPatternContent(i);

			if (p->getOpticNerveDiscPixels(pixels)) {
				voxels += pixels;
			}
		}

		discVolume = voxels * unit_v;
	}

	if (d_ptr->isOpticCup) {
		pixels = 0;
		voxels = 0;
		for (int i = d_ptr->cupStartLine; i <= d_ptr->cupCloseLine; i++) {
			auto p = getProtocolSource()->getPatternContent(i);

			if (p->getOpticNerveCupPixels(pixels)) {
				voxels += pixels;
			}
		}

		cupVolume = voxels * unit_v;
	}

	d_ptr->discVolume = discVolume;
	d_ptr->cupVolume = cupVolume;
	return;
}


void oct_report::OpticDiscScanReport::calculateCupDiscRatio(void)
{


	if (d_ptr->isOpticCup && d_ptr->isOpticDisc)
	{
		int discLines = (d_ptr->discCloseLine - d_ptr->discStartLine + 1);
		// int discWidth = min(abs(d_ptr->discCenterXpos - d_ptr->discXposMax), abs(d_ptr->discCenterXpos - d_ptr->discXposMin)) + 1;
		int discWidth = (d_ptr->discXposMax - d_ptr->discXposMin + 1);

		int cupLines = (d_ptr->cupCloseLine - d_ptr->cupStartLine + 1);
		// int cupWidth = min(abs(d_ptr->cupCenterXpos - d_ptr->cupXposMax), abs(d_ptr->cupCenterXpos - d_ptr->cupXposMin)) + 1;
		int cupWidth = (d_ptr->cupXposMax - d_ptr->cupXposMin + 1);

		int x1, x2;
		int dsum = 0, dcnt = 0;
		int dOffset = max((int)(discLines * 0.2588f) / 2, 2);
		for (int i = (d_ptr->discCenterLine - dOffset); i <= (d_ptr->discCenterLine + dOffset); i++) {
			if (i < 0 || i >= getProtocolSource()->getPatternContentCount()) {
				continue;
			}
			auto p = getProtocolSource()->getPatternContent(i);
			if (p->getOpticNerveDiscRange(x1, x2)) {
				dsum += (x2 - x1 + 1);
				dcnt += 1;
			}
		}
		if (dcnt > 0) {
			discWidth = dsum / dcnt;
		}

		int csum = 0, ccnt = 0;
		int cOffset = max((int)(cupLines * 0.2588f) / 2, 1);
		for (int i = (d_ptr->cupCenterLine - cOffset); i <= (d_ptr->cupCenterLine + cOffset); i++) {
			if (i < 0 || i >= getProtocolSource()->getPatternContentCount()) {
				continue;
			}
			auto p = getProtocolSource()->getPatternContent(i);
			if (p->getOpticNerveCupRange(x1, x2)) {
				csum += (x2 - x1 + 1);
				ccnt += 1;
			}
		}
		if (ccnt > 0) {
			cupWidth = csum / ccnt;
		}

		if (getProtocolSource()->getScanPattern().isHorizontalScan()) {
			d_ptr->cupDiscHorzRatio = (float)cupWidth / (float)discWidth;
			d_ptr->cupDiscVertRatio = (float)cupLines / (float)discLines;
		}
		else {
			d_ptr->cupDiscHorzRatio = (float)cupLines / (float)discLines;
			d_ptr->cupDiscVertRatio = (float)cupWidth / (float)discWidth;
		}
		d_ptr->isCupDiscRatio = true;
	}
	return;
}


std::unique_ptr<segm_scan::OcularEnfaceImage> oct_report::OpticDiscScanReport::createEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto enface = unique_ptr<DiscEnfaceImage>(new DiscEnfaceImage);

	const auto& desc = getProtocolSource()->getScanPattern();
	const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

	bool result = enface->setupEnfaceImage(desc, upper, lower, upperOffset, lowerOffset, bsegms);

	if (result) {
		return std::move(enface);
	}
	return nullptr;
}


std::unique_ptr<segm_scan::OcularEnfaceMap> oct_report::OpticDiscScanReport::createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto plot = unique_ptr<DiscEnfaceMap>(new DiscEnfaceMap);

	const auto& desc = getProtocolSource()->getScanPattern();
	const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

	bool result = plot->setupThicknessMap(desc, upper, lower, upperOffset, lowerOffset, bsegms);

	if (result) {
		return std::move(plot);
	}
	return nullptr;
}



OpticDiscScanReport::OpticDiscScanReportImpl & oct_report::OpticDiscScanReport::getImpl(void) const
{
	return *d_ptr;
}