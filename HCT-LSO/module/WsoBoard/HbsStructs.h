#pragma once

#include "WsoBoard2.h"
#include "HbsDefines.h"

#include <cstdint>
#include <cmath>


namespace wso_board
{
#pragma pack(push, 1)


	// Trajectory Profile Structure
	////////////////////////////////////////////////////////////////////////////////////
	struct TrajectoryProfileParam {
		float time_step_us;
		uint32_t trig_delay;
		uint16_t repeat_num;
		uint16_t sample_size;
		uint16_t cam_trig_cnt;
		uint16_t cam_trig_itv;
		uint16_t trig_st_index;
		uint16_t reserved[23];
	};

	struct TrajectoryProfile {
		TrajectoryProfileParam param;
		int16_t  posx[TRAJECT_SAMPLE_SIZE_MAX];
		int16_t  posy[TRAJECT_SAMPLE_SIZE_MAX];
	};


#pragma pack(pop)
}