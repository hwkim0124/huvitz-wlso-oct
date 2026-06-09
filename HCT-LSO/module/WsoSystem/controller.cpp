#include "pch.h"
#include "Controller.h"
#include "Hardware.h"

using namespace wso_system;
using namespace std;


std::mutex Controller::singleMutex_;


struct Controller::ControllerImpl
{
	ControllerImpl() {
		initializeControllerImpl();
	}

	void initializeControllerImpl(void) {
	}
};



wso_system::Controller::Controller() :
	d_ptr(std::make_unique<ControllerImpl>())
{
}


wso_system::Controller::~Controller()
{
}


Controller* wso_system::Controller::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static Controller instance;
	return &instance;
}

bool wso_system::Controller::changeOctDiopterFocus(float diopt)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctFocusMotor(); p) {
		return p->updatePositionByDiopter(diopt);
	}
	return false;
}

bool wso_system::Controller::changeOctPolarization(float degree)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctPolarMotor(); p) {
		return p->updatePositionByDegree((int)degree);
	}
	return false;
}

bool wso_system::Controller::moveOctReferenceToOrigin(bool is_cornea)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctReferMotor(); p) {
		return p->updatePositionToOrigin((int)is_cornea);
	}
	return false;
}

bool wso_system::Controller::moveOctDiopterFocusToOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctFocusMotor(); p) {
		return p->updatePositionToOrigin();
	}
	return false;
}

bool wso_system::Controller::moveOctPolarizationToOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getOctPolarMotor(); p) {
		return p->updatePositionToOrigin();
	}
	return false;
}
/*
bool wso_system::Controller::moveFixationToOrigin(void)
{
	if (auto* p = Hardware::getInstance()->getMainBoard()->getFixationMotor(); p) {
		return p->updatePositionToOrigin();
	}
	return false;
}
*/

Controller::ControllerImpl& wso_system::Controller::impl(void) const
{
	return *d_ptr;
}
