#include "pch.h"
#include "CircleDataSource.h"


using namespace oct_report;


struct CircleDataSource::CircleDataSourceImpl
{
	CircleDataSourceImpl()
	{
	}
};


CircleDataSource::CircleDataSource() :
	d_ptr(make_unique<CircleDataSourceImpl>())
{
}

oct_report::CircleDataSource::CircleDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<CircleDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::CircleDataSource::CircleDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<CircleDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::CircleDataSource::~CircleDataSource() = default;
oct_report::CircleDataSource::CircleDataSource(CircleDataSource && rhs) = default;
CircleDataSource & oct_report::CircleDataSource::operator=(CircleDataSource && rhs) = default;


CircleDataSource::CircleDataSourceImpl & oct_report::CircleDataSource::getImpl(void) const
{
	return *d_ptr;
}