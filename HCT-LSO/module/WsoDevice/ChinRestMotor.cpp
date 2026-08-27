#include "pch.h"
#include "ChinRestMotor.h"


#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace std;


struct ChinRestMotor::ChinRestMotorImpl
{
	ChinRestMotorImpl()
	{}
};


ChinRestMotor::ChinRestMotor() :
	d_ptr(make_unique<ChinRestMotorImpl>()), StepMotor()
{}


wso_device::ChinRestMotor::ChinRestMotor(MainBoard* board) :
	d_ptr(make_unique<ChinRestMotorImpl>()), StepMotor(board, StepMotorType::CHIN_REST)
{}


ChinRestMotor::~ChinRestMotor()
{}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
ChinRestMotor::ChinRestMotor::ChinRestMotor(ChinRestMotor&& rhs) = default;
ChinRestMotor& ChinRestMotor::ChinRestMotor::operator=(ChinRestMotor&& rhs) = default;


wso_device::ChinRestMotor::ChinRestMotor(const ChinRestMotor& rhs)
	: d_ptr(make_unique<ChinRestMotorImpl>(*rhs.d_ptr))
{}


ChinRestMotor& wso_device::ChinRestMotor::operator=(const ChinRestMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}

bool wso_device::ChinRestMotor::initializeChinRestMotor(void)
{
	return StepMotor::initializeStepMotor();
}

bool wso_device::ChinRestMotor::startMovingUp(void)
{
	if (auto* board = getMainBoard(); board) {
		auto& comm = board->getUsbComm();
		const int UPWARD = 1;
		if (comm.StepMotorVertMove(getMotorId(), UPWARD)) {
			return true;
		}
	}
	return false;
}

bool wso_device::ChinRestMotor::startMovingDown(void)
{
	if (auto* board = getMainBoard(); board) {
		auto& comm = board->getUsbComm();
		const int DOWNWARD = 0;
		if (comm.StepMotorVertMove(getMotorId(), DOWNWARD)) {
			return true;
		}
	}
	return false;
}

bool wso_device::ChinRestMotor::stopMoving(void)
{
	if (auto* board = getMainBoard(); board) {
		auto& comm = board->getUsbComm();
		if (comm.StepMotorStop(getMotorId())) {
			return true;
		}
	}
	return false;
}

bool wso_device::ChinRestMotor::isAtUpperLimit(void)
{
	if (auto* board = getMainBoard(); board) {
		if (auto* p = board->getHbsDataProfile(); p) {
			if (p->loadGpioStatus()) {
				auto* gpio = p->getHbsGpioStatus();
				auto status = gpio->PIstatus;
				if (status & (0x01 << 16)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool wso_device::ChinRestMotor::isAtLowerLimit(void)
{
	if (auto* board = getMainBoard(); board) {
		if (auto* p = board->getHbsDataProfile(); p) {
			if (p->loadGpioStatus()) {
				auto* gpio = p->getHbsGpioStatus();
				auto status = gpio->PIstatus;
				if (status & (0x01 << 15)) {
					return true;
				}
			}
		}
	}
	return false;
}

ChinRestMotor::ChinRestMotorImpl& wso_device::ChinRestMotor::impl(void) const
{
	return *d_ptr;
}
