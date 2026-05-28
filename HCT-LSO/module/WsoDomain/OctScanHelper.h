#pragma once

#include "WsoDomain2.h"
#include "OctScanDefines.h"
#include "OctScanStructs.h"


namespace wso_domain
{
	class WSODOMAIN_DLL_API OctScanHelper
	{
	public:
		OctScanHelper();
		virtual ~OctScanHelper();

		OctScanHelper(const OctScanHelper& rhs) = delete;
		OctScanHelper& operator=(const OctScanHelper& rhs) = delete;

		static OctScanHelper* getInstance(void);
		static std::mutex singleMutex_;

	public:
		int getScanPatternCount(bool not_point);
		int getScanPatternList(vector<OctPatternDescript>& list, bool not_point);
		int getScanPatternList(vector<OctPatternDescript>& list, EyeRegion region, bool not_point);
		bool getPatternCodeList(vector<OctPatternCode>& list);
		bool getPatternCodeList(vector<OctPatternCode>& list, EyeRegion region);

		int getPatternAscanList(vector<int>& list, OctPatternCode code);
		int getPatternBscanList(vector<int>& list, OctPatternCode code, int ascans);
		int getPatternOverlapList(vector<int>& list, OctPatternCode code, int ascans, int bscans);
		int getPatternRangeList(vector<float>& xlist, vector<float>& ylist, OctPatternCode code);

		static std::string getPatternName(OctPatternCode code);
		static EyeRegion getPatternRegion(OctPatternCode code);

		static OctPatternType getPatternType(OctPatternCode code);
		static OctPatternType getPatternType(OctPatternCode code, OctScanDirection direction);
		static OctPatternType getPreviewType(OctPatternCode code, OctScanDirection direction);
		static OctPatternType getPreviewType(OctPatternType type, OctScanDirection direction);

	private:
		struct OctScanHelperImpl;
		std::unique_ptr<OctScanHelperImpl> d_ptr;
		OctScanHelperImpl& impl(void) const;
	};
}

