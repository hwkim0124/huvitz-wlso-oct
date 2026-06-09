#pragma once


#ifdef RETPARAM_EXPORTS
#define RETPARAM_DLL_API __declspec(dllexport)
#else
#define RETPARAM_DLL_API __declspec(dllimport)
#endif

#include "wso_domain.h"
#include "ret_segm.h"
#include "cpp_util.h"

namespace ret_param
{
	using namespace wso_domain;
	using namespace ret_segm;
}