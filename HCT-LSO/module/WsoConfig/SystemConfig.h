#pragma once

#include "WsoConfig2.h"


namespace wso_config
{
	class CameraSettings;
	class FixationSettings;
	class LsoDisplaySettings;
	class LsoCaptureSettings;

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

		void resetToDefaultValues(void);
		bool updateFromBoardProfile(HbsDataProfile* profile);
		bool uploadToBoardProfile(HbsDataProfile* profile);

		bool loadSysConfigFile(const char* name = nullptr);
		bool saveSysConfigFile(const char* name = nullptr);
		std::string getDefaultSysConfigFilePath(void);
		std::string getDefaultSysConfigDirPath(void);

		CameraSettings* getCameraSettings(void) const;
		FixationSettings* getFixationSettings(void) const;
		LsoCaptureSettings* getLsoCaptureSettings(void) const;
		LsoDisplaySettings* getLsoDisplaySettings(void) const;

	protected:
		HbsDataProfile* getBoardProfile(void) const;

	private:
		struct SystemConfigImpl;
		std::unique_ptr<SystemConfigImpl> d_ptr;
		SystemConfigImpl& impl(void) const;
	};
}

