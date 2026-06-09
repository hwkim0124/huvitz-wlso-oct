#pragma once

#include "OctReport.h"
#include "ProtocolSource.h"

#include <string>
#include <memory>
#include <vector>


namespace oct_report
{
	class LineDataSource;
	class CubeDataSource;
	class CircleDataSource;
	class CrossDataSource;
	class RadialDataSource;
	class RasterDataSource;
	class AngioDataSource;


	class OCTREPORT_DLL_API ProtocolSourceSet
	{
	public:
		ProtocolSourceSet();
		virtual ~ProtocolSourceSet();

		ProtocolSourceSet(ProtocolSourceSet&& rhs);
		ProtocolSourceSet& operator=(ProtocolSourceSet&& rhs);
		ProtocolSourceSet(const ProtocolSourceSet& rhs) = delete;
		ProtocolSourceSet& operator=(const ProtocolSourceSet& rhs) = delete;

	public:
		ProtocolSource* getProtocolSource(int sourceId);
		LineDataSource* getOrCreateLineDataSource(int sourceId);
		CubeDataSource* getOrCreateCubeDataSource(int sourceId);
		CircleDataSource* getOrCreateCircleDataSource(int sourceId);
		CrossDataSource* getOrCreateCrossDataSource(int sourceId);
		RadialDataSource* getOrCreateRadialDataSource(int sourceId);
		RasterDataSource* getOrCreateRasterDataSource(int sourceId);
		AngioDataSource* getOrCreateAngioDataSource(int sourceId);

		int generateSourceId(void) const;
		bool isSourceIdValid(int sourceId) const;
		void clearAllSources(void);

	protected:
		template <typename T>
		ProtocolSource* emplaceProtocolSource(int sourceId);

		template <typename T>
		ProtocolSource* fetchProtocolSource(int sourceId);

		template <typename T>
		ProtocolSource * createProtocolSource(int sourceId);

	private:
		struct ProtocolSourceSetImpl;
		std::unique_ptr<ProtocolSourceSetImpl> d_ptr;
		ProtocolSourceSetImpl& getImpl(void) const;
	};


	template<typename T>
	inline ProtocolSource * ProtocolSourceSet::emplaceProtocolSource(int sourceId)
	{
		auto data = fetchProtocolSource<T>(sourceId);
		if (data == nullptr) {
			return createProtocolSource<T>(sourceId);
		}
		return data;
	}

	template<typename T>
	inline ProtocolSource * ProtocolSourceSet::fetchProtocolSource(int sourceId)
	{
		auto iter = getImpl().dataset.find(sourceId);
		if (iter != end(getImpl().dataset)) {
			return dynamic_cast<T*>(iter->second.get());
		}
		return nullptr;
	}

	template<typename T>
	inline ProtocolSource * ProtocolSourceSet::createProtocolSource(int sourceId)
	{
		if (sourceId >= 0) {
			getImpl().dataset[sourceId] = make_unique<T>();
			getImpl().dataset[sourceId]->setSourceId(sourceId);
			return getImpl().dataset[sourceId].get();
		}
		return nullptr;
	}
}
