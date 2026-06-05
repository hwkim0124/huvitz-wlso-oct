#include "pch.h"
#include "SysConfigFile.h"
#include "SysConfiguration.h"
#include "CameraSetting.h"
#include "FixationSetting.h"
#include "MeasureSetting.h"
#include "LsoCaptureSetting.h"
#include "LsoDisplaySetting.h"

#include "tinyxml2.h"


using namespace wso_config;
using namespace std;
using namespace tinyxml2;



struct SysConfigFile::SysConfigFileImpl
{
	tinyxml2::XMLDocument* pDoc;
	tinyxml2::XMLNode* pRoot;
	tinyxml2::XMLNode* pNode;

	SysConfigFileImpl() : pDoc(nullptr), pRoot(nullptr), pNode(nullptr) {
	}
};


SysConfigFile::SysConfigFile() :
	d_ptr(make_unique<SysConfigFileImpl>())
{
}


wso_config::SysConfigFile::~SysConfigFile() = default;
wso_config::SysConfigFile::SysConfigFile(SysConfigFile&& rhs) = default;
SysConfigFile& wso_config::SysConfigFile::operator=(SysConfigFile&& rhs) = default;


wso_config::SysConfigFile::SysConfigFile(const SysConfigFile& rhs)
	: d_ptr(make_unique<SysConfigFileImpl>(*rhs.d_ptr))
{
}


