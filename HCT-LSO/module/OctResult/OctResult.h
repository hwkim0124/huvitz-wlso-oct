#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by compiled with ANSI source file. 
#pragma warning(disable: 4819)

#ifdef OCTRESULT_EXPORTS
#define OCTRESULT_DLL_API		__declspec(dllexport)
#else
#define OCTRESULT_DLL_API		__declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

#include "wso_domain.h"
#include "cpp_util.h"


namespace oct_result
{
	using namespace wso_domain;
	using namespace cpp_util;
	using namespace std;

}