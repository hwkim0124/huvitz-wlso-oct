#pragma once

#include "CppUtil2.h"

namespace cpp_util
{
	class CPPUTIL_DLL_API StopWatch
	{
	public:
		StopWatch();
		virtual ~StopWatch();

	public:
		static void start(std::uint32_t tid = 0);

		static double getElapsedSecs(std::uint32_t tid = 0, bool whole = false);
		static double getElapsedMillis(std::uint32_t tid = 0, bool whole = false);


	protected:
		static double getSampling(std::uint32_t tid, bool whole);

	private:
		struct StopWatchImpl;
		static std::unique_ptr<StopWatchImpl> d_ptr;
		static StopWatchImpl& impl(void) ;
	};
}

