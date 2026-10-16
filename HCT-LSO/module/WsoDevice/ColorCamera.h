#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"



namespace wso_device
{
	class MainBoard;

	class SpinnakerCamera
	{
	public:
		virtual void OnSpinnakerCameraEvent(std::string eventName) = 0;
	};

	class WSODEVICE_DLL_API ColorCamera : public BoardComponent, SpinnakerCamera
	{
	public:
		ColorCamera(MainBoard* board);
		virtual ~ColorCamera();

	public:
		bool initialize(void);
		bool isInitiated(void) const;
		void uninitialize(void);

		void setInitParameters();
		bool setDefaultParameters(bool update = true);

		void startLiveMode(void);
		void pauseLiveMode(void);
		bool isLiveMode(void) const;

		// Hardware-triggered aiming live: the same device setup the HW-trigger capture uses
		// (TriggerMode On, FrameStart, Line3 rising edge, ReadOut overlap, Continuous) plus
		// the FrameStart trigger delay, which setupHwTriggerSetting() does not write, and a
		// bounded arm verification - so the caller can start the board knowing the camera is
		// already waiting on the trigger line. Never throws.
		// Returns false having installed NOTHING (trigger, line and trigger-delay state put
		// back, acquisition ended): a failed start has nothing for the caller to undo.
		// pauseLiveMode() stops it; the worker's per-image wait is bounded, so a stop never
		// blocks on a trigger that never arrives.
		// triggerAlreadyInstalled: the caller (the fundus preview one-call) has already
		// written the trigger, GPIO and TriggerDelay nodes through applyFullSettings, so
		// this must NOT write them again - re-running setupHwTriggerSetting() here is
		// exactly the silent overwrite of an operator's stated values that A-1 removes.
		// With it set, this only begins the acquisition and verifies arming.
		bool startTriggeredLiveMode(unsigned int triggerDelayUs, unsigned int armTimeoutMs,
			bool triggerAlreadyInstalled = false);

		void startSingleFrameLiveMode(void);
		void startFrameSeqROILiveMode(void);
		void startFrameOffsetROILiveMode(void);
		void startFrmaeOffsetROICaptureMode(void);
		void startFrameRollSwTrigOverlabLiveMode(void);
		void startFrameRollSwTrigOverlabCaptureMode(void);

		void startSwTriggerLiveMode(void);
		void stopSwTriggerLiveMode(void);
		void setupSoftwareTrigger(int nMode = 0);
		void shootSwTrigger(void);

		void startHwTriggerLiveMode(void);
		void stopHwTriggerLiveMode(void);
		bool isCameraStreaming(void);
		bool setupHwTriggerSetting(void);
		bool setupCameraTriggerOnOffOnly(bool bOn);

		void startOriginalMode(void);
		void pauseOriginalMode(void);

		// Capture lifecycle, in this order:
		// begin -> optional waitUntilTriggerArmed -> acquire -> abort/cleanup.
		// beginCaptureFrames' admission check (not live, not already capturing) is NOT
		// atomic with the flag it then sets, so the caller must serialize starts - the
		// WsoSystem capture exports do that under their own capture-state lock. It leaves
		// the camera streaming on success and fully torn down on failure (false), so a
		// failed begin needs no abort. hwTrigger selects the trigger/line setup that
		// abortCaptureFrames puts back.
		// waitUntilTriggerArmed polls AcquisitionStatus(FrameTriggerWait) up to timeoutMs;
		// false means the node is unreadable/unwritable or the camera never armed.
		// acquire runs on the capture worker, fires the registered callback per frame up to
		// totalFrameCount, returns OK/CANCELLED/CAMERA with the actual count, and always
		// ends in abortCaptureFrames. Never throws.
		// Clears the cancellation flag for a new session. The ONLY place it is cleared:
		// call it from the starting thread while the caller's capture-state lock is held,
		// before beginCaptureFrames. No step of the running sequence may clear it, or a
		// stop issued right after an accepted start is lost.
		void armCaptureSession(void);
		// triggerAlreadyInstalled has the same meaning as in startTriggeredLiveMode: the
		// caller applied the preset's trigger/GPIO configuration, so this begins the
		// acquisition without re-writing it.
		bool beginCaptureFrames(bool hwTrigger, bool triggerAlreadyInstalled = false);
		bool waitUntilTriggerArmed(unsigned int timeoutMs);
		int acquireCaptureFrames(int* framesAcquired, unsigned int totalFrameCount, bool hwTrigger);
		void abortCaptureFrames(void);
		void cancelCaptureFrames(void);
		bool isCaptureMode(void) const;

