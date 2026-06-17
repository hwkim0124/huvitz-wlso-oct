#pragma once

#include "WsoDevice2.h"

#include <memory>
#include <string>


namespace wso_board
{
	class HbsDataProfile;
}

namespace wso_device
{
	using namespace wso_domain;
	using namespace wso_board;

	class UsbComm;
	class InfraredCamera;

	class OctSldLed;
	class LightLed;
	class LsoWhiteLed;
	class RetinaIrLed;
	class CorneaIrLed;

	class StepMotor;
	class OctFocusMotor;
	class OctPolarMotor;
	class OctReferMotor;
	class LsoFocusMotor;
	class FixationMotor;
	class TiltMotor;
	class SwingMotor;

	class StageMotor;
	class XstageMotor;
	class YstageMotor;
	class ZstageMotor;

	class CorneaCamera;
	class RetinaCamera;
	class ColorCamera;

	class LsoScanner;
	class Galvanometer;
	class SldLaserDriver;
	class ZynqXadcDriver;

	class FirmwareControl;

	class WSODEVICE_DLL_API MainBoard
	{
	public:
		MainBoard();
		virtual ~MainBoard();

		MainBoard(MainBoard&& rhs) noexcept;
		MainBoard& operator=(MainBoard&& rhs) noexcept;

		// Prevent copy construction and assignment. 
		MainBoard(const MainBoard& rhs) = delete;
		MainBoard& operator=(const MainBoard& rhs) = delete;

	public:
		bool initializeMainBoard(int* warnings);
		void releaseMainBoard(void);
		bool isInitialized(void);

		bool initiateBoardComponents(int* numWarns);
		bool initiateRetinaCamera(void);
		bool initiateCorneaCamera(InfraredCameraId camId);
		bool initiateColorCamera();

		void setDeviceInitFlags(unsigned int flags);
		auto getDeviceInitFlags(void) const -> unsigned int;

		bool isMainBoardNotInUse(void) const;
		bool isIrCameraNotInUse(void) const;
		bool isSourceLedsNotInUse(void) const;
		bool isMotorsNotInUse(void) const;
		bool isOctGrabberNotInUse(void) const;
		bool isOpenClNotInUse(void) const;

		bool openFTDIdevices(void);
		bool resetFTDIdevice(void);
		bool resetUsbChannel(void);
		bool resetSubChannel(void);

		bool waitForSystemReady(void);

		bool prepareDevicesForOctScan(void);
		bool releaseDevicesForOctScan(void);
		bool turnOnOctScanBeam(bool flag);
		/*
		bool prepareDevicesForSloScan(bool oct_preview, float diopt);
		bool releaseDevicesForSloScan(void);
		*/

		bool loadHostBufferTable(void);
		bool pullSystemCalibFromMemory(void);
		bool pushSystemCalibToMemory(void);

		bool pullSystemConfigFromMemory(void);
		bool pushSystemConfigToMemory(void);

		bool moveChinrestUp(void);
		bool moveChinrestDown(void);
		bool stopChinrest(void);
		bool isChinrestAtHighLimit(void);
		bool isChinrestAtLowLimit(void);

		bool isTiltMotorAtHighLimit(void);
		bool isTiltMotorAtLowLimit(void);
		bool isSwingMotorAtHighLimit(void);
		bool isSwingMotorAtLowLimit(void);
		bool isFixationMotorAtOrigin(void);
		bool isOctFocusMotorAtOrigin(void);
		bool isOctPolarMotorAtOrigin(void);
		bool isOctReferMotorAtOrigin(void);
		bool isLsoFocusMotorAtOrigin(void);
		bool isYaxisMotorAtHighLimit(void);
		bool isYaxisMotorAtLowLimit(void);

		/*
		bool isAdapterLensDetached(void);
		bool isAnteriorLensAttached(void);
		bool isWideAngleLensAttached(void);
		*/
		bool isOdOsStatusFlagSet(void);
		bool isEyeSideAtOd(void);
		bool isEyeSideAtOs(void);
		EyeSide getEyeSide(void);

		bool setGalvanoDynamicOffset(float mmPosX, float mmPosY, bool cornea);
		bool setGalvanoDynamicOffset(std::int16_t offsetX, std::int16_t offsetY);
		bool getGalvanoDynamicOffset(std::int16_t& offsetX, std::int16_t& offsetY);

		bool setLcdFixationOn(bool flag, int row = 159, int col = 127);
		bool getLcdFixationOn(int& row, int& col);

		bool updateLcdParameters(const InternalFixationParam param);
		bool updateLcdParameters(std::uint8_t bright, std::uint8_t blink, std::uint16_t period, std::uint16_t ontime, std::uint8_t type = 0x00);

		wso_board::HbsDataProfile* getHbsDataProfile(void) const;
		wso_board::HbsDataProfile* getSubDataProfile(void) const;
		UsbComm& getUsbComm(void) const;
		UsbComm& getSubComm(void) const;
			
		OctSldLed* getOctSldLed(void) const;
		SldLaserDriver* getSldLaserDriver(void) const;
		ZynqXadcDriver* getZyncXadcDriver(void) const;

		OctFocusMotor* getOctFocusMotor(void) const;
		OctPolarMotor* getOctPolarMotor(void) const;
		OctReferMotor* getOctReferMotor(void) const;
		LsoFocusMotor* getLsoFocusMotor(void) const;

		XstageMotor* getXstageMotor(void) const;
		YstageMotor* getYstageMotor(void) const;
		ZstageMotor* getZstageMotor(void) const;
		SwingMotor* getSwingMotor(void) const;

		RetinaCamera* getRetinaCamera(void) const;
		RetinaCamera* getCorneaLeftCamera(void) const;
		RetinaCamera* getCorneaRightCamera(void) const;
		RetinaCamera* getCorneaLowerCamera(void) const;
		CorneaCamera* getCorneaCamera(InfraredCameraId eCamId) const;
		ColorCamera* getColorCamera(void);

		LsoWhiteLed* getLsoWhiteLed(void) const;
		RetinaIrLed* getRetinaIrLed(void) const;
		CorneaIrLed* getCorneaIrLeftLed(void) const;
		CorneaIrLed* getCorneaIrRightLed(void) const;

		Galvanometer* getGalvanometer(void) const;
		LsoScanner* getLsoScanner(void) const;
		
		LightLed* getLightLed(LightType type) const;
		StepMotor* getStepMotor(StepMotorType type) const;
		StageMotor* getStageMotor(StageMotorType type) const;
		InfraredCamera* getInfraredCamera(CameraType type) const;
		FirmwareControl* getFirmwareControl(void) const;

	private:
		struct MainBoardImpl;
		std::unique_ptr<MainBoardImpl> d_ptr;
		MainBoardImpl& impl(void) const;
	};
}

