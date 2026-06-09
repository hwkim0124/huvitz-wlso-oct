#include "pch.h"
#include "ProtocolReport.h"
#include "ProtocolSource.h"
#include "OctReport2.h"

#include <forward_list>

using namespace oct_report;


struct ProtocolReport::ProtocolReportImpl
{
	int index = 0;
	EyeSide side;
	ProtocolSource* data;
	forward_list<unique_ptr<OcularEnfaceImage>> shots;
	forward_list<unique_ptr<OcularEnfaceMap>> tmaps;

	ProtocolReportImpl() : side(EyeSide::OD), data(nullptr)
	{
	}
};


oct_report::ProtocolReport::ProtocolReport() :
	d_ptr(make_unique<ProtocolReportImpl>())
{
}


oct_report::ProtocolReport::~ProtocolReport() = default;
oct_report::ProtocolReport::ProtocolReport(ProtocolReport && rhs) = default;
ProtocolReport & oct_report::ProtocolReport::operator=(ProtocolReport && rhs) = default;


void oct_report::ProtocolReport::setProtocolSource(oct_report::ProtocolSource* data, bool update)
{
	d_ptr->data = data;
	if (data) {
		auto side = data->getScanProtocol().getEyeSide();
		setEyeSide(side);

		if (update) {
			updateContents();
		}
	}
	return;
}


oct_report::ProtocolSource* oct_report::ProtocolReport::getProtocolSource(void) const
{
	return d_ptr->data;
}

OctScanProtocol* oct_report::ProtocolReport::getScanProtocol(void) const
{
	if (auto* p = getProtocolSource(); p) {
		return &p->getScanProtocol();
	}
	return nullptr;
}


OctScanPattern* oct_report::ProtocolReport::getScanPattern(void) const
{
	if (auto* p = getProtocolSource(); p) {
		return &p->getScanProtocol().getMeasure();
	}
	return nullptr;
}

int oct_report::ProtocolReport::getReportId(void) const
{
	return getImpl().index;
}

void oct_report::ProtocolReport::setReportId(int index)
{
	getImpl().index = index;
	return;
}


bool oct_report::ProtocolReport::isSourceEmpty(void) const
{
	return (d_ptr->data == nullptr);
}


void oct_report::ProtocolReport::setEyeSide(EyeSide side)
{
	d_ptr->side = side;
	return;
}


EyeSide oct_report::ProtocolReport::getEyeSide(void) const
{
	return d_ptr->side;
}


bool oct_report::ProtocolReport::isEyeOD(void) const
{
	return getEyeSide() == EyeSide::OD;
}

bool oct_report::ProtocolReport::isMacularReport(void) const
{
	if (!isSourceEmpty()) {
		return getScanProtocol()->isMacularScan();
	}
	return false;
}

bool oct_report::ProtocolReport::isOpticDiscReport(void) const
{
	if (!isSourceEmpty()) {
		return getScanProtocol()->isOpticDiscScan();
	}
	return false;
}

bool oct_report::ProtocolReport::isCorneaReport(void) const
{
	if (!isSourceEmpty()) {
		return getScanProtocol()->isCorneaScan();
	}
	return false;
}

bool oct_report::ProtocolReport::isAngioReport(void) const
{
	if (!isSourceEmpty()) {
		return getScanProtocol()->isAngioScan();
	}
	return false;
}

bool oct_report::ProtocolReport::updateContents(void)
{
	return true;
}


void oct_report::ProtocolReport::clearContents(void)
{
	return;
}

bool oct_report::ProtocolReport::getEnfaceImageDescript(OctLayerStratumParam param, OctEnfaceImageDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getAngioImageDescript(OctLayerStratumParam param, OctAngioImageDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getThicknessMapDescript(OctLayerStratumParam param, OctThicknessMapDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getETDRSChartDescript(OctLayerStratumParam param, OctETDRSChartDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getGCCThickChartDescript(OctLayerStratumParam param, OctGCCThickChartDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getClockChartDescript(OctLayerStratumParam param, OctClockChartDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getAngioChartDescript(OctLayerStratumParam param, OctAngioChartDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getMacularSummaryDescript(OctMacularSummaryDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getOpticDiscSummaryDescript(OctOpticDiscSummaryDescript& desc)
{
	return false;
}

bool oct_report::ProtocolReport::getProtocolDescript(OctProtocolDescript& desc)
{
	if (auto* p = getProtocolSource(); p) {
		auto& proto = p->getScanProtocol();
		desc = OctDataUtil::convertToProtocolDescript(proto);
		return true;
	}
	return false;
}

std::unique_ptr<segm_scan::OcularEnfaceImage> oct_report::ProtocolReport::createEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	return nullptr;
}


std::unique_ptr<segm_scan::OcularEnfaceMap> oct_report::ProtocolReport::createEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	return nullptr;
}


segm_scan::OcularEnfaceImage* oct_report::ProtocolReport::prepareEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	if (isSourceEmpty()) {
		return nullptr;
	}

	if (getScanPattern()->isEnfacePattern()) {
	}
	else {
		return nullptr;
	}

	auto p = findEnfaceImage(upper, lower, upperOffset, lowerOffset);
	if (p) {
		return p;
	}

	auto uptr = createEnfaceImage(upper, lower, upperOffset, lowerOffset);
	if (uptr != nullptr) {
		if (registEnfaceImage(std::move(uptr))) {
			return findEnfaceImage(upper, lower, upperOffset, lowerOffset);
		}
	}
	return nullptr;
}


segm_scan::OcularEnfaceImage* oct_report::ProtocolReport::findEnfaceImage(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	for (const auto& enf : d_ptr->shots) {
		if (enf->isIdentical(upper, lower, upperOffset, lowerOffset)) {
			return enf.get();
		}
	}
	return nullptr;
}


bool oct_report::ProtocolReport::registEnfaceImage(std::unique_ptr<segm_scan::OcularEnfaceImage> enface)
{
	if (enface) {
		getImpl().shots.push_front(move(enface));
		return true;
	}
	return false;
}


segm_scan::OcularEnfaceMap* oct_report::ProtocolReport::prepareEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	if (isSourceEmpty()) {
		return nullptr;
	}

	if (getScanPattern()->isEnfacePattern() ||
		getScanPattern()->isRadialPattern() ||
		getScanPattern()->isRasterPattern()) {
	}
	else {
		return nullptr;
	}

	auto p = findEnfaceMap(upper, lower, upperOffset, lowerOffset);
	if (p) {
		return p;
	}

	auto uptr = createEnfaceMap(upper, lower, upperOffset, lowerOffset);
	if (uptr != nullptr) {
		if (registEnfaceMap(std::move(uptr))) {
			return findEnfaceMap(upper, lower, upperOffset, lowerOffset);
		}
	}
	return nullptr;
}


segm_scan::OcularEnfaceMap* oct_report::ProtocolReport::findEnfaceMap(OcularLayerType upper, OcularLayerType lower, float upperOffset, float lowerOffset)
{
	for (const auto& tmap : d_ptr->tmaps) {
		if (tmap->isIdentical(upper, lower, upperOffset, lowerOffset)) {
			return tmap.get();
		}
	}
	return nullptr;
}


bool oct_report::ProtocolReport::registEnfaceMap(std::unique_ptr<segm_scan::OcularEnfaceMap> tmap)
{
	if (tmap) {
		getImpl().tmaps.push_front(move(tmap));
		return true;
	}
	return false;
}


ProtocolReport::ProtocolReportImpl & oct_report::ProtocolReport::getImpl(void) const
{
	return *d_ptr;
}