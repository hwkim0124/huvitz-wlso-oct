#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	class WSOSYSTEM_DLL_API OctScanning
	{
	public:
		OctScanning();
		virtual ~OctScanning();

		OctScanning(const OctScanning& rhs) = delete;
		OctScanning& operator=(const OctScanning& rhs) = delete;

		static OctScanning* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeOctScanning(void);
		void releaseOctScanning(void);
		bool isInitialized(void) const;

		bool prepareScan(const OctProtocolInitParam& param);
		bool startScan(OctPreviewImageCaptured clb_preview, OctEnfaceImageCaptured clb_enface);
		bool closeScan(bool measure, OctScanProtocolCompleted clb_protocol);
		void cancelScan(void);

		bool isWorking(void) const;
		bool isGrabbing(void) const;
		bool isPostProcessing(void) const;
		bool isAutoOptimizing(void) const;
		bool isPreviewing(void) const;
		bool isMeasuring(void) const;

		bool startAutoDiopterFocus(OctAutoFocusOptimized clb_focus);
		bool startAutoReference(OctAutoReferOptimized clb_refer);
		bool startAutoPolarization(OctAutoPolarOptimized clb_polar);
		bool startAutoScanOptimize(OctAutoScanOptimized clb_scan);
		bool cancelAutoOptimizing(void);

	protected:
		bool beginScanProtocol(void);
		bool initScanProcessing(bool enface);
		bool setScanDevicesPrepared(bool init_scan, bool not_align);
		bool setScanDevicesReleased(void);
		bool alignScanFocusToReadyPosition(bool init_scan, bool not_align);

		bool startPreviewScan(bool enface);
		bool closePreviewScan(bool measure);
		void stopGrabbing(bool measure);
		void releaseScanCallbacks(void);

		bool initiateMeasureResult(void);
		bool captureMeasureResult(void);

		bool turnOnInternalFixationByPattern(void);
		bool adjustScanSpeedToPreview(void);
		bool adjustScanSpeedToMeasure(void);
		bool updateGrabberTimeStep(OctScanSpeed speed, bool not_image_grab = false);
		bool updatePatternLayout(void);

	private:
		struct OctScanningImpl;
		std::unique_ptr<OctScanningImpl> d_ptr;
		OctScanningImpl& impl(void) const;

	};
}

