#pragma once

#include "WsoConfig2.h"


namespace wso_config
{
	class CameraSetting;
	class FixationSetting;
	class GalvanoSetting;
	class MeasureSetting;
	class LsoDisplaySetting;
	class LsoCaptureSetting;

	class WSOCONFIG_DLL_API SystemConfig
	{
	public:
		SystemConfig();
		virtual ~SystemConfig();

		SystemConfig(const SystemConfig& rhs) = delete;
		SystemConfig& operator=(const SystemConfig& rhs) = delete;

		static SystemConfig* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void setupBoardProfile(HbsDataProfile* profile);

		void resetToDefaults(void);
		bool updateFromBoardProfile(HbsDataProfile* profile);
		bool uploadToBoardProfile(HbsDataProfile* profile);

		CameraSetting* getCameraSetting(void) const;
		FixationSetting* getFixationSetting(void) const;
		GalvanoSetting* getGalvanoSetting(void) const;	

		MeasureSetting* getMeasureSetting(void) const;
		LsoCaptureSetting* getLsoCaptureSetting(void) const;
		LsoDisplaySetting* getLsoDisplaySetting(void) const;

	protected:
		HbsDataProfile* getBoardProfile(void) const;

	private:
		struct SystemConfigImpl;
		std::unique_ptr<SystemConfigImpl> d_ptr;
		SystemConfigImpl& impl(void) const;
	};
}

