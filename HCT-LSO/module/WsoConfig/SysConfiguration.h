#pragma once

#include "WsoConfig2.h"


namespace wso_config
{
	class CameraSetting;
	class FixationSetting;
	class SignalSetting;
	class GalvanoSetting;
	class MeasureSetting;
	class LsoDisplaySetting;
	class LsoCaptureSetting;

	class WSOCONFIG_DLL_API SysConfiguration
	{
	public:
		SysConfiguration();
		virtual ~SysConfiguration();

		SysConfiguration(const SysConfiguration& rhs) = delete;
		SysConfiguration& operator=(const SysConfiguration& rhs) = delete;

		static SysConfiguration* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void resetToDefaultValues(void);
		bool loadConfigFile(const char* name = nullptr);
		bool saveConfigFile(const char* name = nullptr);

		std::string getDefaultConfigFilePath(void);
		std::string getDefaultConfigDirPath(void);

		CameraSetting* getCameraSetting(void) const;
		FixationSetting* getFixationSetting(void) const;
		SignalSetting* getSignalSetting(void) const;
		GalvanoSetting* getGalvanoSetting(void) const;

		MeasureSetting* getMeasureSetting(void) const;
		LsoCaptureSetting* getLsoCaptureSetting(void) const;
		LsoDisplaySetting* getLsoDisplaySetting(void) const;

	private:
		struct SystemConfigImpl;
		std::unique_ptr<SystemConfigImpl> d_ptr;
		SystemConfigImpl& getImpl(void) const;
	};
}

