#pragma once

#include "OctResult2.h"

#include <memory>
#include <vector>
#include <string>
#include <map>

namespace oct_result
{
	class EnfaceOutput;
	class PreviewOutput;
	class PatternOutput;
	class AngioOutput;
	class CorneaImage;
	class RetinaImage;


	class OCTRESULT_DLL_API ProtocolResult
	{
	public:
		ProtocolResult();
		ProtocolResult(const OctScanProtocol& desc);
		virtual ~ProtocolResult();

		ProtocolResult(ProtocolResult&& rhs);
		ProtocolResult& operator=(ProtocolResult&& rhs);
		ProtocolResult(const ProtocolResult& rhs) = delete;
		ProtocolResult& operator=(const ProtocolResult& rhs) = delete;

	public:
		void setScanProtocol(const OctScanProtocol& desc);
		OctScanProtocol& getScanProtocol(void) const;

		EyeSide getEyeSide(void) const;
		bool isOD(void) const;

		PreviewOutput& getPreviewOutput(void) const;
		PatternOutput& getPatternOutput(void) const;
		EnfaceOutput& getEnfaceOutput(void) const;
		AngioOutput& getAngioOutput(void) const;
		CorneaImage& getCorneaImage(void) const;
		RetinaImage& getRetinaImage(void) const;
		
		bool setRetinaImage(const OctRetinaImageDescript& desc);
		bool setCorneaImage(const OctCorneaImageDescript& desc);
		const OctRetinaImageDescript* getRetinaImageDescript(void) const;
		const OctCorneaImageDescript* getCorneaImageDescript(void) const;

		bool importScanProtocol(const std::wstring& dir_path, const std::wstring& fname = L"protocol.json");
		bool exportScanProtocol(const std::wstring& dir_path, const std::wstring& fname = L"protocol.json");

		bool exportFiles(const std::wstring& dirPath,
						const std::wstring& imagePrefix = L"",
						const std::wstring& enfaceName = L"enface",
						const std::wstring& previewName = L"preview",
						const std::wstring& retinaName = L"retina",
						const std::wstring& corneaName = L"cornea");
		bool importFiles(const std::wstring& dirPath, 
						int numImages = 0,
						const std::wstring& imagePrefix = L"",
						const std::wstring& enfaceName = L"enface",
						const std::wstring& previewName = L"preview",
						const std::wstring& retinaName = L"retina",
						const std::wstring& corneaName = L"cornea");

		bool createExportDirectory(const std::wstring& root_dir, std::wstring& export_dir);

	protected:

	private:
		struct ProtocolResultImpl;
		std::unique_ptr<ProtocolResultImpl> d_ptr;
		ProtocolResultImpl& getImpl(void) const;
	};

	typedef std::vector<ProtocolResult> ProtocolResultList;
	// typedef std::multimap<EyeSide, ProtocolResult> ProtocolResultList;
}
