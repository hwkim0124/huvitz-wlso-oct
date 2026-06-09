#include "pch.h"
#include "NormDeviationMap.h"

#include <stdio.h>
#include <string>
#include <map>

using namespace norm_data;


struct NormDeviationMap::NormDeviationMapImpl
{
	NormDeviationMapImpl()
	{
	}
};


NormDeviationMap::NormDeviationMap() : d_ptr(make_unique<NormDeviationMapImpl>())
{
}


NormDeviationMap::~NormDeviationMap()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
norm_data::NormDeviationMap::NormDeviationMap(NormDeviationMap && rhs) = default;
NormDeviationMap & norm_data::NormDeviationMap::operator=(NormDeviationMap && rhs) = default;


cpp_util::CvImage norm_data::NormDeviationMap::makeImage(Ethinicity race, Gender gender, int age, EyeSide side, const std::vector<float>& data, int lines, int points, int width, int height)
{
	return cpp_util::CvImage();
}

NormDeviationMap::NormDeviationMapImpl & norm_data::NormDeviationMap::getImpl(void) const
{
	return *d_ptr;
}
