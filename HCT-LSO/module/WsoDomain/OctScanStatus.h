#pragma once


#include "WsoDomain2.h"

namespace wso_domain
{
	class WSODOMAIN_DLL_API OctScanStatus
	{
	public:
		OctScanStatus();
		virtual ~OctScanStatus();

		OctScanStatus(const OctScanStatus& rhs) = delete;
		OctScanStatus& operator=(const OctScanStatus& rhs) = delete;

		static OctScanStatus* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool isCapturingToMeasure(optional<bool> flag = nullopt) const;
		bool isPreviewAveragingEnabled(optional<bool> flag = nullopt) const;

	private:
		struct OctScanStatusImpl;
		std::unique_ptr<OctScanStatusImpl> d_ptr;
		OctScanStatusImpl& impl(void) const;
	};
}

