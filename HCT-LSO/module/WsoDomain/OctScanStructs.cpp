#include "pch.h"
#include "OctScanStructs.h"

#include <json\json.hpp>
using json = nlohmann::json;

using namespace wso_domain;


std::string wso_domain::OctScanOffset::serializeToJson(void) const
{
    json j;
    j["offset_x"] = _offsetX;
    j["offset_y"] = _offsetY;
    j["angle"] = _angle;
    j["scale_x"] = _scaleX;
    j["scale_y"] = _scaleY;
    j["mm_as_unit"] = _mmAsUnit;

    auto jstr = j.dump(4);
    return jstr;
}

bool wso_domain::OctScanOffset::populateFromJson(const std::string& json)
{
    auto j = json::parse(json);
	_offsetX = j["offset_x"];
	_offsetY = j["offset_y"];
	_angle = j["angle"];
	_scaleX = j["scale_x"];
	_scaleY = j["scale_y"];
	_mmAsUnit = j["mm_as_unit"];
    return true;
}
