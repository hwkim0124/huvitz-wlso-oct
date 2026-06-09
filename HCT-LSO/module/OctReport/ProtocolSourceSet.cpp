#include "pch.h"
#include "ProtocolSource.h"
#include "ProtocolSourceSet.h"
#include "LineDataSource.h"
#include "CubeDataSource.h"
#include "CircleDataSource.h"
#include "CrossDataSource.h"
#include "RadialDataSource.h"
#include "RasterDataSource.h"
#include "AngioDataSource.h"

#include <map>

using namespace oct_report;


struct ProtocolSourceSet::ProtocolSourceSetImpl
{
	std::map<int, unique_ptr<ProtocolSource>> dataset;

	ProtocolSourceSetImpl()
	{
	}
};


ProtocolSourceSet::ProtocolSourceSet() :
	d_ptr(make_unique<ProtocolSourceSetImpl>())
{
}


oct_report::ProtocolSourceSet::~ProtocolSourceSet() = default;
oct_report::ProtocolSourceSet::ProtocolSourceSet(ProtocolSourceSet && rhs) = default;
ProtocolSourceSet & oct_report::ProtocolSourceSet::operator=(ProtocolSourceSet && rhs) = default;


ProtocolSource * oct_report::ProtocolSourceSet::getProtocolSource(int sourceId)
{
	auto data = fetchProtocolSource<ProtocolSource>(sourceId);
	return data;
}


LineDataSource * oct_report::ProtocolSourceSet::getOrCreateLineDataSource(int sourceId)
{
	auto data = static_cast<LineDataSource*>(emplaceProtocolSource<LineDataSource>(sourceId));
	return data;
}


CubeDataSource * oct_report::ProtocolSourceSet::getOrCreateCubeDataSource(int sourceId)
{
	auto data = static_cast<CubeDataSource*>(emplaceProtocolSource<CubeDataSource>(sourceId));
	return data;
}


CircleDataSource * oct_report::ProtocolSourceSet::getOrCreateCircleDataSource(int sourceId)
{
	auto data = static_cast<CircleDataSource*>(emplaceProtocolSource<CircleDataSource>(sourceId));
	return data;
}


CrossDataSource * oct_report::ProtocolSourceSet::getOrCreateCrossDataSource(int sourceId)
{
	auto data = static_cast<CrossDataSource*>(emplaceProtocolSource<CrossDataSource>(sourceId));
	return data;
}


RadialDataSource * oct_report::ProtocolSourceSet::getOrCreateRadialDataSource(int sourceId)
{
	auto data = static_cast<RadialDataSource*>(emplaceProtocolSource<RadialDataSource>(sourceId));
	return data;
}


RasterDataSource * oct_report::ProtocolSourceSet::getOrCreateRasterDataSource(int sourceId)
{
	auto data = static_cast<RasterDataSource*>(emplaceProtocolSource<RasterDataSource>(sourceId));
	return data;
}

AngioDataSource* oct_report::ProtocolSourceSet::getOrCreateAngioDataSource(int sourceId)
{
	auto data = static_cast<AngioDataSource*>(emplaceProtocolSource<AngioDataSource>(sourceId));
	return data;
}

int oct_report::ProtocolSourceSet::generateSourceId(void) const
{
	for (int i = 0; i < 100; ++i) {
		if (isSourceIdValid(i) == false) {
			return i;
		}
	}
	return 0;
}

bool oct_report::ProtocolSourceSet::isSourceIdValid(int sourceId) const
{
	auto iter = getImpl().dataset.find(sourceId);
	if (iter != end(getImpl().dataset)) {
		return true;
	}
	return false;
}


void oct_report::ProtocolSourceSet::clearAllSources(void)
{
	d_ptr->dataset.clear();
	return;
}


ProtocolSourceSet::ProtocolSourceSetImpl & oct_report::ProtocolSourceSet::getImpl(void) const
{
	return *d_ptr;
}
