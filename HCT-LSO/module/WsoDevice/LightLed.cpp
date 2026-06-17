#include "pch.h"
#include "LightLed.h"
#include "MainBoard.h"
#include "UsbComm.h"

#include <string>


using namespace wso_device;
using namespace std;


struct LightLed::LightLedImpl
{
	LightType type;
	unsigned short value;
	unsigned short valueSet1;
	unsigned short valueSet2;
	unsigned short valueMin;
	unsigned short valueMax;
	unsigned short valueStep;
	unsigned short valueMem;
	unsigned short lightMode;

	LightLedImpl() : 
		lightMode(1), // 0: Strobe mode (pulse by LSO scan param), 1 : Continuous mode 
		value(0), valueSet1(0), valueSet2(0), valueMem(0), valueMin(0), valueMax(100), valueStep(5)
	{
		type = LightType::UNKNOWN;
	}
};


LightLed::LightLed() :
	d_ptr(make_unique<LightLedImpl>())
{
}



wso_device::LightLed::LightLed(MainBoard* board, LightType type) :
	d_ptr(make_unique<LightLedImpl>()), BoardComponent(board)
{
	impl().type = type;

	impl().valueMin = LED_INTENSITY_MIN;
	impl().valueMax = LED_INTENSITY_MAX;
	impl().value = LED_INTENSITY_INIT;
	impl().valueStep = LED_INTENSITY_STEP;
	impl().lightMode = 1;
	return;
}


LightLed::~LightLed()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::LightLed::LightLed(LightLed&& rhs) = default;
LightLed& wso_device::LightLed::operator=(LightLed&& rhs) = default;


wso_device::LightLed::LightLed(const LightLed& rhs)
	: d_ptr(make_unique<LightLedImpl>(*rhs.d_ptr))
{
}


LightLed& wso_device::LightLed::operator=(const LightLed& rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool wso_device::LightLed::initializeLightLed(void)
{
	if (getMainBoard()->isSourceLedsNotInUse()) {
		return true;
	}

	setInitiated(true);

	// Set intensity level as default. 
	unsigned short value = getInitialValue();
	if (value > 0) {
		bool ret = setIntensity(value);
		if (!ret) {
			LogError() << "Light LED init value failed!, name: " << getName();
		}
	}

	if (loadConfigFromIniFile()) {
		auto mode = getLightMode();
		if (mode > 0) {
			if (!setLightMode(mode)) {
				LogError() << "Light LED init mode failed!, name: " << getName();
			}
		}
	}
	return true;
}


bool wso_device::LightLed::updateIntensityByOffset(int offset)
{
	int value = getIntensity() + offset;
	return updateIntensity(value);
}


bool wso_device::LightLed::updateIntensity(int value)
{
	if (setIntensity(value)) {
		return true;
	}
	return false;
}


bool wso_device::LightLed::setIntensity(unsigned short value)
{
	if (!isInitiated()) {
		return false;
	}

	value = (value < impl().valueMin ? impl().valueMin : value);
	value = (value > impl().valueMax ? impl().valueMax : value);

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSetIntensity(impl().type, (uint8_t)value)) {
		impl().value = value;
		return true;
	}
	LogDebug() << "LightLed::setIntensity() failed!, name=" << getName() << ", value=" << value;
	return false;
}


