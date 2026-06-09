#include "pch.h"
#include "RasterDataSource.h"

using namespace oct_report;


struct RasterDataSource::RasterDataSourceImpl
{
	RasterDataSourceImpl()
	{
	}
};


RasterDataSource::RasterDataSource() :
	d_ptr(make_unique<RasterDataSourceImpl>())
{
}

oct_report::RasterDataSource::RasterDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<RasterDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::RasterDataSource::RasterDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<RasterDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::RasterDataSource::~RasterDataSource() = default;
oct_report::RasterDataSource::RasterDataSource(RasterDataSource && rhs) = default;
RasterDataSource & oct_report::RasterDataSource::operator=(RasterDataSource && rhs) = default;


RasterDataSource::RasterDataSourceImpl & oct_report::RasterDataSource::getImpl(void) const
{
	return *d_ptr;
}