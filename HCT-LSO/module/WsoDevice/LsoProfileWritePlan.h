#pragma once

#include "wso_hbs.h"

#include <cstdint>
#include <limits>

namespace wso_device
{
	namespace detail
	{
		struct LsoProfileWritePlan
		{
			std::uint32_t address = 0;
			std::uint32_t size = 0;
			const unsigned char* payload = nullptr;

			explicit operator bool() const { return payload != nullptr && size != 0; }
		};

		inline LsoProfileWritePlan makeLsoProfileWritePlan(std::uint8_t patternId,
			const wso_board::lso_scan_profile_st* data, std::uint32_t tableBase, std::uint32_t tableSize)
		{
			LsoProfileWritePlan plan;
			if (data == nullptr || tableBase == 0 || tableSize == 0 || patternId > MAX_LSO_TRAJ_NUM) {
				return plan;
			}

			const std::uint64_t slotOffset = (std::uint64_t)sizeof(wso_board::lso_capture_status_st)
				+ (std::uint64_t)patternId * sizeof(wso_board::lso_scan_profile_st);
			const std::uint64_t slotEnd = slotOffset + sizeof(wso_board::lso_scan_profile_st);
			const std::uint64_t address = (std::uint64_t)tableBase + slotOffset;
			const std::uint64_t addressEnd = address + sizeof(wso_board::lso_scan_profile_st);
			if (slotEnd > tableSize
				|| addressEnd > (std::uint64_t)(std::numeric_limits<std::uint32_t>::max)() + 1u) {
				return plan;
			}

			plan.address = (std::uint32_t)address;
			plan.size = (std::uint32_t)sizeof(wso_board::lso_scan_profile_st);
			plan.payload = reinterpret_cast<const unsigned char*>(data);
			return plan;
		}
	}
}
