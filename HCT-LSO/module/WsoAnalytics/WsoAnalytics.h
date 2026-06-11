#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

#ifdef WSOANALYTICS_EXPORTS
#define WSOANALYTICS_DLL_API		__declspec(dllexport)
#else
#define WSOANALYTICS_DLL_API		__declspec(dllimport)
#endif

