#pragma once
#pragma once

namespace wso_domain
{
	struct LsoScannerControlParam
	{
	public:
		float timeStepUs = 0.0;
		float exposureTimeUs = 0.0;

		unsigned short int trgSrc = 0;
		unsigned short int acqMode = 0;

		unsigned int acqFrameSize = 0;

		unsigned short int sampleSize = 0;
		unsigned short int subFrameSize = 0;

		short int prescanPosRewindOffset = 0;

		int yGalvoStartPos = 0;
		int yGalvoEndPos = 0;

		unsigned short int ledOnPosIndex = 0;
		unsigned short int ledOffPosIndex = 0;

	public:
		LsoScannerControlParam() {

		}
	};
}
