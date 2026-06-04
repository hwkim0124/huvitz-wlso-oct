#pragma once

#include "WsoConfig2.h"


#include <memory>
#include <vector>


namespace wso_config
{
	class WSOCONFIG_DLL_API FixationSetting
	{
	public:
		FixationSetting();
		virtual ~FixationSetting();

		FixationSetting(FixationSetting&& rhs);
		FixationSetting& operator=(FixationSetting&& rhs);
		FixationSetting(const FixationSetting& rhs);
		FixationSetting& operator=(const FixationSetting& rhs);

	public:
		void initializeFixationSetting(void);
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		InternalFixationPreset* getInternalFixationPreset(void) const;
		void setInternalFixationPreset(const InternalFixationPreset& param);

		std::pair<int, int> getCenter(EyeSide side) const;
		std::pair<int, int> getFundus(EyeSide side) const;
		std::pair<int, int> getOpticDisc(EyeSide side) const;
		std::pair<int, int> getLeftSide(EyeSide side) const;
		std::pair<int, int> getLeftTop(EyeSide side) const;
		std::pair<int, int> getLeftBottom(EyeSide side) const;
		std::pair<int, int> getRightSide(EyeSide side) const;
		std::pair<int, int> getRightTop(EyeSide side) const;
		std::pair<int, int> getRightBottom(EyeSide side) const;

		int getBrightness(void) const;
		int getBlinkPeriod(void) const;
		int getBlinkOnTime(void) const;
		int getFixationType(void) const;

		void setCenter(EyeSide side, std::pair<int, int> pos);
		void setFundus(EyeSide side, std::pair<int, int> pos);
		void setOpticDisc(EyeSide side, std::pair<int, int> pos);
		void setLeftSide(EyeSide side, std::pair<int, int> pos);
		void setLeftTop(EyeSide side, std::pair<int, int> pos);
		void setLeftBottom(EyeSide side, std::pair<int, int> pos);
		void setRightSide(EyeSide side, std::pair<int, int> pos);
		void setRightTop(EyeSide side, std::pair<int, int> pos);
		void setRightBottom(EyeSide side, std::pair<int, int> pos);

		void setBrightness(int bright);
		void setBlinkPeriod(int period);
		void setBlinkOnTime(int onTime);
		void setFixationType(int type);

		bool useLcdFixation(bool isset = false, bool flag = false) const;
		bool useLcdBlinkOn(bool isset = false, bool flag = false) const;

	protected:
		std::pair<int, int> getPairOfFixation(const FixationPoint& point) const;
		void setFixationByPair(FixationPoint& point, std::pair<int, int> data) const;

	private:
		struct FixationSettingImpl;
		std::unique_ptr<FixationSettingImpl> d_ptr;
		FixationSettingImpl& impl(void) const;
	};
}