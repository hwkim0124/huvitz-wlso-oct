#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API CameraSettings
	{
	public:
		CameraSettings();
		virtual ~CameraSettings();

		CameraSettings(CameraSettings&& rhs);
		CameraSettings& operator=(CameraSettings&& rhs);
		CameraSettings(const CameraSettings& rhs);
		CameraSettings& operator=(const CameraSettings& rhs);

	public:
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		CorneaCameraConfigParam* getCorneaCameraConfigParam(void) const;
		void setCorneaCameraConfigParam(const CorneaCameraConfigParam& param);

		float getCorneaAgain(CameraType type) const;
		float getCorneaDgain(CameraType type) const;

		void setCorneaAgain(CameraType type, float value);
		void setCorneaDgain(CameraType type, float value);

	protected:
		struct CameraSettingsImpl;
		std::unique_ptr<CameraSettingsImpl> d_ptr;
		CameraSettingsImpl& impl(void) const;
	};
}