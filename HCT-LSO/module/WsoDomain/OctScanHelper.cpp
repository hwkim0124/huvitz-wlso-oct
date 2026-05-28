#include "pch.h"
#include "OctScanHelper.h"
#include "OctScanDefines.h"
#include "OctScanStructs.h"


using namespace wso_domain;


std::mutex OctScanHelper::singleMutex_;


struct OctScanHelper::OctScanHelperImpl
{
	std::map<OctPatternCode, OctPatternDescript> patternDescriptMap;
	std::map<OctPatternCode, std::string> patternNameMap;
	std::vector<OctPatternCode> patternCodeList;

	OctScanHelperImpl() {
		initializeOctScanHelperImpl();
	}

	void initializeOctScanHelperImpl(void) {
		patternNameMap = {
			{OctPatternCode::MACULAR_POINT, "Macular Point"},
			{OctPatternCode::MACULAR_LINE, "Macular Line"},
			{OctPatternCode::MACULAR_CROSS, "Macular Cross"},
			{OctPatternCode::MACULAR_RASTER, "Macular Raster"},
			{OctPatternCode::MACULAR_RADIAL, "Macular Radial"},
			{OctPatternCode::MACULAR_CUBE, "Macular Cube"},
			{OctPatternCode::MACULAR_ANGIO, "Macular Angio"},
			{OctPatternCode::DISC_POINT, "Disc Point"},
			{OctPatternCode::DISC_RASTER, "Disc Raster"},
			{OctPatternCode::DISC_RADIAL, "Disc Radial"},
			{OctPatternCode::DISC_CIRCLE, "Disc Circle"},
			{OctPatternCode::DISC_CUBE, "Disc Cube"},
			{OctPatternCode::DISC_ANGIO, "Disc Angio"},
			{OctPatternCode::FUNDUS_LINE, "Fundus Line"},
			{OctPatternCode::FUNDUS_RASTER, "Fundus Raster"},
			{OctPatternCode::FUNDUS_CUBE, "Fundus Cube"},
			{OctPatternCode::FUNDUS_ANGIO, "Fundus Angio"},
		};

		patternCodeList = {
			OctPatternCode::MACULAR_POINT,
			OctPatternCode::MACULAR_LINE,
			OctPatternCode::MACULAR_CROSS,
			OctPatternCode::MACULAR_RASTER,
			OctPatternCode::MACULAR_RADIAL,
			OctPatternCode::MACULAR_CUBE,
			OctPatternCode::MACULAR_ANGIO,
			OctPatternCode::DISC_POINT,
			OctPatternCode::DISC_RASTER,
			OctPatternCode::DISC_RADIAL,
			OctPatternCode::DISC_CIRCLE,
			OctPatternCode::DISC_CUBE,
			OctPatternCode::DISC_ANGIO
		};

		patternDescriptMap = {
			{OctPatternCode::MACULAR_POINT, { OctPatternCode::MACULAR_POINT, OctPatternType::POINT, EyeRegion::MACULAR, "Macular Point"}},
			{OctPatternCode::MACULAR_LINE, { OctPatternCode::MACULAR_LINE, OctPatternType::LINE, EyeRegion::MACULAR, "Macular Line"}},
			{OctPatternCode::MACULAR_CROSS, { OctPatternCode::MACULAR_CROSS, OctPatternType::CROSS, EyeRegion::MACULAR, "Macular Cross"}},
			{OctPatternCode::MACULAR_RASTER, { OctPatternCode::MACULAR_RASTER, OctPatternType::RASTER, EyeRegion::MACULAR, "Macular Raster"}},
			{OctPatternCode::MACULAR_RADIAL, { OctPatternCode::MACULAR_RADIAL, OctPatternType::RADIAL, EyeRegion::MACULAR, "Macular Radial"}},
			{OctPatternCode::MACULAR_CUBE, { OctPatternCode::MACULAR_CUBE, OctPatternType::CUBE, EyeRegion::MACULAR, "Macular Cube"}},
			{OctPatternCode::MACULAR_ANGIO, { OctPatternCode::MACULAR_ANGIO, OctPatternType::ANGIO, EyeRegion::MACULAR, "Macular Angio"}},
			{OctPatternCode::DISC_POINT, { OctPatternCode::DISC_POINT, OctPatternType::POINT, EyeRegion::OPTIC_DISC, "Disc Point"}},
			{OctPatternCode::DISC_RASTER, { OctPatternCode::DISC_RASTER, OctPatternType::RASTER, EyeRegion::OPTIC_DISC, "Disc Raster"}},
			{OctPatternCode::DISC_RADIAL, { OctPatternCode::DISC_RADIAL, OctPatternType::RADIAL, EyeRegion::OPTIC_DISC, "Disc Radial"}},
			{OctPatternCode::DISC_CIRCLE, { OctPatternCode::DISC_CIRCLE, OctPatternType::CIRCLE, EyeRegion::OPTIC_DISC, "Disc Circle"}},
			{OctPatternCode::DISC_CUBE, { OctPatternCode::DISC_CUBE, OctPatternType::CUBE, EyeRegion::OPTIC_DISC, "Disc Cube"}},
			{OctPatternCode::DISC_ANGIO, { OctPatternCode::DISC_ANGIO, OctPatternType::ANGIO, EyeRegion::OPTIC_DISC, "Disc Angio"}},
			/*
			{OctPatternCode::FUNDUS_LINE, { OctPatternCode::FUNDUS_LINE, OctPatternType::LINE, EyeRegion::OPTIC_DISC, "Fundus Line"}},
			{OctPatternCode::FUNDUS_RASTER, { OctPatternCode::FUNDUS_RASTER, OctPatternType::RASTER, EyeRegion::OPTIC_DISC, "Fundus Raster"}},
			{OctPatternCode::FUNDUS_CUBE, { OctPatternCode::FUNDUS_CUBE, OctPatternType::CUBE, EyeRegion::OPTIC_DISC, "Fundus Cube"}},
			{OctPatternCode::FUNDUS_ANGIO, { OctPatternCode::FUNDUS_ANGIO, OctPatternType::ANGIO, EyeRegion::OPTIC_DISC, "Fundus Angio"}},
			*/
			{OctPatternCode::CORNEA_POINT, { OctPatternCode::CORNEA_POINT, OctPatternType::POINT, EyeRegion::CORNEA, "Anterior Point"}},
			{OctPatternCode::CORNEA_LINE, { OctPatternCode::CORNEA_LINE, OctPatternType::LINE, EyeRegion::CORNEA, "Anterior Line"}},
			{OctPatternCode::CORNEA_RADIAL, { OctPatternCode::CORNEA_RADIAL, OctPatternType::RADIAL, EyeRegion::CORNEA,"Anterior Radial"}},
			{OctPatternCode::CORNEA_CUBE, { OctPatternCode::CORNEA_CUBE, OctPatternType::CUBE, EyeRegion::CORNEA,"Anterior 3D" }}
		};
	}
};


