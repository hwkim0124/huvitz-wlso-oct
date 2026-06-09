#pragma once

#include "OctResult2.h"
#include "BscanBundle.h"

#include <memory>
#include <vector>
#include <string>


namespace oct_result
{
	class BscanSection;
	class BscanBundle;
	class EnfaceImage;

	class OCTRESULT_DLL_API PatternOutput : public BscanBundle
	{
	public:
		PatternOutput();
		virtual ~PatternOutput();

		PatternOutput(PatternOutput&& rhs);
		PatternOutput& operator=(PatternOutput&& rhs);
		PatternOutput(const PatternOutput& rhs) = delete;
		PatternOutput& operator=(const PatternOutput& rhs) = delete;

	public:
		int updateImages(void);
		int exportImages(const std::wstring dirPath, const std::wstring& prefix = L"");
		int importImages(int numImages, const std::wstring dirPath, const std::wstring& prefix = L"");
		int importImages(const std::vector<std::wstring>& fileList);

		bool updateImage(int sectIdx);
		bool exportImage(int sectIdx, const std::wstring& dirPath, const std::wstring& prefix);
		bool importImage(int sectIdx, const std::wstring& dirPath, const std::wstring& prefix);
		bool importImage(int sectIdx, const std::wstring& filePath);


		bool applyAveraging(void);
		float getAverageOfQualityIndex(void);

	private:
		struct PatternOutputImpl;
		std::unique_ptr<PatternOutputImpl> d_ptr;
		PatternOutputImpl& getImpl(void) const;
	};


	typedef std::vector<PatternOutput> PatternOutputList;
}

