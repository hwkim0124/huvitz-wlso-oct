#pragma once

#include "OctScan2.h"

#include <memory>
#include <functional>

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

namespace sig_chain {
	class OctProcess;
}


namespace oct_scan
{
	class OCTSCAN_DLL_API ScanGrab
	{
	public:
		ScanGrab();
		virtual ~ScanGrab();

		ScanGrab(const ScanGrab& rhs) = delete;
		ScanGrab& operator=(const ScanGrab& rhs) = delete;

		static ScanGrab* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void initiateScanGrab(wso_device::MainBoard* board, oct_grab::Usb3Grabber* grabber);
		void releaseScanGrab(void);

		bool grabPreview(oct_pattern::PatternPlan* pattern, sig_chain::OctProcess* process, bool noImageGrab);
		bool grabMeasure(bool start, OctScanProtocol* protocol);

		float getMeasureAcquisitionTime(void);

		bool isInitiated(void);
		bool startScanGrab(void);
		bool closeScanGrab(bool measure);
		void cancelMeasureGrab(void);

		bool isGrabbing(void);
		bool isPreviewing(void);
		bool isMeasuring(void);
		bool isMeasureCanceled(void);
		bool isMeasureCompleted(void);

		void threadGrabFunction(void);
		bool waitForPreviewAndEnfaceProcessed(void);
		bool checkIfScanOptimizing(void);
		void notifyMeasureFrameReceived(void);

		bool processGrabHidden(bool& initiate, bool& completed);
		bool processGrabPreview(bool& initiate);
		bool processGrabEnface(bool& initiate);
		bool processGrabPreviewHD(void);
		bool processGrabMeasure(void);
		bool proceedToMeasure(void);

		bool assignGrabBufferToPreview(int frameIdx, int buffIdx, bool repeat);
		int assignGrabBufferToMeasure(int frameIdx, int buffIdx);
		bool assignGrabBufferToEnface(int frameIdx, int buffIdx);

		int processGrabBufferToPreview(int frameIdx, int buffIdx, bool repeat);
		int processGrabBufferToMeasure(int frameIdx, int buffIdx);
		int processGrabBufferToEnface(int frameIdx, int buffIdx);

		bool triggerLineTracesToPreview(int frameIdx, bool repeat);
		bool triggerLineTracesToMeasure(int frameIdx, bool guessOffset, std::int16_t dynOffsetX, std::int16_t dynOffsetY, std::int16_t& startX, std::int16_t& startY);
		bool triggerLineTracesToEnface(int frameIdx);

		bool triggerTraceProfile(oct_pattern::LineTrace& line, short numLines = 1, short offsetX = 0, short offsetY = 0);
	
		bool updateGrabberTimeStepToMeasure(void);
		bool setGrabberTimeStepToFaster(bool noImageGrab = false);
		bool setGrabberTimeStepToNormal(bool noImageGrab = false);
		bool setGrabberTimeStepToSlower(bool noImageGrab = false);
		bool setGrabberTimeStepToCustom(bool noImageGrab = false);

		bool isScanSpeedFaster(void);
		bool isScanSpeedNormal(void);
		bool isScanSpeedSlower(void);
		bool isScanSpeedCustom(void);
		bool setUsbCameraExposureTime(float expTime);
		float getUsbCameraExposureTime(void);

		void increaseGrabErrorCount(void);
		void clearGrabErrorCount(void);
		bool isGrabErrorOverLimit(void);
		bool isGrabErrorOverDelay(void);

		bool isPreviewImagesRepeated(void);
		void resetCountOfPreviewImages(bool repeat);
		void resetCountOfMeasureImages(void);
		void resetCountOfEnfaceImages(void);

		int getIndexOfPreviewImageNext(void);
		int getIndexOfMeasureImageNext(bool autoinc=true);
		int getIndexOfEnfaceImageNext(void);
		void retreatIndexOfMeasureImage(int size);

	protected:
		void onOctPreviewBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index);
		void onOctMeasureBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index);
		void onOctEnfaceBufferGrabbed(unsigned short* buff, int size_x, int size_y, int index);

		oct_pattern::PatternPlan * getPattern(void);
		sig_chain::OctProcess* getProcess(void);
		oct_grab::Usb3Grabber* getUsb3Grabber(void);
		wso_device::MainBoard* getMainboard(void);
		wso_device::Galvanometer* getGalvanometer(void);

	private:
		struct ScanGrabImpl;
		std::unique_ptr<ScanGrabImpl> d_ptr;
		ScanGrabImpl& impl(void);

		// friend class Scanner;
	};
}
