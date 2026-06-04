#pragma once


#include "WsoConfig2.h"

#include <memory>
#include <string>


namespace tinyxml2 {
	class XMLElement;
}


namespace wso_config
{
	class SysConfiguration;
	class CameraSetting;
	class FixationSetting;
	class LsoDisplaySetting;
	class LsoCaptureSetting;
	class MeasureSetting;

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
		bool loadSystemConfig(const char* filename, SysConfiguration* config);
		bool saveSystemConfig(const char* filename, SysConfiguration* config);

	protected:
		bool loadCameraSetting(CameraSetting* cset);
		bool loadFixationSetting(FixationSetting* pset);
		bool loadMeasureSetting(MeasureSetting* mset);
		bool loadLsoCaptureSetting(LsoCaptureSetting* lcset);
		bool loadLsoDisplaySetting(LsoDisplaySetting* ldset);

		bool saveCameraSetting(const CameraSetting* cset);
		bool saveFixationSetting(const FixationSetting* pset);
		bool saveMeasureSetting(const MeasureSetting* mset);
		bool saveLsoCaptureSetting(const LsoCaptureSetting* lcset);
		bool saveLsoDisplaySetting(const LsoDisplaySetting* ldset);

		bool loadSectionOfFixation(FixationSetting* pset, tinyxml2::XMLElement* group);
		bool loadSectionOfCamera(CameraSetting* cset, tinyxml2::XMLElement* group);
		bool loadSectionOfMeasure(MeasureSetting* mset, tinyxml2::XMLElement* group);
		bool loadSectionOfLsoCapture(LsoCaptureSetting* lcset, tinyxml2::XMLElement* group);
		bool loadSectionOfLsoDisplay(LsoDisplaySetting* ldset, tinyxml2::XMLElement* group);

		bool saveSectionOfFixation(const FixationSetting* pset, tinyxml2::XMLElement* group);
		bool saveSectionOfCamera(const CameraSetting* cset, tinyxml2::XMLElement* group);
		bool saveSectionOfMeasure(const MeasureSetting* mset, tinyxml2::XMLElement* group);
		bool saveSectionOfLsoCapture(const LsoCaptureSetting* lcset, tinyxml2::XMLElement* group);
		bool saveSectionOfLsoDisplay(const LsoDisplaySetting* ldset, tinyxml2::XMLElement* group);

	private:
		struct SysConfigFileImpl;
		std::unique_ptr<SysConfigFileImpl> d_ptr;
		SysConfigFileImpl& getImpl(void);

		bool checkXMLResult(int result);
	};
}

