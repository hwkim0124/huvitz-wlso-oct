#include "pch.h"
#include "CrossDataSource.h"


using namespace oct_report;


struct CrossDataSource::CrossDataSourceImpl
{
	CrossDataSourceImpl()
	{
	}
};


CrossDataSource::CrossDataSource() :
	d_ptr(make_unique<CrossDataSourceImpl>())
{
}

oct_report::CrossDataSource::CrossDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<CrossDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::CrossDataSource::CrossDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<CrossDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::CrossDataSource::~CrossDataSource() = default;
oct_report::CrossDataSource::CrossDataSource(CrossDataSource && rhs) = default;
CrossDataSource & oct_report::CrossDataSource::operator=(CrossDataSource && rhs) = default;


CrossDataSource::CrossDataSourceImpl & oct_report::CrossDataSource::getImpl(void) const
{
	return *d_ptr;
}