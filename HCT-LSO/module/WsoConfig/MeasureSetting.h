#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API MeasureSetting
	{
	public:
		MeasureSetting();
		virtual ~MeasureSetting();

		MeasureSetting(MeasureSetting&& rhs);
		MeasureSetting& operator=(MeasureSetting&& rhs);
		MeasureSetting(const MeasureSetting& rhs);
		MeasureSetting& operator=(const MeasureSetting& rhs);

	public:
		void initializeMeasureSetting(void);
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		StageReadyPositionPreset* getStageReadyPositionPreset(void) const;
		void setStageReadyPositionPreset(const StageReadyPositionPreset& preset);

		std::tuple<int, int, int> getCenterPos(void) const;
		std::tuple<int, int, int> getReadyPosOD(void) const;
		std::tuple<int, int, int> getReadyPosOS(void) const;
		std::tuple<int, int, int> getReadyPosME(void) const;

		void setCenterPos(std::tuple<int, int, int> pos);
		void setReadyPosOD(std::tuple<int, int, int> pos);
		void setReadyPosOS(std::tuple<int, int, int> pos);
		void setReadyPosME(std::tuple<int, int, int> pos);

	protected:
		struct MeasureSettingImpl;
		std::unique_ptr<MeasureSettingImpl> d_ptr;
		MeasureSettingImpl& impl(void) const;
	};
}

