#pragma once

#include "WsoAnalytics.h"


namespace wso_analytics
{
	extern "C"
	{
		bool WSOANALYTICS_DLL_API __stdcall initializeWsoAnalytics(void);
	}
}