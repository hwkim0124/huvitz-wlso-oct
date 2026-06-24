#pragma once

#include "WsoSystem2.h"


namespace wso_device
{
	class MainBoard;
}


namespace wso_system
{
	class WSOSYSTEM_DLL_API Hardware
	{
	public:
		Hardware();
		virtual ~Hardware();

		Hardware(const Hardware& rhs) = delete;
		Hardware& operator=(const Hardware& rhs) = delete;

		static Hardware* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeBoardDevices(void);
		bool initializeOctScanner(void);

		void connectJoystickButtonPressed(JoystickButtonPressedCallback clb);
		void connectOptimizeButtonPressed(OptimizeButtonPressedCallback clb);
		void connectAdapterLensAttached(AdapterLensAttachedCallback clb);
		void connectStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb);

		void connectCorneaCameraImageCaptured(CameraType type, CorneaCameraFrameCaptured clb);

		void releaseJoystickButtonPressed(void);
		void releaseOptimizeButtonPressed(void);
		void releaseAdapterLensAttached(void);

		void disconnectStepMotorPositionChanged(MotorType type);
		void disconnectCorneaCameraImageCaptured(CameraType type);

		void excludeDeviceFromInit(bool mainboard, bool src_leds, bool motors, bool ir_camera);
		void excludeGrabberFromInit(bool oct_grab);

		MainBoard* getMainBoard(void) const;
		StepMotor* getStepMotor(MotorType type) const;
		LightLed* getLightLed(LightType type) const;
		Usb3Grabber* getUsb3Grabber(void) const;

	private:
		struct HardwareImpl;
		std::unique_ptr<HardwareImpl> d_ptr;
		HardwareImpl& impl(void) const;
	};
}

