#include "pch.h"
#include "ProtocolReportSet.h"
#include "MacularScanReport.h"
#include "OpticDiscScanReport.h"
#include "CorneaScanReport.h"
#include "MacularAngioReport.h"
#include "OpticDiscAngioReport.h"

#include <map>

using namespace oct_report;


struct ProtocolReportSet::ProtocolReportSetImpl
{
	std::map<int, unique_ptr<ProtocolReport>> dataset;

	ProtocolReportSetImpl()
	{
	}
};


ProtocolReportSet::ProtocolReportSet() :
	d_ptr(make_unique<ProtocolReportSetImpl>())
{
}


oct_report::ProtocolReportSet::~ProtocolReportSet() = default;
oct_report::ProtocolReportSet::ProtocolReportSet(ProtocolReportSet && rhs) = default;
ProtocolReportSet & oct_report::ProtocolReportSet::operator=(ProtocolReportSet && rhs) = default;


ProtocolReport* oct_report::ProtocolReportSet::getProtocolReport(int reportId)
{
	auto data = fetchProtocolReport<ProtocolReport>(reportId);
	return data;
}

MacularScanReport * oct_report::ProtocolReportSet::getOrCreateMacularScanReport(int reportId)
{
	auto data = static_cast<MacularScanReport*>(emplaceProtocolReport<MacularScanReport>(reportId));
	return data;
}


OpticDiscScanReport * oct_report::ProtocolReportSet::getOrCreateOpticDiscScanReport(int reportId)
{
	auto data = static_cast<OpticDiscScanReport*>(emplaceProtocolReport<OpticDiscScanReport>(reportId));
	return data;
}


CorneaScanReport * oct_report::ProtocolReportSet::getOrCreateCorneaScanReport(int reportId)
{
	auto data = static_cast<CorneaScanReport*>(emplaceProtocolReport<CorneaScanReport>(reportId));
	return data;
}


MacularAngioReport* oct_report::ProtocolReportSet::getOrCreateMacularAngioReport(int reportId)
{
	auto data = static_cast<MacularAngioReport*>(emplaceProtocolReport<MacularAngioReport>(reportId));
	return data;
}


OpticDiscAngioReport* oct_report::ProtocolReportSet::getOrCreateOpticDiscAngioReport(int reportId)
{
	auto data = static_cast<OpticDiscAngioReport*>(emplaceProtocolReport<OpticDiscAngioReport>(reportId));
	return data;
}


int oct_report::ProtocolReportSet::generateReportId(void) const
{
	for (int i = 0; i < 100; ++i) {
		if (isReportIdValid(i) == false) {
			return i;
		}
	}
	return 0;
}

bool oct_report::ProtocolReportSet::isReportIdValid(int reportId) const
{
	auto iter = getImpl().dataset.find(reportId);
	if (iter != end(getImpl().dataset)) {
		return true;
	}
	return false;
}


void oct_report::ProtocolReportSet::clearAllReports(void)
{
	d_ptr->dataset.clear();
	return;
}


ProtocolReportSet::ProtocolReportSetImpl & oct_report::ProtocolReportSet::getImpl(void) const
{
	return *d_ptr;
}