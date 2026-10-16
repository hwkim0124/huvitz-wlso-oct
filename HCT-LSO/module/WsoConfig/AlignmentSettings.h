#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	// Persistent parameters of the EyeAlign auto alignment (stage positions, pixel
	// tolerances, frame counts, timeouts, the per-camera pupil target geometry and the
	// pupil-centre network settings).
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

		// Pupil-centre network (model path, presence threshold, threads). Native only; stored
		// in the <Detector> element of the group, never exposed through P/Invoke.
		PupilModelConfig getModelConfig(void) const;
		void setModelConfig(const PupilModelConfig& config);

	protected:
		struct AlignmentSettingsImpl;
		std::unique_ptr<AlignmentSettingsImpl> d_ptr;
		AlignmentSettingsImpl& impl(void) const;
	};
}
