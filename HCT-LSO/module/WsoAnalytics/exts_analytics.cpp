#include "pch.h"
#include "WsoAnalytics2.h"
#include "exts_analytics.h"
#include "FTD3XX.h"
#include "Spinnaker.h"


using namespace wso_analytics;

bool WSOANALYTICS_DLL_API __stdcall wso_analytics::initializeWsoAnalytics(void)
{
	WsoLogDebug("WsoAnalytics initialized.");
	CvImage image;
	TrajectoryProfile profile;
	CameraSetting camset;
	FT_HANDLE ftHandle;
	Spinnaker::SystemPtr systemPtr;
	return true;
}
