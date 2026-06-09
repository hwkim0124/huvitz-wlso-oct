#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class OCTREPORT_DLL_API CrossDataSource : public ProtocolSource
	{
	public:
		CrossDataSource();
		CrossDataSource(const OctScanProtocol& desc);
		CrossDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~CrossDataSource();

		CrossDataSource(CrossDataSource&& rhs);
		CrossDataSource& operator=(CrossDataSource&& rhs);
		CrossDataSource(const CrossDataSource& rhs) = delete;
		CrossDataSource& operator=(const CrossDataSource& rhs) = delete;

	private:
		struct CrossDataSourceImpl;
		std::unique_ptr<CrossDataSourceImpl> d_ptr;
		CrossDataSourceImpl& getImpl(void) const;
	};
}
