#include "pch.h"
#include "numeric.h"

#include <numbers>


double CPPUTIL_DLL_API cpp_util::degreeToRadian(double degree)
{
	return ((degree * std::numbers::pi) / 180.0);
}

double CPPUTIL_DLL_API cpp_util::radianToDegree(double radian)
{
	return ((radian * 180.0) / std::numbers::pi);
}