bool wso_device::LightLed::setLightMode(unsigned short value, bool setBoard)
{
	if (!isInitiated()) {
		return false;
	}

	if (!setBoard) {
		impl().lightMode = value;
		return true;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (usbComm.LedSetMode(impl().type, (uint8_t)value)) {
		impl().lightMode = value;
		saveConfigToIniFile();
		return true;
	}
	LogDebug() << "LightLed::setMode() failed!, name=" << getName() << ", value=" << value;
	return false;
}


unsigned short wso_device::LightLed::getIntensity(void) const
{
	return impl().value;
}

unsigned short wso_device::LightLed::getLightMode(void) const
{
	return impl().lightMode;
}

unsigned short wso_device::LightLed::getValueSet1(void) const
{
	return impl().valueSet1;
}


unsigned short wso_device::LightLed::getValueSet2(void) const
{
	return impl().valueSet2;
}


const char* wso_device::LightLed::getName(void) const
{
	switch (impl().type) {
		using enum LightType;
		case LSO_WHITE_LED:
			return LED_LSO_WHITE_NAME;
		case RETINA_IR_LED:
			return LED_RETINA_IR_NAME;
		case CORNEA_IR_LEFT_LED:
			return LED_CORNEA_LEFT_IR_NAME;
		case CORNEA_IR_RIGHT_LED:
			return LED_CORNEA_RIGHT_IR_NAME;
	}
	return LED_UNKNOWN_NAME;
}


LightType wso_device::LightLed::getType(void) const
{
	return impl().type;
}


bool wso_device::LightLed::isLightOn(void)
{
	auto value = getIntensity();
	return (value > 0);
}


bool wso_device::LightLed::turnLaserOn(void)
{
	unsigned short value = getIntensity();

	if (value > 0) {
		impl().valueMem = value;
	}
	else if (impl().valueMem > 0) {
		value = impl().valueMem;
	}
	else {
		value = 1;
	}

	LogDebug() << "LightLed turned on, name: " << getName() << ", value: " << value;
	// return setIntensity(value);
	return updateIntensity(value);
}


bool wso_device::LightLed::turnLaserOff(void)
{
	unsigned short value = getIntensity();

	if (value > 0) {
		impl().valueMem = value;
	}
	// return setIntensity(0);

	LogDebug() << "LightLed turned off, name: " << getName() << ", value: " << value;
	return updateIntensity(0);
}


bool wso_device::LightLed::control(bool flag)
{
	return (flag ? turnLaserOn() : turnLaserOff());
}


bool wso_device::LightLed::loadCalibParamFromProfile(void)
{
	return false;
}


bool wso_device::LightLed::saveCalibParamToProfile(void)
{
	return false;
}

std::uint8_t wso_device::LightLed::getLightLedId(void) const
{
	return std::uint8_t();
}


std::uint16_t wso_device::LightLed::getInitialValue(void) const
{
	switch (impl().type) {
		using enum LightType;
	case LSO_WHITE_LED:
		return LED_LSO_WHITE_INIT_VALUE;
	case RETINA_IR_LED:
		return LED_RETINA_IR_INIT_VALUE;
	case CORNEA_IR_LEFT_LED:
		return LED_CORNEA_LEFT_IR_INIT_VALUE;
	case CORNEA_IR_RIGHT_LED:
		return LED_CORNEA_RIGHT_IR_INIT_VALUE;
	}
	return 0;
}


bool wso_device::LightLed::loadConfigFromIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		switch (impl().type) {
		case LightType::LSO_WHITE_LED:
		{
			impl().lightMode = ini->ReadInt(L"LSO_WHITE_LED", L"mode");
			impl().value = ini->ReadInt(L"LSO_WHITE_LED", L"value");
			impl().valueMem = ini->ReadInt(L"LSO_WHITE_LED", L"valueMem");
			impl().valueMin = ini->ReadInt(L"LSO_WHITE_LED", L"valueMin");
			impl().valueMax = ini->ReadInt(L"LSO_WHITE_LED", L"valueMax");
			impl().valueStep = ini->ReadInt(L"LSO_WHITE_LED", L"valueStep");
			bRet = true;
		}
		break;
		case LightType::RETINA_IR_LED:
		{
			impl().lightMode = ini->ReadInt(L"RETINA_IR_LED", L"mode");
			impl().value = ini->ReadInt(L"RETINA_IR_LED", L"value");
			impl().valueMem = ini->ReadInt(L"RETINA_IR_LED", L"valueMem");
			impl().valueMin = ini->ReadInt(L"RETINA_IR_LED", L"valueMin");
			impl().valueMax = ini->ReadInt(L"RETINA_IR_LED", L"valueMax");
			impl().valueStep = ini->ReadInt(L"RETINA_IR_LED", L"valueStep");
			bRet = true;
		}
		break;
		case LightType::CORNEA_IR_LEFT_LED:
		{
			impl().value = ini->ReadInt(L"CORNEA_IR_LEFT_LED", L"value");
			impl().valueMem = ini->ReadInt(L"CORNEA_IR_LEFT_LED", L"valueMem");
			impl().valueMin = ini->ReadInt(L"CORNEA_IR_LEFT_LED", L"valueMin");
			impl().valueMax = ini->ReadInt(L"CORNEA_IR_LEFT_LED", L"valueMax");
			impl().valueStep = ini->ReadInt(L"CORNEA_IR_LEFT_LED", L"valueStep");
			bRet = true;
		}
		break;
		case LightType::CORNEA_IR_RIGHT_LED:
		{
			impl().value = ini->ReadInt(L"CORNEA_IR_RIGHT_LED", L"value");
			impl().valueMem = ini->ReadInt(L"CORNEA_IR_RIGHT_LED", L"valueMem");
			impl().valueMin = ini->ReadInt(L"CORNEA_IR_RIGHT_LED", L"valueMin");
			impl().valueMax = ini->ReadInt(L"CORNEA_IR_RIGHT_LED", L"valueMax");
			impl().valueStep = ini->ReadInt(L"CORNEA_IR_RIGHT_LED", L"valueStep");
			bRet = true;
		}
		break;
		default:
		{
			bRet = false;
		}
		break;
		}
	}
	catch (exception ex)
	{
		bRet = true;
	}
	return bRet;
}

