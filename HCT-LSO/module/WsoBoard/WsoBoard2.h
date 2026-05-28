#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

#ifdef WSOBOARD_EXPORTS
#define WSOBOARD_DLL_API		__declspec(dllexport)
#else
#define WSOBOARD_DLL_API		__declspec(dllimport)
#endif

#include <memory>
#include <string>
#include <mutex>

#include "wso_hbs.h"
#include "wso_domain.h"
#include "cpp_util.h"


namespace wso_board
{
	using namespace wso_domain;
	using namespace cpp_util;
	using namespace std;
}