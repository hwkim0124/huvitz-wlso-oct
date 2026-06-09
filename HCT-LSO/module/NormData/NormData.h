#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)


#ifdef NORMDATA_EXPORTS
#define NORMDATA_DLL_API __declspec(dllexport)
#else
#define NORMDATA_DLL_API __declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>


#include "wso_domain.h"
#include "cpp_util.h"


namespace norm_data
{
	using namespace wso_domain;
	using namespace cpp_util;
	using namespace std;

}