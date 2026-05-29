#include "pch.h"
#include "CorneaCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

using namespace wso_device;
using namespace std;


struct CorneaCamera::CorneaCameraImpl
{
	CorneaCameraImpl()
	{
		initializeCorneaCameraImpl();
	}

	void initializeCorneaCameraImpl(void)
	{
	}
};


CorneaCamera::CorneaCamera() :
	d_ptr(make_unique<CorneaCameraImpl>()), InfraredCamera()
{
}


CorneaCamera::CorneaCamera(MainBoard* board, CameraType type, InfraredCameraId camId, std::uint8_t epid) :
	d_ptr(make_unique<CorneaCameraImpl>()), InfraredCamera(board, type, camId, epid)
{
}


CorneaCamera::~CorneaCamera()
{
}


bool wso_device::CorneaCamera::initializeCorneaCamera(void)
{
	if (!InfraredCamera::initializeInfraredCamera()) {
		return false;
	}
	// loadParamsFromProfile();
	loadConfigFromIniFile();
	return true;
}


bool wso_device::CorneaCamera::loadCalibParamFromProfile(void)
{
	/*
	if (!isInitiated()) {
		return;
	}

	uint8_t data = BoardConfig::corneaCameraAgain();
	float gain;
	float a_gain, d_gain;

	if (data > 8) {
		gain = (float)(data - 8);
		if (gain == 1.0f) {
			gain += 0.25f;
		}
		else if (gain == 2.0f) {
			gain += 0.5f;
		}
		else if (gain == 4.0f) {
			gain += 1.0f;
		}
		else if (gain == 8.0f) {
			gain += 2.0f;
		}
	}
	else {
		gain = (float)data;
	}

	setAnalogGain(gain, false);
	a_gain = gain;

	data = BoardConfig::corneaCameraDgain();
	gain = (float)(data >> 5);
	gain += (float)((data & 0x1F) * exp2(-5));

	setDigitalGain(gain, false);
	d_gain = gain;

	// LogDebug() << "Cornea ir camera, a-gain: " << a_gain << ", d-gain: " << d_gain;
	*/
	return true;
}


bool wso_device::CorneaCamera::saveCalibParamToProfile(void)
{
	/*
	if (!isInitiated()) {
		return;
	}

	float gain = getAnalogGain();
	BoardConfig::corneaCameraAgain(true, getAnalogGainData(gain));

	gain = getDigitalGain();
	BoardConfig::corneaCameraDgain(true, getDigitalGainData(gain));

	BoardConfig::setModified(true);
	*/
	return true;
}


bool wso_device::CorneaCamera::setAnalogGain(float gain, bool control)
{
	if (!InfraredCamera::setAnalogGain(gain, control)) {
		return false;
	}
	saveConfigToIniFile();
	return true;
}

bool wso_device::CorneaCamera::setDigitalGain(float gain, bool control)
{
	if (!InfraredCamera::setDigitalGain(gain, control)) {
		return false;
	}
	saveConfigToIniFile();
	return true;
}

bool wso_device::CorneaCamera::loadConfigFromIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		CameraType type = getType();
		float fAgain = 0;
		float fDgain = 0;

		if (type == CameraType::IR_CORNEA_LEFT)
		{
			fAgain = (float)ini->ReadInt(L"CorneaCameraLeft", L"Again");
			fDgain = (float)ini->ReadInt(L"CorneaCameraLeft", L"Dgain");
		}
		else if (type == CameraType::IR_CORNEA_RIGHT)
		{
			fAgain = (float)ini->ReadInt(L"CorneaCameraRight", L"Again");
			fDgain = (float)ini->ReadInt(L"CorneaCameraRight", L"Dgain");
		}
		else if (type == CameraType::IR_CORNEA_LOWER)
		{
			fAgain = (float)ini->ReadInt(L"CorneaCameraLower", L"Again");
			fDgain = (float)ini->ReadInt(L"CorneaCameraLower", L"Dgain");
		}

		setAnalogGain(fAgain, true);
		setDigitalGain(fDgain, true);

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}
	return bRet;
}


bool wso_device::CorneaCamera::saveConfigToIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		CameraType type = getType();
		float fAgain = getAnalogGain();
		float fDgain = getDigitalGain();

		if (type == CameraType::IR_CORNEA_LEFT) // Left
		{
			ini->WriteInt(L"CorneaCameraLeft", L"Again", (int)fAgain);
			ini->WriteInt(L"CorneaCameraLeft", L"Dgain", (int)fDgain);
		}
		else if (type == CameraType::IR_CORNEA_RIGHT) // Right
		{
			ini->WriteInt(L"CorneaCameraRight", L"Again", (int)fAgain);
			ini->WriteInt(L"CorneaCameraRight", L"Dgain", (int)fDgain);
		}
		else if (type == CameraType::IR_CORNEA_LOWER) // Lower
		{
			ini->WriteInt(L"CorneaCameraLower", L"Again", (int)fAgain);
			ini->WriteInt(L"CorneaCameraLower", L"Dgain", (int)fDgain);
		}
		else
		{
			throw exception();
		}

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}

	return bRet;
}


CorneaCamera::CorneaCameraImpl& wso_device::CorneaCamera::impl(void) const
{
	return *d_ptr;
}
