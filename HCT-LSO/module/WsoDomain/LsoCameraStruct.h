#pragma once

namespace wso_domain
{
	constexpr int LSO_CAPTURE_FRAME_ROI_MAX = 255;

	struct ColorCameraSettingParam
	{
	public:
		unsigned int roi_max_width;
		unsigned int roi_max_Height;
		unsigned int roi_x_width;
		unsigned int roi_y_height;
		unsigned int roi_x_offset;
		unsigned int roi_y_offset;
		unsigned int exposure_time;
		unsigned int acqusitionMode;
		unsigned int acqusitionFrameCount;
		unsigned int pixelFormat;
		unsigned int binningHorizontal;
		unsigned int binningVertical;
		float		 gain;
		unsigned int adcDepthIndex;

	public:
		ColorCameraSettingParam() {
			roi_max_width = 0;
			roi_max_Height = 0;
			roi_x_width = 0;
			roi_y_height = 0;
			roi_x_offset = 0;
			roi_y_offset = 0;
			exposure_time = 0;
			acqusitionMode = 0;
			acqusitionFrameCount = 0;
			pixelFormat = 0;
			binningHorizontal = 0;
			binningVertical = 0;
			gain = 0.0f;
			adcDepthIndex = 0;
		}
	};

	struct LsoCaptureFrameROI
	{
		int width = 0;
		int height = 0;
		int offsetX = 0;
		int offsetY = 0;
		int startY = 0;
		int endY = 0;
	};

	// Color Camera, Rolling Shutter, SW Trigger, Overlab
	struct LsoCaptureFrameRollSwTrigOverlap
	{
		int frameWidth = 0;
		int frameHeight = 0;
		int offsetX = 0;
		int offsetY = 0;
		int overlapFrameCount = 0;
		int triggerIntervalMs = 0;
		int brightness = 0;
	};

	//Image Display Setting
	struct LsoImageMask
	{
		int irRadius = 0;
		int colorRadius = 0;
	};

	//Image Adjust Setting
	struct LsoImageAdjust
	{
		int brightness = 0;
	};

	struct LsoCaptureFrameROIPreset
	{
		int frameCount = 0;
		LsoCaptureFrameROI frameRois[LSO_CAPTURE_FRAME_ROI_MAX];
	};

	struct LsoCaptureFrameSeqROIPreset
	{
		int frameCount = 0;
		LsoCaptureFrameROI frameRois[LSO_CAPTURE_FRAME_ROI_MAX];
	};

	struct LsoCaptureFrameOffsetROIPreset
	{
		int frameCount = 0;
		int roiWidth = 0;
		int roiHeight = 0;
		LsoCaptureFrameROI frameRois[LSO_CAPTURE_FRAME_ROI_MAX];
	};

	struct LsoCaptureFrameRollSwTrigOverlapPreset
	{
		LsoCaptureFrameRollSwTrigOverlap overlap;
	};

	struct LsoImageAdjustPreset
	{
		LsoImageAdjust adjust;
	};

	struct LsoImageMaskPreset
	{
		LsoImageMask mask;
	};
}
