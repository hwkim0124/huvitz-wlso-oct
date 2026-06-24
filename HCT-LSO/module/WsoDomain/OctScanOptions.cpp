#include "pch.h"
#include "OctScanOptions.h"
#include "OctScanDefines.h"

using namespace wso_domain;


std::mutex OctScanOptions::singleMutex_;


struct OctScanOptions::OctScanOptionsImpl
{
	bool optDebugOut = false;
	bool optEnfaceImageEnhance = true; 
	bool optOclTasksDefaultPreset = false;

	int enfacePreviewSlabY1 = 0;
	int enfacePreviewSlabY2 = 767;

	float corneaPatternRangeScaleX[3][5];
	float corneaPatternRangeScaleY[3][5];
	float retinaPatternRangeScaleX[3][5];
	float retinaPatternRangeScaleY[3][5];
	float corneaPatternRangeOffsetX[3];
	float corneaPatternRangeOffsetY[3];
	float retinaPatternRangeOffsetX[3];
	float retinaPatternRangeOffsetY[3];

	OctScanOptionsImpl()
	{
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 5; j++) {
				corneaPatternRangeScaleX[i][j] = 1.0f;
				corneaPatternRangeScaleY[i][j] = 1.0f;
				retinaPatternRangeScaleX[i][j] = 1.0f;
				retinaPatternRangeScaleY[i][j] = 1.0f;
			}
			corneaPatternRangeOffsetX[i] = 0.0f;
			corneaPatternRangeOffsetY[i] = 0.0f;
			retinaPatternRangeOffsetX[i] = 0.0f;
			retinaPatternRangeOffsetY[i] = 0.0f;
		} 
	}
};


wso_domain::OctScanOptions::OctScanOptions() :
	d_ptr(std::make_unique<OctScanOptionsImpl>())
{
}


wso_domain::OctScanOptions::~OctScanOptions()
{
}


OctScanOptions* wso_domain::OctScanOptions::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static OctScanOptions instance;
	return &instance;
}

bool wso_domain::OctScanOptions::isDebugOutEnabled(optional<bool> flag) const
{
	if (flag.has_value()) {
		impl().optDebugOut = flag.value();
	}
	return impl().optDebugOut;
}

bool wso_domain::OctScanOptions::isEnfaceImageEnhanceEnabled(optional<bool> flag) const
{
	if (flag.has_value()) {
		impl().optEnfaceImageEnhance = flag.value();
	}
	return impl().optEnfaceImageEnhance;
}

bool wso_domain::OctScanOptions::isOclTasksDefaultPresetEnabled(optional<bool> flag) const
{
	if (flag.has_value()) {
		impl().optOclTasksDefaultPreset = flag.value();
	}
	return impl().optOclTasksDefaultPreset;
}

std::tuple<int, int> wso_domain::OctScanOptions::getEnfacePreviewSlabRange(void) const
{
	return std::tuple<int, int>(impl().enfacePreviewSlabY1, impl().enfacePreviewSlabY2);
}

float wso_domain::OctScanOptions::getCorneaPatternRangeScaleX(int speed, int index) const
{
	return impl().corneaPatternRangeScaleX[speed][index];
}

float wso_domain::OctScanOptions::getCorneaPatternRangeScaleY(int speed, int index) const
{
	return impl().corneaPatternRangeScaleY[speed][index];
}

float wso_domain::OctScanOptions::getRetinaPatternRangeScaleX(int speed, int index) const
{
	return impl().retinaPatternRangeScaleX[speed][index];
}

float wso_domain::OctScanOptions::getRetinaPatternRangeScaleY(int speed, int index) const
{
	return impl().retinaPatternRangeScaleY[speed][index];
}

std::tuple<float, float> wso_domain::OctScanOptions::getRetinaPatternRangeOffset(int speed) const
{
	return std::tuple<float, float>(impl().retinaPatternRangeOffsetX[speed], impl().retinaPatternRangeOffsetY[speed]);
}

std::tuple<float, float> wso_domain::OctScanOptions::getCorneaPatternRangeOffset(int speed) const
{
	return std::tuple<float, float>(impl().corneaPatternRangeOffsetX[speed], impl().corneaPatternRangeOffsetY[speed]);
}

void wso_domain::OctScanOptions::setEnfacePreviewSlabRange(int y_start, int y_end)
{
	impl().enfacePreviewSlabY1 = y_start;
	impl().enfacePreviewSlabY2 = y_end;
}

void wso_domain::OctScanOptions::setCorneaPatternRangeScale(int speed, int index, float x_scale, float y_scale)
{
	impl().corneaPatternRangeScaleX[speed][index] = x_scale;
	impl().corneaPatternRangeScaleY[speed][index] = y_scale;
}

void wso_domain::OctScanOptions::setRetinaPatternRangeScale(int speed, int index, float x_scale, float y_scale)
{
	impl().retinaPatternRangeScaleX[speed][index] = x_scale;
	impl().retinaPatternRangeScaleY[speed][index] = y_scale;
}

void wso_domain::OctScanOptions::setCorneaPatternRangeOffset(int speed, float x_offset, float y_offset)
{
	impl().corneaPatternRangeOffsetX[speed] = x_offset;
	impl().corneaPatternRangeOffsetY[speed] = y_offset;
}

void wso_domain::OctScanOptions::setRetinaPatternRangeOffset(int speed, float x_offset, float y_offset)
{
	impl().retinaPatternRangeOffsetX[speed] = x_offset;
	impl().retinaPatternRangeOffsetY[speed] = y_offset;
}

OctScanOptions::OctScanOptionsImpl& wso_domain::OctScanOptions::impl(void) const
{
	return *d_ptr;
}
