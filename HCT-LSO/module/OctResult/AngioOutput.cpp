#include "pch.h"
#include "AngioOutput.h"


#include <boost/format.hpp>
#include <thread>
#include <vector>
#include <algorithm>

using namespace oct_result;


struct AngioOutput::AngioOutputImpl
{
	vector<vector<CvImage>> amplitudes;
	vector<vector<CvImage>> imags;
	vector<vector<CvImage>> reals;

	AngioOutputImpl()
	{
	}
};


AngioOutput::AngioOutput() :
	d_ptr(make_unique<AngioOutputImpl>())
{
}


oct_result::AngioOutput::~AngioOutput() = default;
oct_result::AngioOutput::AngioOutput(AngioOutput&& rhs) = default;
AngioOutput& oct_result::AngioOutput::operator=(AngioOutput&& rhs) = default;



bool oct_result::AngioOutput::setAmplitudes(std::vector<std::vector<CvImage>>&& ampls)
{
	impl().amplitudes = std::move(ampls);
	return true;
}

std::vector<std::vector<CvImage>>& oct_result::AngioOutput::getAmplitudes(void) const
{
	return impl().amplitudes;
}

AngioOutput::AngioOutputImpl& oct_result::AngioOutput::impl(void) const
{
	return *d_ptr;
}
