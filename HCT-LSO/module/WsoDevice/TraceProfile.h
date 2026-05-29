#pragma once

#include "WsoDevice2.h"

#include <memory>
#include <string>


namespace wso_device
{
	using namespace wso_board;

	class TraceProfile
	{
	public:
		TraceProfile();
		virtual ~TraceProfile();

		TraceProfile(TraceProfile&& rhs);
		TraceProfile& operator=(TraceProfile&& rhs);
		TraceProfile(const TraceProfile& rhs);
		TraceProfile& operator=(const TraceProfile& rhs);

	public:
		void setNumberOfRepeats(int repeats);
		void setTimeStepInUs(float timeStep);
		void setSampleSize(int samples);
		void setTriggerCount(int count);
		void setTriggerDelay(int delay);
		void setTriggerStartIndex(int index);

		int getTriggerCount(void) const;
		int getSampleSize(void) const;
		int getTriggerStartIndex(void) const;
		short* getPositionsX(void) const;
		short* getPositionsY(void) const;

		void getTrajectoryProfileParam(TrajectoryProfileParam& params);

	private:
		struct TraceProfileImpl;
		std::unique_ptr<TraceProfileImpl> d_ptr;
		TraceProfileImpl& impl(void) const;
	};
}

