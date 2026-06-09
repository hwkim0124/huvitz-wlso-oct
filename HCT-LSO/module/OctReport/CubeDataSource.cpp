#include "pch.h"
#include "CubeDataSource.h"

using namespace oct_report;


struct CubeDataSource::CubeDataSourceImpl
{
	CubeDataSourceImpl()
	{
	}
};


CubeDataSource::CubeDataSource() :
	d_ptr(make_unique<CubeDataSourceImpl>())
{
}

oct_report::CubeDataSource::CubeDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<CubeDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::CubeDataSource::CubeDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<CubeDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::CubeDataSource::~CubeDataSource() = default;
oct_report::CubeDataSource::CubeDataSource(CubeDataSource && rhs) = default;
CubeDataSource & oct_report::CubeDataSource::operator=(CubeDataSource && rhs) = default;



CubeDataSource::CubeDataSourceImpl & oct_report::CubeDataSource::getImpl(void) const
{
	return *d_ptr;
}
