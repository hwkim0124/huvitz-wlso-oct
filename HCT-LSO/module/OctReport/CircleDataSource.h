#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class OCTREPORT_DLL_API CircleDataSource : public ProtocolSource
	{
	public:
		CircleDataSource();
		CircleDataSource(const OctScanProtocol& desc);
		CircleDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~CircleDataSource();

		CircleDataSource(CircleDataSource&& rhs);
		CircleDataSource& operator=(CircleDataSource&& rhs);
		CircleDataSource(const CircleDataSource& rhs) = delete;
		CircleDataSource& operator=(const CircleDataSource& rhs) = delete;

	private:
		struct CircleDataSourceImpl;
		std::unique_ptr<CircleDataSourceImpl> d_ptr;
		CircleDataSourceImpl& getImpl(void) const;
	};
}