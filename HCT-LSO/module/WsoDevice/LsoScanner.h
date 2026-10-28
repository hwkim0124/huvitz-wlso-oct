#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>

namespace wso_device
{
	class WSODEVICE_DLL_API LsoScanner : public BoardComponent
	{
	public:
		LsoScanner();
		LsoScanner(MainBoard* board);
		virtual ~LsoScanner();

		LsoScanner(LsoScanner&& rhs);
		LsoScanner& operator=(LsoScanner&& rhs);
		LsoScanner(const LsoScanner& rhs) = delete;
		LsoScanner& operator=(const LsoScanner& rhs) = delete;
	
	public:
		virtual bool initializeLsoScanner(void);
		bool updateScannerStatus(void);

		bool obtainControlParameters(int patternId, LsoScannerControlParam* param);
		bool submitControlParameters(int patternId, const LsoScannerControlParam* param);
		// Per-slot override of the generated pos[] table. While a table is set for a slot,
		// submitControlParameters() copies it into traj.pos instead of the linear sweep -
		// including the Preview/Capture one-calls, which resubmit the profile on every start.
		// count must equal the profile's sampleSize at submit time; count 0 (or null) clears.
		// Process-lifetime only: the caller re-sends it after start-up.
		bool setCustomTrajectory(int patternId, const short* pos, int count);
		bool obtainCaptureParameters(int patternId, LsoScannerCaptureParam* param);
		bool submitCaptureParameters(int patternId, const LsoScannerCaptureParam* param);
		//bool storeControlParameters(void);

		bool controlYGalvoMove(int ypos);
		// Board-global scan sequence: 0 = TRIG_FIRST (board waits on EXPOSURE_ACTIVE),
		// 1 = GALVANO_FIRST (mirror pre-moves, board fires the camera trigger).
		bool controlScanSequence(int seqMode, int preMoveOffset, int preMoveDelayUs);
		bool controlCapture(int nPatternId, int onOff, std::uint8_t fundusMode);
		bool controlTriggerMode(int onOff);
		bool startGrabbing(int nPatternId, std::uint8_t fundusMode);
		bool pauseGrabbing(int nPatternId);

		//virtual bool loadConfigFromIniFile(void) override;
		//virtual bool saveConfigToIniFile(void) override;

		bool generateGalvanoPositions(short startPos, short endPos, int sampleSize, short* coords);
		bool generateGalvanoPositions(short startPos, short endPos, int sampleSize,
			int leadInSamples, int leadOutSamples, short* coords);
		void calcGalvanoPos(int nPatternId, int nGalvanoSampleSize);

	protected:
		MainBoard* getMainBoard(void) const;

	private:
		struct LsoScannerImpl;
		std::unique_ptr<LsoScannerImpl> d_ptr;
		LsoScannerImpl& impl(void) const;
	};
}



