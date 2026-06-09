#include "pch.h"
#include "Hardware.h"
#include "HbsDataProfile.h"

using namespace wso_system;


std::mutex Hardware::singleMutex_;


struct Hardware::HardwareImpl
{
	MainBoard mainboard;
	Usb3Grabber usb3Grab;

	HardwareImpl() {
		initializeHardwareImpl();
	}

	void initializeHardwareImpl(void) {
	}
};



wso_system::Hardware::Hardware() :
	d_ptr(std::make_unique<HardwareImpl>())
{
}


wso_system::Hardware::~Hardware()
{
}


Hardware* wso_system::Hardware::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static Hardware instance;
	return &instance;
}


bool wso_system::Hardware::initializeBoardDevices(void)
{
	auto* p = getMainBoard();

	int n_warns;
	if (!p->initializeMainBoard(&n_warns)) {
		WsoLogError("Mainboard init failed!");
		return false;
	}
	else {
		WsoLogInfo("Mainboard initialized ... ok");
	}

	if (!p->initiateBoardComponents(&n_warns)) {
		WsoLogError("Board devices init failed!");
		return false;
	}
	else {
		WsoLogInfo("Board devices initialized ... ok");
	}

	return true;
}

bool wso_system::Hardware::initializeOctScanner(void)
{
	if (auto* board = getMainBoard(); board) {
		if (board->isOctGrabberNotInUse()) {
			return true;
		}
		else {
			if (auto* grab = getUsb3Grabber(); grab) {
				if (!grab->initializeUsb3Grabber()) {
					WsoLogError("Oct grabber init failed!");
					return false;
				}
				else {
					WsoLogInfo("Oct grabber initialized ... ok");
					return true;
				}
			}
		}
	}
	return false;
}

void wso_system::Hardware::connectJoystickButtonPressed(JoystickButtonPressedCallback clb)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setJoystickButtonPressed(clb);
	}
	return;
}

void wso_system::Hardware::connectOptimizeButtonPressed(OptimizeButtonPressedCallback clb)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setOptimizeButtonPressed(clb);
	}
	return;
}

void wso_system::Hardware::connectAdapterLensAttached(AdapterLensAttachedCallback clb)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setAdapterLensAttached(clb);
	}
	return;
}

void wso_system::Hardware::connectStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setStepMotorPositionChanged(type, clb);
	}
	return;
}

void wso_system::Hardware::connectCorneaCameraImageCaptured(CorneaCameraFrameCaptured clb)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setCorneaCameraFrameCaptured(clb);
	}
}

void wso_system::Hardware::releaseJoystickButtonPressed(void)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setJoystickButtonPressed(nullptr);
	}
}

void wso_system::Hardware::releaseOptimizeButtonPressed(void)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setOptimizeButtonPressed(nullptr);
	}
}

void wso_system::Hardware::releaseAdapterLensAttached(void)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setAdapterLensAttached(nullptr);
	}
}

void wso_system::Hardware::disconnectStepMotorPositionChanged(MotorType type)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setStepMotorPositionChanged(type, nullptr);
	}
	return;
}

void wso_system::Hardware::disconnectCorneaCameraImageCaptured(void)
{
	if (auto* inst = CallbackRegistry::getInstance(); inst) {
		inst->setCorneaCameraFrameCaptured(nullptr);
	}
}

void wso_system::Hardware::excludeDeviceFromInit(bool mainboard, bool src_leds, bool motors, bool ir_camera)
{
	auto* p = getMainBoard();
	unsigned int flags = p->getDeviceInitFlags();

	if (mainboard) {
		flags |= DEV_FLAG_NOT_MAINBOARD;

		// All devices integrated in mainboard should be not available. 
		src_leds = true;
		ir_camera = true;
		motors = true;
	}
	if (src_leds) {
		flags |= DEV_FLAG_NOT_SOURCE_LEDS;
	}
	if (ir_camera) {
		flags |= DEV_FLAG_NOT_IR_CAMERA;
	}
	if (motors) {
		flags |= DEV_FLAG_NOT_MOTORS;
	}

	p->setDeviceInitFlags(flags);
	return;
}

void wso_system::Hardware::excludeGrabberFromInit(bool oct_grab)
{
	auto* p = getMainBoard();
	unsigned int flags = p->getDeviceInitFlags();

	if (oct_grab) {
		flags |= DEV_FLAG_NOT_OCT_GRAB;
	}
	p->setDeviceInitFlags(flags);
	return;
}

wso_device::MainBoard* wso_system::Hardware::getMainBoard(void) const
{
	return &impl().mainboard;
}

StepMotor* wso_system::Hardware::getStepMotor(MotorType type) const
{
	auto step_type = static_cast<StepMotorType>(type);
	auto* mainboard = Hardware::getInstance()->getMainBoard();
	auto* motor = mainboard->getStepMotor(step_type);
	return motor;
}

LightLed* wso_system::Hardware::getLightLed(LightType type) const
{
	auto* mainboard = Hardware::getInstance()->getMainBoard();
	auto* led = mainboard->getLightLed(type);
	return led;
}

Usb3Grabber* wso_system::Hardware::getUsb3Grabber(void) const
{
	return &impl().usb3Grab;
}


Hardware::HardwareImpl& wso_system::Hardware::impl(void) const
{
	return *d_ptr;
}
