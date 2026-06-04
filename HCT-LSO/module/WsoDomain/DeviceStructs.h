#pragma once

namespace wso_domain
{
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