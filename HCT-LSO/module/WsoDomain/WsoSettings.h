#pragma once

#include "WsoDomain2.h"


namespace wso_domain
{
	class WSODOMAIN_DLL_API WsoSettings
	{
	public:
		WsoSettings();
		virtual ~WsoSettings();

		WsoSettings(const WsoSettings& rhs) = delete;
		WsoSettings& operator=(const WsoSettings& rhs) = delete;

		static WsoSettings* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool isUserModeOn(void);
		bool isOctTasksDefaultPresetOn(void);
		bool isOctEnfaceImageCorrectOn(void);

		void applyUserModeSettings(bool flag);

		void setUserModeOn(bool flag);
		void setOctTasksDefaultPresetOn(bool flag);
		void setOctEnfaceImageCorrectOn(bool flag);

	private:
		struct WsoSettingsImpl;
		std::unique_ptr<WsoSettingsImpl> d_ptr;
		WsoSettingsImpl& impl(void) const;
	};
}
