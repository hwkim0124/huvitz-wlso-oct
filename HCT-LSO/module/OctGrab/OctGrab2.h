#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

#ifdef OCTGRAB_EXPORTS
#define OCTGRAB_DLL_API		__declspec(dllexport)
#else
#define OCTGRAB_DLL_API		__declspec(dllimport)
#endif

#include <cstdint>
#include <cmath>
#include <vector>
#include <memory>
#include <string>


#include "wso_domain.h"
#include "cpp_util.h"


namespace oct_grab
{
	using namespace wso_domain;
	using namespace cpp_util;
	using namespace oct_grab;
	using namespace std;


	// Constants Definition
	//////////////////////////////////////////////////////////////////////////
	constexpr int GRABBER_BUFFER_X_SIZE = 2048;
	constexpr int GRABBER_BUFFER_Y_SIZE_MAX = 2048;

	constexpr int JOYSTICK_PRESSED_EVENT_COUNT = 3;
	constexpr int JOYSTICK_PRESSED_EVENT_DELAY = 5;
	constexpr int GRAB_FIRST_RETRY_COUNT_MAX = 300;
	constexpr int GRAB_FIRST_RETRY_DELAY_TIME = 5;

	constexpr int LEFT_SIDE_IO_VALUE = 0;
	constexpr int RIGHT_SIDE_IO_VALUE = 1;
}