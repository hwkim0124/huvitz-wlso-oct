#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API LsoDisplaySetting
	{
	public:
		LsoDisplaySetting();
		virtual ~LsoDisplaySetting();

		LsoDisplaySetting(LsoDisplaySetting&& rhs);
		LsoDisplaySetting& operator=(LsoDisplaySetting&& rhs);
		LsoDisplaySetting(const LsoDisplaySetting& rhs);
		LsoDisplaySetting& operator=(const LsoDisplaySetting& rhs);

	public:
		void initializeLsoDisplaySetting(void);
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		LsoImageMaskPreset* getImageMaskPreset(void) const;
		void setImageMaskPreset(const LsoImageMaskPreset& param);

		int getMaskIrRadius(void) const;
		int getMaskColorRadius(void) const;
		void setMaskIrRadius(int radius);
		void setMaskColorRadius(int radius);

		LsoImageAdjustPreset* getImageAdjustPreset(void) const;
		void setImageAdjustPreset(const LsoImageAdjustPreset& param);

		int getAdjustBrightness(void) const;
		void setAdjustBrightness(int bright);

	protected:
		struct LsoDisplaySettingImpl;
		std::unique_ptr<LsoDisplaySettingImpl> d_ptr;
		LsoDisplaySettingImpl& impl(void) const;
	};
}

