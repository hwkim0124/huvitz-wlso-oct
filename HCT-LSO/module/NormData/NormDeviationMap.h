#pragma once


#include "NormData.h"

#include <string>
#include <memory>
#include <map>


namespace cpp_util {
	class CvImage;
}

namespace norm_data
{
	class NORMDATA_DLL_API NormDeviationMap
	{
	public:
		NormDeviationMap();
		virtual ~NormDeviationMap();

		NormDeviationMap(NormDeviationMap&& rhs);
		NormDeviationMap& operator=(NormDeviationMap&& rhs);

		// Prevent copy construction and assignment. 
		NormDeviationMap(const NormDeviationMap& rhs) = delete;
		NormDeviationMap& operator=(const NormDeviationMap& rhs) = delete;

	public:
		static const int MAP_WIDTH = 512;
		static const int MAP_HEIGHT = 96;
		static const int MAP_SIZE = MAP_WIDTH * MAP_HEIGHT;

		virtual cpp_util::CvImage makeImage(Ethinicity race, Gender gender, int age, EyeSide side,
			const std::vector<float>& data, 
			int lines, int points, int width, int height);

	private:
		struct NormDeviationMapImpl;
		std::unique_ptr<NormDeviationMapImpl> d_ptr;
		NormDeviationMapImpl& getImpl(void) const;
	};
}

