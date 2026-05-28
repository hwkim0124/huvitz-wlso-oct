#pragma once

#include "WsoDomain2.h"
#include "OctScanDefines.h"
#include "OctScanProtocol.h"


namespace wso_domain
{
	class WSODOMAIN_DLL_API OctDataUtil
	{
	public:
		static std::string serializeToJson(const OctScanOffset& offset);
		static std::string serializeToJson(const OctScanPattern& pattern);
		static std::string serializeToJson(const OctScanProtocol& proto);

		static bool populateFromJson(const std::string& jstr, OctScanOffset& offset);
		static bool populateFromJson(const std::string& jstr, OctScanPattern& pattern);
		static bool populateFromJson(const std::string& jstr, OctScanProtocol& proto);

		static bool saveScanProtocolSnapshot(const std::wstring& path, const OctScanProtocol& proto);
		static bool loadScanProtocolSnapshot(const std::wstring& path, OctScanProtocol& proto);

		static OctProtocolDescript convertToProtocolDescript(const OctScanProtocol& proto);
	};

}

