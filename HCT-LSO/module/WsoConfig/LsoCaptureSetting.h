#pragma once

#include "WsoConfig2.h"

#include <memory>

namespace wso_config
{
	class WSOCONFIG_DLL_API LsoCaptureSetting
	{
	public:
		LsoCaptureSetting();
		virtual ~LsoCaptureSetting();

		LsoCaptureSetting(LsoCaptureSetting&& rhs);
		LsoCaptureSetting& operator=(LsoCaptureSetting&& rhs);
		LsoCaptureSetting(const LsoCaptureSetting& rhs);
		LsoCaptureSetting& operator=(const LsoCaptureSetting& rhs);

	public:
		void initializeLsoCaptureSetting(void);
		void resetToDefaultValues(void);

		bool importFromBoardProfile(const HbsConfiguration* config);
		bool exportToBoardProfile(HbsConfiguration* config) const;

		LsoCaptureFrameROIPreset* getLsoCaptureFrameROIPreset(void) const;
		void setLsoCaptureFrameROIPreset(const LsoCaptureFrameROIPreset& param);

		int getCaptureFrameCount(void) const;
		LsoCaptureFrameROI getCaptureFrameROI(int index) const;

		void setCaptureFrameCount(int count);
		void setCaptureFrameROI(int index, LsoCaptureFrameROI param);

		//Sequencer
		LsoCaptureFrameSeqROIPreset* getLsoCaptureFrameSeqROIPreset(void) const;
		void setLsoCaptureFrameSeqROIPreset(const LsoCaptureFrameSeqROIPreset& param);

		int getSequencerFrameCount(void) const;
		LsoCaptureFrameROI getSequencerFrameROI(int index) const;

		void setSequencerFrameCount(int count);
		void setSequencerFrameROI(int index, LsoCaptureFrameROI param);

		//Offset ROI
		LsoCaptureFrameOffsetROIPreset* getLsoCaptureFrameOffsetROIPreset(void) const;
		void setLsoCaptureFrameOffsetROIPreset(const LsoCaptureFrameOffsetROIPreset& param);

		int getOffsetRoiWidth(void) const;
		int getOffsetRoiHeight(void) const;
		void setOffsetRoiWidth(int width);
		void setOffsetRoiHeight(int height);

		int getOffsetFrameCount(void) const;
		LsoCaptureFrameROI getOffsetFrameROI(int index) const;

		void setOffsetFrameCount(int count);
		void setOffsetFrameROI(int index, LsoCaptureFrameROI param);

		//Roll SWTrig Overlab
		LsoCaptureFrameRollSwTrigOverlapPreset* getLsoCaptureFrameRollSwTrigOverlapPreset(void) const;
		void setLsoCaptureFrameRollSwTrigOverlapPreset(const LsoCaptureFrameRollSwTrigOverlapPreset& param);

		void setFrameRollSwTrigOverlapCount(int count);
		void setFrameRollSwTrigOverlapRoiWidth(int roiWidth);
		void setFrameRollSwTrigOverlapRoiHeight(int roiHeight);
		void setFrameRollSwTrigOverlapOffsetX(int offsetX);
		void setFrameRollSwTrigOverlapOffsetY(int offsetY);
		void setFrameRollSwTrigOverlapTriggerInterval(int interval);
		void setFrameRollSwTrigOverlapBrightness(int bright);

		int getFrameRollSwTrigOverlapCount(void) const;
		int getFrameRollSwTrigOverlapRoiWidth(void) const;
		int getFrameRollSwTrigOverlapRoiHeight(void) const;
		int getFrameRollSwTrigOverlapOffsetX(void) const;
		int getFrameRollSwTrigOverlapOffsetY(void) const;
		int getFrameRollSwTrigOverlapTriggerInterval(void) const;
		int getFrameRollSwTrigOverlapBrightness(void) const;

	protected:
		struct LsoCaptureSettingImpl;
		std::unique_ptr<LsoCaptureSettingImpl> d_ptr;
		LsoCaptureSettingImpl& impl(void) const;
	};
}

