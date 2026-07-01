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
		// bool updateScannerStatus(void);

		bool obtainControlParameters(int patternId, LsoScannerControlParam* param);
		bool submitControlParameters(int patternId, const LsoScannerControlParam* param);
		bool obtainCaptureParameters(int patternId, LsoScannerCaptureParam* param);
		bool submitCaptureParameters(int patternId, const LsoScannerCaptureParam* param);

		bool controlYGalvoMove(int ypos);
		bool controlCapture(int nPatternId, int onOff);
		bool controlTriggerMode(int onOff);
		bool startGrabbing(int nPatternId);
		bool pauseGrabbing(int nPatternId);

		// virtual bool loadConfigFromIniFile(void) override;
		// virtual bool saveConfigToIniFile(void) override;

		bool generateGalvanoPositions(short startPos, short endPos, int sampleSize, short* coords);
		// void calcGalvanoPos(int nPatternId, int nGalvanoSampleSize);

	protected:
		MainBoard* getMainBoard(void) const;

	private:
		struct LsoScannerImpl;
		std::unique_ptr<LsoScannerImpl> d_ptr;
		LsoScannerImpl& impl(void) const;
	};
}



