#include "pch.h"
#include "CorneaScanReport.h"
#include "ProtocolSource.h"
#include "OctReport2.h"

using namespace oct_report;


struct CorneaScanReport::CorneaScanReportImpl
{
	CorneaEplot radiusPlot;
	CorneaRadiusMap radiusMap;

	CorneaScanReportImpl()
	{
	}
};


oct_report::CorneaScanReport::CorneaScanReport() :
	d_ptr(make_unique<CorneaScanReportImpl>())
{
}


oct_report::CorneaScanReport::~CorneaScanReport() = default;
oct_report::CorneaScanReport::CorneaScanReport(CorneaScanReport && rhs) = default;
CorneaScanReport & oct_report::CorneaScanReport::operator=(CorneaScanReport && rhs) = default;

bool oct_report::CorneaScanReport::updateContents(void)
{
	return true;
}


void oct_report::CorneaScanReport::clearContents(void)
{
	return;
}

bool oct_report::CorneaScanReport::getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc)
{
	CorneaThicknessMap* tmap;
	if (composeThicknessMap(param, &tmap)) {
		if (tmap->fetchThicknessMapDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool oct_report::CorneaScanReport::composeCorneaETDRSChart(OctLayerStratumParam param, segm_scan::CorneaETDRSChart& chart)
{
	chart = CorneaETDRSChart();

	auto tmap = getCorneaEplot(param.upperType, param.lowerType);
	if (tmap) {
		chart.setEnfaceData(tmap->getThicknessMap());
		chart.setLocation(param.chartCenterX, param.chartCenterY);
		chart.setEyeSide(getEyeSide());
		chart.updateMetrics();
		return true;
	}

	return false;
}

bool oct_report::CorneaScanReport::composeCurvatureRadiusMap(OctLayerStratumParam param, segm_scan::CorneaRadiusMap** rmap)
{
	if (d_ptr->radiusMap.isEmpty()) {
		const auto& desc = getProtocolSource()->getScanPattern();
		const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

		bool result = d_ptr->radiusMap.setupRadiusMap(desc, bsegms);
		if (result) {
			*rmap = &d_ptr->radiusMap;
			return true;
		}
	}
	return false;
}

bool oct_report::CorneaScanReport::composeThicknessMap(OctLayerStratumParam param, segm_scan::CorneaThicknessMap** thick)
{
	auto tmap = getThicknessMap(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
	if (tmap) {
		*thick = tmap;
		return true;
	}
	return false;
}


segm_scan::CorneaThicknessMap * oct_report::CorneaScanReport::getThicknessMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto tmap = prepareEnfaceMap(upper, lower, upperOffset, lowerOffset);
	return static_cast<CorneaThicknessMap*>(tmap);
}


segm_scan::CorneaEplot * oct_report::CorneaScanReport::getCorneaEplot(OcularLayerType upper, OcularLayerType lower)
{
	auto tmap = getThicknessMap(upper, lower);
	return static_cast<CorneaEplot*>(tmap);
}


segm_scan::CorneaRadiusMap * oct_report::CorneaScanReport::getCurvatureRadiusMap(void)
{
	if (d_ptr->radiusMap.isEmpty()) {
		const auto& desc = getProtocolSource()->getScanPattern();
		const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

		bool result = d_ptr->radiusMap.setupRadiusMap(desc, bsegms);
	}

	return &d_ptr->radiusMap;
}


std::unique_ptr<segm_scan::OcularEnfaceMap> oct_report::CorneaScanReport::createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	auto plot = unique_ptr<CorneaEnfaceMap>(new CorneaEnfaceMap);

	const auto& desc = getProtocolSource()->getScanPattern();
	const auto& bsegms = getProtocolSource()->getPatternBscanSegmList();

	bool result = plot->setupThicknessMap(desc, upper, lower, upperOffset, lowerOffset, bsegms);

	if (result) {
		return std::move(plot);
	}
	return nullptr;
}


CorneaScanReport::CorneaScanReportImpl & oct_report::CorneaScanReport::getImpl(void) const
{
	return *d_ptr;
}