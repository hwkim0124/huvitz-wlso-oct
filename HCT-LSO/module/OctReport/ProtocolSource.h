#pragma once

#include "OctReport.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_result {
	class ProtocolResult;
	class PatternOutput;
	class PreviewOutput;
}


namespace segm_scan {
	class OcularBsegm;
}


namespace oct_report
{
	class BscanContent;

	class OCTREPORT_DLL_API ProtocolSource
	{
	public:
		ProtocolSource();
		ProtocolSource(const OctScanProtocol& desc);
		ProtocolSource(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual ~ProtocolSource();

		ProtocolSource(ProtocolSource&& rhs);
		ProtocolSource& operator=(ProtocolSource&& rhs);
		ProtocolSource(const ProtocolSource& rhs) = delete;
		ProtocolSource& operator=(const ProtocolSource& rhs) = delete;

	public:
		int getSourceId(void) const;
		void setSourceId(int sourceId);
		bool isContentEmpty(void) const;

		BscanContent* getPatternContent(int index) const;
		BscanContent* getPatternContentOfSection(int sectIdx, int imageIdx) const;
		int getPatternContentCount(void) const;
		int getPatternSectionCount(void) const;
		int updatePatternContentList(bool vflip = false);

		BscanContent* getPreviewContent(int index) const;
		int getPreviewContentCount(void) const;
		int updatePreviewContentList(bool vflip = false);

		std::vector<segm_scan::OcularBsegm*> getPatternBscanSegmList(void) const;

		oct_result::ProtocolResult* getProtocolResult(void) const;
		oct_result::PreviewOutput* getPreviewOutput(void) const;
		oct_result::PatternOutput* getPatternOutput(void) const;
		oct_result::AngioOutput* getAngioOutput(void) const;

		OctScanProtocol& getScanProtocol(void) const;
		void setScanProtocol(const OctScanProtocol& desc);
		OctScanPattern& getScanPattern(void) const;

		const OctScanImageDescript* getPreviewImageDescript(int sectIdx, int imageIdx = 0);
		const OctScanImageDescript* getPatternImageDescript(int sectIdx, int imageIdx = 0);

		bool getRetinaBsegmDescriptor(int sectIdx, int imageIdx, OctRetinaBsegmDescriptor& desc);
		bool getCorneaBsegmDescriptor(int sectIdx, int imageIdx, OctCorneaBsegmDescriptor& desc);
		bool getRetinaBsegmTraits(int sectIdx, int imageIdx, OctRetinaBsegmTraits& traits);
		bool getCorneaBsegmTraits(int sectIdx, int imageIdx, OctCorneaBsegmTraits& traits);
		bool getBsegmLayerPoints(int sectIdx, int imageIdx, OcularLayerType type, OctBsegmLayerPoints& layer);
	
		std::vector<int> getLayerPoints(int sectIdx, int imageIdx, OcularLayerType layer) const;
		bool getRetinaLayerMapArrays(LayerMapArrays& layers) const;

		const OctRetinaImageDescript* getRetinaImageDescript(void) const;
		const OctCorneaImageDescript* getCorneaImageDescript(void) const;

		void setupProtocolResult(std::unique_ptr<oct_result::ProtocolResult>&& result);
		virtual bool importProtocolContent(const std::wstring& dir_path);
		virtual bool exportProtocolContent(const std::wstring& dir_path, std::wstring& out_path);

		virtual int importPatternImages(const std::wstring& dirPath, int numImages = 0);
		virtual int importPatternImages(const std::vector<std::wstring>& fileList);
		virtual int importPreviewImages(const std::wstring& dirPath, int numImages = 0);

		virtual int updatePatternImages(void);
		virtual int updatePreviewImages(void);

		bool exportBsegmResults(const std::wstring dirPath = L"./export");
		bool importBsegmResults(const std::wstring dirPath = L"./export");		
		void clear(void);

	protected:
		

	private:
		struct ProtocolSourceImpl;
		std::unique_ptr<ProtocolSourceImpl> d_ptr;
		ProtocolSourceImpl& getImpl(void) const;
	};
}

