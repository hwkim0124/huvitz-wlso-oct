#pragma once

#include "OctReport.h"
#include "ProtocolReport.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class MacularScanReport;
	class OpticDiscScanReport;
	class CorneaScanReport;
	class MacularAngioReport;
	class OpticDiscAngioReport;

	class OCTREPORT_DLL_API ProtocolReportSet
	{
	public:
		ProtocolReportSet();
		virtual ~ProtocolReportSet();

		ProtocolReportSet(ProtocolReportSet&& rhs);
		ProtocolReportSet& operator=(ProtocolReportSet&& rhs);
		ProtocolReportSet(const ProtocolReportSet& rhs) = delete;
		ProtocolReportSet& operator=(const ProtocolReportSet& rhs) = delete;

	public:
		ProtocolReport* getProtocolReport(int reportId);
		MacularScanReport* getOrCreateMacularScanReport(int reportId);
		MacularAngioReport* getOrCreateMacularAngioReport(int reportId);
		OpticDiscScanReport* getOrCreateOpticDiscScanReport(int reportId);
		OpticDiscAngioReport* getOrCreateOpticDiscAngioReport(int reportId);
		CorneaScanReport* getOrCreateCorneaScanReport(int reportId);
		
		int generateReportId(void) const;
		bool isReportIdValid(int reportId) const;
		void clearAllReports(void);

	protected:
		template <typename T>
		ProtocolReport* emplaceProtocolReport(int reportId);

		template <typename T>
		ProtocolReport* fetchProtocolReport(int reportId);

		template <typename T>
		ProtocolReport * createProtocolReport(int reportId);

	private:
		struct ProtocolReportSetImpl;
		std::unique_ptr<ProtocolReportSetImpl> d_ptr;
		ProtocolReportSetImpl& getImpl(void) const;
	};

	template<typename T>
	inline ProtocolReport * ProtocolReportSet::emplaceProtocolReport(int reportId)
	{
		auto data = fetchProtocolReport<T>(reportId);
		if (data == nullptr) {
			return createProtocolReport<T>(reportId);
		}
		return data;
	}

	template<typename T>
	inline ProtocolReport * ProtocolReportSet::fetchProtocolReport(int reportId)
	{
		auto iter = getImpl().dataset.find(reportId);
		if (iter != end(getImpl().dataset)) {
			return dynamic_cast<T*>(iter->second.get());
		}
		return nullptr;
	}

	template<typename T>
	inline ProtocolReport * ProtocolReportSet::createProtocolReport(int reportId)
	{
		if (reportId >= 0) {
			getImpl().dataset[reportId] = make_unique<T>();
			getImpl().dataset[reportId]->setReportId(reportId);
			return getImpl().dataset[reportId].get();
		}
		return nullptr;
	}
}

