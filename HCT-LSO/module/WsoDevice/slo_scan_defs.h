#pragma once


namespace wso_device
{


	struct SloScanControl
	{
	public:
		int forwardScanOffset = 0;
		int backwardScanOffset = 0;
		int pixelResH = 0;
		int pixelResV = 0;
		int pixelClockDiv = 0;
		int pixelFormat = 0;
		int redIrSel = 0;
		int blueSel = 0;
		int greenSel = 0;
		int scanDirection = 0;
		int hsyncMode = 0;
		int hsyncFreq = 0;

		float adcGain[3] = { 0.0f };
		float adcOffset[3] = { 0.0f };

		int yGalvoStartPos = 0;
		int yGalvoEndPos = 0;
		int yGalvoCaptureStartIndex = 0;
		int rampUpSize = 0;
		int rampDownSize = 0;
		int idleSampleSize = 0;

		float scanRange = 6.0f;
		int scannerType = 1;

	public:
		SloScanControl() {

		}
	};

	struct QldLaserDataParam {
		int mode;
		int intensity;
		int limit;
		int gainDcode;
		float calibQld;
		float calibIpd;
		unsigned char serialNo[16];
	};
}
