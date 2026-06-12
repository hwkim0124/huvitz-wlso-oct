#include "pch.h"
#include "LsoScanner.h"
#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace wso_board;

struct LsoScanner::LsoScannerImpl
{
	MainBoard* board;
	bool initiated = false;
	bool turnedOn = false;

	unsigned int ctrlStatus = 0;
	unsigned int acqCnt = 0;
	lso_scan_profile_st scanProfile[8] = {};

	LsoScannerImpl() : board(nullptr), initiated(false) {
	}
};

LsoScanner::LsoScanner() :
	d_ptr(make_unique<LsoScannerImpl>())
{
}

wso_device::LsoScanner::LsoScanner(MainBoard* board) :
	d_ptr(make_unique<LsoScannerImpl>())
{
	impl().board = board;

	return;
}

LsoScanner::~LsoScanner()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::LsoScanner::LsoScanner(LsoScanner&& rhs) = default;
LsoScanner& wso_device::LsoScanner::operator=(LsoScanner&& rhs) = default;

bool wso_device::LsoScanner::initializeLsoScanner(void)
{
	/*
	if (getMainBoard()->isNoLeds()) {
	return true;
	}
	*/
	if (impl().board == nullptr) {
		return false;
	}

	updateScannerStatus();
	//controlSwitch(0);
	loadConfigFromIniFile();

	if (!storeControlParameters())
	{
		LogDebug() << "Lso scanner control parameters failed to change!";
		return false;
	}

	impl().initiated = true;
	return true;
}

bool wso_device::LsoScanner::updateScannerStatus(void)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();

	auto* hbs = getMainBoard()->getHbsDataProfile();
	auto* scanner = hbs->getHbsLsoScanner();
	auto* desc = hbs->getHbsTableDescriptor();
	if (!usbComm.readLsoScannerParam(scanner, desc)) {
		LogDebug() << "Reading slo scanner status failed!";
		return false;
	}

	auto& capture_status = scanner->lso_capture_status;
	auto& scan_traj_profile = scanner->traj_profile;

	impl().ctrlStatus = capture_status.ctrl_status;
	impl().acqCnt = capture_status.acq_cnt;

	impl().scanProfile[0] = scan_traj_profile[0];
	impl().scanProfile[1] = scan_traj_profile[1];

	return true;
}

bool wso_device::LsoScanner::fetchControlParameters(int PatternId, LsoScannerControlParam* pParam)
{
	if (pParam == nullptr)
	{
		return false;
	}

	int nPatternId = PatternId;

	if (nPatternId < 0 || nPatternId > (std::size(impl().scanProfile) - 1))
	{
		return false;
	}

	pParam->timeStepUs = impl().scanProfile[nPatternId].time_step_us;
	pParam->exposureTimeUs = impl().scanProfile[nPatternId].exposure_time_us;
	pParam->trgSrc = impl().scanProfile[nPatternId].TrgSrc;
	pParam->acqMode = impl().scanProfile[nPatternId].AcqMode;
	pParam->acqFrameSize = impl().scanProfile[nPatternId].AcqFrameSize;
	pParam->sampleSize = impl().scanProfile[nPatternId].sample_size;
	pParam->subFrameSize = impl().scanProfile[nPatternId].subframe_size;
	pParam->prescanPosRewindOffset = impl().scanProfile[nPatternId].prescan_pos_rewind_offset;
	pParam->yGalvoStartPos = impl().scanProfile[nPatternId].pos[0];
	pParam->yGalvoEndPos = impl().scanProfile[nPatternId].pos[impl().scanProfile[nPatternId].sample_size-1];
	pParam->ledOnPosIndex = impl().scanProfile[nPatternId].led_on_pos_index;
	pParam->ledOffPosIndex = impl().scanProfile[nPatternId].led_off_pos_index;
	return true;
}

