#include "pch.h"
#include "RetinaCamera.h"
#include "MainBoard.h"
#include "UsbComm.h"
#include "UsbPort.h"

using namespace wso_device;
using namespace std;


struct RetinaCamera::RetinaCameraImpl
{
	RetinaCameraImpl()
	{
		initializeRetinaCameraImpl();
	}

	void initializeRetinaCameraImpl(void)
	{
	}
};


RetinaCamera::RetinaCamera() :
	d_ptr(make_unique<RetinaCameraImpl>()), InfraredCamera()
{
}


RetinaCamera::RetinaCamera(MainBoard* board, CameraType type, InfraredCameraId camId, std::uint8_t epid) :
	d_ptr(make_unique<RetinaCameraImpl>()), InfraredCamera(board, type, camId, epid)
{
}


RetinaCamera::~RetinaCamera()
{
}


bool wso_device::RetinaCamera::initializeRetinaCamera(void)
{
	if (!InfraredCamera::initializeInfraredCamera()) {
		return false;
	}
	// loadParamsFromProfile();

	loadConfigFromIniFile();
	return true;
}

bool wso_device::RetinaCamera::setAnalogGain(float gain, bool control)
{
	if (!InfraredCamera::setAnalogGain(gain, control))
	{
		return false;
	}

	saveConfigToIniFile();
	return true;
}

bool wso_device::RetinaCamera::setDigitalGain(float gain, bool control)
{
	if (!InfraredCamera::setDigitalGain(gain, control))
	{
		return false;
	}

	saveConfigToIniFile();
	return true;
}

bool wso_device::RetinaCamera::loadCalibParamFromProfile(void)
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


bool wso_device::RetinaCamera::saveCalibParamToProfile(void)
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

bool wso_device::RetinaCamera::loadConfigFromIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		float fAgain = (float)ini->ReadInt(L"RetinaCamera", L"Again");
		float fDgain = (float)ini->ReadInt(L"RetinaCamera", L"Dgain");
		
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

bool wso_device::RetinaCamera::saveConfigToIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		float fAgain = getAnalogGain();
		float fDgain = getDigitalGain();

		ini->WriteInt(L"RetinaCamera", L"Again", (int)fAgain);
		ini->WriteInt(L"RetinaCamera", L"Dgain", (int)fDgain);

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}

	return bRet;
}

RetinaCamera::RetinaCameraImpl& wso_device::RetinaCamera::impl(void) const
{
	return *d_ptr;
}