wso_domain::OctScanHelper::OctScanHelper() :
	d_ptr(std::make_unique<OctScanHelperImpl>())
{
}


wso_domain::OctScanHelper::~OctScanHelper()
{
}


OctScanHelper* wso_domain::OctScanHelper::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static OctScanHelper instance;
	return &instance;
}

int wso_domain::OctScanHelper::getScanPatternCount(bool not_point)
{
int count = 0;
	for (const auto& pattern : impl().patternDescriptMap) {
		if (not_point && pattern.second.patternType == OctPatternType::POINT) {
			continue;
		}
		count++;
	}
	return count;
}

int wso_domain::OctScanHelper::getScanPatternList(vector<OctPatternDescript>& list, bool not_point)
{
	list.clear();
	for (const auto& pattern : impl().patternDescriptMap) {
		if (not_point && pattern.second.patternType == OctPatternType::POINT) {
			continue;
		}
		list.push_back(pattern.second);
	}
	return (int)list.size();
}

int wso_domain::OctScanHelper::getScanPatternList(vector<OctPatternDescript>& list, EyeRegion region, bool not_point)
{
	list.clear();
	for (const auto& pattern : impl().patternDescriptMap) {
		if (pattern.second.eyeRegion == region) {
			if (not_point && pattern.second.patternType == OctPatternType::POINT) {
				continue;
			}
			list.push_back(pattern.second);
		}
	}
	return (int)list.size();
}

