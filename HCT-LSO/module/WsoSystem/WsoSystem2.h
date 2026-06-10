#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

#ifdef WSOSYSTEM_EXPORTS
#define WSOSYSTEM_DLL_API		__declspec(dllexport)
#else
#define WSOSYSTEM_DLL_API		__declspec(dllimport)
#endif


#include <cstdint>
#include <cmath>
#include <memory>
#include <mutex>

#include "wso_domain.h"
#include "wso_board.h"
#include "wso_device.h"
#include "wso_config.h"
#include "oct_grab.h"
#include "oct_scan.h"
#include "oct_result.h"
#include "oct_pattern.h"
#include "oct_angio.h"
#include "sig_proc.h"
#include "sig_chain.h"
#include "cpp_util.h"
#include "oct_report.h"
#include "segm_proc.h"


namespace wso_system
{
	using namespace wso_domain;
	using namespace wso_board;
	using namespace wso_device;
	using namespace wso_config;
	using namespace oct_grab;
	using namespace oct_scan;
	using namespace oct_result;
	using namespace oct_report;
	using namespace oct_pattern;
	using namespace oct_angio;
	using namespace sig_proc;
	using namespace sig_chain;
	using namespace segm_proc;
	using namespace cpp_util;
	using namespace std;
}