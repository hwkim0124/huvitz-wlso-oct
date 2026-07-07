#pragma once


#include "WsoConfig2.h"

#include <memory>
#include <string>


namespace tinyxml2 {
	class XMLElement;
}


namespace wso_config
{
	class SystemConfig;
	class CameraSettings;
	class FixationSettings;
	class LsoDisplaySettings;
	class LsoCaptureSettings;

	class WSOCONFIG_DLL_API SysConfigFile
	{
	public:
		SysConfigFile();
		virtual ~SysConfigFile();

		SysConfigFile(SysConfigFile&& rhs);
		SysConfigFile& operator=(SysConfigFile&& rhs);
		SysConfigFile(const SysConfigFile& rhs);
		SysConfigFile& operator=(const SysConfigFile& rhs);

	public:
		bool loadSystemConfig(const char* filename, SystemConfig* config);
		bool saveSystemConfig(const char* filename, SystemConfig* config);

	protected:
		bool loadCameraSettings(CameraSettings* cset);
		bool loadFixationSettings(FixationSettings* pset);
		bool loadLsoCaptureSettings(LsoCaptureSettings* lcset);
		bool loadLsoDisplaySettings(LsoDisplaySettings* ldset);

		bool saveCameraSettings(const CameraSettings* cset);
		bool saveFixationSettings(const FixationSettings* pset);
		bool saveLsoCaptureSettings(const LsoCaptureSettings* lcset);
		bool saveLsoDisplaySettings(const LsoDisplaySettings* ldset);

		bool loadSectionOfFixation(FixationSettings* pset, tinyxml2::XMLElement* group);
		bool loadSectionOfCamera(CameraSettings* cset, tinyxml2::XMLElement* group);
		bool loadSectionOfLsoCapture(LsoCaptureSettings* lcset, tinyxml2::XMLElement* group);
		bool loadSectionOfLsoDisplay(LsoDisplaySettings* ldset, tinyxml2::XMLElement* group);

		bool saveSectionOfFixation(const FixationSettings* pset, tinyxml2::XMLElement* group);
		bool saveSectionOfCamera(const CameraSettings* cset, tinyxml2::XMLElement* group);
		bool saveSectionOfLsoCapture(const LsoCaptureSettings* lcset, tinyxml2::XMLElement* group);
		bool saveSectionOfLsoDisplay(const LsoDisplaySettings* ldset, tinyxml2::XMLElement* group);

	private:
		struct SysConfigFileImpl;
		std::unique_ptr<SysConfigFileImpl> d_ptr;
		SysConfigFileImpl& getImpl(void);

		bool checkXMLResult(int result);
	};
}