std::string wso_domain::OctScanHelper::getPatternName(OctPatternCode code) 
{
	switch (code) {
	case OctPatternCode::CALIBRATION_POINT:
		return PATTERN_CALIBRATION_POINT_NAME;
	case OctPatternCode::MACULAR_POINT:
		return PATTERN_MACULAR_POINT_NAME;
	case OctPatternCode::MACULAR_LINE:
		return PATTERN_MACULAR_LINE_NAME;
	case OctPatternCode::MACULAR_CROSS:
		return PATTERN_MACULAR_CROSS_NAME;
	case OctPatternCode::MACULAR_RASTER:
		return PATTERN_MACULAR_RASTER_NAME;
	case OctPatternCode::MACULAR_RADIAL:
		return PATTERN_MACULAR_RADIAL_NAME;
	case OctPatternCode::MACULAR_CUBE:
		return PATTERN_MACULAR_CUBE_NAME;
	case OctPatternCode::MACULAR_ANGIO:
		return PATTERN_MACULAR_ANGIO_NAME;
	case OctPatternCode::DISC_POINT:
		return PATTERN_DISC_POINT_NAME;
	case OctPatternCode::DISC_RASTER:
		return PATTERN_DISC_RASTER_NAME;
	case OctPatternCode::DISC_RADIAL:
		return PATTERN_DISC_RADIAL_NAME;
	case OctPatternCode::DISC_CIRCLE:
		return PATTERN_DISC_CIRCLE_NAME;
	case OctPatternCode::DISC_CUBE:
		return PATTERN_DISC_CUBE_NAME;
	case OctPatternCode::DISC_ANGIO:
		return PATTERN_DISC_ANGIO_NAME;
	case OctPatternCode::FUNDUS_LINE:
		return PATTERN_FUNDUS_LINE_NAME;
	case OctPatternCode::FUNDUS_RASTER:
		return PATTERN_FUNDUS_RASTER_NAME;
	case OctPatternCode::FUNDUS_CUBE:
		return PATTERN_FUNDUS_CUBE_NAME;
	case OctPatternCode::FUNDUS_ANGIO:
		return PATTERN_FUNDUS_ANGIO_NAME;
	case OctPatternCode::CORNEA_POINT:
		return PATTERN_CORNEA_POINT_NAME;
	case OctPatternCode::CORNEA_LINE:
		return PATTERN_CORNEA_LINE_NAME;
	case OctPatternCode::CORNEA_RADIAL:
		return PATTERN_CORNEA_RADIAL_NAME;
	case OctPatternCode::CORNEA_CUBE:
		return PATTERN_CORNEA_CUBE_NAME;
	}
	return std::string();
}

EyeRegion wso_domain::OctScanHelper::getPatternRegion(OctPatternCode code)
{
	int data = (int)code / 1000;
	return (EyeRegion)data;
}

OctPatternType wso_domain::OctScanHelper::getPatternType(OctPatternCode code)
{
	int data = (int)code % 100;
	return (OctPatternType)data;
}

OctPatternType wso_domain::OctScanHelper::getPatternType(OctPatternCode code, OctScanDirection direction)
{
	auto type = getPatternType(code);
	
	if (direction == OctScanDirection::Y_TO_X) {
		switch (type) {
		case OctPatternType::LINE:
			return OctPatternType::VERT_LINE;
		case OctPatternType::RASTER:
			return OctPatternType::VERT_RASTER;
		case OctPatternType::CUBE:
			return OctPatternType::VERT_CUBE;
		case OctPatternType::ANGIO:
			return OctPatternType::VERT_ANGIO;
		}
	}
	else {
		switch (type) {
		case OctPatternType::LINE:
			return OctPatternType::LINE;
		case OctPatternType::RASTER:
			return OctPatternType::RASTER;
		case OctPatternType::CUBE:
			return OctPatternType::CUBE;
		case OctPatternType::ANGIO:
			return OctPatternType::ANGIO;
		}
	}
	return type;
}

OctPatternType wso_domain::OctScanHelper::getPreviewType(OctPatternCode code, OctScanDirection direction)
{
	auto type = getPatternType(code);
	if (direction == OctScanDirection::Y_TO_X) {
		return OctPatternType::VERT_LINE;
	}
	else {
		return OctPatternType::LINE;
	}
}

