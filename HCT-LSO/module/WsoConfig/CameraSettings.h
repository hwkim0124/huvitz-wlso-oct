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
		RetinaCameraConfigParam* getRetinaCameraConfigParam(void) const;
		void setCorneaCameraConfigParam(const CorneaCameraConfigParam& param);
		void setRetinaCameraConfigParam(const RetinaCameraConfigParam& param);

		float getRetinaAgain(void) const;
		float getRetinaDgain(void) const;
		float getRetinaExposureTime(void) const;
		float getCorneaAgain(CameraType type) const;
		float getCorneaDgain(CameraType type) const;
		float getCorneaExposureTime(CameraType type) const;

		void setRetinaAgain(float value);
		void setRetinaDgain(float value);
		void setRetinaExposureTime(float value);
		void setCorneaAgain(CameraType type, float value);
		void setCorneaDgain(CameraType type, float value);
		void setCorneaExposureTime(CameraType type, float value);

	protected:
		struct CameraSettingsImpl;
		std::unique_ptr<CameraSettingsImpl> d_ptr;
		CameraSettingsImpl& impl(void) const;
	};
}