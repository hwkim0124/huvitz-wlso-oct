#include "pch.h"
#include "TraceProfile.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct TraceProfile::TraceProfileImpl
{
	float timeStepInUs;
	uint32_t triggerDelay;
	uint16_t numRepeats;
	uint16_t sampleSize;
	uint16_t cameraTriggerCount;
	uint16_t cameraTriggerInterval;
	uint16_t triggerStartIndex;
	int16_t  xPos[TRAJECT_SAMPLE_SIZE_MAX];
	int16_t  yPos[TRAJECT_SAMPLE_SIZE_MAX];

	TraceProfileImpl() : timeStepInUs(TRAJECT_TIME_STEP_IN_US), triggerDelay(TRAJECT_TRIGGER_DELAY),
		numRepeats(TRAJECT_NUMBER_OF_REPEATS), sampleSize(TRAJECT_SAMPLE_SIZE),
		cameraTriggerCount(TRAJECT_CAMERA_TRIGGER_COUNT), cameraTriggerInterval(TRAJECT_CAMERA_TRIGGER_INTERVAL),
		triggerStartIndex(TRAJECT_TRIGGER_START_INDEX)
	{
	}
};


TraceProfile::TraceProfile() :
	d_ptr(make_unique<TraceProfileImpl>())
{
}


wso_device::TraceProfile::~TraceProfile() = default;
wso_device::TraceProfile::TraceProfile(TraceProfile&& rhs) = default;
TraceProfile& wso_device::TraceProfile::operator=(TraceProfile&& rhs) = default;


wso_device::TraceProfile::TraceProfile(const TraceProfile& rhs)
	: d_ptr(make_unique<TraceProfileImpl>(*rhs.d_ptr))
{
}


TraceProfile& wso_device::TraceProfile::operator=(const TraceProfile& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


void wso_device::TraceProfile::setNumberOfRepeats(int repeats)
{
	impl().numRepeats = repeats;
	return;
}


void wso_device::TraceProfile::setTimeStepInUs(float timeStep)
{
	impl().timeStepInUs = timeStep;
	return;
}


void wso_device::TraceProfile::setSampleSize(int samples)
{
	impl().sampleSize = samples;
	return;
}


void wso_device::TraceProfile::setTriggerCount(int count)
{
	impl().cameraTriggerCount = count;
	return;
}


void wso_device::TraceProfile::setTriggerDelay(int delay)
{
	impl().triggerDelay = delay;
	return;
}


void wso_device::TraceProfile::setTriggerStartIndex(int index)
{
	impl().triggerStartIndex = index;
	return;
}


int wso_device::TraceProfile::getTriggerCount(void) const
{
	return impl().cameraTriggerCount;
}


int wso_device::TraceProfile::getSampleSize(void) const
{
	return impl().sampleSize;
}


int wso_device::TraceProfile::getTriggerStartIndex(void) const
{
	return impl().triggerStartIndex;
}


short* wso_device::TraceProfile::getPositionsX(void) const
{
	return impl().xPos;
}


short* wso_device::TraceProfile::getPositionsY(void) const
{
	return impl().yPos;
}


void wso_device::TraceProfile::getTrajectoryProfileParam(TrajectoryProfileParam& params)
{
	params.time_step_us = impl().timeStepInUs;
	params.trig_delay = impl().triggerDelay;
	params.repeat_num = impl().numRepeats;
	params.sample_size = impl().sampleSize;
	params.cam_trig_cnt = impl().cameraTriggerCount;
	params.cam_trig_itv = impl().cameraTriggerInterval;
	params.trig_st_index = impl().triggerStartIndex;
	return;
}


TraceProfile::TraceProfileImpl& wso_device::TraceProfile::impl(void) const
{
	return *d_ptr;
}