OctPatternType wso_domain::OctScanHelper::getPreviewType(OctPatternType type, OctScanDirection direction)
{
	if (direction == OctScanDirection::Y_TO_X) {
		switch (type) {
			case OctPatternType::LINE:
				return OctPatternType::VERT_LINE;
			case OctPatternType::VERT_LINE:
				return OctPatternType::LINE;
			default:
				return type;
		}
	}
	else {
		switch (type) {
			case OctPatternType::LINE:
				return OctPatternType::LINE;
			case OctPatternType::VERT_LINE:
				return OctPatternType::VERT_LINE;
			default:
				return type;
		}
	}
	return type;
}

int wso_domain::OctScanHelper::getPatternAscanList(vector<int>& list, OctPatternCode code)
{
	list.clear();
	switch (code) {
	case OctPatternCode::MACULAR_POINT:
		case OctPatternCode::DISC_POINT:
		case OctPatternCode::CORNEA_POINT:
		case OctPatternCode::CALIBRATION_POINT:
			list = { 1024, 512, 256 };
			break;
		case OctPatternCode::MACULAR_LINE:
		case OctPatternCode::FUNDUS_LINE:
		case OctPatternCode::DISC_CIRCLE:
		case OctPatternCode::CORNEA_LINE:
			list = { 1024, 512 };
			break;
		case OctPatternCode::MACULAR_CROSS:
		case OctPatternCode::MACULAR_RASTER:
		case OctPatternCode::DISC_RASTER:
		case OctPatternCode::FUNDUS_RASTER:
		case OctPatternCode::MACULAR_RADIAL:
		case OctPatternCode::DISC_RADIAL:
		case OctPatternCode::CORNEA_RADIAL:
			list = { 1024, 512 };
			break;
		case OctPatternCode::MACULAR_CUBE:
		case OctPatternCode::DISC_CUBE:
		case OctPatternCode::FUNDUS_CUBE:
		case OctPatternCode::CORNEA_CUBE:
			list = { 512, 384, 256 };
			break;
		case OctPatternCode::MACULAR_ANGIO:
		case OctPatternCode::DISC_ANGIO:
		case OctPatternCode::FUNDUS_ANGIO:
			list = { 512, 384, 256 };
			break;
	}
	return (int)list.size();
}

int wso_domain::OctScanHelper::getPatternBscanList(vector<int>& list, OctPatternCode code, int ascans)
{
	list.clear();
	switch (code) {
		case OctPatternCode::MACULAR_POINT:
		case OctPatternCode::DISC_POINT:
		case OctPatternCode::CORNEA_POINT:
		case OctPatternCode::CALIBRATION_POINT:
			list = { 1 };
			break;
		case OctPatternCode::MACULAR_LINE:
		case OctPatternCode::FUNDUS_LINE:
		case OctPatternCode::DISC_CIRCLE:
		case OctPatternCode::CORNEA_LINE:
			list = { 1 };
			break;
		case OctPatternCode::MACULAR_CROSS:
			list = { 10 };
			break;
		case OctPatternCode::MACULAR_RADIAL:
		case OctPatternCode::DISC_RADIAL:
		case OctPatternCode::CORNEA_RADIAL:
			list = { 24, 18, 12 };
			break;
		case OctPatternCode::MACULAR_RASTER:
		case OctPatternCode::DISC_RASTER:
		case OctPatternCode::FUNDUS_RASTER:
			list = { 25 };
			break;
		case OctPatternCode::MACULAR_CUBE:
		case OctPatternCode::DISC_CUBE:
		case OctPatternCode::FUNDUS_CUBE:
		case OctPatternCode::CORNEA_CUBE:
			if (ascans >= 512) {
				list = { 512, 256, 128, 96 };
			}
			else if (ascans >= 384) {
				list = { 384, 256, 128 };
			}
			else {
				list = { 256, 128 };
			}
			break;
		case OctPatternCode::MACULAR_ANGIO:
		case OctPatternCode::DISC_ANGIO:
		case OctPatternCode::FUNDUS_ANGIO:
			if (ascans >= 512) {
				list = { 512, 256, 128, 96 };
			}
			else if (ascans >= 384) {
				list = { 384, 256, 128 };
			}
			else {
				list = { 256, 128 };
			}
			break;
	}
	return (int)list.size();
}

