#include "pch.h"
#include "ProtocolResult.h"
#include "PatternOutput.h"
#include "PreviewOutput.h"
#include "EnfaceOutput.h"
#include "AngioOutput.h"

#include "CorneaImage.h"
#include "RetinaImage.h"

#include <format>

using namespace oct_result;


struct ProtocolResult::ProtocolResultImpl
{
	OctScanProtocol protocol;

	PreviewOutput preview;
	PatternOutput pattern;
	AngioOutput angio;
	EnfaceOutput enface;

	CorneaImage corneaImage;
	RetinaImage retinaImage;

	ProtocolResultImpl() 
	{
	}
};


ProtocolResult::ProtocolResult() :
	d_ptr(make_unique<ProtocolResultImpl>())
{
}


oct_result::ProtocolResult::ProtocolResult(const OctScanProtocol& desc) :
	d_ptr(make_unique<ProtocolResultImpl>())
{
	setScanProtocol(desc);
}


oct_result::ProtocolResult::~ProtocolResult() = default;
oct_result::ProtocolResult::ProtocolResult(ProtocolResult && rhs) = default;
ProtocolResult & oct_result::ProtocolResult::operator=(ProtocolResult && rhs) = default;


void oct_result::ProtocolResult::setScanProtocol(const OctScanProtocol& desc)
{
	d_ptr->protocol = desc;

	auto& measure = getScanProtocol().getMeasure();
	getPatternOutput().setDescript(measure);
	getPreviewOutput().setDescript(measure);
	getAngioOutput().setDescript(measure);
	return;
}


OctScanProtocol & oct_result::ProtocolResult::getScanProtocol(void) const
{
	return d_ptr->protocol;
}


EyeSide oct_result::ProtocolResult::getEyeSide(void) const
{
	return getScanProtocol().getEyeSide();
}


bool oct_result::ProtocolResult::isOD(void) const
{
	return getScanProtocol().isOD();
}


PreviewOutput & oct_result::ProtocolResult::getPreviewOutput(void) const
{
	return d_ptr->preview;
}


PatternOutput & oct_result::ProtocolResult::getPatternOutput(void) const
{
	return d_ptr->pattern;
}


EnfaceOutput & oct_result::ProtocolResult::getEnfaceOutput(void) const
{
	return d_ptr->enface;
}

AngioOutput& oct_result::ProtocolResult::getAngioOutput(void) const
{
	return d_ptr->angio;
}


CorneaImage & oct_result::ProtocolResult::getCorneaImage(void) const
{
	return d_ptr->corneaImage;
}

RetinaImage& oct_result::ProtocolResult::getRetinaImage(void) const
{
	return d_ptr->retinaImage;
}


bool oct_result::ProtocolResult::setRetinaImage(const OctRetinaImageDescript& desc)
{
	d_ptr->retinaImage.setData(desc);
	return true;
}

bool oct_result::ProtocolResult::setCorneaImage(const OctCorneaImageDescript & desc)
{
	d_ptr->corneaImage.setData(desc);
	return true;
}

const OctRetinaImageDescript* oct_result::ProtocolResult::getRetinaImageDescript(void) const
{
	return &d_ptr->retinaImage.getDescript();
}


const OctCorneaImageDescript* oct_result::ProtocolResult::getCorneaImageDescript(void) const
{
	return &d_ptr->corneaImage.getDescript();
}


bool oct_result::ProtocolResult::importScanProtocol(const std::wstring& dir_path, const std::wstring& fname)
{
	auto path = dir_path + L"\\" + fname;
	auto& proto = getScanProtocol();
	if (OctDataUtil::loadScanProtocolSnapshot(path, proto)) {
		LogD() << "Protocol result descript imported, path: " << wtoa(path);

		auto& measure = getScanProtocol().getMeasure();
		getPatternOutput().setDescript(measure);
		getPreviewOutput().setDescript(measure);
		getAngioOutput().setDescript(measure);
		return true;
	}
	return false;
}


bool oct_result::ProtocolResult::exportScanProtocol(const std::wstring& dir_path, const std::wstring& fname)
{
	auto path = dir_path + L"\\" + fname;
	auto& proto = getScanProtocol();
	if (OctDataUtil::saveScanProtocolSnapshot(path, proto)) {
		LogD() << "Protocol result descript exported, path: " << wtoa(path);
		return true;
	}
	return false;
}


bool oct_result::ProtocolResult::exportFiles(const std::wstring& dirPath,
	const std::wstring& imagePrefix, const std::wstring& enfaceName,
	const std::wstring& previewName, const std::wstring& retinaName,
	const std::wstring& corneaName)
{
	LogD() << "Protocol result files exporting, path: " << wtoa(dirPath);
	if (!getPatternOutput().exportImages(dirPath, imagePrefix)) {
		// return false;
	}

	if (!previewName.empty() && !getPreviewOutput().exportImages(dirPath, previewName)) {
		// return false;
	}

	if (!enfaceName.empty() && !getEnfaceOutput().exportImage(dirPath, enfaceName)) {
		// return false;
	}

	if (!corneaName.empty() && !getCorneaImage().exportFile(dirPath, corneaName)) {
		// return false;
	}
	if (!retinaName.empty() && !getRetinaImage().exportFile(dirPath, retinaName)) {
		// return false;
	}
	return true;
}


bool oct_result::ProtocolResult::importFiles(const std::wstring & dirPath, 
	int numImages,
	const std::wstring & imagePrefix, const std::wstring & enfaceName, 
	const std::wstring & previewName, const std::wstring & retinaName, 
	const std::wstring & corneaName)
{
	LogD() << "Protocol result files importing, path: " << wtoa(dirPath);
	if (!getPatternOutput().importImages(numImages, dirPath, imagePrefix)) {
		// return false;
	}

	int numPreviews = (getScanProtocol().isPreviewCrossLines() ? 2 : 1);
	if (!previewName.empty() && !getPreviewOutput().importImages(numPreviews, dirPath, previewName)) {
		// return false;
	}

	if (!enfaceName.empty() && !getEnfaceOutput().importImage(dirPath, enfaceName)) {
		// return false;
	}

	if (!corneaName.empty() && !getCorneaImage().importFile(dirPath, corneaName)) {
		// return false;
	}

	if (!retinaName.empty() && !getRetinaImage().importFile(dirPath, retinaName)) {
		// return false;
	}
	return true;
}


bool oct_result::ProtocolResult::createExportDirectory(const std::wstring& root_dir, std::wstring& export_dir)
{
	wstring path = root_dir;
	if (path.empty()) {
		path = L".\\";
	}
	else {
		path += L"\\";
	}

	wstring dtype = L"OCT_";
	string dtime = cpp_util::datetime_string(false);
	wstring wtime = atow(dtime);
	wstring dhead = std::format(L"{}{}_", dtype, wtime);

	string label = getScanProtocol().getProtocolTag();
	wstring wstr = L"";
	wstr.assign(label.begin(), label.end());
	
	wstring dest = std::format(L"{}{}{}", path, dhead, wstr);

	if (CreateDirectory(dest.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
		export_dir = dest;
		return true;
	}
	return false;
}


ProtocolResult::ProtocolResultImpl & oct_result::ProtocolResult::getImpl(void) const
{
	return *d_ptr;
}