SysConfigFile& wso_config::SysConfigFile::operator=(const SysConfigFile& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_config::SysConfigFile::loadSystemConfig(const char* filename, SysConfiguration* config)
{
	tinyxml2::XMLDocument doc;
	if (!checkXMLResult(doc.LoadFile(filename))) {
		return false;
	}

	XMLNode* pRoot = doc.FirstChildElement(SYS_CONFIG_FILE_ROOT);
	if (pRoot == nullptr) {
		return false;
	}

	getImpl().pDoc = &doc;
	getImpl().pRoot = pRoot;
	getImpl().pNode = pRoot;

	try {
		if (!loadFixationSetting(config->getFixationSetting())) {
			return false;
		}
		if (!loadCameraSetting(config->getCameraSetting())) {
			return false;
		}
		if (!loadMeasureSetting(config->getMeasureSetting())) {
			return false;
		}
		if (!loadLsoCaptureSetting(config->getLsoCaptureSetting())) {
			return false;
		}
		if (!loadLsoDisplaySetting(config->getLsoDisplaySetting())) {
			return false;
		}
	}
	catch (...) {
		return false;
	}

	return true;
}

bool wso_config::SysConfigFile::saveSystemConfig(const char* filename, SysConfiguration* config)
{
	tinyxml2::XMLDocument doc;

	XMLDeclaration* decl = doc.NewDeclaration();
	doc.LinkEndChild(decl);

	XMLElement* root = doc.NewElement(SYS_CONFIG_FILE_ROOT);
	doc.LinkEndChild(root);

	XMLComment* comment = doc.NewComment(SYS_CONFIG_FILE_ROOT_COMMENT);
	root->LinkEndChild(comment);

	getImpl().pDoc = &doc;
	getImpl().pRoot = root;
	getImpl().pNode = root;

	saveFixationSetting(config->getFixationSetting());
	saveCameraSetting(config->getCameraSetting());
	saveMeasureSetting(config->getMeasureSetting());
	saveLsoCaptureSetting(config->getLsoCaptureSetting());
	saveLsoDisplaySetting(config->getLsoDisplaySetting());

	return checkXMLResult(doc.SaveFile(filename));
}

bool wso_config::SysConfigFile::loadCameraSetting(CameraSetting* cset)
{
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = root->FirstChildElement(CAMERA_SETTING_GROUP);
	if (group == nullptr) return false;

	if (!loadSectionOfCamera(cset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::loadFixationSetting(FixationSetting* pset)
{
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = root->FirstChildElement(FIXATION_SETTING_GROUP);
	if (group == nullptr) return false;

	if (!loadSectionOfFixation(pset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::loadMeasureSetting(MeasureSetting* mset)
{
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = root->FirstChildElement(MEASURE_SETTING_GROUP);
	if (group == nullptr) return false;

	if (!loadSectionOfMeasure(mset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::loadLsoCaptureSetting(LsoCaptureSetting* lcset)
{
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = root->FirstChildElement(LSO_CAPTURE_SETTING_GROUP);
	if (group == nullptr) return false;

	if (!loadSectionOfLsoCapture(lcset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::loadLsoDisplaySetting(LsoDisplaySetting* ldset)
{
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = root->FirstChildElement(LSO_DISPLAY_SETTING_GROUP);
	if (group == nullptr) return false;

	if (!loadSectionOfLsoDisplay(ldset, group)) {
		return false;
	}
	return true;
}


bool wso_config::SysConfigFile::saveCameraSetting(const CameraSetting* cset)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = pdoc->NewElement(CAMERA_SETTING_GROUP);
	root->LinkEndChild(group);

	if (!saveSectionOfCamera(cset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::saveFixationSetting(const FixationSetting* pset)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = pdoc->NewElement(FIXATION_SETTING_GROUP);
	root->LinkEndChild(group);

	if (!saveSectionOfFixation(pset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::saveMeasureSetting(const MeasureSetting* mset)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = pdoc->NewElement(MEASURE_SETTING_GROUP);
	root->LinkEndChild(group);

	if (!saveSectionOfMeasure(mset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::saveLsoCaptureSetting(const LsoCaptureSetting* lcset)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = pdoc->NewElement(LSO_CAPTURE_SETTING_GROUP);
	root->LinkEndChild(group);

	if (!saveSectionOfLsoCapture(lcset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::saveLsoDisplaySetting(const LsoDisplaySetting* ldset)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	tinyxml2::XMLNode* root = getImpl().pRoot;

	XMLElement* group = pdoc->NewElement(LSO_DISPLAY_SETTING_GROUP);
	root->LinkEndChild(group);

	if (!saveSectionOfLsoDisplay(ldset, group)) {
		return false;
	}
	return true;
}

bool wso_config::SysConfigFile::loadSectionOfFixation(FixationSetting* pset, tinyxml2::XMLElement* group)
{
	XMLNode* child;
	XMLElement* elem;
	bool flag = false;
	int row, col;
	int bright;
	int period = 0, onTime = 0, fixType = 0;

	XMLElement* items = group->FirstChildElement("LcdFixation");
	if (items != nullptr)
	{
		for (child = items->FirstChild(); child != nullptr; child = child->NextSibling()) {
			if ((elem = child->ToElement()) != nullptr) {
				if (checkXMLResult(elem->QueryIntAttribute("row", &row)) &&
					checkXMLResult(elem->QueryIntAttribute("col", &col))) {
					const char* name = elem->Attribute("name");
					auto pos = std::pair<int, int>(row, col);
					if (!strcmp(name, "centerOD")) {
						pset->setCenter(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "fundusOD")) {
						pset->setFundus(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "opticDiskOD")) {
						pset->setOpticDisc(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "leftSideOD")) {
						pset->setLeftSide(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "leftTopOD")) {
						pset->setLeftTop(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "leftBottomOD")) {
						pset->setLeftBottom(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "rightSideOD")) {
						pset->setRightSide(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "rightTopOD")) {
						pset->setRightTop(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "rightBottomOD")) {
						pset->setRightBottom(EyeSide::OD, pos);
					}
					else if (!strcmp(name, "centerOS")) {
						pset->setCenter(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "fundusOS")) {
						pset->setFundus(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "opticDiskOS")) {
						pset->setOpticDisc(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "leftSideOS")) {
						pset->setLeftSide(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "leftTopOS")) {
						pset->setLeftTop(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "leftBottomOS")) {
						pset->setLeftBottom(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "rightSideOS")) {
						pset->setRightSide(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "rightTopOS")) {
						pset->setRightTop(EyeSide::OS, pos);
					}
					else if (!strcmp(name, "rightBottomOS")) {
						pset->setRightBottom(EyeSide::OS, pos);
					}
				}
				else
				{
					const char* name = elem->Attribute("name");
					if (name != nullptr)
					{
						if (!strcmp(name, "brightness")) {
							if (checkXMLResult(elem->QueryIntAttribute("value", &bright))) {
								pset->setBrightness(bright);
							}
						}
						else if (!strcmp(name, "blink")) {
							if (checkXMLResult(elem->QueryBoolAttribute("value", &flag))) {
								pset->useLcdBlinkOn(true, flag);
							}
							if (checkXMLResult(elem->QueryIntAttribute("period", &period))) {
								pset->setBlinkPeriod(period);
							}
							if (checkXMLResult(elem->QueryIntAttribute("onTime", &onTime))) {
								pset->setBlinkOnTime(onTime);
							}
							if (checkXMLResult(elem->QueryIntAttribute("type", &fixType))) {
								pset->setFixationType(fixType);
							}
						}
					}
					else {
						if (checkXMLResult(elem->QueryBoolAttribute("valid", &flag))) {
							pset->useLcdFixation(true, flag);
						}
					}
				}
			}
		}
	}

	return true;
}

bool wso_config::SysConfigFile::loadSectionOfCamera(CameraSetting* cset, tinyxml2::XMLElement* group)
{
	XMLNode* child;
	XMLElement* elem;
	XMLElement* items;
	string name;
	float fValue;
	int nValue;

	items = group->FirstChildElement("CorneaIr");
	if (items != nullptr)
	{
		child = items->FirstChild();
		if (child != nullptr && (elem = child->ToElement()) != nullptr) {
			if (checkXMLResult(elem->QueryFloatAttribute("analogGain", &fValue))) {
				cset->setCorneaAgain(fValue);
			}
			if (checkXMLResult(elem->QueryFloatAttribute("digitalGain", &fValue))) {
				cset->setCorneaDgain(fValue);
			}
		}
		else {
			return false;
		}

		child = child->NextSibling();
		if (child != nullptr && (elem = child->ToElement()) != nullptr) {
			if (checkXMLResult(elem->QueryIntAttribute("value1", &nValue))) {
				cset->setWdotIntensity(0, nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("value2", &nValue))) {
				cset->setWdotIntensity(1, nValue);
			}
		}
		else {
			return false;
		}
	}
	return true;
}

bool wso_config::SysConfigFile::loadSectionOfMeasure(MeasureSetting* mset, tinyxml2::XMLElement* group)
{
	XMLNode* child;
	XMLElement* elem;
	XMLElement* items;
	string name;

	int x, y, z;

	items = group->FirstChildElement("StagePosition");
	if (items != nullptr)
	{
		for (child = items->FirstChild(); child != nullptr; child = child->NextSibling()) {
			if ((elem = child->ToElement()) != nullptr) {
				if (checkXMLResult(elem->QueryIntAttribute("x", &x)) &&
					checkXMLResult(elem->QueryIntAttribute("y", &y)) && 
					checkXMLResult(elem->QueryIntAttribute("z", &z))) {
					const char* name = elem->Attribute("name");
					auto pos = std::tuple<int, int, int>(x, y, z);
					if (!strcmp(name, "center")) {
						mset->setCenterPos(pos);
					}
					else if (!strcmp(name, "readyOD")) {
						mset->setReadyPosOD(pos);
					}
					else if (!strcmp(name, "readyOS")) {
						mset->setReadyPosOS(pos);
					}
					else if (!strcmp(name, "readyME")) {
						mset->setReadyPosME(pos);
					}
				}
			}
		}
	}
	return true;
}

bool wso_config::SysConfigFile::loadSectionOfLsoCapture(LsoCaptureSetting* lcset, tinyxml2::XMLElement* group)
{
	XMLNode* child;
	XMLElement* elem;
	XMLElement* items;
	string name;

	int y1, y2, index;
	int offsetX, offsetY;
	int width, height;
	int count;
	int nValue;

	items = group->FirstChildElement("CaptureFrameROI");
	if (items != nullptr)
	{
		count = 0;
		for (child = items->FirstChild(); child != nullptr; child = child->NextSibling()) {
			if ((elem = child->ToElement()) != nullptr) {
				if (checkXMLResult(elem->QueryIntAttribute("startY", &y1)) &&
					checkXMLResult(elem->QueryIntAttribute("endY", &y2)) && 
					checkXMLResult(elem->QueryIntAttribute("index", &index))) {
					LsoCaptureFrameROI roi;
					roi.startY = y1;
					roi.endY = y2;
					lcset->setCaptureFrameROI(index, roi);
					count++;
				}
			}
		}
		lcset->setCaptureFrameCount(count);
	}

	items = group->FirstChildElement("SequencerFrameROI");
	if (items != nullptr)
	{
		count = 0;
		for (child = items->FirstChild(); child != nullptr; child = child->NextSibling()) {
			if ((elem = child->ToElement()) != nullptr) {
				if (checkXMLResult(elem->QueryIntAttribute("width", &width)) &&
					checkXMLResult(elem->QueryIntAttribute("height", &height)) &&
					checkXMLResult(elem->QueryIntAttribute("offsetX", &offsetX)) &&
					checkXMLResult(elem->QueryIntAttribute("offsetY", &offsetY)) &&
					checkXMLResult(elem->QueryIntAttribute("index", &index))) {
					LsoCaptureFrameROI roi;
					roi.width = width;
					roi.height = height;
					roi.offsetX = offsetX;
					roi.offsetY = offsetY;
					lcset->setSequencerFrameROI(index, roi);
					count++;
				}
			}
		}
		lcset->setSequencerFrameCount(count);
	}

	items = group->FirstChildElement("OffsetFrameROI");
	if (items != nullptr)
	{
		child = items->FirstChild();
		if (child != nullptr && (elem = child->ToElement()) != nullptr) {
			if (checkXMLResult(elem->QueryIntAttribute("roiWidth", &nValue))) {
				lcset->setOffsetRoiWidth(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("roiHeight", &nValue))) {
				lcset->setOffsetRoiHeight(nValue);
			}
		}

		count = 0;
		for (/*child = items->FirstChild()*/child = child->NextSibling(); child != nullptr; child = child->NextSibling()) {
			if ((elem = child->ToElement()) != nullptr) {
				if (checkXMLResult(elem->QueryIntAttribute("offsetX", &offsetX)) &&
					checkXMLResult(elem->QueryIntAttribute("offsetY", &offsetY)) &&
					checkXMLResult(elem->QueryIntAttribute("index", &index))) {
					LsoCaptureFrameROI roi;
					roi.offsetX = offsetX;
					roi.offsetY = offsetY;
					lcset->setOffsetFrameROI(index, roi);
					count++;
				}
			}
		}
		lcset->setOffsetFrameCount(count);
	}

	items = group->FirstChildElement("RollSwTriggerOverlap");
	if (items != nullptr)
	{
		child = items->FirstChild();
		if (child != nullptr && (elem = child->ToElement()) != nullptr) {
			if (checkXMLResult(elem->QueryIntAttribute("overlapCount", &nValue))) {
				lcset->setFrameRollSwTrigOverlapCount(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("roiWidth", &nValue))) {
				lcset->setFrameRollSwTrigOverlapRoiWidth(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("roiHeight", &nValue))) {
				lcset->setFrameRollSwTrigOverlapRoiHeight(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("offsetX", &nValue))) {
				lcset->setFrameRollSwTrigOverlapOffsetX(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("offsetY", &nValue))) {
				lcset->setFrameRollSwTrigOverlapOffsetY(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("triggerInterval", &nValue))) {
				lcset->setFrameRollSwTrigOverlapTriggerInterval(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("brightness", &nValue))) {
				lcset->setFrameRollSwTrigOverlapBrightness(nValue);
			}
		}
		else {
			return false;
		}
	}
	return true;
}

bool wso_config::SysConfigFile::loadSectionOfLsoDisplay(LsoDisplaySetting* ldset, tinyxml2::XMLElement* group)
{
	XMLNode* child;
	XMLElement* elem;
	XMLElement* items;
	string name;

	int nValue;

	items = group->FirstChildElement("ImageMask");
	if (items != nullptr)
	{
		child = items->FirstChild();
		if (child != nullptr && (elem = child->ToElement()) != nullptr) {
			if (checkXMLResult(elem->QueryIntAttribute("irRadius", &nValue))) {
				ldset->setMaskIrRadius(nValue);
			}
			if (checkXMLResult(elem->QueryIntAttribute("colorRadius", &nValue))) {
				ldset->setMaskColorRadius(nValue);
			}
		}
		else {
			return false;
		}
	}

	items = group->FirstChildElement("ImageAdjust");
	if (items != nullptr)
	{
		child = items->FirstChild();
		if (child != nullptr && (elem = child->ToElement()) != nullptr) {
			if (checkXMLResult(elem->QueryIntAttribute("brightness", &nValue))) {
				ldset->setAdjustBrightness(nValue);
			}
		}
		else {
			return false;
		}
	}
	return true;
}

bool wso_config::SysConfigFile::saveSectionOfFixation(const FixationSetting* pset, tinyxml2::XMLElement* group)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	string name;

	XMLElement* list = pdoc->NewElement("LcdFixation");
	group->LinkEndChild(list);

	XMLElement* item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "centerOD");
	item->SetAttribute("row", pset->getCenter(EyeSide::OD).first);
	item->SetAttribute("col", pset->getCenter(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "fundusOD");
	item->SetAttribute("row", pset->getFundus(EyeSide::OD).first);
	item->SetAttribute("col", pset->getFundus(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "opticDiskOD");
	item->SetAttribute("row", pset->getOpticDisc(EyeSide::OD).first);
	item->SetAttribute("col", pset->getOpticDisc(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "leftSideOD");
	item->SetAttribute("row", pset->getLeftSide(EyeSide::OD).first);
	item->SetAttribute("col", pset->getLeftSide(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "leftTopOD");
	item->SetAttribute("row", pset->getLeftTop(EyeSide::OD).first);
	item->SetAttribute("col", pset->getLeftTop(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "leftBottomOD");
	item->SetAttribute("row", pset->getLeftBottom(EyeSide::OD).first);
	item->SetAttribute("col", pset->getLeftBottom(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "rightSideOD");
	item->SetAttribute("row", pset->getRightSide(EyeSide::OD).first);
	item->SetAttribute("col", pset->getRightSide(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "rightTopOD");
	item->SetAttribute("row", pset->getRightTop(EyeSide::OD).first);
	item->SetAttribute("col", pset->getRightTop(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "rightBottomOD");
	item->SetAttribute("row", pset->getRightBottom(EyeSide::OD).first);
	item->SetAttribute("col", pset->getRightBottom(EyeSide::OD).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "centerOS");
	item->SetAttribute("row", pset->getCenter(EyeSide::OS).first);
	item->SetAttribute("col", pset->getCenter(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "fundusOS");
	item->SetAttribute("row", pset->getFundus(EyeSide::OS).first);
	item->SetAttribute("col", pset->getFundus(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "opticDiskOS");
	item->SetAttribute("row", pset->getOpticDisc(EyeSide::OS).first);
	item->SetAttribute("col", pset->getOpticDisc(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "leftSideOS");
	item->SetAttribute("row", pset->getLeftSide(EyeSide::OS).first);
	item->SetAttribute("col", pset->getLeftSide(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "leftTopOS");
	item->SetAttribute("row", pset->getLeftTop(EyeSide::OS).first);
	item->SetAttribute("col", pset->getLeftTop(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "leftBottomOS");
	item->SetAttribute("row", pset->getLeftBottom(EyeSide::OS).first);
	item->SetAttribute("col", pset->getLeftBottom(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "rightSideOS");
	item->SetAttribute("row", pset->getRightSide(EyeSide::OS).first);
	item->SetAttribute("col", pset->getRightSide(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "rightUpOS");
	item->SetAttribute("row", pset->getRightTop(EyeSide::OS).first);
	item->SetAttribute("col", pset->getRightTop(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "rightDownOS");
	item->SetAttribute("row", pset->getRightBottom(EyeSide::OS).first);
	item->SetAttribute("col", pset->getRightBottom(EyeSide::OS).second);

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "brightness");
	item->SetAttribute("value", pset->getBrightness());

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "blink");
	item->SetAttribute("value", pset->useLcdBlinkOn());
	item->SetAttribute("period", pset->getBlinkPeriod());
	item->SetAttribute("onTime", pset->getBlinkOnTime());
	item->SetAttribute("type", pset->getFixationType());

	item = pdoc->NewElement("status");
	list->LinkEndChild(item);
	item->SetAttribute("valid", pset->useLcdFixation());
	return true;
}


bool wso_config::SysConfigFile::saveSectionOfCamera(const CameraSetting* cset, tinyxml2::XMLElement* group)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	string name;

	XMLElement* list = pdoc->NewElement("CorneaIr");
	group->LinkEndChild(list);

	XMLElement* item = pdoc->NewElement("Preset");
	list->LinkEndChild(item);
	item->SetAttribute("analogGain", cset->getCorneaAgain());
	item->SetAttribute("digitalGain", cset->getCorneaDgain());

	item = pdoc->NewElement("WorkingDots");
	list->LinkEndChild(item);
	item->SetAttribute("value1", cset->getWdotIntensity(0));
	item->SetAttribute("value2", cset->getWdotIntensity(1));
	return true;
}

bool wso_config::SysConfigFile::saveSectionOfMeasure(const MeasureSetting* mset, tinyxml2::XMLElement* group)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	string name;

	XMLElement* list = pdoc->NewElement("StagePosition");
	group->LinkEndChild(list);

	XMLElement* item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "center");
	item->SetAttribute("x", std::get<0>(mset->getCenterPos()));
	item->SetAttribute("y", std::get<1>(mset->getCenterPos()));
	item->SetAttribute("z", std::get<2>(mset->getCenterPos()));

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "readyOD");
	item->SetAttribute("x", std::get<0>(mset->getReadyPosOD()));
	item->SetAttribute("y", std::get<1>(mset->getReadyPosOD()));
	item->SetAttribute("z", std::get<2>(mset->getReadyPosOD()));

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "readyOS");
	item->SetAttribute("x", std::get<0>(mset->getReadyPosOS()));
	item->SetAttribute("y", std::get<1>(mset->getReadyPosOS()));
	item->SetAttribute("z", std::get<2>(mset->getReadyPosOS()));

	item = pdoc->NewElement("attribute");
	list->LinkEndChild(item);
	item->SetAttribute("name", "readyME");
	item->SetAttribute("x", std::get<0>(mset->getReadyPosME()));
	item->SetAttribute("y", std::get<1>(mset->getReadyPosME()));
	item->SetAttribute("z", std::get<2>(mset->getReadyPosME()));
	return true;
}

bool wso_config::SysConfigFile::saveSectionOfLsoCapture(const LsoCaptureSetting* lcset, tinyxml2::XMLElement* group)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	string name;

	int count;

	XMLElement* list = pdoc->NewElement("CaptureFrameROI");
	group->LinkEndChild(list);

	count = lcset->getCaptureFrameCount();
	for (int i = 0; i < count; i++) {
		LsoCaptureFrameROI roi = lcset->getCaptureFrameROI(i);
		XMLElement* item = pdoc->NewElement("attribute");
		list->LinkEndChild(item);
		item->SetAttribute("index", i);
		item->SetAttribute("startY", roi.startY);
		item->SetAttribute("endY", roi.endY);
	}

	list = pdoc->NewElement("SequencerFrameROI");
	group->LinkEndChild(list);

	count = lcset->getSequencerFrameCount();
	for (int i = 0; i < count; i++) {
		LsoCaptureFrameROI roi = lcset->getSequencerFrameROI(i);
		XMLElement* item = pdoc->NewElement("attribute");
		list->LinkEndChild(item);
		item->SetAttribute("index", i);
		item->SetAttribute("width", roi.width);
		item->SetAttribute("height", roi.height);
		item->SetAttribute("offsetX", roi.offsetX);
		item->SetAttribute("offsetY", roi.offsetY);
	}

	list = pdoc->NewElement("OffsetFrameROI");
	group->LinkEndChild(list);

	XMLElement* item = pdoc->NewElement("attribute");
	group->LinkEndChild(item);
	item->SetAttribute("roiWidth", lcset->getOffsetRoiWidth());
	item->SetAttribute("roiHeight", lcset->getOffsetRoiHeight());

	count = lcset->getOffsetFrameCount();
	for (int i = 0; i < count; i++) {
		LsoCaptureFrameROI roi = lcset->getOffsetFrameROI(i);
		XMLElement* item = pdoc->NewElement("attribute");
		list->LinkEndChild(item);
		item->SetAttribute("index", i);
		item->SetAttribute("offsetX", roi.offsetX);
		item->SetAttribute("offsetY", roi.offsetY);
	}

	list = pdoc->NewElement("RollSwTriggerOverlap");
	group->LinkEndChild(list);

	item = pdoc->NewElement("attribute");
	group->LinkEndChild(item);
	item->SetAttribute("overlapCount", lcset->getFrameRollSwTrigOverlapCount());
	item->SetAttribute("roiWidth", lcset->getFrameRollSwTrigOverlapRoiWidth());
	item->SetAttribute("roiHeight", lcset->getFrameRollSwTrigOverlapRoiHeight());
	item->SetAttribute("offsetX", lcset->getFrameRollSwTrigOverlapOffsetX());
	item->SetAttribute("offsetY", lcset->getFrameRollSwTrigOverlapOffsetY());
	item->SetAttribute("triggerInterval", lcset->getFrameRollSwTrigOverlapTriggerInterval());
	item->SetAttribute("brightness", lcset->getFrameRollSwTrigOverlapBrightness());
	return true;
}

bool wso_config::SysConfigFile::saveSectionOfLsoDisplay(const LsoDisplaySetting* ldset, tinyxml2::XMLElement* group)
{
	tinyxml2::XMLDocument* pdoc = getImpl().pDoc;
	string name;

	XMLElement* item = pdoc->NewElement("ImageMask");
	group->LinkEndChild(item);
	item->SetAttribute("irRadius", ldset->getMaskIrRadius());
	item->SetAttribute("colorRadius", ldset->getMaskColorRadius());

	item = pdoc->NewElement("ImageAdjust");
	group->LinkEndChild(item);
	item->SetAttribute("brightness", ldset->getAdjustBrightness());
	return true;
}


wso_config::SysConfigFile::SysConfigFileImpl& wso_config::SysConfigFile::getImpl(void)
{
	return *d_ptr;
}


bool wso_config::SysConfigFile::checkXMLResult(int result)
{
	return (result == XML_SUCCESS);
}