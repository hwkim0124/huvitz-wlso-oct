#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class OCTREPORT_DLL_API RadialDataSource : public ProtocolSource
	{
	public:
		RadialDataSource();
		RadialDataSource(const OctScanProtocol& desc);
		RadialDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~RadialDataSource();

		RadialDataSource(RadialDataSource&& rhs);
		RadialDataSource& operator=(RadialDataSource&& rhs);
		RadialDataSource(const RadialDataSource& rhs) = delete;
		RadialDataSource& operator=(const RadialDataSource& rhs) = delete;

	private:
		struct RadialDataSourceImpl;
		std::unique_ptr<RadialDataSourceImpl> d_ptr;
		RadialDataSourceImpl& getImpl(void) const;
	};
}