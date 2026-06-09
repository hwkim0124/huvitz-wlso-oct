#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>

namespace oct_angio {
	class Angiogram;
}

namespace oct_report
{
	class OCTREPORT_DLL_API AngioDataSource : public ProtocolSource
	{
	public:
		AngioDataSource();
		AngioDataSource(const OctScanProtocol& desc);
		AngioDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~AngioDataSource();

		AngioDataSource(AngioDataSource&& rhs);
		AngioDataSource& operator=(AngioDataSource&& rhs);
		AngioDataSource(const AngioDataSource& rhs) = delete;
		AngioDataSource& operator=(const AngioDataSource& rhs) = delete;

	public:
		virtual bool importProtocolContent(const std::wstring& dir_path) override;
		virtual bool exportProtocolContent(const std::wstring& dir_path, std::wstring& out_path) override;

		oct_angio::Angiogram* getAngiogram(void) const;

	private:
		struct AngioDataSourceImpl;
		std::unique_ptr<AngioDataSourceImpl> d_ptr;
		AngioDataSourceImpl& impl(void) const;
	};
}

