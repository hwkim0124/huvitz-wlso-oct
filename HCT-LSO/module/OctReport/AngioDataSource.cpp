#include "pch.h"
#include "OctReport2.h"
#include "AngioDataSource.h"

using namespace oct_report;


struct AngioDataSource::AngioDataSourceImpl
{
	Angiogram angiogram;

	AngioDataSourceImpl()
	{
	}
};


AngioDataSource::AngioDataSource() :
	d_ptr(make_unique<AngioDataSourceImpl>())
{
}

oct_report::AngioDataSource::AngioDataSource(const OctScanProtocol& desc) :
	d_ptr(make_unique<AngioDataSourceImpl>()), ProtocolSource(desc)
{
}


oct_report::AngioDataSource::AngioDataSource(std::unique_ptr<oct_result::ProtocolResult>&& result) :
	d_ptr(make_unique<AngioDataSourceImpl>()), ProtocolSource(std::move(result))
{
}


oct_report::AngioDataSource::~AngioDataSource() = default;
oct_report::AngioDataSource::AngioDataSource(AngioDataSource&& rhs) = default;
AngioDataSource& oct_report::AngioDataSource::operator=(AngioDataSource&& rhs) = default;

bool oct_report::AngioDataSource::importProtocolContent(const std::wstring& dir_path)
{
	if (ProtocolSource::importProtocolContent(dir_path)) {
		auto& pattern = getScanPattern();
		impl().angiogram.setupAngioPattern(pattern);
		if (impl().angiogram.loadDataFiles(wtoa(dir_path))) {
			auto& ampls = impl().angiogram.Data().getAmplitudes();
			getAngioOutput()->setAmplitudes(std::move(ampls));
			return true;
		}
		/*
		if (impl().angiogram.loadDataFile2(wtoa(dir_path))) {
			return true;
		}
		*/
	}
	return false;
}


bool oct_report::AngioDataSource::exportProtocolContent(const std::wstring& dir_path, std::wstring& out_path)
{
	if (!impl().angiogram.isAmplitduesValid()) {
		LogD() << "Amplitude data is not valid.";
		return false;
	}
	if (ProtocolSource::exportProtocolContent(dir_path, out_path)) {
		if (impl().angiogram.saveDataFiles(wtoa(out_path))) {
			return true;
		}
	}
	/*
	if (!impl().angiogram.isDecorrelationsValid()) {
		LogD() << "Decorrelation data is not valid.";
		return false;
	}
	if (ProtocolSource::exportProtocolResult(dir_path, out_path)) {
		if (impl().angiogram.saveDataFile2(wtoa(out_path))) {
			return true;
		}
	}
	*/
	return false;
}

oct_angio::Angiogram* oct_report::AngioDataSource::getAngiogram(void) const
{
	return &impl().angiogram;
}


AngioDataSource::AngioDataSourceImpl& oct_report::AngioDataSource::impl(void) const
{
	return *d_ptr;
}
