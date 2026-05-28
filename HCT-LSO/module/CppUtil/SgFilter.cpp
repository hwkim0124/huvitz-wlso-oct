#include "pch.h"
#include "SgFilter.h"
#include "sgsmooth.h"

using namespace cpp_util;
using namespace std;

SgFilter::SgFilter()
{
}


SgFilter::~SgFilter()
{
}

std::vector<double> cpp_util::SgFilter::smoothDoubles(std::vector<double> data, int window, int degree)
{
	vector<double> res = sg_smooth(data, window, degree);
	return res;
}

std::vector<int> cpp_util::SgFilter::smoothInts(std::vector<int> data, int window, int degree)
{
	vector<double> outs = smoothDoubles(vector<double>(data.cbegin(), data.cend()), window, degree);
	vector<int> ints = vector<int>(outs.size());
	for (int i = 0; i < outs.size(); i++) {
		ints[i] = static_cast<int>(outs[i]);
	}
	return ints;
}


