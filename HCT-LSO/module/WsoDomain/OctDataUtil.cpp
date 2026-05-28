#include "pch.h"
#include "OctDataUtil.h"

#include <json\json.hpp>
using json = nlohmann::ordered_json;

using namespace wso_domain;



std::string wso_domain::OctDataUtil::serializeToJson(const OctScanOffset& offset)
{
    json j;
    j["offset_x"] = offset._offsetX;
    j["offset_y"] = offset._offsetY;
    j["angle"] = offset._angle;
    j["scale_x"] = offset._scaleX;
    j["scale_y"] = offset._scaleY;
    j["mm_as_unit"] = offset._mmAsUnit;

    auto jstr = j.dump(4);
    return jstr;
}

std::string wso_domain::OctDataUtil::serializeToJson(const OctScanPattern& pattern)
{
    json j;
    j["pattern_code"] = pattern.patternCode;
    j["pattern_type"] = pattern.patternType;
    j["preview_type"] = pattern.previewType;
    j["eye_region"] = pattern.eyeRegion;

    j["range_x"] = pattern.rangeX;
    j["range_y"] = pattern.rangeY;
    j["num_ascan"] = pattern.numAscan;
    j["num_bscan"] = pattern.numBscan;
    j["overlaps"] = pattern.overlaps;
    j["line_space"] = pattern.lineSpace;

    j["scan_direction"] = pattern.scanDirection;
    j["scan_speed"] = pattern.scanSpeed;
    j["scan_offset"] = json::parse(serializeToJson(pattern.scanOffset));

    auto jstr = j.dump(4);
    return jstr;
}

std::string wso_domain::OctDataUtil::serializeToJson(const OctScanProtocol& proto)
{
    json j;
	j["eye_side"] = proto.eyeSide;
	j["diopter"] = proto.diopter;

	j["preview"] = json::parse(serializeToJson(proto.preview));
	j["measure"] = json::parse(serializeToJson(proto.measure));
	j["enface"] = json::parse(serializeToJson(proto.enface));

	j["initial_scan"] = proto.isInitialScan;
	j["phasing_enface"] = proto.isPhasingEnface;
	j["not_focus_align"] = proto.isNotFocusAlign;
	j["not_image_grabbing"] = proto.isNotImageGrabbing;
	j["fast_raster_scan"] = proto.isFastRasterScan;
	j["preview_pattern"] = proto.isPreviewPattern;

	auto jstr = j.dump(4);
	return jstr;
}

bool wso_domain::OctDataUtil::populateFromJson(const std::string& jstr, OctScanOffset& offset)
{
    auto j = json::parse(jstr);
    offset._offsetX = j["offset_x"];
    offset._offsetY = j["offset_y"];
    offset._angle = j["angle"];
    offset._scaleX = j["scale_x"];
    offset._scaleY = j["scale_y"];
    offset._mmAsUnit = j["mm_as_unit"];
    return true;
}

bool wso_domain::OctDataUtil::populateFromJson(const std::string& jstr, OctScanPattern& pattern)
{
    auto j = json::parse(jstr);
    pattern.patternCode = j["pattern_code"];
    pattern.patternType = j["pattern_type"];
    pattern.previewType = j["preview_type"];
    pattern.eyeRegion = j["eye_region"];

    pattern.rangeX = j["range_x"];
    pattern.rangeY = j["range_y"];
    pattern.numAscan = j["num_ascan"];
    pattern.numBscan = j["num_bscan"];
    pattern.overlaps = j["overlaps"];
    pattern.lineSpace = j["line_space"];

    pattern.scanDirection = j["scan_direction"];
    pattern.scanSpeed = j["scan_speed"];
    populateFromJson(j["scan_offset"].dump(), pattern.scanOffset);
    return true;
}

bool wso_domain::OctDataUtil::populateFromJson(const std::string& jstr, OctScanProtocol& proto)
{
	auto j = json::parse(jstr);
	proto.eyeSide = j["eye_side"];
	proto.diopter = j["diopter"];

	populateFromJson(j["preview"].dump(), proto.preview);
	populateFromJson(j["measure"].dump(), proto.measure);
	populateFromJson(j["enface"].dump(), proto.enface);

	proto.isInitialScan = j["initial_scan"];
	proto.isPhasingEnface = j["phasing_enface"];
	proto.isNotFocusAlign = j["not_focus_align"];
	proto.isNotImageGrabbing = j["not_image_grabbing"];
	proto.isFastRasterScan = j["fast_raster_scan"];
	proto.isPreviewPattern = j["preview_pattern"];
    return true;
}

bool wso_domain::OctDataUtil::saveScanProtocolSnapshot(const std::wstring& path, const OctScanProtocol& proto)
{
    try {
        auto json = serializeToJson(proto);
        if (!json.empty()) {
            auto jpath = wtoa(path);
            ofstream file(jpath);
            if (file.is_open()) {
                file.write(json.c_str(), json.size());
                file.close();
                return true;
            }
        }
    }
    catch (exception e) {
        LogE() << "Failed to save scan protocol snapshot: " << e.what();
    }
	return false;
}

bool wso_domain::OctDataUtil::loadScanProtocolSnapshot(const std::wstring& path, OctScanProtocol& proto)
{
    try {
        auto jpath = wtoa(path);
        ifstream file(jpath);
        if (file.is_open()) {
            string jstr((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            file.close();
            auto result = populateFromJson(jstr, proto);
            return result;
        }
    } 
    catch (exception e) {
		LogE() << "Failed to load scan protocol snapshot: " << e.what();
    }
    return false;
}

OctProtocolDescript wso_domain::OctDataUtil::convertToProtocolDescript(const OctScanProtocol& proto)
{
    auto desc = OctProtocolDescript();

    desc.eyeSide = proto.eyeSide;   
    desc.eyeRegion = proto.measure.eyeRegion;
    desc.diopter = proto.diopter;

    desc.patternCode = proto.measure.patternCode;
    desc.patternType = proto.measure.patternType;
    desc.previewType = proto.measure.previewType;

    desc.scanPoints = proto.measure.numAscan;
    desc.scanLines = proto.measure.numBscan;
    desc.scanOverlaps = proto.measure.overlaps;

    desc.scanRangeX = proto.measure.rangeX;
    desc.scanRangeY = proto.measure.rangeY;

    desc.scanDirection = proto.measure.scanDirection;
    desc.scanSpeed = proto.measure.scanSpeed;

    auto name = OctScanHelper::getPatternName(desc.patternCode);
    strcpy_s(desc.protocolName, name.c_str());
    return desc;
}
