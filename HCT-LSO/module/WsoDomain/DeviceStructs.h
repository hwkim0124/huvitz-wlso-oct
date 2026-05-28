#pragma once

namespace wso_domain
{
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

	struct ColorCameraConfigParam
	{
	public:
		unsigned int roi_max_width;
		unsigned int roi_max_Height;
		unsigned int roi_x_width;
		unsigned int roi_y_height;
		unsigned int roi_x_offset;
		unsigned int roi_y_offset;
		unsigned int exposure_time;
		unsigned int pixelFormat;
		unsigned int acqusitionMode;


	public:
		ColorCameraConfigParam() {
			roi_max_width = 0;
			roi_max_Height = 0;
			roi_x_width = 0;
			roi_y_height = 0;
			roi_x_offset = 0;
			roi_y_offset = 0;
			exposure_time = 0;
			pixelFormat = 0;
			acqusitionMode = 0;
		}
	};

	struct FrameSeqROIParam
	{
		//int ExposureTime = 0;
		//int Gain = 0;
		int Width = 0;
		int Height = 0;
		int OffsetX = 0;
		int OffsetY = 0;
	};

	struct CaptureFrameSeqROIPreset
	{
		int FrameCount = 0;
		FrameSeqROIParam* pParams = nullptr;
	};

	struct FrameOffsetROIParam
	{
		int OffsetX = 0;
		int OffsetY = 0;
	};

	struct CaptureFrameOffsetROIPreset
	{
		int RoiWidth = 0;
		int RoiHeight = 0;
		int FrameCount = 0;
		FrameOffsetROIParam* pParams = nullptr;
	};

	// Color Camera, Rolling Shutter, SW Trigger, Overlab
	struct CaptureFrameRollSWTrigOverlapPreset
	{
		int FrameWidth = 0;
		int FrameHeight = 0;
		int OffsetX = 0;
		int OffsetY = 0;
		int OverlapFrameCount = 0;
		int TriggerIntervalMs = 0;
		int Brightness = 0;
	};

	//Image Display Setting
	struct ImageMaskPreset
	{
		int IrRadius = 0;
		int ColorRadius = 0;
	};

	//Image Adjust Setting
	struct ImageAdjustPreset
	{
		int Brightness = 0;
	};


	struct StepMotorStatus
	{
	public:
		int rangeMin;
		int rangeMax;
		int currPos;
		int maxSpeed;
		int minSpeed;
		int accelStep;
		int sliderStepSize;
		int sliderPageSize;
		float rangeMinValue;
		float rangeMaxValue;

	public:
		StepMotorStatus() {
			rangeMin = 0; 
			rangeMax = 0;
			currPos = 0; 
			maxSpeed = 0;
			minSpeed = 0;
			accelStep = 0;
			sliderStepSize = 0;
			sliderPageSize = 0;
			rangeMinValue = 0.0f;
			rangeMaxValue = 0.0f;
		}
	};


	struct InternalFixationParam
	{
		int brightness = 0;
		int blinkMode = 0;
		int blinkPeriod = 0;
		int blinkOnTime = 0;
		int fixationType = 0;
	};

	struct QldCalibParam
	{
		int apcMode;
		int intensity;
		int currentLimit;
		int apcGainDcode;
		int status;
		float calibQldCurrent;
		float calibIpdCurrent;

		char serialName[16] = { 0 };

		QldCalibParam() {
			apcMode = 0;
			intensity = 0;
			currentLimit = 0;
			apcGainDcode = 0;
			status = 0;
			calibQldCurrent = 0.0f;
			calibIpdCurrent = 0.0f;
		}
	};

	struct QldLedStatus
	{
	public:
		QldCalibParam calibParam;

		float qldCurrent;
		float ipdCurrent;
		int externalPd;
		float temperature;

	public:
		QldLedStatus() {
			qldCurrent = 0.0f;
			ipdCurrent = 0.0f;
			externalPd = 0;
			temperature = 0.0f;
		}
	};

	struct OctSldCalibParam
	{
		int highCode;
		int lowCode1;
		int lowCode2;
		int rsiCode;

		OctSldCalibParam() {
			highCode = 0;
			lowCode1 = 0;
			lowCode2 = 0;
			rsiCode = 0;
		}
	};

	struct OctSldStatusParam
	{
	public:
		OctSldCalibParam calibParam;

		float sldCurrent;
		float ipdCurrent;
		int externalPd;
		float temperature;

	public:
		OctSldStatusParam() {
			sldCurrent = 0.0f;
			ipdCurrent = 0.0f;
			externalPd = 0;
			temperature = 0.0f;
		}
	};

	struct AdcSensorStatus
	{
	public:
		float cpuTempature;
		int externalMonitorPd;
		int lensHallSensor1;
		int lensHallSensor2;
		int odOs;

	public:
		AdcSensorStatus() {
			cpuTempature = 0.0f;
			externalMonitorPd = 0;
			lensHallSensor1 = 0;
			lensHallSensor2 = 0;
			odOs = 0;
		}
	};

	
}