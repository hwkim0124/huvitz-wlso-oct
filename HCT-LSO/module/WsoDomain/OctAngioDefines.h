#pragma once

#include "WsoDomain2.h"


namespace wso_domain
{
	typedef std::vector<std::vector<cpp_util::CvImage>> Amplitudes;
	typedef std::vector<std::vector<cpp_util::CvImage>> Imaginaries;
	typedef std::vector<std::vector<cpp_util::CvImage>> Reals;

	typedef std::vector<std::vector<int>> LayerArrays;
	typedef std::map<OcularLayerType, LayerArrays> LayerMapArrays;

	typedef std::vector<int> LayerPoints;
	typedef std::vector<float> AngioProfile;
}