int wso_domain::OctScanHelper::getPatternOverlapList(vector<int>& list, OctPatternCode code, int ascans, int bscans)
{
	list.clear();
	switch (code) {
	case OctPatternCode::MACULAR_POINT:
		case OctPatternCode::DISC_POINT:
		case OctPatternCode::MACULAR_LINE:
		case OctPatternCode::DISC_CIRCLE:
		case OctPatternCode::FUNDUS_LINE:
		case OctPatternCode::CORNEA_POINT:
		case OctPatternCode::CORNEA_LINE:
		case OctPatternCode::CALIBRATION_POINT:
			if (ascans > 512) {
				list = { 1, 5, 10, 15 };
			}
			else {
				list = { 1, 15, 30, 45 };
			}
			break;
		case OctPatternCode::MACULAR_CROSS:
		case OctPatternCode::MACULAR_RADIAL:
		case OctPatternCode::DISC_RADIAL:
		case OctPatternCode::CORNEA_RADIAL:
			if (ascans > 512) {
				list = { 1, 5, 10 };
			}
			else {
				list = { 1, 5, 10, 15 };
			}
			break;
		case OctPatternCode::MACULAR_RASTER:
		case OctPatternCode::DISC_RASTER:
		case OctPatternCode::FUNDUS_RASTER:
			if (ascans > 512) {
				list = { 1, 5 };
			}
			else {
				list = { 1, 5, 10 };
			}
			break;
		case OctPatternCode::MACULAR_CUBE:
		case OctPatternCode::DISC_CUBE:
		case OctPatternCode::FUNDUS_CUBE:
		case OctPatternCode::CORNEA_CUBE:
			list = { 1 };
			break;
		case OctPatternCode::MACULAR_ANGIO:
		case OctPatternCode::DISC_ANGIO:
		case OctPatternCode::FUNDUS_ANGIO:
			list = { 2, 3 };
			break;
	}
	return (int)list.size();
}

int wso_domain::OctScanHelper::getPatternRangeList(vector<float>& xlist, vector<float>& ylist, OctPatternCode code)
{
	xlist.clear();
	ylist.clear();

	switch (code) {
		case OctPatternCode::MACULAR_LINE:
			xlist = { 4.5f, 6.0f, 9.0f, 12.0f, 16.5f };
			ylist = { 4.5f, 6.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::CORNEA_LINE:
			xlist = { 4.5f, 6.0f, 8.0f, 9.0f, 12.0f, 16.5f };
			ylist = { 4.5f, 6.0f, 8.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::FUNDUS_LINE:
			xlist = { 9.0f, 13.0f };
			break;
		case OctPatternCode::DISC_CIRCLE:
			xlist = { 3.25f };
			ylist = { 3.25f };
			break;
		case OctPatternCode::MACULAR_CROSS:
			xlist = { 4.5f, 6.0f, 9.0f, 12.0f };
			ylist = { 4.5f, 6.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::MACULAR_RADIAL:
		case OctPatternCode::DISC_RADIAL:
			xlist = { 3.0f, 4.5f, 6.0f, 9.0f, 12.0f };
			ylist = { 3.0f, 4.5f, 6.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::CORNEA_RADIAL:
			xlist = { 3.0f, 4.5f, 6.0f, 8.0f, 9.0f, 12.0f };
			ylist = { 3.0f, 4.5f, 6.0f, 8.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::MACULAR_RASTER:
		case OctPatternCode::DISC_RASTER:
		case OctPatternCode::FUNDUS_RASTER:
			xlist = { 4.5f, 6.0f, 9.0f, 12.0f };
			ylist = { 4.5f, 6.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::MACULAR_CUBE:
		case OctPatternCode::DISC_CUBE:
		case OctPatternCode::FUNDUS_CUBE:
		case OctPatternCode::CORNEA_CUBE:
			xlist = { 3.0f, 4.5f, 6.0f, 9.0f, 12.0f, 16.5f };
			ylist = { 3.0f, 4.5f, 6.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::MACULAR_ANGIO:
		case OctPatternCode::DISC_ANGIO:
			xlist = { 3.0f, 4.5f, 6.0f, 9.0f, 12.0f, 16.5f };
			ylist = { 3.0f, 4.5f, 6.0f, 9.0f, 12.0f };
			break;
		case OctPatternCode::FUNDUS_ANGIO:
			xlist = { 6.0f, 9.0f, 13.0f };
			ylist = { 6.0f, 9.0f, 9.0f };
			break;
	}
	return (int)xlist.size();
}


OctScanHelper::OctScanHelperImpl& wso_domain::OctScanHelper::impl(void) const
{
	return *d_ptr;
}
