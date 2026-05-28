#pragma once


// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)


#ifdef WSODOMAIN_EXPORTS
#define WSODOMAIN_DLL_API		__declspec(dllexport)
#else
#define WSODOMAIN_DLL_API		__declspec(dllimport)
#endif

#include "cpp_util.h"

#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <unordered_map>
#include <map>
#include <sstream>

namespace wso_domain
{
	using namespace cpp_util;
	using namespace std;
}