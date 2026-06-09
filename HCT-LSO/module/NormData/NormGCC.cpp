#include "pch.h"
#include "NormGCC.h"

#include <stdio.h>
#include <string>
#include <map>

#include <boost/math/distributions/normal.hpp>


using namespace norm_data;


struct NormGCC::NormGCCImpl
{
	std::map<std::string, GCC_Data> asianMaleData;
	std::map<std::string, GCC_Data> asianFemaleData;
	std::map<std::string, GCC_Data> cocasMaleData;
	std::map<std::string, GCC_Data> cocasFemaleData;
	std::map<std::string, GCC_Data> latinoMaleData;
	std::map<std::string, GCC_Data> latinoFemaleData;
	std::map<std::string, GCC_Data> mixedMaleData;
	std::map<std::string, GCC_Data> mixedFemaleData;

	NormGCCImpl()
	{
	}
};


NormGCC::NormGCC() : d_ptr(make_unique<NormGCCImpl>())
{
}


NormGCC::~NormGCC()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
norm_data::NormGCC::NormGCC(NormGCC && rhs) = default;
NormGCC & norm_data::NormGCC::operator=(NormGCC && rhs) = default;


void norm_data::NormGCC::insertData(Ethinicity race, Gender gender, const char * sector, float inclin,
	float inter1, float inter2, float inter3, float inter4, float inter5)
{
	auto& dataMap = getDataMap(race, gender);

	float inters[GCC_Data::N_VALUES];
	inters[0] = inter5;
	inters[1] = inter4;
	// inters[2] = inter1;
	inters[2] = inter2;
	inters[3] = inter3;

	dataMap.insert({ sector, GCC_Data(inclin, inters) });
	return;
}

void norm_data::NormGCC::insertData(Ethinicity race, Gender gender, const char * sector, float inclin, float inter, float stddev)
{
	auto& dataMap = getDataMap(race, gender);
	dataMap.insert({ sector, GCC_Data(inclin, inter, stddev) });
	return;
}


int norm_data::NormGCC::getPercentile(Ethinicity race, Gender gender, int age, EyeSide side, int sector, float value)
{
	auto& dataMap = getDataMap(race, gender);
	if (dataMap.empty()) {
		return 0;
	}

	auto sectStr = std::string(getSectorStr(side, sector));

	auto data = dataMap[sectStr];
	int percentiles[GCC_Data::N_VALUES+1] = { 1, 5, 95, 99, 100 };

	float mean = data.getMean(age);
	boost::math::normal dist(mean, data.stddev);

	int index = 0;
	for (; index < GCC_Data::N_VALUES; index++) {
		// float norm = data.getNormValue(age, index);
		float norm = (float)boost::math::quantile(dist, percentiles[index] * 0.01f);
		if (value <= norm) {
			return percentiles[index];
		}
	}
	return percentiles[GCC_Data::N_VALUES];
}


const char* norm_data::NormGCC::getSectorStr(EyeSide side, int sector)
{
	switch (sector) {
	case 0:
		return "SUP";
	case 1:
		return "S_N"; // side == EyeSide::OD ? "S_N" : "T_S";
	case 2:
		return "N_I"; // side == EyeSide::OD ? "N_I" : "I_T";
	case 3:
		return "INF";
	case 4:
		return "I_T"; // side == EyeSide::OD ? "I_T" : "N_I";
	case 5:
		return "T_S"; // side == EyeSide::OD ? "T_S" : "S_N";
	}
	return "SUP";
}


std::map<std::string, GCC_Data>& norm_data::NormGCC::getDataMap(Ethinicity race, Gender gender)
{
	if (race == Ethinicity::ASIAN) {
		if (gender == Gender::MALE) {
			return getImpl().asianMaleData;
		}
		else {
			return getImpl().asianFemaleData;
		}
	}
	else if (race == Ethinicity::COCASIAN) {
		if (gender == Gender::MALE) {
			return getImpl().cocasMaleData;
		}
		else {
			return getImpl().cocasFemaleData;
		}
	}
	else if (race == Ethinicity::LATINO) {
		if (gender == Gender::MALE) {
			return getImpl().latinoMaleData;
		}
		else {
			return getImpl().latinoFemaleData;
		}
	}
	else {
		if (gender == Gender::MALE) {
			return getImpl().mixedMaleData;
		}
		else {
			return getImpl().mixedFemaleData;
		}
	}
}


NormGCC::NormGCCImpl & norm_data::NormGCC::getImpl(void) const
{
	return *d_ptr;
}
