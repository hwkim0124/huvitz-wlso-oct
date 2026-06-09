#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class OCTREPORT_DLL_API CubeDataSource : public ProtocolSource
	{
	public:
		CubeDataSource();
		CubeDataSource(const OctScanProtocol& desc);
		CubeDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~CubeDataSource();

		CubeDataSource(CubeDataSource&& rhs);
		CubeDataSource& operator=(CubeDataSource&& rhs);
		CubeDataSource(const CubeDataSource& rhs) = delete;
		CubeDataSource& operator=(const CubeDataSource& rhs) = delete;

	private:
		struct CubeDataSourceImpl;
		std::unique_ptr<CubeDataSourceImpl> d_ptr;
		CubeDataSourceImpl& getImpl(void) const;
	};
}

