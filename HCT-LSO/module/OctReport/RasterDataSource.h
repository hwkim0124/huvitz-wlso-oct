#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class OCTREPORT_DLL_API RasterDataSource : public ProtocolSource
	{
	public:
		RasterDataSource();
		RasterDataSource(const OctScanProtocol& desc);
		RasterDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~RasterDataSource();

		RasterDataSource(RasterDataSource&& rhs);
		RasterDataSource& operator=(RasterDataSource&& rhs);
		RasterDataSource(const RasterDataSource& rhs) = delete;
		RasterDataSource& operator=(const RasterDataSource& rhs) = delete;

	private:
		struct RasterDataSourceImpl;
		std::unique_ptr<RasterDataSourceImpl> d_ptr;
		RasterDataSourceImpl& getImpl(void) const;
	};
}