#include "pch.h"
#include "LsoFocusMotor.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include "wso_board.h"

using namespace wso_device;
using namespace wso_board;



struct LsoFocusMotor::LsoFocusMotorImpl
{
	int32_t posOfOrigin;
	int currentPos;

	LsoFocusMotorImpl() : posOfOrigin(0), currentPos(0)
	{

	}
};


LsoFocusMotor::LsoFocusMotor() :
	d_ptr(make_unique<LsoFocusMotorImpl>()), OctFocusMotor()
{
}


wso_device::LsoFocusMotor::LsoFocusMotor(MainBoard* board) :
	d_ptr(make_unique<LsoFocusMotorImpl>()), OctFocusMotor(board)
{
	setType(StepMotorType::LSO_FOCUS);
}


LsoFocusMotor::~LsoFocusMotor()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
LsoFocusMotor::LsoFocusMotor::LsoFocusMotor(LsoFocusMotor&& rhs) = default;
LsoFocusMotor& LsoFocusMotor::LsoFocusMotor::operator=(LsoFocusMotor&& rhs) = default;


wso_device::LsoFocusMotor::LsoFocusMotor(const LsoFocusMotor& rhs)
	: d_ptr(make_unique<LsoFocusMotorImpl>(*rhs.d_ptr))
{
}


LsoFocusMotor& wso_device::LsoFocusMotor::operator=(const LsoFocusMotor& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::LsoFocusMotor::initializeLsoFocusMotor(bool ready)
{
	if (StepMotor::initializeStepMotor()) {
		if (ready) {
			setPositionsPerDiopter(MOTOR_SLO_FOCUS_STEPS_PER_DIOPTER);
			//loadParamsFromProfile();
			loadConfigFromIniFile();
			updatePosition(impl().currentPos);
			//updatePositionToZeroDiopter();
		}
		return true;
	}
	return false;
}

bool wso_device::LsoFocusMotor::updatePosition(int pos)
{
	if (!StepMotor::updatePosition(pos))
	{
		return false;
	}

	impl().currentPos = pos;

	saveConfigToIniFile();
	return true;
}


int wso_device::LsoFocusMotor::getZeroDiopterPosition(void) const
{
	return impl().posOfOrigin;
}


void wso_device::LsoFocusMotor::setZeroDiopterPosition(int pos)
{
	pos = (pos > getRangeMax() ? getRangeMax() : pos);
	pos = (pos < getRangeMin() ? getRangeMin() : pos);

	impl().posOfOrigin = pos;
	return;
}


void wso_device::LsoFocusMotor::loadParamsFromProfile(void)
{
	//auto p = getMainBoard()->getHbsDataProfile()->getHbsCalibration();
	//setZeroDiopterPosition(p->DiopterCal.Scan_focus_zeroD_pos);

	//auto range = abs(getRangeMax() - getRangeMin());	
	//auto zero = getZeroDiopterPosition();
	//auto upper = getRangeMax() - zero;
	//auto lower = zero - getRangeMin();

	//auto half = min(upper, lower);
	//auto rmax = zero + half;
	//auto rmin = zero - half;
	//// setRangeMax(rmax);
	//// setRangeMin(rmin);

	//range = half * 2;
	//auto step = int(range / MOTOR_SLO_FOCUS_DIOPTER_RANGE);
	//setPositionsPerDiopter(step);
	return;
}


void wso_device::LsoFocusMotor::saveParamsToProfile(void)
{
	//auto p = const_cast<HbsCalibration*>(getMainBoard()->getHbsDataProfile()->getHbsCalibration());
	//p->DiopterCal.Scan_focus_zeroD_pos = getZeroDiopterPosition();
	return;
}

bool wso_device::LsoFocusMotor::loadConfigFromIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		impl().currentPos = ini->ReadInt(L"LSO_FOCUS_MOTOR", L"CurrentPosition");
		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}
	return bRet;
}

bool wso_device::LsoFocusMotor::saveConfigToIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		ini->WriteInt(L"LSO_FOCUS_MOTOR", L"CurrentPosition", impl().currentPos);
		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}

	return bRet;
}

LsoFocusMotor::LsoFocusMotorImpl& wso_device::LsoFocusMotor::impl(void) const
{
	return *d_ptr;
}


