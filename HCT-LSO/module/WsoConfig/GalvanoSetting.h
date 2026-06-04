#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API GalvanoSetting
	{
	public:
		GalvanoSetting();
		virtual ~GalvanoSetting();

		GalvanoSetting(GalvanoSetting&& rhs);
		GalvanoSetting& operator=(GalvanoSetting&& rhs);
		GalvanoSetting(const GalvanoSetting& rhs);
		GalvanoSetting& operator=(const GalvanoSetting& rhs);

	public:
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		OctGalvanoConfigParam* getOctGalvanoConfigParam(void) const;
		void setOctGalvanoConfigParam(const OctGalvanoConfigParam& param);

		void setScaleX(int index, float value);
		void setScaleY(int index, float value);
		float getScaleX(int index) const;	
		float getScaleY(int index) const;

		void setOffsetX(float value);
		void setOffsetY(float value);
		float getOffsetX(void) const;
		float getOffsetY(void) const;

	protected:
		struct GalvanoSettingImpl;
		std::unique_ptr<GalvanoSettingImpl> d_ptr;
		GalvanoSettingImpl& impl(void) const;
	};
}