#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class OCTREPORT_DLL_API LineDataSource : public ProtocolSource
	{
	public:
		LineDataSource();
		LineDataSource(const OctScanProtocol& desc);
		LineDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~LineDataSource();

		LineDataSource(LineDataSource&& rhs);
		LineDataSource& operator=(LineDataSource&& rhs);
		LineDataSource(const LineDataSource& rhs) = delete;
		LineDataSource& operator=(const LineDataSource& rhs) = delete;

	private:
		struct LineDataSourceImpl;
		std::unique_ptr<LineDataSourceImpl> d_ptr;
		LineDataSourceImpl& getImpl(void) const;
	};
}

