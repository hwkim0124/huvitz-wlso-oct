#pragma once

#include "OctScan2.h"

#include <memory>

namespace oct_pattern {
	class LineTrace;
	class PatternPlan;
	class PatternFrame;
}

namespace oct_grab {
	class Usb3Grabber;
}

namespace wso_device {
	class MainBoard;
	class Galvanometer;
}


namespace oct_scan
{
	class OCTSCAN_DLL_API ScanFunc
	{
	public:
		ScanFunc();
		virtual ~ScanFunc();

	public:
		static bool initializeScanFunc(wso_device::MainBoard* board, oct_grab::Usb3Grabber* grabber);
		static bool isInitialized(void);

		static bool prepareChainSetup(OctScanPattern pattern, bool measure);
		static bool updateLineTrace(oct_pattern::LineTrace* line, OctScanSpeed speed, bool foreDist = false);

		static bool updatePatternPositions(EyeSide eyeSide, const oct_pattern::PatternPlan& pattern, OctScanSpeed speed);
		static bool updatePreviewPositions(EyeSide eyeSide, const oct_pattern::PatternPlan& pattern, OctScanSpeed speed);
		static bool updateMeasurePositions(EyeSide eyeSide, const oct_pattern::PatternPlan& pattern, OctScanSpeed speed);
		static bool updateEnfacePositions(EyeSide eyeSide, const oct_pattern::PatternPlan& pattern, OctScanSpeed speed);
		static bool updateTracePositions(EyeSide eyeSide, const oct_pattern::PatternPlan& pattern, oct_pattern::LineTrace& line, bool cornea);

		static bool exportPatternPositions(const oct_pattern::PatternPlan& pattern, std::string filename);

		static bool uploadPatternProfiles(const oct_pattern::PatternPlan& pattern, bool measure);
		static bool uploadPreviewProfiles(const oct_pattern::PatternPlan& pattern);
		static bool uploadMeasureProfiles(const oct_pattern::PatternPlan& pattern, int startIdx = 0, int maxLines = 32);
		static bool uploadEnfaceProfiles(const oct_pattern::PatternPlan& pattern);
		static bool uploadTraceProfile(oct_pattern::LineTrace& line);

		static bool updatePatternBuffers(const oct_pattern::PatternPlan& pattern, bool measure, bool reset=true);
		static bool updatePreviewBuffers(const oct_pattern::PatternPlan& pattern);
		static bool updateMeasureBuffers(const oct_pattern::PatternPlan& pattern);
		static bool updateEnfaceBuffers(const oct_pattern::PatternPlan& pattern);
		static void releaseGrabberBuffers(void);

	protected:
		static oct_grab::Usb3Grabber* getUsb3Grabber(void);
		static wso_device::Galvanometer* getGalvanometer(void);

	private:
		struct ScanFuncImpl;
		static std::unique_ptr<ScanFuncImpl> d_ptr;
		static ScanFuncImpl& getImpl(void);
	};
}

