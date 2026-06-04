#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API CameraSetting
	{
	public:
		CameraSetting();
		virtual ~CameraSetting();

		CameraSetting(CameraSetting&& rhs);
		CameraSetting& operator=(CameraSetting&& rhs);
		CameraSetting(const CameraSetting& rhs);
		CameraSetting& operator=(const CameraSetting& rhs);

	public:
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		CorneaCameraConfigParam* getCorneaCameraConfigParam(void) const;
		void setCorneaCameraConfigParam(const CorneaCameraConfigParam& param);

		float getCorneaAgain(void) const;
		float getCorneaDgain(void) const;
		int getWdotIntensity(int index) const;

		void setCorneaAgain(float value);
		void setCorneaDgain(float value);
		void setWdotIntensity(int index, int value);

	protected:
		struct CameraSettingImpl;
		std::unique_ptr<CameraSettingImpl> d_ptr;
		CameraSettingImpl& impl(void) const;
	};
}