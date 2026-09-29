#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	// Persistent parameters of the EyeAlign auto alignment (stage positions, pixel
	// tolerances, frame counts, timeouts and the per-camera pupil target geometry).
	// Stored in the "AlignmentSetting" group of WsoConfig.xml.
	class WSOCONFIG_DLL_API AlignmentSettings
	{
	public:
		AlignmentSettings();
		virtual ~AlignmentSettings();

		AlignmentSettings(AlignmentSettings&& rhs);
		AlignmentSettings& operator=(AlignmentSettings&& rhs);
		AlignmentSettings(const AlignmentSettings& rhs);
		AlignmentSettings& operator=(const AlignmentSettings& rhs);

	public:
		void resetToDefaultValues(void);

		EyeAlignConfigParam* getParam(void) const;
		void setParam(const EyeAlignConfigParam& param);

		PupilDetectParam getDetectParam(CameraType type) const;
		void setDetectParam(CameraType type, const PupilDetectParam& param);

	protected:
		struct AlignmentSettingsImpl;
		std::unique_ptr<AlignmentSettingsImpl> d_ptr;
		AlignmentSettingsImpl& impl(void) const;
	};
}