bool wso_device::LsoScanner::changeControlParameters(int PatternId, const LsoScannerControlParam* pParam)
{
	if (pParam == nullptr)
	{
		return false;
	}

	int nPatternId = PatternId;

	if (nPatternId < 0 || nPatternId >(std::size(impl().scanProfile) - 1))
	{
		return false;
	}

	impl().scanProfile[nPatternId].time_step_us = pParam->timeStepUs;
	impl().scanProfile[nPatternId].exposure_time_us = pParam->exposureTimeUs;
	impl().scanProfile[nPatternId].TrgSrc = pParam->trgSrc;
	impl().scanProfile[nPatternId].AcqMode = pParam->acqMode;
	impl().scanProfile[nPatternId].AcqFrameSize = pParam->acqFrameSize;
	impl().scanProfile[nPatternId].sample_size = pParam->sampleSize;
	impl().scanProfile[nPatternId].subframe_size = pParam->subFrameSize;
	impl().scanProfile[nPatternId].prescan_pos_rewind_offset = pParam->prescanPosRewindOffset;
	impl().scanProfile[nPatternId].led_on_pos_index = pParam->ledOnPosIndex;
	impl().scanProfile[nPatternId].led_off_pos_index = pParam->ledOffPosIndex;

	size_t count = 0;
	int start = pParam->yGalvoStartPos;
	int end = pParam->yGalvoEndPos;

	// 구간 수 = N-1 (양 끝점을 모두 포함)
	double delta = double(end - start);
	double step = delta / double(pParam->sampleSize - 1);

	std::fill_n(impl().scanProfile[nPatternId].pos, (int)MAX_LSO_DAC_SAMPLE_SIZE, 0); // 전체 사이즈를 0으로 초기화

	for (std::size_t i = 0; i < pParam->sampleSize; ++i) {
		double value = start + step * double(i);
		impl().scanProfile[nPatternId].pos[i] = static_cast<short int>(std::round(value));
	}

	if (!storeControlParameters())
	{
		LogDebug() << "Lso scanner control parameters failed to change!";
		return false;
	}

	updateScannerStatus();
	saveConfigToIniFile();
	return true;
}

bool wso_device::LsoScanner::storeControlParameters(void)
{
	auto* hbs = getMainBoard()->getHbsDataProfile();
	auto* scanner = const_cast<HbsLsoScanner*>(hbs->getHbsLsoScanner());

	auto& capture_status = scanner->lso_capture_status;
	auto& scan_traj_profile = scanner->traj_profile;
	
	capture_status.ctrl_status = impl().ctrlStatus;
	capture_status.acq_cnt = impl().acqCnt;

	scan_traj_profile[0] = impl().scanProfile[0];
	scan_traj_profile[1] = impl().scanProfile[1];

	if (!hbs->saveLsoScannerParam())
	{
		LogDebug() << "Lso scanner control parameters failed to store!";
		return false;
	}
	return true;
}

bool wso_device::LsoScanner::controlYGalvoMove(int ypos)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LsoGalvanoMoveY(ypos)) {
		LogDebug() << "Lso scanner, control y galvo move failed!";
		return false;
	}
	LogDebug() << "Lso scanner, control y galvo moved to pos: " << ypos;
	return true;
}


bool wso_device::LsoScanner::controlCapture(int nPatternId, int onOff)
{
	if (!impl().initiated) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LsoScannerControl(nPatternId, onOff)) {
		LogDebug() << "Lso scanner, control capture failed!";
		return false;
	}
	LogDebug() << "Lso scanner, capture switched: " << onOff;
	return true;
}

bool wso_device::LsoScanner::controlTriggerMode(int onOff)
{
	if (!impl().initiated) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LsoScannerTriggerControl(onOff)) {
		LogDebug() << "Lso scanner, control trigger mode failed!";
		return false;
	}
	LogDebug() << "Lso scanner, control trigger mode : " << onOff;
	return true;
}

bool wso_device::LsoScanner::startGrabbing(int nPatternId)
{
	if (!impl().initiated) {
		return false;
	}

	pauseGrabbing(nPatternId);

	if (!controlCapture(nPatternId, 1)) {
		pauseGrabbing(nPatternId);
		return false;
	}
	return true;
}

bool wso_device::LsoScanner::pauseGrabbing(int nPatternId)
{
	if (!impl().initiated) {
		return false;
	}

	if (!controlCapture(nPatternId, 0)) {
		return false;
	}
	return true;
}

