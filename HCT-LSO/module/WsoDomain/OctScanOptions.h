#pragma once

#include "WsoDomain2.h"

namespace wso_domain
{
	class WSODOMAIN_DLL_API OctScanOptions
	{
	public:
		OctScanOptions();
		virtual ~OctScanOptions();

		OctScanOptions(const OctScanOptions& rhs) = delete;
		OctScanOptions& operator=(const OctScanOptions& rhs) = delete;

		static OctScanOptions* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool isDebugOutEnabled(optional<bool> flag = nullopt) const;
		bool isEnfaceImageEnhanceEnabled(optional<bool> flag = nullopt) const;
		bool isOclTasksDefaultPresetEnabled(optional<bool> flag = nullopt) const;

		std::tuple<int, int> getEnfacePreviewSlabRange(void) const;
		float getCorneaPatternRangeScaleX(int speed, int index) const;
		float getRetinaPatternRangeScaleX(int speed, int index) const;
		float getCorneaPatternRangeScaleY(int speed, int index) const;
		float getRetinaPatternRangeScaleY(int speed, int index) const;
		std::tuple<float, float> getCorneaPatternRangeOffset(int speed) const;
		std::tuple<float, float> getRetinaPatternRangeOffset(int speed) const;
		
		void setEnfacePreviewSlabRange(int y_start, int y_end);
		void setCorneaPatternRangeScale(int speed, int index, float x_scale, float y_scale);
		void setRetinaPatternRangeScale(int speed, int index, float x_scale, float y_scale);
		void setCorneaPatternRangeOffset(int index, float x_offset, float y_offset);
		void setRetinaPatternRangeOffset(int index, float x_offset, float y_offset);

	private:
		struct OctScanOptionsImpl;
		std::unique_ptr<OctScanOptionsImpl> d_ptr;
		OctScanOptionsImpl& impl(void) const;
	};
}

