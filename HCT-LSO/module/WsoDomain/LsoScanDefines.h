#pragma once

namespace wso_domain
{
	constexpr int LSO_SCANNER_SAMPLE_SIZE_MAX = 8192;
	constexpr int LSO_SCANNER_PATTERN_NUM_MAX = 8;


	struct LsoScannerControlParam
	{
	public:
		float timeStepUs = 0.0;			// galvano scanner control sample time.
		float exposureTimeUs = 0.0;		// camera exposure time in us.

		unsigned short triggerSource = 0;	// camera flash out source 0: ColorCamera,1:IR camera. 
		unsigned short acquisitionMode = 0;	// 0:continuous, 1:fixed frame Mode.
		unsigned int acqFrameSize = 0;		// valid on Fixed FrameMode

		unsigned short sampleSize = 0;		// galvano control pattern sample size
		unsigned short subFrameSize = 0;	// subframe size of a Full frame.
		short prescanPosRewindOffset = 0;	// gavano rewind offset of subframe
		unsigned short ledOnPosIndex = 0;	// led on sample index of gavano pattern.
		unsigned short ledOffPosIndex = 0;	// led off sample index of galvano pattern.
		
		// unsigned short samplePos[LSO_SCANNER_SAMPLE_SIZE_MAX] = { 0 }; // scanning pattern.
	    short yGalvoStartPos = 0;
		short yGalvoEndPos = 0;

	public:
		LsoScannerControlParam() {

		}
	};

	struct LsoScannerCaptureParam
	{
	public:
		unsigned int controlStatus = 0;			// on,off
		unsigned int acquisitionCount = 0;     // current acq.cnt

	public:
		LsoScannerCaptureParam() {
		}
	};
	
}
