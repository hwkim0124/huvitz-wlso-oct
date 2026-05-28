#pragma once


namespace wso_domain
{

	// Constant Definitions
	/////////////////////////////////////////////////////////////////////////////////////////////
	constexpr int QLD_POWER_PROFILE_SIZE = 4;
	constexpr int QLD_REFERENCE_LEVEL_SIZE = 2;
	constexpr int QLD_CHANNEL_SIZE = 4;
	constexpr int QLD_SERIAL_NAME_SIZE = 16;


	// Structure Definitions
	/////////////////////////////////////////////////////////////////////////////////////////////
	struct QldChannelStatus
	{
	public:
		float sldCurrent;
		float ipdCurrent;
		float temperature;
		int activeProfile;
		int controlResult;

	public:
		QldChannelStatus() {
			sldCurrent = 0.0f;
			ipdCurrent = 0.0f;
			temperature = 0.0f;
			activeProfile = 0;
			controlResult = 0;
		}
	};

	struct QldDeviceData
	{
	public:
		float thresholdCurrent;
		float operatingCurrent;
		float maxOpCurrent;
		float monitorCurrent;
		char serialName[16] = { 0 };

	public:
		QldDeviceData() {
			thresholdCurrent = 0.0f;
			operatingCurrent = 0.0f;
			maxOpCurrent = 0.0f;
			monitorCurrent = 0.0f;
		}
	};

	struct QldCalibLevel
	{
	public:
		float calLdCurrent;
		float calPdCurrent;
		int calExtPdLevel;

	public:
		QldCalibLevel() {
			calLdCurrent = 0.0f;
			calPdCurrent = 0.0f;
			calExtPdLevel = 0;
		}
	};

	struct QldPowerProfile
	{
	public:
		int apcModeGainDcode;
		int referLevels[QLD_REFERENCE_LEVEL_SIZE] = { 0 };

		QldCalibLevel calbLevels[QLD_REFERENCE_LEVEL_SIZE];

	public:
		QldPowerProfile() {
			apcModeGainDcode = 0;
		}
	};

	struct QldCalibrationData
	{
	public:
		int profileSize;
		int currentLimitLevel;
		int accApcMode;
		QldDeviceData deviceData;
		QldPowerProfile powerProfiles[QLD_POWER_PROFILE_SIZE];

	public:
		QldCalibrationData() {
			profileSize = 0;
			currentLimitLevel = 0;
			accApcMode = 0;
		}
	};

	struct QldStatusParam
	{
	public:
		QldChannelStatus channelStatus;
		QldCalibrationData calibData;
	};



}