bool wso_device::LsoScanner::loadConfigFromIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		impl().scanProfile[0].exposure_time_us = (float)ini->ReadInt(L"LSO_SCANNER_0", L"ExposureTime");
		impl().scanProfile[0].TrgSrc = ini->ReadInt(L"LSO_SCANNER_0", L"TrgSrc");
		impl().scanProfile[0].AcqMode = ini->ReadInt(L"LSO_SCANNER_0", L"AcqMode");
		impl().scanProfile[0].AcqFrameSize = ini->ReadInt(L"LSO_SCANNER_0", L"AcqFrameSize");
		impl().scanProfile[0].subframe_size = ini->ReadInt(L"LSO_SCANNER_0", L"SubFrameSize");
		impl().scanProfile[0].led_on_pos_index = ini->ReadInt(L"LSO_SCANNER_0", L"LedOnPosIndex");
		impl().scanProfile[0].led_off_pos_index = ini->ReadInt(L"LSO_SCANNER_0", L"LedOffPosIndex");
		impl().scanProfile[0].prescan_pos_rewind_offset = ini->ReadInt(L"LSO_SCANNER_0", L"GalvanoRewindOffset");
		impl().scanProfile[0].sample_size = ini->ReadInt(L"LSO_SCANNER_0", L"GalvoPatternSize");
		impl().scanProfile[0].time_step_us = (float)ini->ReadInt(L"LSO_SCANNER_0", L"TimeStep");
		impl().scanProfile[0].pos[0] = ini->ReadInt(L"LSO_SCANNER_0", L"GalvoStartPos");
		impl().scanProfile[0].pos[impl().scanProfile[0].sample_size - 1] = ini->ReadInt(L"LSO_SCANNER_0", L"GalvoEndPos");
		calcGalvanoPos(0, impl().scanProfile[0].sample_size);

		impl().scanProfile[1].exposure_time_us = (float)ini->ReadInt(L"LSO_SCANNER_1", L"ExposureTime");
		impl().scanProfile[1].TrgSrc = ini->ReadInt(L"LSO_SCANNER_1", L"TrgSrc");
		impl().scanProfile[1].AcqMode = ini->ReadInt(L"LSO_SCANNER_1", L"AcqMode");
		impl().scanProfile[1].AcqFrameSize = ini->ReadInt(L"LSO_SCANNER_1", L"AcqFrameSize");
		impl().scanProfile[1].subframe_size = ini->ReadInt(L"LSO_SCANNER_1", L"SubFrameSize");
		impl().scanProfile[1].led_on_pos_index = ini->ReadInt(L"LSO_SCANNER_1", L"LedOnPosIndex");
		impl().scanProfile[1].led_off_pos_index = ini->ReadInt(L"LSO_SCANNER_1", L"LedOffPosIndex");
		impl().scanProfile[1].prescan_pos_rewind_offset = ini->ReadInt(L"LSO_SCANNER_1", L"GalvanoRewindOffset");
		impl().scanProfile[1].sample_size = ini->ReadInt(L"LSO_SCANNER_1", L"GalvoPatternSize");
		impl().scanProfile[1].time_step_us = (float)ini->ReadInt(L"LSO_SCANNER_1", L"TimeStep");
		impl().scanProfile[1].pos[0] = ini->ReadInt(L"LSO_SCANNER_1", L"GalvoStartPos");
		impl().scanProfile[1].pos[impl().scanProfile[1].sample_size - 1] = ini->ReadInt(L"LSO_SCANNER_1", L"GalvoEndPos");
		calcGalvanoPos(1, impl().scanProfile[1].sample_size);

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}
	return bRet;
}

