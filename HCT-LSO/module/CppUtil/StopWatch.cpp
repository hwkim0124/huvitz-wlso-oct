#include "pch.h"
#include "StopWatch.h"

#include <map>

using namespace cpp_util;
using namespace std;


struct StopWatch::StopWatchImpl {

	LARGE_INTEGER freq;

	std::map<uint32_t, LARGE_INTEGER> startTimes;
	std::map<uint32_t, LARGE_INTEGER> recordTimes;

	StopWatchImpl() {
		QueryPerformanceFrequency(&freq);
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<StopWatch::StopWatchImpl> StopWatch::d_ptr(new StopWatchImpl());


StopWatch::StopWatch()
{
}


StopWatch::~StopWatch()
{
}


StopWatch::StopWatchImpl& cpp_util::StopWatch::impl(void)
{
	return *d_ptr;
}


void cpp_util::StopWatch::start(std::uint32_t tid)
{
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	d_ptr->startTimes[tid] = start;
	d_ptr->recordTimes[tid] = start;
	return;
}


double cpp_util::StopWatch::getElapsedSecs(std::uint32_t tid, bool whole)
{
	return getSampling(tid, whole);
}


double cpp_util::StopWatch::getElapsedMillis(std::uint32_t tid, bool whole)
{
	return getSampling(tid, whole) * 1e3;
}


double cpp_util::StopWatch::getSampling(std::uint32_t tid, bool whole)
{
	LARGE_INTEGER present, record;

	QueryPerformanceCounter(&present);
	record = (whole ? d_ptr->startTimes[tid] : d_ptr->recordTimes[tid]);

	double elapsed = (double)(present.QuadPart - record.QuadPart) / (double)(d_ptr->freq.QuadPart);
	d_ptr->recordTimes[tid] = present;
	return elapsed;
}


