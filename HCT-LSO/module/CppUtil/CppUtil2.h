#pragma once

// Warning diabled caused by any class members with std container classes 
// not being exported from a dll. 
#pragma warning(disable:4251)

// Warning diabled caused by The file contains a character that cannot
// be represented in the current code page (code page number).
#pragma warning (disable:4819)


#include <memory>
#include <string>

/*
// Opencv library should be linked before using this Opencv to Gdiplus helper class, 
// which is modified version from the following source. 
// http://stackoverflow.com/questions/24725155/opencv-tesseract-how-to-replace-libpng-libtiff-etc-with-gdi-bitmap-load-in
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
*/

// Add USE_OPENCV_IN_CPPUTIL macro to the preprocessor definitions of target project, 
// or
#define USE_OPENCV_IN_CPPUTIL
#ifdef USE_OPENCV_IN_CPPUTIL 
// IMPORTANT:
// This must be included AFTER gdiplus !!
// (OpenCV #undefine's min(), max())
#include <opencv2/core/core.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/tracking.hpp>
#endif

#ifdef CPPUTIL_EXPORTS
#define CPPUTIL_DLL_API		__declspec(dllexport)
#else
#define CPPUTIL_DLL_API		__declspec(dllimport)
#endif

namespace cpp_util
{
	using namespace std;
}