bool wso_device::LsoScanner::saveConfigToIniFile()
{
	bool bRet = false;

	try
	{
		IniFile* ini = getConfigIniFile();
		if (!ini) {
			return false;
		}

		ini->WriteInt(L"LSO_SCANNER_0", L"ExposureTime", (int)impl().scanProfile[0].exposure_time_us);
		ini->WriteInt(L"LSO_SCANNER_0", L"TrgSrc", impl().scanProfile[0].TrgSrc);
		ini->WriteInt(L"LSO_SCANNER_0", L"AcqMode", impl().scanProfile[0].AcqMode);
		ini->WriteInt(L"LSO_SCANNER_0", L"AcqFrameSize", impl().scanProfile[0].AcqFrameSize);
		ini->WriteInt(L"LSO_SCANNER_0", L"SubFrameSize", impl().scanProfile[0].subframe_size);
		ini->WriteInt(L"LSO_SCANNER_0", L"LedOnPosIndex", impl().scanProfile[0].led_on_pos_index);
		ini->WriteInt(L"LSO_SCANNER_0", L"LedOffPosIndex", impl().scanProfile[0].led_off_pos_index);
		ini->WriteInt(L"LSO_SCANNER_0", L"GalvanoRewindOffset", impl().scanProfile[0].prescan_pos_rewind_offset);
		ini->WriteInt(L"LSO_SCANNER_0", L"GalvoPatternSize", impl().scanProfile[0].sample_size);
		ini->WriteInt(L"LSO_SCANNER_0", L"TimeStep", (int)impl().scanProfile[0].time_step_us);
		ini->WriteInt(L"LSO_SCANNER_0", L"GalvoStartPos", impl().scanProfile[0].pos[0]);
		ini->WriteInt(L"LSO_SCANNER_0", L"GalvoEndPos", impl().scanProfile[0].pos[impl().scanProfile[0].sample_size - 1]);

		ini->WriteInt(L"LSO_SCANNER_1", L"ExposureTime", (int)impl().scanProfile[1].exposure_time_us);
		ini->WriteInt(L"LSO_SCANNER_1", L"TrgSrc", impl().scanProfile[1].TrgSrc);
		ini->WriteInt(L"LSO_SCANNER_1", L"AcqMode", impl().scanProfile[1].AcqMode);
		ini->WriteInt(L"LSO_SCANNER_1", L"AcqFrameSize", impl().scanProfile[1].AcqFrameSize);
		ini->WriteInt(L"LSO_SCANNER_1", L"SubFrameSize", impl().scanProfile[1].subframe_size);
		ini->WriteInt(L"LSO_SCANNER_1", L"LedOnPosIndex", impl().scanProfile[1].led_on_pos_index);
		ini->WriteInt(L"LSO_SCANNER_1", L"LedOffPosIndex", impl().scanProfile[1].led_off_pos_index);
		ini->WriteInt(L"LSO_SCANNER_1", L"GalvanoRewindOffset", impl().scanProfile[1].prescan_pos_rewind_offset);
		ini->WriteInt(L"LSO_SCANNER_1", L"GalvoPatternSize", impl().scanProfile[1].sample_size);
		ini->WriteInt(L"LSO_SCANNER_1", L"TimeStep", (int)impl().scanProfile[1].time_step_us);
		ini->WriteInt(L"LSO_SCANNER_1", L"GalvoStartPos", impl().scanProfile[1].pos[0]);
		ini->WriteInt(L"LSO_SCANNER_1", L"GalvoEndPos", impl().scanProfile[1].pos[impl().scanProfile[1].sample_size - 1]);

		bRet = true;
	}
	catch (exception ex)
	{
		bRet = false;
	}

	return bRet;
}

void wso_device::LsoScanner::calcGalvanoPos(int nPatternId, int nGalvanoSampleSize)
{
	size_t count = 0;
	int start = impl().scanProfile[nPatternId].pos[0];
	int end = impl().scanProfile[nPatternId].pos[nGalvanoSampleSize - 1];

	// 구간 수 = N-1 (양 끝점을 모두 포함)
	double delta = double(end - start);
	double step = delta / double(nGalvanoSampleSize - 1);

	std::fill_n(impl().scanProfile[nPatternId].pos, (int)MAX_LSO_DAC_SAMPLE_SIZE, 0); // 전체 사이즈를 0으로 초기화

	for (std::size_t i = 0; i < nGalvanoSampleSize; ++i) {
		double value = start + step * double(i);
		impl().scanProfile[nPatternId].pos[i] = static_cast<short int>(std::round(value));
	}
}

MainBoard* wso_device::LsoScanner::getMainBoard(void) const
{
	return impl().board;
}

LsoScanner::LsoScannerImpl& wso_device::LsoScanner::impl(void) const
{
	return *d_ptr;
}