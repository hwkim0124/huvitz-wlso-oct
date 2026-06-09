#include "pch.h"
#include "RadialDataSource.h"


using namespace oct_report;


struct RadialDataSource::RadialDataSourceImpl
{
	RadialDataSourceImpl()
	{
	}
};


RadialDataSource::RadialDataSource() :
	d_ptr(make_unique<RadialDataSourceImpl>())
{
}

oct_report::RadialDataSource::RadialDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<RadialDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::RadialDataSource::RadialDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<RadialDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::RadialDataSource::~RadialDataSource() = default;
oct_report::RadialDataSource::RadialDataSource(RadialDataSource && rhs) = default;
RadialDataSource & oct_report::RadialDataSource::operator=(RadialDataSource && rhs) = default;


RadialDataSource::RadialDataSourceImpl & oct_report::RadialDataSource::getImpl(void) const
{
	return *d_ptr;
}