		int getFrameWidth(void) const;
		int getFrameHeight(void) const;
		int getFrameSizeInBytes(void) const;
		int getFrameSizeForBpp(int nBytesPerPixel) const; 

		ColorCameraSettingParam& getCameraSettings(void);
		void setCameraSettings(ColorCameraSettingParam params);

		// --- A-1 extended configuration (GenApi node map, symbolic values) ---------------
		// Everything the 14-field ColorCameraSettingParam cannot carry: ExposureAuto, the
		// burst count, the binning selector/modes, TestPattern, DeviceLinkThroughputLimit,
		// SensorShutterMode, IspEnable, the five trigger nodes, TriggerDelay and the four
		// GPIO line rows.
		//
		// A field the block leaves unstated is not written at all, so a caller that states
		// nothing changes nothing. Every stated field is written and then READ BACK: a
		// setter that returned without throwing is not evidence, so the per-field result
		// distinguishes "node absent" from "not writable now" from "write refused" from
		// "read-back differs". Never throws.
		//
		// Returns true only when every stated field ended APPLIED. `result` may be null,
		// in which case only the boolean is available.
		bool applyExtendedSettings(const ColorCameraExtendedParam& params, ColorCameraApplyResult* result);

		// Current device state of the same fields, as symbolic names. Unreadable nodes come
		// back unstated, so a snapshot taken here and applied later writes only what could
		// actually be read. Never throws.
		// includeLines: reading a GPIO row means WRITING LineSelector, so the four rows are
		// only walked when the caller will use them, and only when the selector can be read
		// back afterwards - a read must not leave the operator's selection moved.
		bool readExtendedSettings(ColorCameraExtendedParam* out, bool includeLines = true);

		// The whole camera contract in node-dependency order, which is the order D-4 of
		// docs/DevKit_vs_WsoToolkit_Preset_Settings.html prescribes:
		//   TriggerMode Off -> sensor/binning modes -> the 14 structural/acquisition fields
		//   (with Gain deferred) -> throughput/ISP/test pattern -> GPIO/trigger nodes ->
		//   final checked ExposureTime -> final checked Gain.
		// Turning the trigger off first is what makes the structural nodes writable; writing
		// TriggerMode last within its group stops a half-configured camera from arming.
		// The return value covers the EXTENDED fields only; the structural plus final
		// exposure/gain read-back verdict is written to `baseVerified` (and into the result
		// table) separately.
		// They are reported apart on purpose: the capture path has always required the
		// structural verify, while the preview has always accepted the camera's own
		// quantisation of a preset window, and folding the two would silently change one
		// of them. Never throws.
		bool applyFullSettings(const ColorCameraSettingParam& base,
			const ColorCameraExtendedParam& ext, ColorCameraApplyResult* result,
			bool* baseVerified = nullptr);

		// Capability probes for the editor: what this camera actually implements, rather
		// than a hardcoded list the UI claims to support.
		// enumerateNodeEntries returns the number of AVAILABLE symbolic entries written
		// (0 when the node is absent), queryNodeRange fills an integer or float node's
		// min/max/increment. Neither throws.
		int enumerateNodeEntries(const char* nodeName, ColorCameraEnumValue* buffer, int maxCount);
		bool queryNodeRange(const char* nodeName, double* minValue, double* maxValue,
			double* increment, bool* writable);

		// Current value of an integer or float node (AcquisitionLineRate, ResultingFrameRate,
		// ...). False when the node is absent or unreadable, and *value is left at 0. The
		// range sibling above answers "what may I ask for"; this answers "what is it now".
		// Never throws.
		bool queryNodeValue(const char* nodeName, double* value);

		// setCameraSettings + re-read verification of the structural/acquisition fields
		// (pixel format, binning, ROI, acq mode/count, ADC). Tunables (exposure, gain) are
		// camera-quantized and therefore not compared. Returns false on mismatch so a
		// partial/silent apply cannot go unnoticed. Never throws.
		bool setCameraSettingsChecked(const ColorCameraSettingParam& params);

