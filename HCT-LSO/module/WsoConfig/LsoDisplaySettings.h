#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API LsoDisplaySettings
	{
	public:
		LsoDisplaySettings();
		virtual ~LsoDisplaySettings();

		LsoDisplaySettings(LsoDisplaySettings&& rhs);
		LsoDisplaySettings& operator=(LsoDisplaySettings&& rhs);
		LsoDisplaySettings(const LsoDisplaySettings& rhs);
		LsoDisplaySettings& operator=(const LsoDisplaySettings& rhs);

	public:
		void initializeLsoDisplaySettings(void);
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
		struct LsoDisplaySettingsImpl;
		std::unique_ptr<LsoDisplaySettingsImpl> d_ptr;
		LsoDisplaySettingsImpl& impl(void) const;
	};
}

