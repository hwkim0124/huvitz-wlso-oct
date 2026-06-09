#include "pch.h"
#include "MacularAngioReport.h"
#include "ProtocolSource.h"
#include "AngioDataSource.h"
#include "OctReport2.h"

using namespace oct_report;

struct MacularAngioReport::MacularAngioReportImpl
{
	MacularAngioReportImpl() 
	{
	}
};


oct_report::MacularAngioReport::MacularAngioReport() :
	d_ptr(make_unique<MacularAngioReportImpl>())
{
}


oct_report::MacularAngioReport::~MacularAngioReport() = default;
oct_report::MacularAngioReport::MacularAngioReport(MacularAngioReport&& rhs) = default;
MacularAngioReport& oct_report::MacularAngioReport::operator=(MacularAngioReport&& rhs) = default;


bool oct_report::MacularAngioReport::updateContents(void)
{
	if (!MacularScanReport::updateContents()) {
		return false;
	}

	auto* source = (AngioDataSource*)getProtocolSource();
	auto* angio = source->getAngiogram();

	const auto& desc = getProtocolSource()->getScanPattern();
	angio->setupAngioPattern(desc);

	auto& data = getProtocolSource()->getAngioOutput()->getAmplitudes();
	if (!angio->setupAngioData(std::move(data))) {
		return false;
	}

	LayerMapArrays layers;
	if (!getProtocolSource()->getRetinaLayerMapArrays(layers)) {
		return false;
	}
	if (!angio->setupAngioLayers(std::move(layers))) {
		return false;
	}

	if (!angio->prepareData(true)) {
		return false;
	}
	return true;
}

void oct_report::MacularAngioReport::clearContents(void)
{
	MacularScanReport::clearContents();
	return;
}

bool oct_report::MacularAngioReport::getAngioImageDescript(OctLayerStratumParam param, OctAngioImageDescript& desc)
{
	if (composeAngioImage(param)) {
		if (auto* s = (AngioDataSource*)getProtocolSource(); s) {
			if (auto* angio = s->getAngiogram(); angio) {
				desc.width = angio->imageWidth();
				desc.height = angio->imageHeight();
				desc.pData = angio->imageBits();
				return true;
			}
		}
	}
	return false;
}

bool oct_report::MacularAngioReport::getAngioChartDescript(OctLayerStratumParam param, OctAngioChartDescript& desc)
{
	AngioEtdrsChart chart;
	if (composeAngioETDRSChart(param, chart)) {
		if (chart.fetchAngioChartDescript(desc)) {
			return true;
		}
	}
	return false;
}

bool oct_report::MacularAngioReport::composeAngioImage(OctLayerStratumParam param) const
{
	if (auto* s = (AngioDataSource*)getProtocolSource(); s) {
		if (auto* angio = s->getAngiogram(); angio) {
			auto rangeX = getScanPattern()->rangeX;
			auto rangeY = getScanPattern()->rangeY;
			angio->resetSlabRange(param.upperType, param.lowerType, param.upperOffset, param.lowerOffset);
			angio->resetScanRange(rangeX, rangeY, param.chartCenterX, param.chartCenterY);
			if (!angio->processData2()) {
				LogD() << "Angio report: failed to process angiogram data";
				return false;
			}
			return true;
		}
	}
	return false;
}

bool oct_report::MacularAngioReport::composeAngioETDRSChart(OctLayerStratumParam param, oct_angio::AngioEtdrsChart& chart) const
{
	chart = oct_angio::AngioEtdrsChart();

	if (auto* s = (AngioDataSource*)getProtocolSource(); s) {
		if (auto* angio = s->getAngiogram(); angio) {
			auto image = angio->createAnigoImage(false);
			if (image.isEmpty()) {
				return false;
			}

			auto rangeX = angio->scanRangeX();
			auto rangeY = angio->scanRangeY();
			auto width = angio->imageWidth();
			auto height = angio->imageHeight();
			auto centerX = param.chartCenterX;
			auto centerY = param.chartCenterY;

			chart.setupAngioChart(image, rangeX, rangeY, width, height);
			chart.setEyeSide(getEyeSide());
			chart.setCenterPosition(centerX, centerY);
			chart.setSectionDiameters(1.5f, 2.5f);
			chart.setThreshold(0.15f);
			chart.updateAngioChart();
			return true;
		}
	}
	return false;
}


MacularAngioReport::MacularAngioReportImpl& oct_report::MacularAngioReport::impl(void) const
{
	return *d_ptr;
}