bool wso_device::LightLed::saveConfigToIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		switch (impl().type) {
		case LightType::LSO_WHITE_LED:
		{
			ini->WriteInt(L"LSO_WHITE_LED", L"mode", impl().lightMode);
			ini->WriteInt(L"LSO_WHITE_LED", L"value", impl().value);
			ini->WriteInt(L"LSO_WHITE_LED", L"valueMem", impl().valueMem);
			ini->WriteInt(L"LSO_WHITE_LED", L"valueMin", impl().valueMin);
			ini->WriteInt(L"LSO_WHITE_LED", L"valueMax", impl().valueMax);
			ini->WriteInt(L"LSO_WHITE_LED", L"valueStep", impl().valueStep);
			bRet = true;
		}
		break;
		case LightType::RETINA_IR_LED:
		{
			ini->WriteInt(L"RETINA_IR_LED", L"mode", impl().lightMode);
			ini->WriteInt(L"RETINA_IR_LED", L"value", impl().value);
			ini->WriteInt(L"RETINA_IR_LED", L"valueMem", impl().valueMem);
			ini->WriteInt(L"RETINA_IR_LED", L"valueMin", impl().valueMin);
			ini->WriteInt(L"RETINA_IR_LED", L"valueMax", impl().valueMax);
			ini->WriteInt(L"RETINA_IR_LED", L"valueStep", impl().valueStep);
			bRet = true;
		}
		break;
		case LightType::CORNEA_IR_LEFT_LED:
		{
			ini->WriteInt(L"CORNEA_IR_LEFT_LED", L"value", impl().value);
			ini->WriteInt(L"CORNEA_IR_LEFT_LED", L"valueMem", impl().valueMem);
			ini->WriteInt(L"CORNEA_IR_LEFT_LED", L"valueMin", impl().valueMin);
			ini->WriteInt(L"CORNEA_IR_LEFT_LED", L"valueMax", impl().valueMax);
			ini->WriteInt(L"CORNEA_IR_LEFT_LED", L"valueStep", impl().valueStep);
			bRet = true;
		}
		break;
		case LightType::CORNEA_IR_RIGHT_LED:
		{
			ini->WriteInt(L"CORNEA_IR_RIGHT_LED", L"value", impl().value);
			ini->WriteInt(L"CORNEA_IR_RIGHT_LED", L"valueMem", impl().valueMem);
			ini->WriteInt(L"CORNEA_IR_RIGHT_LED", L"valueMin", impl().valueMin);
			ini->WriteInt(L"CORNEA_IR_RIGHT_LED", L"valueMax", impl().valueMax);
			ini->WriteInt(L"CORNEA_IR_RIGHT_LED", L"valueStep", impl().valueStep);
			bRet = true;
		}
		break;
		default:
		{
			bRet = false;
		}
		break;
		}
	}
	catch (exception ex)
	{
		bRet = false;
	}
	return bRet;
}


LightLed::LightLedImpl& wso_device::LightLed::impl(void) const
{
	return *d_ptr;
}