		unsigned int getROI_Max_Width();
		unsigned int getROI_Max_Height();
		unsigned int getROI_X_Width();
		unsigned int getROI_Y_Height();
		unsigned int getROI_X_Offset();
		unsigned int getROI_Y_Offset();
		void setROI_X_Width(unsigned int val);
		void setROI_Y_Height(unsigned int val);
		void setROI_X_Offset(unsigned int val);
		void setROI_Y_Offset(unsigned int val);

		// Crops the currently applied ROI to a centered square (side = min(width, height))
		// so a non-square preview frame can fill a square view. Reads the actual applied
		// Width/Height, so it is robust to binning/increment rounding. Never throws.
		bool squareRoi(void);

		unsigned int getExposureTime();
		void setExposureTime(unsigned int val);

		unsigned int getAcquisitionMode();
		void setAcquisitionMode(unsigned int val);

		unsigned int getAcquisitionFrameCount();
		void setAcquisitionFrameCount(unsigned int val);

		unsigned int getPixelFormat();
		void setPixelFormat(unsigned int val);

		unsigned int getBinningHorizontal();
		void setBinningHorizontal(unsigned int val);

		unsigned int getBinningVertical();
		void setBinningVertical(unsigned int val);

		float getGain();
		void setGain(float val);

		unsigned int getAdcBitDepth();
		void setAdcBitDepth(unsigned int val);

		unsigned int getFlipMode();

		void setCaptureSequencerPreset(LsoCaptureFrameSeqROIPreset preset);
		void setCaptureOffsetRoiPreset(LsoCaptureFrameOffsetROIPreset preset);
		void setRollSwTrigOverlapPreset(LsoCaptureFrameRollSwTrigOverlapPreset preset);

		bool loadConfig();
		bool saveConfig();

		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;
		void OnSpinnakerCameraEvent(std::string eventName) override;

	private:
		struct ColorCameraImpl;
		std::unique_ptr<ColorCameraImpl> d_ptr;
		ColorCameraImpl& impl(void) const;

		void acquireCameraData(void);
		void acquireCameraTriggeredLiveData(void);
		void acquireCameraSingleFrameData(void);
		void acquireCameraSingleSequencerROIData(void);
		void acquireCameraMultiSequencerROIData(void);

		void acquireCameraOffsetROIData(void);
		void acquireCaptureCameraOffsetROIData(void);
		void acquireCameraRollSwTrigOverlapLive(void);
		void acquireCameraRollSwTrigOverlapCapture(void);
		void acquireCameraRollHwTriggerCaptureData(void);

		bool onSequencer(void);
		bool setSequencer(std::vector<LsoCaptureFrameROI> paramList);
		void offSequencer(void);
		
		std::vector<std::vector<LsoCaptureFrameROI>> divideParamVector_(int nFrameCount, LsoCaptureFrameROI* arrayParams);

		void saveUserSet_(int eUserSet);
		void loadUserSet_(int eUserSet);

		void acquireCameraCaptureData(void);
		int getBytesPerPixel();

		// Recovers a faulted Spinnaker camera (device reset + bounded re-enumerate)
		// so the init retry path can reuse a valid, writable handle. Never throws.
		bool recoverCameraAfterFault(void);

		// Puts back the FrameStart trigger delay startTriggeredLiveMode() overwrote.
		// No-op unless this class actually changed it, so nothing else's value is invented.
		void restoreTriggerDelay_(void);

		// applyExtendedSettings without clearing `result`: it APPENDS its rows. That is what
		// lets applyFullSettings run the extended block in two halves around the structural
		// write and still hand back one table - clearing on the second half would drop the
		// first half's rows and the structural row between them.
		bool applyExtendedInto_(const ColorCameraExtendedParam& params, ColorCameraApplyResult* result);

		// Full apply defers Gain until all ADC/shutter/ISP writes are complete. These variants
		// keep the public direct setters on their legacy write order.
		void setCameraSettings_(ColorCameraSettingParam params, bool applyGain);
		bool setCameraSettingsChecked_(const ColorCameraSettingParam& params, bool applyGain);

	private :
		void SetEventMode(bool enableEvents);
	};
}



