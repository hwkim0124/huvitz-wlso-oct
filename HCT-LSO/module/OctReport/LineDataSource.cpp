#include "pch.h"
#include "LineDataSource.h"


using namespace oct_report;


struct LineDataSource::LineDataSourceImpl
{
	LineDataSourceImpl()
	{
	}
};


LineDataSource::LineDataSource() :
	d_ptr(make_unique<LineDataSourceImpl>())
{
}

oct_report::LineDataSource::LineDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<LineDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::LineDataSource::LineDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<LineDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::LineDataSource::~LineDataSource() = default;
oct_report::LineDataSource::LineDataSource(LineDataSource && rhs) = default;
LineDataSource & oct_report::LineDataSource::operator=(LineDataSource && rhs) = default;


LineDataSource::LineDataSourceImpl & oct_report::LineDataSource::getImpl(void) const
{
	return *d_ptr;
}
